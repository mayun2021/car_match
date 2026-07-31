/**
 * @file k230_protocol.c
 * @brief K230 第 3 问双向文本协议实现。
 */

#include "k230_protocol.h"
#include "hal.h"
#include "robot_config.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define K230_LINE_BUF_SIZE 128u
#define K230_ABS_MM_LIMIT 500.0f

static char s_line_buf[K230_LINE_BUF_SIZE];
static uint8_t s_line_len;
static k230_status_t s_status;

static bool take_long(char **cursor, long *value)
{
    char *end;

    if (cursor == NULL || *cursor == NULL || value == NULL)
    {
        return false;
    }

    *value = strtol(*cursor, &end, 10);
    if (end == *cursor)
    {
        return false;
    }

    if (*end == ',')
    {
        *cursor = end + 1;
    }
    else if (*end == '\0')
    {
        *cursor = end;
    }
    else
    {
        return false;
    }
    return true;
}

static bool take_float(char **cursor, float *value)
{
    char *end;

    if (cursor == NULL || *cursor == NULL || value == NULL)
    {
        return false;
    }

    *value = strtof(*cursor, &end);
    if (end == *cursor)
    {
        return false;
    }

    if (*end == ',')
    {
        *cursor = end + 1;
    }
    else if (*end == '\0')
    {
        *cursor = end;
    }
    else
    {
        return false;
    }
    return true;
}

static void mark_frame(uint32_t now_ms)
{
    s_status.last_update_ms = now_ms;
    s_status.link_alive = true;
    ++s_status.frames;
}

static bool parse_ack(char *cursor, uint32_t now_ms)
{
    long seq;

    if (!take_long(&cursor, &seq) || seq < 0 || seq > 65535 || cursor == NULL)
    {
        return false;
    }

    if (strcmp(cursor, "START") == 0)
    {
        s_status.ack = K230_ACK_START;
    }
    else if (strcmp(cursor, "ABORT") == 0)
    {
        s_status.ack = K230_ACK_ABORT;
    }
    else if (strcmp(cursor, "NEUTRAL") == 0)
    {
        s_status.ack = K230_ACK_NEUTRAL;
    }
    else
    {
        return false;
    }

    s_status.ack_seq = (uint16_t)seq;
    mark_frame(now_ms);
    return true;
}

static bool parse_h3(char *cursor, uint32_t now_ms)
{
    long seq;
    long state;
    long servo_us;
    long valid;
    long elapsed_ms;
    float x_mm;
    float target_mm;
    float error_mm;

    if (!take_long(&cursor, &seq) ||
        !take_long(&cursor, &state) ||
        !take_float(&cursor, &x_mm) ||
        !take_float(&cursor, &target_mm) ||
        !take_float(&cursor, &error_mm) ||
        !take_long(&cursor, &servo_us) ||
        !take_long(&cursor, &valid) ||
        !take_long(&cursor, &elapsed_ms))
    {
        return false;
    }

    if (seq < 0 || seq > 65535 ||
        state < K230_TASK_IDLE || state > K230_TASK_FAULT ||
        servo_us < 500 || servo_us > 2500 ||
        elapsed_ms < 0 ||
        !isfinite(x_mm) || !isfinite(target_mm) || !isfinite(error_mm) ||
        fabsf(x_mm) > K230_ABS_MM_LIMIT ||
        fabsf(target_mm) > K230_ABS_MM_LIMIT ||
        fabsf(error_mm) > K230_ABS_MM_LIMIT)
    {
        return false;
    }

    s_status.seq = (uint16_t)seq;
    s_status.task_state = (uint8_t)state;
    s_status.x_mm = x_mm;
    s_status.target_mm = target_mm;
    s_status.error_mm = error_mm;
    s_status.servo_us = (uint16_t)servo_us;
    s_status.vision_valid = valid != 0;
    s_status.elapsed_ms = (uint32_t)elapsed_ms;
    s_status.done = state == K230_TASK_DONE;
    s_status.fault = state == K230_TASK_FAULT;
    mark_frame(now_ms);
    return true;
}

static bool parse_done(char *cursor, uint32_t now_ms)
{
    long seq;
    long elapsed_ms;
    float error_mm;

    if (!take_long(&cursor, &seq) ||
        !take_long(&cursor, &elapsed_ms) ||
        !take_float(&cursor, &error_mm) ||
        seq < 0 || seq > 65535 || elapsed_ms < 0 ||
        !isfinite(error_mm) || fabsf(error_mm) > K230_ABS_MM_LIMIT)
    {
        return false;
    }

    s_status.seq = (uint16_t)seq;
    s_status.task_state = K230_TASK_DONE;
    s_status.elapsed_ms = (uint32_t)elapsed_ms;
    s_status.error_mm = error_mm;
    s_status.done = true;
    s_status.fault = false;
    mark_frame(now_ms);
    return true;
}

static bool parse_fault(char *cursor, uint32_t now_ms)
{
    long seq;
    long code;

    if (!take_long(&cursor, &seq) ||
        !take_long(&cursor, &code) ||
        seq < 0 || seq > 65535 || code < 0 || code > 255)
    {
        return false;
    }

    s_status.seq = (uint16_t)seq;
    s_status.task_state = K230_TASK_FAULT;
    s_status.fault_code = (uint8_t)code;
    s_status.done = false;
    s_status.fault = true;
    mark_frame(now_ms);
    return true;
}

static bool parse_legacy_ball(char *cursor, uint32_t now_ms)
{
    float x_mm;
    float unused_y;
    long valid;

    if (!take_float(&cursor, &x_mm) ||
        !take_float(&cursor, &unused_y) ||
        !take_long(&cursor, &valid) ||
        !isfinite(x_mm) || !isfinite(unused_y) ||
        fabsf(x_mm) > K230_ABS_MM_LIMIT)
    {
        return false;
    }

    (void)unused_y;
    s_status.x_mm = x_mm;
    s_status.vision_valid = valid != 0;
    /*
     * 旧 BALL 帧只更新调试位置，不作为 Q3 心跳；任务监督必须依赖带 seq
     * 的 ACK/H3/DONE/FAULT，避免旧脚本制造“假在线”。
     */
    (void)now_ms;
    return true;
}

static void parse_line(char *line, uint32_t now_ms)
{
    bool ok = false;

    if (strncmp(line, "$ACK,", 5) == 0)
    {
        ok = parse_ack(line + 5, now_ms);
    }
    else if (strncmp(line, "$H3,", 4) == 0)
    {
        ok = parse_h3(line + 4, now_ms);
    }
    else if (strncmp(line, "$DONE,", 6) == 0)
    {
        ok = parse_done(line + 6, now_ms);
    }
    else if (strncmp(line, "$FAULT,", 7) == 0)
    {
        ok = parse_fault(line + 7, now_ms);
    }
    else if (strncmp(line, "$BALL,", 6) == 0)
    {
        ok = parse_legacy_ball(line + 6, now_ms);
    }

    if (!ok)
    {
        ++s_status.parse_errors;
    }
}

void k230_protocol_init(void)
{
    memset(s_line_buf, 0, sizeof(s_line_buf));
    memset(&s_status, 0, sizeof(s_status));
    s_line_len = 0u;
}

void k230_protocol_poll(uint32_t now_ms)
{
    int byte_value;

    while ((byte_value = hal_uart_k230_read_byte()) >= 0)
    {
        const char c = (char)byte_value;

        if (c == '\r')
        {
            continue;
        }
        if (c == '\n')
        {
            s_line_buf[s_line_len] = '\0';
            if (s_line_len > 0u)
            {
                parse_line(s_line_buf, now_ms);
            }
            s_line_len = 0u;
            continue;
        }

        if (s_line_len < K230_LINE_BUF_SIZE - 1u)
        {
            s_line_buf[s_line_len++] = c;
        }
        else
        {
            s_line_len = 0u;
            ++s_status.parse_errors;
        }
    }

    if (s_status.frames == 0u ||
        (now_ms - s_status.last_update_ms) > ROBOT_Q3_LINK_TIMEOUT_MS)
    {
        s_status.link_alive = false;
        s_status.vision_valid = false;
    }
}

k230_status_t k230_protocol_get(void)
{
    return s_status;
}

void k230_protocol_send_q3_start(uint16_t seq,
                                 int16_t target_pos_mm,
                                 int16_t target_neg_mm)
{
    char command[56];

    (void)snprintf(command,
                   sizeof(command),
                   "$CMD,Q3,START,%u,%d,%d\n",
                   (unsigned int)seq,
                   (int)target_pos_mm,
                   (int)target_neg_mm);
    hal_uart_k230_write(command);
}

void k230_protocol_send_q3_abort(uint16_t seq)
{
    char command[40];

    (void)snprintf(command,
                   sizeof(command),
                   "$CMD,Q3,ABORT,%u\n",
                   (unsigned int)seq);
    hal_uart_k230_write(command);
}

void k230_protocol_send_q3_neutral(uint16_t seq)
{
    char command[40];

    (void)snprintf(command,
                   sizeof(command),
                   "$CMD,Q3,NEUTRAL,%u\n",
                   (unsigned int)seq);
    hal_uart_k230_write(command);
}

void k230_protocol_send_q3_keepalive(uint16_t seq)
{
    char command[40];

    (void)snprintf(command,
                   sizeof(command),
                   "$CMD,Q3,KEEP,%u\n",
                   (unsigned int)seq);
    hal_uart_k230_write(command);
}
