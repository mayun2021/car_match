/**
 * @file k230_protocol.h
 * @brief K230 视觉模块串口协议解析接口。
 *
 * K230 负责识别凹槽/水管内钢球位置，并通过串口把 x_mm 发送给 MSPM0。
 * 本模块把 ASCII 文本帧解析成可供滚球 PID 使用的位置数据。
 */

#ifndef K230_PROTOCOL_H
#define K230_PROTOCOL_H

#include <stdbool.h>
#include <stdint.h>

typedef struct
{
    bool valid;
    float x_mm;
    float y_px;
    uint32_t last_update_ms;
    uint32_t frames;
    uint32_t parse_errors;
} k230_vision_t;

/**
 * @brief 初始化 K230 协议解析状态。
 */
void k230_protocol_init(void);

/**
 * @brief 从串口取出所有已收到字节并解析完整帧。
 *
 * @param now_ms 当前系统时间，单位 ms，用于判断视觉数据是否超时。
 */
void k230_protocol_poll(uint32_t now_ms);

/**
 * @brief 获取最近一次视觉识别结果。
 *
 * @return 最近的钢球位置、有效标志、更新时间和统计信息。
 */
k230_vision_t k230_protocol_get(void);

#endif
