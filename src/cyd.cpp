#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>
#include "cyd.h"

// LVGL display buffer and resolution
#define TFT_HOR_RES 320
#define TFT_VER_RES 240
#define DRAW_BUF_SIZE (TFT_HOR_RES * TFT_VER_RES / 10 * (LV_COLOR_DEPTH / 8))

// RGB LED Pins
#define CYD_LED_BLUE 17
#define CYD_LED_RED 4
#define CYD_LED_GREEN 16

// Color Filters
#define RED_FILTER 0.25
#define GREEN_FILTER 1
#define BLUE_FILTER 0.4

lv_color_t sliderColor = lv_color_make(255, 255, 255);
lv_obj_t *sliders[3];
lv_obj_t *labels[3];

SoftSPI SoftwareSpi(XPT2046_MOSI, XPT2046_MISO, XPT2046_CLK);
XPT2046_Touchscreen ts(XPT2046_CS, XPT2046_IRQ);
TFT_eSPI tft = TFT_eSPI();

// Custom touchpad read for LVGL
void my_touchpad_read(lv_indev_t *indev, lv_indev_data_t *data)
{
  if (ts.tirqTouched() && ts.touched())
  {
    TS_Point p = ts.getPoint();
    data->point.x = map(p.x, 200, 3700, 0, TFT_HOR_RES);
    data->point.y = map(p.y, 240, 3800, 0, TFT_VER_RES);
    data->state = LV_INDEV_STATE_PRESSED;
  }
  else
  {
    data->state = LV_INDEV_STATE_RELEASED;
  }
}

// Slider event callback
static void slider_event_cb(lv_event_t *e)
{
  int red = lv_slider_get_value(sliders[0]);
  int green = lv_slider_get_value(sliders[1]);
  int blue = lv_slider_get_value(sliders[2]);

  sliderColor = lv_color_make(red, green, blue);

  apply_slider_styles(sliders[0]);
  apply_slider_styles(sliders[1]);
  apply_slider_styles(sliders[2]);

  update_label_text(labels[0], 'R', red);
  update_label_text(labels[1], 'G', green);
  update_label_text(labels[2], 'B', blue);
}

// Helper functions
void apply_slider_styles(lv_obj_t *slider)
{
  lv_obj_set_style_bg_color(slider, sliderColor, LV_PART_MAIN);
  lv_obj_set_style_bg_color(slider, sliderColor, LV_PART_INDICATOR);
  lv_obj_set_style_bg_color(slider, lv_color_lighten(sliderColor, LV_OPA_30), LV_PART_KNOB);
}

void update_label_text(lv_obj_t *label, char symbol, int number)
{
  char buf[128];
  snprintf(buf, sizeof(buf), "%c: %d", symbol, number);
  lv_label_set_text(label, buf);
}

void setup_ui(lv_obj_t *parent)
{
  for (int i = 0; i < 3; i++)
  {
    sliders[i] = lv_slider_create(parent);
    lv_obj_set_width(sliders[i], 220);
    lv_obj_align(sliders[i], LV_ALIGN_TOP_MID, 0, 40 + i * 50);
    lv_slider_set_range(sliders[i], 0, 255);
    lv_slider_set_value(sliders[i], 255, LV_ANIM_OFF);
    apply_slider_styles(sliders[i]);
    lv_obj_add_event_cb(sliders[i], slider_event_cb, LV_EVENT_VALUE_CHANGED, NULL);

    labels[i] = lv_label_create(parent);
    lv_obj_set_width(labels[i], 42);
    lv_obj_align(labels[i], LV_ALIGN_BOTTOM_LEFT, 50 + i * 50, -30);
    char buf[16];
    snprintf(buf, sizeof(buf), "%c: 255", 'R' + i);
    lv_label_set_text(labels[i], buf);
  }
}

void initCydHardware()
{

  // Start the SPI for the touch screen and init the TS library
  SoftwareSpi.setDataMode(SPI_MODE2);
  SoftwareSpi.setBitOrder(1);
  SoftwareSpi.setFrequency(2500000);
  SoftwareSpi.setClockDivider(SPI_CLOCK_DIV2);
  SoftwareSpi.begin();
  ts.begin(SoftwareSpi);
  ts.setRotation(0);

// setup backlight
#if ESP_IDF_VERSION_MAJOR == 5
  ledcAttach(LCD_BACK_LIGHT_PIN, LEDC_BASE_FREQ, LEDC_TIMER_12_BIT);
#else
  ledcSetup(LEDC_CHANNEL_0, LEDC_BASE_FREQ, LEDC_TIMER_12_BIT);
  ledcAttachPin(LCD_BACK_LIGHT_PIN, LEDC_CHANNEL_0);
#endif

  // write to backlight
  ledcAnalogWrite(LEDC_CHANNEL_0, 100);

  // setup ldr
  analogSetAttenuation(ADC_0db);

  pinMode(LDR_PIN, INPUT);

  // Start the tft display and set it to black
  tft.init(3);

  // Clear the screen before writing to it
  tft.fillScreen(TFT_BLACK);
}

void ledcAnalogWrite(uint8_t channel, uint32_t value, uint32_t valueMax)
{
  // calculate duty, 4095 from 2 ^ 12 - 1
  uint32_t duty = (4095 / valueMax) * min(value, valueMax);

  // write duty to LEDC
  ledcWrite(channel, duty);
}

int readLdr()
{
  return analogRead(LDR_PIN);
}

void initUi(lv_obj_t *parent)
{
  for (int i = 0; i < 3; i++)
  {
    sliders[i] = lv_slider_create(parent);
    lv_obj_set_width(sliders[i], 220);
    lv_obj_align(sliders[i], LV_ALIGN_TOP_MID, 0, 40 + i * 50);
    lv_slider_set_range(sliders[i], 0, 255);
    lv_slider_set_value(sliders[i], 255, LV_ANIM_OFF);
    apply_slider_styles(sliders[i]);
    lv_obj_add_event_cb(sliders[i], slider_event_cb, LV_EVENT_VALUE_CHANGED, NULL);

    labels[i] = lv_label_create(parent);
    lv_obj_set_width(labels[i], 42);
    char buf[16];
    snprintf(buf, sizeof(buf), "%c: 255", 'R' + i);
    lv_label_set_text(labels[i], buf);
  }
}

void setupUi()
{
  lv_init();
  uint8_t *draw_buf = new uint8_t[DRAW_BUF_SIZE];
  lv_display_t *disp = lv_tft_espi_create(TFT_HOR_RES, TFT_VER_RES, draw_buf, DRAW_BUF_SIZE);
  lv_display_set_rotation(disp, LV_DISPLAY_ROTATION_270);

  lv_obj_t *scr = lv_scr_act();
  lv_obj_set_style_bg_color(scr, lv_color_black(), 0);

  lv_indev_t *indev = lv_indev_create();
  lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
  lv_indev_set_read_cb(indev, my_touchpad_read);

  setup_ui(scr);
}

void loopBacklight()
{
  if (readLdr() > 300)
  {
    ledcAnalogWrite(LEDC_CHANNEL_0, 0);
  }
  else
  {
    ledcAnalogWrite(LEDC_CHANNEL_0, 150);
  }
}

uint32_t lastTick = 0;

void loopDisplay(void *param)
{
  for (;;)
  {
    lv_tick_inc(millis() - lastTick); // update the tick timer
    lastTick = millis();
    lv_timer_handler();

    analogWrite(CYD_LED_RED, 255 - (sliderColor.red * RED_FILTER));
    analogWrite(CYD_LED_GREEN, 255 - (sliderColor.green * GREEN_FILTER));
    analogWrite(CYD_LED_BLUE, 255 - (sliderColor.blue * BLUE_FILTER));
    vTaskDelay(10 / portTICK_PERIOD_MS); // Add a dela
  }
}