#include "mini_display_thread.hpp"

#include "ticks.hpp"

#define BUFF_SIZE (128 * 10)

// Frame buffers
/*A static or global variable to store the buffers*/
inline static lv_disp_draw_buf_t disp_buf;
/*A variable to hold the drivers. Must be static or global.*/
inline static lv_disp_drv_t disp_drv;

/*Static or global buffer(s). The second buffer is optional*/
inline static lv_color_t buf_1[BUFF_SIZE];

inline SSD1306 *display;

void flush_cb(lv_disp_drv_t *disp_drv, const lv_area_t *area, lv_color_t *color_p){
    /*The most simple case (but also the slowest) to put all pixels to the screen one-by-one
     *`put_px` is just an example, it needs to be implemented by you.*/
    int32_t x, y;
    uint8_t *buffer       = (uint8_t *) color_p;
    const int max_columns = 64;

    for (y = area->y1; y <= area->y2; y += 8) {
        int columns = area->x2 - area->x1;

        for (x = 0; x <= columns / max_columns; x++) {
            int column_remains  = (columns - x * max_columns);
            int section_columns = column_remains % max_columns;
            if (column_remains >= max_columns) {
                section_columns = max_columns;
            }
            display->Set_address(y / 8, area->x1 + x * max_columns);
            std::vector<uint8_t> data(buffer, buffer + section_columns);
            display->Set_content(data);
            rtos::Yield();
            buffer += section_columns;
        }
    }

    /* IMPORTANT!!!
     * Inform LVGL that you are ready with the flushing and buf is not used anymore*/
    lv_disp_flush_ready(disp_drv);
}

#define BIT_SET(a, b)   ((a) |= (1U << (b)))
#define BIT_CLEAR(a, b) ((a) &= ~(1U << (b)))
void set_px_cb(lv_disp_drv_t *disp_drv, uint8_t *buf, lv_coord_t buf_w, lv_coord_t x, lv_coord_t y, lv_color_t color, lv_opa_t opa){
    UNUSED(disp_drv);
    UNUSED(opa);
    uint16_t byte_index = x + (( y >> 3 ) * buf_w);
    uint8_t bit_index   = y & 0x7;
    // == 0 inverts, so we get blue on black
    if (color.full == 0) {
        BIT_SET(buf[ byte_index ], bit_index);
    } else {
        BIT_CLEAR(buf[ byte_index ], bit_index);
    }
}

void rounder_cb(lv_disp_drv_t *disp_drv, lv_area_t *area){
    UNUSED(disp_drv);
    area->y1 = (area->y1 & (~0x7));
    area->y2 = (area->y2 & (~0x7)) + 7;
}

Mini_display_thread::Mini_display_thread(std::string name, uint32_t cycle_time)
    : Thread(name, 4096, 6),
    cycle_time(cycle_time){
    Start();
};

void Mini_display_thread::Run(){
    I2C_bus *i2c = new I2C_bus(i2c0, 16, 17, 1000000, true);

    display = new SSD1306(128, 64, *i2c, 0x3c);
    display->Init();
    display->On();
    display->Clear_all();
    display->Set_contrast(0x8f);

    lv_init();
    lv_disp_draw_buf_init(&disp_buf, buf_1, NULL, BUFF_SIZE);
    lv_disp_drv_init(&disp_drv);     /*Basic initialization*/
    disp_drv.draw_buf   = &disp_buf; /*Set an initialized buffer*/
    disp_drv.flush_cb   = flush_cb;  /*Set a flush callback to draw to the display*/
    disp_drv.set_px_cb  = set_px_cb;
    disp_drv.rounder_cb = rounder_cb;
    disp_drv.hor_res    = 128;       /*Set the horizontal resolution in pixels*/
    disp_drv.ver_res    = 64;        /*Set the vertical resolution in pixels*/

    lv_disp_drv_register(&disp_drv); /*Register the driver and save the created display objects*/

    lv_obj_t *label_top;
    lv_obj_t *label_bottom;
    lv_obj_t *label_ambient;
    lv_obj_t *label_timer;

    label_top     = lv_label_create(lv_scr_act());
    label_bottom  = lv_label_create(lv_scr_act());
    label_ambient = lv_label_create(lv_scr_act());
    label_timer   = lv_label_create(lv_scr_act());

    lv_label_set_text(label_top, "");
    lv_obj_set_pos(label_top, 8, 0);

    lv_label_set_text(label_bottom, "");
    lv_obj_set_pos(label_bottom, 8, 16);

    lv_label_set_text(label_ambient, "");
    lv_obj_set_pos(label_ambient, 8, 32);

    lv_label_set_text(label_timer, "");
    lv_obj_set_pos(label_timer, 8, 48);

    unsigned int current_ticks = 0;
    unsigned int time_past_ms = cycle_time;

    while (1) {
        unsigned int past_ticks = cpp_freertos::Ticks::GetTicks();
        rtos::Delay(cycle_time);
        timer -= time_past_ms;
        lv_tick_inc(time_past_ms);
        lv_timer_handler();

        if (timer <= 0) {
            timer = 0;
        }

        double top_temp = 1.0;
        std::string top_temp_label = emio::format("Top: {:.1f}°C", top_temp);

        double bottom_temp = 2.0;
        std::string bottom_temp_label = emio::format("Bottom: {:.1f}°C", bottom_temp);

        double ambient_temp = 3.0;
        std::string ambient_temp_label = emio::format("Ambient: {:.1f}°C", ambient_temp);

        std::string time = emio::format("Timer: {:02}:{:02}", (timer / 1000) / 60, (timer / 1000) % 60);

        lv_label_set_text(label_top, top_temp_label.c_str());
        lv_label_set_text(label_bottom, bottom_temp_label.c_str());
        lv_label_set_text(label_ambient, ambient_temp_label.c_str());
        lv_label_set_text(label_timer, time.c_str());

        current_ticks = cpp_freertos::Ticks::GetTicks();
        time_past_ms = current_ticks - past_ticks;
    }
}; // Run

uint32_t Mini_display_thread::Set_timer(int timer_ms){
    timer = timer_ms;
    return timer;
};
