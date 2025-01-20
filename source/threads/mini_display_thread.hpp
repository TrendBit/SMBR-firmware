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
#include "logger.hpp"

#define UNUSED(x) (void)(x)

#define BUFF_SIZE (128 * 10)

/**
 * @brief   Thread handling the mini OLED display operations and UI updates
 *          Manages display initialization, LVGL integration, and periodic UI updates
 */
class Mini_display_thread : public cpp_freertos::Thread {
private:
    /**
     * @brief  I2C bus instance for communication with the display
     *
     */
    I2C_bus *i2c = nullptr;

    /**
     * @brief   Display driver instance
     */
    SSD1306 *display = nullptr;

    // Display buffer
    lv_disp_draw_buf_t display_buffer;
    lv_disp_drv_t display_driver;
    lv_color_t buffer_memory[BUFF_SIZE];

    /**
     * @brief   User interface labels
     */
    struct {
        lv_obj_t* sid;
        lv_obj_t* serial;
        lv_obj_t* hostname;
        lv_obj_t* ip;
    } labels;

    /**
     * @brief   Time between display updates in ms
     */
    uint32_t cycle_time;

    /**
     * @brief Singleton instance of the display thread
     */
    inline static Mini_display_thread* instance;

public:
    /**
     * @brief Create display thread instance
     *
     * @param cycle_time Time between display updates in ms
     * @param name Thread name
     */
    Mini_display_thread(uint32_t cycle_time = 200, std::string name = "Display");

    /**
     * @brief Get singleton instance of the display thread
     *
     * @return Mini_display_thread* Pointer to the singleton instance
     */
    static Mini_display_thread* Get_instance() { return instance; }

    /**
     * @brief Get pointer to the display driver instance
     *
     * @return SSD1306* Pointer to the display driver
     */
    SSD1306 * Get_display() { return display; }

    /**
     * @brief Update the System ID displayed on screen
     *
     * @param sid System ID to display
     */
    void Update_SID(uint16_t sid);

    /**
     * @brief Update the serial number displayed on screen
     *
     * @param serial Serial number to display
     */
    void Update_serial(uint32_t serial);

    /**
     * @brief Update the hostname displayed on screen
     *
     * @param hostname Hostname string to display
     */
    void Update_hostname(std::string hostname);

    /**
     * @brief Update the IP address displayed on screen
     *
     * @param ip Array containing the 4 octets of the IP address
     */
    void Update_ip(std::array<uint8_t, 4> ip);

protected:
    /**
     * @brief Main thread execution function
     *        Handles the display update loop
     */
    void Run() override;

private:
    /**
     * @brief Initialize display hardware components
     *
     * @return true If initialization was successful
     * @return false If initialization failed
     */
    bool Initialize_hardware();

    /**
     * @brief Initialize LVGL library and configure display driver
     *
     * @return true If initialization was successful
     * @return false If initialization failed
     */
    bool Initialize_lvgl();

    /**
     * @brief Initialize user interface elements
     */
    void Initialize_ui();

    /**
     * @brief Main display update loop function
     */
    void Display_loop();

    /**
     * @brief LVGL callback for flushing display buffer
     *
     * @param drv Display driver instance
     * @param area Area to flush
     * @param pixels Pixel data to flush
     */
    static void Display_flush(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *pixels);

    /**
     * @brief LVGL callback for setting individual pixels
     *
     * @param drv Display driver instance
     * @param buf Display buffer
     * @param buf_w Buffer width
     * @param x X coordinate
     * @param y Y coordinate
     * @param color Pixel color
     * @param opa Pixel opacity
     */
    static void Set_pixel(lv_disp_drv_t *drv, uint8_t *buf, lv_coord_t buf_w,
                         lv_coord_t x, lv_coord_t y, lv_color_t color, lv_opa_t opa);

    /**
     * @brief LVGL callback for rounding display areas
     *
     * @param drv Display driver instance
     * @param area Area to round
     */
    static void Round_area(lv_disp_drv_t *drv, lv_area_t *area);
};
