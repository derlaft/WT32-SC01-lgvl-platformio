#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <TFT_eSPI.h> 
#include <Adafruit_FT6206.h>
#include "lvgl.h"
#include "esp_freertos_hooks.h"

TFT_eSPI tft = TFT_eSPI();
Adafruit_FT6206 touchScreen = Adafruit_FT6206();

static lv_disp_buf_t disp_buf;
static lv_color_t buf[LV_HOR_RES_MAX * 10];

lv_obj_t *btn1;
lv_obj_t *btn2;
lv_obj_t *screenMain;
lv_obj_t *label;

bool my_input_read2(lv_indev_drv_t * drv, lv_indev_data_t*data) {
#ifdef TOUCH_DEBUG
  Serial.println("#");
#endif

  static uint16_t lastx = 0;
  static uint16_t lasty = 0;

  if (!touchScreen.touched()) {
     data->state = LV_INDEV_STATE_REL;
     data->point.x = lastx;
     data->point.y = lasty;
     return false;
  }

  TS_Point touchPos = touchScreen.getPoint();
  data->state = LV_INDEV_STATE_PR;

#ifdef TOUCH_DEBUG
  Serial.println("touch at " + String(touchPos.x) + "x" + String(touchPos.y));
#endif
  auto xpos = touchPos.y;
  auto ypos = TFT_WIDTH - touchPos.x;
#ifdef TOUCH_DEBUG
  Serial.println("mapped to " + String(xpos) + "x" + String(ypos));
#endif

  data->point.x = xpos;
  data->point.y = ypos;
  lastx = xpos;
  lasty = ypos;

  return false;
}

static void event_handler_btn(lv_obj_t * obj, lv_event_t event){
    if(event == LV_EVENT_CLICKED) {
        if (obj == btn1)
          lv_label_set_text(label, "Hello");
        else if (obj == btn2){
          lv_label_set_text(label, "Goodbye");
        }
    }
}

void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p)
{
    uint32_t w = (area->x2 - area->x1 + 1);
    uint32_t h = (area->y2 - area->y1 + 1);

    tft.startWrite();
    tft.setAddrWindow(area->x1, area->y1, w, h);
    tft.pushColors(&color_p->full, w * h, true);
    tft.endWrite();

    lv_disp_flush_ready(disp);
}

static void lv_tick_task(void)
{
 lv_tick_inc(portTICK_RATE_MS);
}

void setupPlatform() {
  esp_err_t err = esp_register_freertos_tick_hook((esp_freertos_tick_cb_t)lv_tick_task); 
}

void setup() {
  lv_disp_drv_t disp_drv;
  lv_indev_drv_t indev_drv;

  Serial.begin(9600);
  lv_init();

  setupPlatform();

  // Enable TFT
  tft.begin();

  tft.setRotation(1);

  // Enable Backlight
  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL,1);

  // Start TouchScreen
  // requires custom I2C pinout
  Wire.begin(TOUCH_SDA, TOUCH_SCL);
  if (!touchScreen.begin(40)) { 
    Serial.println("Unable to start touchscreen.");
  }

  // Display Buffer
  lv_disp_buf_init(&disp_buf, buf, NULL, LV_HOR_RES_MAX * 10);

  // Init Display
  lv_disp_drv_init(&disp_drv);
  disp_drv.hor_res = TFT_HEIGHT;
  disp_drv.ver_res = TFT_WIDTH;
  disp_drv.flush_cb = my_disp_flush;
  disp_drv.buffer = &disp_buf;
  lv_disp_drv_register(&disp_drv);

  // Init Touchscreen
  lv_indev_drv_init(&indev_drv);
  indev_drv.type = LV_INDEV_TYPE_POINTER;
  indev_drv.read_cb = my_input_read2;
  lv_indev_t *indev_drv_t = lv_indev_drv_register(&indev_drv);

  // Screen Object
  screenMain = lv_obj_create(NULL, NULL);

  // Text
  label = lv_label_create(screenMain, NULL);
  lv_label_set_long_mode(label, LV_LABEL_LONG_BREAK);
  lv_label_set_text(label, "Press a button");
  lv_label_set_align(label, LV_LABEL_ALIGN_CENTER);
  lv_obj_set_size(label, 240, 40);
  lv_obj_set_pos(label, 0, 15);

  // BUtton 1
  btn1 = lv_btn_create(screenMain, NULL);
  lv_obj_set_event_cb(btn1, event_handler_btn);
  lv_obj_set_width(btn1, 70);
  lv_obj_set_height(btn1, 32);
  lv_obj_set_pos(btn1, 32, 100);
  lv_obj_t * label1 = lv_label_create(btn1, NULL);
  lv_label_set_text(label1, "Hello");


  // Button 2
  btn2 = lv_btn_create(screenMain, NULL);
  lv_obj_set_event_cb(btn2, event_handler_btn);
  lv_obj_set_width(btn2, 70);
  lv_obj_set_height(btn2, 32);
  lv_obj_set_pos(btn2, 142, 100);
  lv_obj_t * label2 = lv_label_create(btn2, NULL);
  lv_label_set_text(label2, "Goodbye");

  lv_indev_set_cursor(indev_drv_t, label);
  // Screen load
  lv_scr_load(screenMain);
}

void loop() {
  lv_task_handler();
  delay(1);
}
