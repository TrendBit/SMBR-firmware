set(LV_CONF_BUILD_DISABLE_EXAMPLES)
set(LV_CONF_BUILD_DISABLE_DEMOS)
set(LVGL_ROOT_DIR ${CMAKE_SOURCE_DIR}/library/lvgl CACHE STRING "" FORCE)
set(LV_CONF_PATH ${CMAKE_SOURCE_DIR}/source/config/lv_conf.h CACHE STRING "" FORCE)

include(lvgl/env_support/cmake/custom.cmake)


