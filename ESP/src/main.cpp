#include <Arduino.h>
#include <ArduinoOTA.h>
#include <esp32_smartdisplay.h>
#include <lvgl.h>

//LVGL Stuffs
static lv_obj_t *label_date;

class Button {
private:
    const char* Label;
    uint16_t PosX;
    uint16_t PosY;
    uint16_t Width;
    uint16_t Height;
    lv_obj_t* button;

public:
    Button(const char* Label, uint16_t PosX, uint16_t PosY, uint16_t Width, uint16_t Height);
    void createButton(lv_obj_t* parent);
    static void OnClicked(lv_event_t *e); 
};

Button::Button(const char* Label, uint16_t PosX, uint16_t PosY, uint16_t Width, uint16_t Height) 
    : Label(Label), PosX(PosX), PosY(PosY), Width(Width), Height(Height), button(nullptr) {
}

void Button::createButton(lv_obj_t* parent) {
    button = lv_btn_create(parent);
    lv_obj_set_pos(button, PosX, PosY);
    lv_obj_set_size(button, Width, Height);
    lv_obj_t* label = lv_label_create(button);
    lv_label_set_text(label, Label);
    lv_obj_center(label);

    lv_obj_add_event_cb(button, OnClicked, LV_EVENT_CLICKED, NULL);
}

void Button::OnClicked(lv_event_t *e) {
    Serial.println("Button clicked!");
}

void display_update() {
    char buffer[32];
    itoa(millis(), buffer, 10);
    lv_label_set_text(label_date, buffer);
}

void mainscreen() {
    lv_obj_clean(lv_scr_act());

    label_date = lv_label_create(lv_scr_act());
    lv_obj_set_style_text_font(label_date, &lv_font_montserrat_22, LV_STATE_DEFAULT);
    lv_obj_align(label_date, LV_ALIGN_BOTTOM_MID, 0, -50);

    Button button1("Button1", 10, 10, 100, 50);
    button1.createButton(lv_scr_act());

    Button button2("Button2", 120, 10, 100, 50);
    button2.createButton(lv_scr_act());

    Button button3("Button3", 230, 10, 100, 50);
    button3.createButton(lv_scr_act());
}

void setup() {
    Serial.begin(9600);
    Serial.println("Serial On");
    smartdisplay_init();
    smartdisplay_set_led_color(lv_color32_t({.ch = {.blue = 0, .green = 0, .red = 0}}));
    mainscreen();
}

void loop() {
    display_update();
    lv_timer_handler();
}
