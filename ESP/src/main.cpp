#include <Arduino.h>
#include <ArduinoOTA.h>
#include <esp32_smartdisplay.h>
#include <lvgl.h>
#include <SD.h>
#include <SPI.h>
#include <vector>

// LVGL Stuffs
static lv_obj_t *label_date;
const char *labelFilePath = "/Labels";
#define TIMEOUT_MS 5000
std::vector<String> buttonLabels;
bool setupReceived = false;

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
    //lv_obj_set_style_local_bg_color(button, LV_BTN_PART_MAIN, LV_STATE_DEFAULT, lv_color_hex(0xFFD1DC));
    lv_obj_t* label = lv_label_create(button);
    lv_label_set_text(label, Label);
    lv_obj_center(label);
    lv_obj_add_event_cb(button, OnClicked, LV_EVENT_CLICKED, NULL);
}

void Button::OnClicked(lv_event_t *e) {
    lv_obj_t* obj = lv_event_get_target(e);
    lv_obj_t* label = lv_obj_get_child(obj, NULL);
    if (label != NULL) {
        const char* labelText = lv_label_get_text(label);
        unsigned long startTime = millis();
        while (millis() - startTime < TIMEOUT_MS) {
            Serial.print("Button clicked: ");
            Serial.println(labelText);
            String receivedMsg;
            if (Serial.available() > 0) {
                receivedMsg = Serial.readStringUntil('\n');
                receivedMsg.trim();
                if (receivedMsg == "ACK") {  // Trim whitespace characters
                    break; 
                }
            }
            delay(25);
        }
    }
    delay(250);
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

    if (SD.begin(5)) {
        File labelsFile = SD.open(labelFilePath);
        if (!labelsFile) {
            Serial.println("Failed to open Labels file");
            return;
        }

        const uint16_t numRows = 4;
        const uint16_t numCols = 3;
        const uint16_t buttonWidth = LV_HOR_RES / numCols;
        const uint16_t buttonHeight = LV_VER_RES / numRows;

        for (uint16_t row = 0; row < numRows; row++) {
            for (uint16_t col = 0; col < numCols; col++) {
                char buttonLabel[16];
                if (labelsFile.available()) {
                    labelsFile.readStringUntil('\n').toCharArray(buttonLabel, sizeof(buttonLabel));
                } else {
                    strcpy(buttonLabel, "Empty");
                }
                Button button(buttonLabel, col * buttonWidth, row * buttonHeight, buttonWidth, buttonHeight);
                button.createButton(lv_scr_act());
            }
        }

        labelsFile.close();
    } else {
        Serial.println("SD card not detected, cannot load labels.");
    }
}

void setup() {
    Serial.begin(9600);
    Serial.println("Serial On");
    if (!SD.begin(5)) {
        Serial.println("SD Card initialization failed!");
    }
    smartdisplay_init();
    smartdisplay_set_led_color(lv_color32_t({.ch = {.blue = 0, .green = 0, .red = 0}}));
    mainscreen();
}

void loop() {
    if (Serial.available() > 0) {
        String message = Serial.readStringUntil('\n');

        if (message.startsWith("Setup:")) {
            setupReceived = true;
            message.remove(0, 6); // Remove "Setup:"

            while (message.length() > 0) {
                int commaIndex = message.indexOf(',');

                if (commaIndex != -1) {
                    String value = message.substring(0, commaIndex);

                    buttonLabels.push_back(value);
                    message.remove(0, commaIndex + 1);
                } else {
                    buttonLabels.push_back(message);
                    message = "";
                }
            }
        }
    }

    if (setupReceived) {
        if (SD.begin(5)) {
            File labelsFile = SD.open(labelFilePath, FILE_WRITE);
            if (labelsFile) {
                for (size_t i = 0; i < buttonLabels.size(); i++) {
                    labelsFile.println(buttonLabels[i]);
                }
                labelsFile.close();
            } else {
                Serial.println("Failed to open Labels file for writing");
            }

            mainscreen();
        } else {
            Serial.println("SD card not detected, cannot save labels.");
        }

        setupReceived = false;
        buttonLabels.clear();
    }

    display_update();
    lv_timer_handler();
}