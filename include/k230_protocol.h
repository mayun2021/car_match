/**
 * @file k230_protocol.h
 * @brief MSPM0 与 K230 的第 3 问任务协议。
 *
 * MSPM0 负责按键、总计时、OLED 和安全监督；K230 在本地完成视觉与舵机闭环。
 * 帧均为一行 ASCII，以 '\n' 结尾，便于现场串口抓包。
 */

#ifndef K230_PROTOCOL_H
#define K230_PROTOCOL_H

#include <stdbool.h>
#include <stdint.h>

typedef enum
{
    K230_TASK_IDLE = 0,
    K230_TASK_TO_POS = 1,
    K230_TASK_TO_NEG = 2,
    K230_TASK_HOLD_NEG = 3,
    K230_TASK_DONE = 4,
    K230_TASK_FAULT = 5
} k230_task_state_t;

typedef enum
{
    K230_ACK_NONE = 0,
    K230_ACK_START,
    K230_ACK_ABORT,
    K230_ACK_NEUTRAL
} k230_ack_t;

typedef struct
{
    bool link_alive;
    bool vision_valid;
    bool done;
    bool fault;
    uint16_t seq;
    uint16_t ack_seq;
    uint16_t servo_us;
    uint8_t task_state;
    uint8_t fault_code;
    k230_ack_t ack;
    float x_mm;
    float target_mm;
    float error_mm;
    uint32_t elapsed_ms;
    uint32_t last_update_ms;
    uint32_t frames;
    uint32_t parse_errors;
} k230_status_t;

void k230_protocol_init(void);
void k230_protocol_poll(uint32_t now_ms);
k230_status_t k230_protocol_get(void);

void k230_protocol_send_q3_start(uint16_t seq,
                                 int16_t target_pos_mm,
                                 int16_t target_neg_mm);
void k230_protocol_send_q3_abort(uint16_t seq);
void k230_protocol_send_q3_neutral(uint16_t seq);
void k230_protocol_send_q3_keepalive(uint16_t seq);

#endif
