// This file was generated by SquareLine Studio
// SquareLine Studio version: SquareLine Studio 1.4.2
// LVGL version: 8.3.11
// Project name: SquareLine_Project

#include "ui.h"

void ui_TimeDisplay_screen_init(void)
{
    ui_TimeDisplay = lv_obj_create(NULL);
    lv_obj_clear_flag(ui_TimeDisplay, LV_OBJ_FLAG_SCROLLABLE);      /// Flags

    ui_Label2 = lv_label_create(ui_TimeDisplay);
    lv_obj_set_height(ui_Label2, 21);
    lv_obj_set_width(ui_Label2, LV_SIZE_CONTENT);   /// 21
    lv_obj_set_align(ui_Label2, LV_ALIGN_CENTER);
    lv_label_set_text(ui_Label2, "Time: 00:00");

}
