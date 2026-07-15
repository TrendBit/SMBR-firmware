#include "mini_display_thread.hpp"
#include "config.hpp"


#include <cmath>
#include <cstdio>
#include <src/core/lv_obj_style.h>
#include <src/font/lv_symbol_def.h>
#include <src/misc/lv_color.h>
#include <src/misc/lv_style.h>
#include <src/misc/lv_txt.h>
#include <src/widgets/lv_label.h>
#include <src/widgets/lv_line.h>
#include <string>
#include "resources/trendbit_logo.hpp"
#include <src/core/lv_obj.h>
#include <src/core/lv_obj_pos.h>
#include <src/font/lv_font.h>
#include <src/misc/lv_area.h>
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
    Initialize_styles();
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

void lv_log_callback(const char* msg){
    std::string message = msg;
    message.pop_back(); //remove endline
    Logger::Error("LVGL LOG: {}",message);
}

bool Mini_display_thread::Initialize_lvgl(){
    lv_init();
    lv_log_register_print_cb(lv_log_callback);

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

void Mini_display_thread::Initialize_styles(void){
    lv_style_init(&style_inverted);

    lv_style_set_bg_color(&style_inverted, lv_color_black());
    lv_style_set_bg_opa(&style_inverted, LV_OPA_COVER);

    lv_style_set_text_color(&style_inverted, lv_color_white());

    lv_style_init(&style_large_text);

    lv_style_set_text_font(&style_large_text, &lv_font_montserrat_12);

    lv_style_init(&style_centered_text);
    lv_style_set_text_align(&style_centered_text, LV_TEXT_ALIGN_CENTER);
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

const short full_width = 126;
const short full_height = 62;
const short main_col_width = 90;
const short col_gap = 1;
const short side_col_width = full_width - main_col_width - col_gap;
const short small_text_height = 12;
const short large_text_height = 14;

void Mini_display_thread::Initialize_ui(){

    main_screen = lv_obj_create(NULL);
    // header
    ui.header.background = lv_obj_create(main_screen);
    lv_obj_set_pos(ui.header.background, 0, 0);
    lv_obj_set_size(ui.header.background, main_col_width, large_text_height);
    lv_obj_add_style(ui.header.background, &style_inverted, 0);
    
    ui.header.title = lv_label_create(ui.header.background);
    lv_obj_set_pos(ui.header.title, 1, 0);
    lv_obj_add_style(ui.header.title, &style_large_text, 0);
    
    ui.header.icon = lv_label_create(ui.header.background);
    lv_obj_set_align(ui.header.icon, LV_ALIGN_RIGHT_MID);

    // info lines
    ui.info.background = lv_obj_create(main_screen);
    lv_obj_set_size(ui.info.background, main_col_width, full_height - large_text_height);
    lv_obj_set_pos(ui.info.background, 0, large_text_height);
    for (size_t i = 0; i < std::size(ui.info.lines); i++){
        auto& line = ui.info.lines[i];
        line = lv_label_create(ui.info.background);
        lv_obj_set_pos(line, 0, (i)*small_text_height);
        lv_obj_set_width(line, main_col_width);
    }
    lv_label_set_long_mode(ui.info.lines[0], LV_LABEL_LONG_SCROLL);
    lv_obj_add_style(ui.info.lines[0], &style_centered_text, 0);

    // gap line
    ui.gap_line = lv_obj_create(main_screen);
    lv_obj_set_pos(ui.gap_line, main_col_width, 0);
    lv_obj_set_size(ui.gap_line, col_gap, full_height);
    lv_obj_add_style(ui.gap_line, &style_inverted, 0);
    
    // side column
    ui.side_col.background = lv_obj_create(main_screen);
    lv_obj_set_pos(ui.side_col.background, main_col_width + col_gap, 0);
    lv_obj_set_size(ui.side_col.background, side_col_width, full_height);
    for (size_t i = 0; i < std::size(ui.side_col.lines); i++){
        auto& line = ui.side_col.lines[i];
        line = lv_label_create(ui.side_col.background);
        lv_obj_set_pos(line, 1, i*small_text_height);
    }

    ui.popup.background = lv_obj_create(main_screen);
    lv_obj_set_size(ui.popup.background, full_width, large_text_height);
    lv_obj_set_pos(ui.popup.background, 0, full_height-large_text_height);
    lv_obj_add_style(ui.popup.background, &style_inverted, 0);
    lv_obj_add_style(ui.popup.background, &style_large_text, 0);

    ui.popup.text = lv_label_create(ui.popup.background);
    lv_obj_set_width(ui.popup.text, full_width);
    lv_label_set_long_mode(ui.popup.text, LV_LABEL_LONG_SCROLL);
    lv_obj_add_style(ui.popup.text, &style_centered_text, 0);

    // Set initial values
    Update_SID(0);
    Update_ip({ 0, 0, 0, 0 });
    Update_hostname("--none--");
    Clear_custom_text();
    Redraw_Temperature_lines();
    Redraw_Hostname_line();
    Redraw_Recipe_lines();
    Redraw_SID_line();
    Redraw_IP_line();
    Redraw_Version_line();

    // Preview logo and then switch to main screen
    lv_scr_load(screen_saver);
    lv_scr_load_anim(main_screen, LV_SCR_LOAD_ANIM_MOVE_TOP, 2000, 2000, false);
}

void Mini_display_thread::Display_loop(){
    uint32_t last_tick = cpp_freertos::Ticks::GetTicks();

    while (true) {
        rtos::Delay(cycle_time);
        
        if(redraws){
            cpp_freertos::LockGuard lock(lvgl_render_mtx);
            
            if(redraws & 0x01){
                Redraw_Recipe_lines();
            }
            if(redraws & 0x02){
                Redraw_Temperature_lines();
            }
            if(redraws & 0x04){
                Redraw_Hostname_line();
            }
            if(redraws & 0x08){
                Redraw_Version_line();
            }
            if(redraws & 0x0f){
                Redraw_IP_line();
            }
            if(redraws & 0x10){
                Redraw_SID_line();
            }
            // redraw the whole screen or else the whole screen distorts
            lv_obj_invalidate(main_screen);
            redraws = 0;
        }

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
    cpp_freertos::LockGuard lock(lvgl_render_mtx);
    this->sid = sid;
    redraws = redraws | 0x10;
}

void Mini_display_thread::Update_hostname(std::string hostname){
    cpp_freertos::LockGuard lock(lvgl_render_mtx);
    this->hostname = hostname;
    redraws = redraws | 0x04;
}

void Mini_display_thread::Update_ip(std::array<uint8_t, 4> ip){
    cpp_freertos::LockGuard lock(lvgl_render_mtx);
    this->ip_label = emio::format("{:d}.{:d}.{:d}.{:d}", ip[0], ip[1], ip[2], ip[3]);
    redraws = redraws | 0x0f;
}

void Mini_display_thread::Update_recipe(std::string recipe_name){
    cpp_freertos::LockGuard lock(lvgl_render_mtx);
    loaded_recipe = recipe_name;
    redraws = redraws | 0x01;
}
void Mini_display_thread::Update_scheduler_state(Scheduler_state state){
    cpp_freertos::LockGuard lock(lvgl_render_mtx);
    scheduler_state = state;
    redraws = redraws | 0x01;
}

void Mini_display_thread::Update_target_temperature(float temperature){
    cpp_freertos::LockGuard lock(lvgl_render_mtx);
    target_temperature = temperature;
    redraws = redraws | 0x02;
}
void Mini_display_thread::Update_plate_temperature(float temperature){
    cpp_freertos::LockGuard lock(lvgl_render_mtx);
    plate_temperature = temperature;
    redraws = redraws | 0x02;
}
void Mini_display_thread::Update_bottle_temperature(float temperature){
    cpp_freertos::LockGuard lock(lvgl_render_mtx);
    bottle_temperature = temperature;
    redraws = redraws | 0x02;
}
void Mini_display_thread::Update_fluorometer_temperature(float temperature){
    cpp_freertos::LockGuard lock(lvgl_render_mtx);
    fluorometer_temperature = temperature;
    redraws = redraws | 0x02;
}

void Mini_display_thread::Print_custom_text(std::string text){
    custom_text += text;
    // Format to wider with to clear previous text
    lv_label_set_text(ui.popup.text, emio::format("{}", custom_text).c_str());
    lv_obj_set_pos(ui.popup.background, 0, full_height - large_text_height);
}

void Mini_display_thread::Clear_custom_text(){
    custom_text = "";
    // Format to wider with to clear previous text
    lv_label_set_text(ui.popup.text, emio::format("{:20s}", custom_text).c_str());
    lv_obj_set_pos(ui.popup.background, 0, full_height*2);
}

void Mini_display_thread::Redraw_Temperature_lines(){
    lv_label_set_text(ui.side_col.lines[0], emio::format("{:04.1f}",bottle_temperature).c_str());
    lv_label_set_text(ui.side_col.lines[1], emio::format("{:04.1f}",plate_temperature).c_str());
    if(std::isinf(target_temperature) || std::isnan(target_temperature)){ //std::isinf does not detect this correctly, thus this workaround
        lv_label_set_text(ui.side_col.lines[2], "---");
    }else{
        lv_label_set_text(ui.side_col.lines[2], emio::format("{:04.1f}",target_temperature).c_str());
    }
    lv_label_set_text(ui.side_col.lines[3], emio::format("{:04.1f}",fluorometer_temperature).c_str());
}

void Mini_display_thread::Redraw_Hostname_line(){
    lv_label_set_text(ui.header.title, emio::format("{:8s}", hostname).c_str());
}

void Mini_display_thread::Redraw_Recipe_lines(){
    switch (scheduler_state) {
        case Mini_display_thread::Scheduler_state::Stopped:{
            lv_label_set_text(ui.header.icon, LV_SYMBOL_STOP);
        } break;
        
        case Mini_display_thread::Scheduler_state::Paused:{
            lv_label_set_text(ui.header.icon, LV_SYMBOL_PAUSE);
        } break;
        
        case Mini_display_thread::Scheduler_state::Running:{
            lv_label_set_text(ui.header.icon, LV_SYMBOL_PLAY);
        } break;
        
        default:{
            lv_label_set_text(ui.header.icon, "?");
        }break;
    }
    lv_label_set_text(ui.info.lines[0], emio::format("{}", (loaded_recipe=="")?"--no recipe--":loaded_recipe).c_str());
}

void Mini_display_thread::Redraw_IP_line(){
    lv_label_set_text(ui.info.lines[2], emio::format("IP: {}", ip_label).c_str());
}

void Mini_display_thread::Redraw_Version_line(){
    lv_label_set_text(ui.info.lines[1], emio::format("Ver: {:d}.{:d}.{:d}", FW_VERSION_MAJOR, FW_VERSION_MINOR, FW_VERSION_PATCH).c_str());
}

void Mini_display_thread::Redraw_SID_line(){
    lv_label_set_text(ui.info.lines[3], emio::format("SID: 0x{:04x}", sid).c_str());
}