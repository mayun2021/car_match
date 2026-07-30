/**
 * @file oled.h
 * @brief SSD1306 128x64 I2C OLED 显示接口。
 *
 * 提供最小的取模字库文字显示能力：清屏、画字符/字符串、整屏刷新。
 * 应用层不需要关心具体的页寻址和取模格式，只需要按坐标画文字。
 */

#ifndef OLED_H
#define OLED_H

#include <stdbool.h>
#include <stdint.h>

/** 屏幕像素宽度。 */
#define OLED_WIDTH  128u
/** 屏幕像素高度。 */
#define OLED_HEIGHT 64u
/** 页数，每页 8 行像素。 */
#define OLED_PAGES  (OLED_HEIGHT / 8u)

/**
 * @brief 初始化 SSD1306（标准初始化命令序列）并清屏。
 */
void oled_init(void);

/**
 * @brief 清空内部帧缓冲（不会立即刷新到屏幕，需要调用 oled_flush()）。
 */
void oled_clear(void);

/**
 * @brief 在指定像素坐标画一个 8x8 点阵字符。
 *
 * @param x 字符左上角 x 坐标，0-127。
 * @param y 字符左上角 y 坐标，建议取 8 的倍数（0/8/16/...）。
 * @param c 可打印 ASCII 字符，超出 0x20-0x7E 范围按空格处理。
 */
void oled_draw_char(uint8_t x, uint8_t y, char c);

/**
 * @brief 从指定坐标开始逐字符画一行文本，超出屏幕宽度的部分会被截断。
 *
 * @param x 起始 x 坐标。
 * @param y 起始 y 坐标。
 * @param text 以 '\0' 结尾的 ASCII 字符串。
 */
void oled_draw_string(uint8_t x, uint8_t y, const char *text);

/**
 * @brief 把内部帧缓冲通过 I2C 整屏发送到 SSD1306。
 */
void oled_flush(void);

/**
 * @brief 获取内部帧缓冲只读指针，供桌面仿真/测试离线渲染核对排版用。
 *
 * @return 指向 OLED_PAGES * OLED_WIDTH 字节帧缓冲的指针，按页存储，
 *         每字节的 bit0-bit7 对应该页内从上到下 8 行像素。
 */
const uint8_t *oled_get_framebuffer(void);

#endif
