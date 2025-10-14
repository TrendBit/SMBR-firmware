#include "mini_display_thread.hpp"


#include "resources/trendbit_logo.hpp"
/**
 * @brief   TrendBit logo image declaration
 */
LV_IMG_DECLARE(TrendBit)

void Mini_display_thread::Display_flush(lv_disp_drv_t *disp_drv, const lv_area_t *area, lv_color_t *color_p){
    auto display = Mini_display_thread::Get_instance()->Get_display();

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

    lv_disp_flush_ready(disp_drv);
}

#define BIT_SET(a, b)   ((a) |= (1U << (b)))
#define BIT_CLEAR(a, b) ((a) &= ~(1U << (b)))

void Mini_display_thread::Set_pixel(lv_disp_drv_t *disp_drv, uint8_t *buf, lv_coord_t buf_w, lv_coord_t x, lv_coord_t y, lv_color_t color, lv_opa_t opa){
    UNUSED(disp_drv);
    uint16_t byte_index = x + (( y >> 3 ) * buf_w);
    uint8_t bit_index   = y & 0x7;
    // == 0 inverts, so we get blue on black
    if ((color.full == 0) && (LV_OPA_TRANSP != opa)) {
        BIT_SET(buf[byte_index], bit_index);
    } else {
        BIT_CLEAR(buf[byte_index], bit_index);
    }
}

void Mini_display_thread::Round_area(lv_disp_drv_t *disp_drv, lv_area_t *area){
    UNUSED(disp_drv);
    area->y1 = (area->y1 & (~0x7));
    area->y2 = area->y2 | 0x07;
}

Mini_display_thread::Mini_display_thread(uint32_t cycle_time, std::string name)
    : Thread(name, 4096, 6),
    cycle_time(cycle_time){
    instance = this;
    Start();
}

void Mini_display_thread::Run(){
    Initialize_hardware();
    Initialize_lvgl();
    Initialize_screen_saver();
    Initialize_ui();
    Display_loop();
}

bool Mini_display_thread::Initialize_hardware(){
    i2c = new I2C_bus(i2c0, 16, 17, 400000, true);
    if (!i2c) return false;

    display = new SSD1306(128, 64, *i2c, 0x3c);
    if (!display) return false;

    display->Init();
    display->On();
    display->Clear_all();
    display->Set_contrast(0x8f);

    return true;
}

bool Mini_display_thread::Initialize_lvgl(){
    lv_init();

    // Initialize display buffer
    lv_disp_draw_buf_init(&display_buffer, buffer_memory, nullptr, BUFF_SIZE);

    // Configure display driver
    lv_disp_drv_init(&display_driver);
    display_driver.draw_buf   = &display_buffer;
    display_driver.flush_cb   = Display_flush;
    display_driver.set_px_cb  = Set_pixel;
    display_driver.rounder_cb = Round_area;
    display_driver.hor_res    = 128;
    display_driver.ver_res    = 64;

    return lv_disp_drv_register(&display_driver) != nullptr;
}

void Mini_display_thread::Initialize_screen_saver(){
    // Create delayed execution to return to main screen after screen saver
    auto return_data = new rtos::Delayed_execution(std::function<void()>([this](){
        lv_scr_load_anim(main_screen, LV_SCR_LOAD_ANIM_MOVE_TOP, 2000, 0, false);
    }), 2000, false);

    // Create repeated execution to shift pixels and roll screen saver periodically
    new rtos::Repeated_execution(std::function<void()>([this,return_data](){
        static short phase = 0;

        switch (phase) {
            case 0:
                lv_obj_set_pos(main_screen, 1, 0);
                break;
            case 1:
                lv_obj_set_pos(main_screen, 1, 1);
                break;
            case 2:
                lv_obj_set_pos(main_screen, 0, 1);
                break;
            case 3:
                lv_obj_set_pos(main_screen, 0, 0);
                break;
            case 4:
                // Roll screen saver and execute return to main screen
                lv_scr_load_anim(screen_saver, LV_SCR_LOAD_ANIM_MOVE_TOP, 2000, 0, false);
                return_data->Execute();
                break;
        }

        phase = (phase + 1) % 5;

    }),6 * 60 * 1000, true);

    screen_saver = lv_obj_create(NULL);

    static lv_style_t style_line;
    lv_style_init(&style_line);
    lv_style_set_line_width(&style_line, 3);
    lv_style_set_line_color(&style_line, lv_color_black());
    lv_style_set_line_rounded(&style_line, false);

    static lv_point_t points_top[] = {{0, 3}, {127, 3}};
    lv_obj_t *line_top = lv_line_create(screen_saver);
    lv_line_set_points(line_top, points_top, 2);
    lv_obj_add_style(line_top, &style_line, 0);

    static lv_point_t points_bottom[] = {{0, 61}, {127, 61}};
    lv_obj_t *line_bottom = lv_line_create(screen_saver);
    lv_line_set_points(line_bottom, points_bottom, 2);
    lv_obj_add_style(line_bottom, &style_line, 0);

    LV_IMG_DECLARE(TrendBit)
    lv_obj_t *img = lv_img_create(screen_saver);
    lv_img_set_src(img, &TrendBit);
    lv_obj_align(img, LV_ALIGN_CENTER, 0, 0);
}

void Mini_display_thread::Initialize_ui(){

    main_screen = lv_obj_create(NULL);

    // Create labels
    labels.line_1 = lv_label_create(main_screen);
    labels.line_2 = lv_label_create(main_screen);
    labels.line_3 = lv_label_create(main_screen);
    labels.line_4 = lv_label_create(main_screen);

    // Position labels
    lv_obj_set_pos(labels.line_1, 8, 0);
    lv_obj_set_pos(labels.line_2, 8, 16);
    lv_obj_set_pos(labels.line_3, 8, 32);
    lv_obj_set_pos(labels.line_4, 8, 48);
    lv_label_set_long_mode(labels.line_4, LV_LABEL_LONG_SCROLL);
    lv_obj_set_width(labels.line_4, 116);

    // Set initial values
    Update_SID(0);
    Update_ip({ 0, 0, 0, 0 });
    Update_hostname("none");
    Clear_custom_text();
    Update_serial(0);
    Update_temps();

    // Preview logo and then switch to main screen
    lv_scr_load(screen_saver);
    lv_scr_load_anim(main_screen, LV_SCR_LOAD_ANIM_MOVE_TOP, 2000, 2000, false);
}

void Mini_display_thread::Display_loop(){
    uint32_t last_tick = cpp_freertos::Ticks::GetTicks();

    while (true) {
        rtos::Delay(cycle_time);

        uint32_t current_tick = cpp_freertos::Ticks::GetTicks();
        uint32_t elapsed      = (current_tick >= last_tick) ?
          current_tick - last_tick :
          (UINT32_MAX - last_tick) + current_tick + 1;

        // Update time from last redraw and call LVGL handler
        lv_tick_inc(elapsed);
        lv_timer_handler();

        last_tick = current_tick;
    }
}

void Mini_display_thread::Update_SID(uint16_t sid){
    this->sid = sid;
    Update_ID_line();
}

void Mini_display_thread::Update_serial(uint32_t serial){
    if(custom_text.empty()){
        lv_label_set_text(labels.line_4, emio::format("Serial: {:d}", serial).c_str());
    }
}

void Mini_display_thread::Update_hostname(std::string hostname){
    this->hostname = hostname;
    Update_ID_line();
}

void Mini_display_thread::Update_ip(std::array<uint8_t, 4> ip){
    std::string ip_label = emio::format("IP: {:d}.{:d}.{:d}.{:d}", ip[0], ip[1], ip[2], ip[3]);
    lv_label_set_text(labels.line_2, ip_label.c_str());
}

void Mini_display_thread::Print_custom_text(std::string text){
    custom_text += text;
    // Format to wider with to clear previous text
    lv_label_set_text(labels.line_4, emio::format("{:20s}", custom_text).c_str());
}

void Mini_display_thread::Clear_custom_text(){
    custom_text = "";
    // Format to wider with to clear previous text
    lv_label_set_text(labels.line_4, emio::format("{:20s}", custom_text).c_str());
}

void Mini_display_thread::Update_temps(){
    lv_label_set_text(labels.line_3, emio::format("B{:04.1f}  P{:04.1f}  T{:04.1f}", bottle_temperature, plate_temperature, target_temperature).c_str());
}

void Mini_display_thread::Update_ID_line(){
    lv_label_set_text(labels.line_1, emio::format("{:12s} 0x{:04x}", hostname, sid).c_str());
}
