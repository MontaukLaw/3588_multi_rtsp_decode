#ifndef __SCREEN_TEST_H_
#define __SCREEN_TEST_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <cstdint>

void hdmi_init();

void draw_hdmi_screen_rgb(uint8_t *data, uint32_t dataSize);

void draw_hdmi_screen_rgb_quarter(uint8_t *data, uint32_t dataSize, uint8_t quarter);

void draw_hdmi_screen_rgb_nine(uint8_t *data, uint32_t dataSize, uint8_t part);

void draw_hdmi_screen_rgb_dynamic(uint8_t *data, uint32_t dataSize, uint8_t screenNum, uint8_t rows, uint8_t cols);

#endif
