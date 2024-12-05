/**
 * @file mini_display_thread.hpp
 * @author Petr Malaník (TheColonelYoung(at)gmail(dot)com)
 * @version 0.1
 * @date 05.12.2024
 */

#pragma once

#include "thread.hpp"
#include "rtos/wrappers.hpp"

#include <src/widgets/lv_bar.h>

#include <stdint.h>
#include <string>

#include "emio/emio.hpp"

#include "hal/i2c/i2c_bus.hpp"
#include "hal/i2c/i2c_device.hpp"
#include "display/SSD1306.hpp"

#include "lvgl.h"

#define UNUSED(x) (void)(x)

void flush_cb(lv_disp_drv_t *disp_drv, const lv_area_t *area, lv_color_t *color_p);
void set_px_cb(lv_disp_drv_t *disp_drv, uint8_t *buf, lv_coord_t buf_w, lv_coord_t x, lv_coord_t y, lv_color_t color, lv_opa_t opa);
void rounder_cb(lv_disp_drv_t *disp_drv, lv_area_t *area);

class Mini_display_thread : public cpp_freertos::Thread {
public:
    Mini_display_thread(std::string name = "Mini_OLED", uint32_t cycle_time = 250);

    uint32_t Set_timer(int timer_ms);

protected:
    virtual void Run();

private:
    uint32_t cycle_time;

    int timer = 0;
};
