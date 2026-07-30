/**
 * @file k230_protocol.c
 * @brief K230 视觉串口协议解析实现。
 *
 * 支持两种文本帧：`B:<x_mm>` 和 `$BALL,<x_mm>,<y_px>,<valid>`。
 * 文本协议便于比赛现场直接用串口助手调试。
 */

#include "k230_protocol.h"
#include "hal.h"
#include "robot_config.h"

#include <stdlib.h>
#include <string.h>

#define K230_LINE_BUF_SIZE 64u

static char s_line_buf[K230_LINE_BUF_SIZE];
static uint8_t s_line_len;
static k230_vision_t s_vision;

/**
 * @brief 解析带固定前缀的浮点数文本。
 *
 * @param line 输入的一整行字符串。
 * @param prefix 期望前缀，例如 "B:"。
 * @param value 解析出的浮点数。
 * @return true 表示前缀匹配且浮点数解析成功。
 */
static bool parse_float_after_prefix(const char *line, const char *prefix, float *value)
{
    char *end = NULL;
    const size_t prefix_len = strlen(prefix);

    if (strncmp(line, prefix, prefix_len) != 0)
    {
        return false;
    }

    *value = strtof(line + prefix_len, &end);
    return end != line + prefix_len;
}

/**
 * @brief 解析一整帧 K230 文本数据。
 *
 * @param line 去掉换行符后的文本帧。
 * @param now_ms 当前系统时间，用于记录数据更新时间。
 */
static void parse_line(const char *line, uint32_t now_ms)
{
    float x = 0.0f;

    if (parse_float_after_prefix(line, "B:", &x))
    {
        s_vision.valid = true;
        s_vision.x_mm = x;
        s_vision.y_px = 0.0f;
        s_vision.last_update_ms = now_ms;
        s_vision.frames++;
        return;
    }

    if (strncmp(line, "$BALL,", 6) == 0)
    {
        char *cursor = (char *)line + 6;
        char *end = NULL;
        float y = 0.0f;
        long valid = 0;

        x = strtof(cursor, &end);
        if (*end != ',')
        {
            s_vision.parse_errors++;
            return;
        }

        cursor = end + 1;
        y = strtof(cursor, &end);
        if (*end != ',')
        {
            s_vision.parse_errors++;
            return;
        }

        cursor = end + 1;
        valid = strtol(cursor, &end, 10);

        s_vision.valid = valid != 0;
        s_vision.x_mm = x;
        s_vision.y_px = y;
        s_vision.last_update_ms = now_ms;
        s_vision.frames++;
        return;
    }

    s_vision.parse_errors++;
}

/**
 * @brief 初始化协议解析缓冲区和视觉结果。
 */
void k230_protocol_init(void)
{
    memset(s_line_buf, 0, sizeof(s_line_buf));
    s_line_len = 0;
    memset(&s_vision, 0, sizeof(s_vision));
}

/**
 * @brief 从 K230 串口读取字节并解析完整行。
 *
 * @param now_ms 当前系统时间，单位 ms。
 */
void k230_protocol_poll(uint32_t now_ms)
{
    int byte_value;

    while ((byte_value = hal_uart_k230_read_byte()) >= 0)
    {
        char c = (char)byte_value;

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
            s_line_len = 0;
            continue;
        }

        if (s_line_len < K230_LINE_BUF_SIZE - 1u)
        {
            s_line_buf[s_line_len++] = c;
        }
        else
        {
            /* 一帧过长说明串口数据异常，丢弃到下一行。 */
            s_line_len = 0;
            s_vision.parse_errors++;
        }
    }

    if (s_vision.valid && (now_ms - s_vision.last_update_ms > ROBOT_VISION_TIMEOUT_MS))
    {
        s_vision.valid = false;
    }
}

/**
 * @brief 获取最近一次视觉解析结果。
 *
 * @return 最近一次钢球位置与协议统计信息。
 */
k230_vision_t k230_protocol_get(void)
{
    return s_vision;
}
