/**
 * @file mini_display_thread.hpp
 * @author Petr Malaník (TheColonelYoung(at)gmail(dot)com)
 * @version 0.1
 * @date 05.12.2024
 */

#pragma once

#include "mutex.hpp"
#include "thread.hpp"
#include "rtos/wrappers.hpp"
#include "rtos/repeated_execution.hpp"
#include "rtos/delayed_execution.hpp"
#include <cstddef>
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
public:
    enum class Scheduler_state : uint8_t {
        Stopped   = 0x00,
        Paused    = 0x01,
        Running   = 0x02,
    };
    enum class Redraw_segments : uint8_t {
        temps    = 0x01,
        recipe   = 0x02,
        hostname = 0x04,
        version  = 0x08,
        ip       = 0x0f,
        sid      = 0x10
    };
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

    /**
     * @brief   LVGL display buffer configuration
     */
    lv_disp_draw_buf_t display_buffer;

    /**
     * @brief   LVGL display driver configuration
     */
    lv_disp_drv_t display_driver;

    /**
     * @brief   Memory for the display buffer
     */
    lv_color_t buffer_memory[BUFF_SIZE];

    /**
     * @brief   Main screen with data widgets
     */
    lv_obj_t * main_screen;

    /**
     * @brief   Screen saver screen with logo and lines to refresh pixels
     */
    lv_obj_t * screen_saver;

    /**
     * @brief   Main screen structure
     * @note    background is the area in which a certain part is displayed,
     *          all other components of this part should be children of
     *          background
     */
    struct {
        // header with hostname and recipe scheduler state
        struct {
            lv_obj_t * background = nullptr;
            lv_obj_t * title = nullptr;
            lv_obj_t * icon = nullptr;
        } header;

        // info lines
        struct {
            lv_obj_t * background = nullptr;
            lv_obj_t * lines[4] = {nullptr};
        } info;

        // separating line
        lv_obj_t * gap_line = nullptr;

        // temperature lines
        struct {
            lv_obj_t * background = nullptr;
            lv_obj_t * lines[4] = {nullptr};
        } side_col;

        // custom text popup
        struct {
            lv_obj_t * background  = nullptr;
            lv_obj_t * text  = nullptr;
        } popup;
    } ui;

    /**
     * @brief   LVGL style inverting the color of background and text to black on white
     */
    lv_style_t style_inverted;

    /**
     * @brief   LVGL style changing the text to large size
     */
    lv_style_t style_large_text;

    /**
     * @brief   LVGL style changing the text align to center
     */
    lv_style_t style_centered_text;

    /**
     * @brief   Time between display updates in ms
     */
    uint32_t cycle_time;

    /**
     * @brief Singleton instance of the display thread
     */
    inline static Mini_display_thread* instance;

    /**
     * @brief   Custom text to display on the screen
     */
    std::string custom_text = "";

    /**
     * @brief   SID to display
     */
    uint16_t sid = 0x0000;

    /**
     * @brief   Hostname of device to display
     */
    std::string hostname = "none";

    /**
     * @brief   Loaded recipe name ("" if there is no recipe loaded)
     */
    std::string loaded_recipe = "";

    /**
     * @brief   Formatted IP address string to display
     */
    std::string ip_label = "";

    /**
     * @brief   Target temperature to display
     */
    float target_temperature = std::numeric_limits<float>::infinity();

    /**
     * @brief   Heater plate temperature to display
     */
    float plate_temperature = 0.0f;

    /**
     * @brief   Bottle temperature to display
     */
    float bottle_temperature = 0.0f;

    /**
     * @brief   Fluorometer temperature to display (emitor)
     */
    float fluorometer_temperature = 0.0f;

    /**
     * @brief   State of the scheduler (use stopped if no script is loaded)
     */
    Scheduler_state scheduler_state = Scheduler_state::Stopped;

    /**
     * @brief   Flag that controls if certain parts of the display should be redrawn
     *  temps    = 0x01
     *  recipe   = 0x02
     *  hostname = 0x04
     *  version  = 0x08
     *  ip       = 0x0f
     *  sid      = 0x10
     */
    uint8_t redraws = 0x00;

    /**
     * @brief   Mutex, locking the displayed values while LVGL is redrawing display
     */
    cpp_freertos::MutexStandard lvgl_render_mtx;

public:
    /**
     * @brief Create display thread instance
     *
     * @param cycle_time Time between display updates in ms
     * @param name Thread name
     */
    Mini_display_thread(uint32_t cycle_time = 100, std::string name = "Display");

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

    /**
     * @brief Update the display with custom text
     *
     * @param text Text to display
     */
    void Print_custom_text(std::string text);

    /**
     * @brief Clear the custom text from the display
     */
    void Clear_custom_text();

    /**
     * @brief   Set the loaded recipe. Use "" if no recipe is loaded.
     * 
     * @param recipe_name   Name of the new recipe.
     */
    void Update_recipe(std::string recipe_name);

    /**
     * @brief   Set the recipe scheduler state.
     * 
     * @param state   New state of the scheduler.
     */
    void Update_scheduler_state(Scheduler_state state);
    
    /**
     * @brief   Set the target temperature of heater to display
     *
     * @param temperature   Target temperature of heater to display
     */
    void Update_target_temperature(float temperature);

    /**
     * @brief   Set the heater plate temperature to display
     *
     * @param temperature   Heater plate temperature to display
     */
    void Update_plate_temperature(float temperature);

    /**
     * @brief   Set the bottle temperature to display
     *
     * @param temperature   Bottle temperature to display
     */
    void Update_bottle_temperature(float temperature);

    /**
     * @brief   Set the fluorometer emitor temperature to display
     *
     * @param temperature   Fluorometer emitor temperature to display
     */
    void Update_fluorometer_temperature(float temperature);

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
     * @brief Initialize LVGL styles
     */
    void Initialize_styles();

    /**
     * @brief Initialize user interface elements for data screen
     */
    void Initialize_ui();

    /**
     * @brief Initialize screen saver screen
     *        Display is OLED so pixels needs to be turned on and off periodically to prevent burn-in
     *            Burn-in can be observed as lower intensity of pixels which are not changed for long time
     *        Screen saver will move data screen by one pixel in all direction and sometimes roll logo
     *             over screen to refresh all pixels over time
     */
    void Initialize_screen_saver();

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


    /**
     * @brief Update displayed value temperature lines
     */
    void Redraw_Temperature_lines();
    
    /**
     * @brief Update displayed value for hostname line
     */
    void Redraw_Hostname_line();

    /**
     * @brief Update displayed values for lines relating to current recipe and scheduler state
     */
    void Redraw_Recipe_lines();

    /**
     * @brief Update displayed value for SID line
     */
    void Redraw_SID_line();

    /**
     * @brief Update displayed value for IP line
     */
    void Redraw_IP_line();

    /**
     * @brief Update displayed value for Version line
     */
    void Redraw_Version_line();
};
