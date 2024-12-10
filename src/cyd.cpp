#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>
#include "cyd.h"

// LVGL display buffer and resolution
#define TFT_HOR_RES 240
#define TFT_VER_RES 320
#define DRAW_BUF_SIZE (TFT_HOR_RES * TFT_VER_RES / 10 * (LV_COLOR_DEPTH / 8))

lv_color_t sliderColor = lv_color_make(255, 255, 255);
lv_obj_t *slider;
lv_obj_t *label;

TFT_eSPI tft = TFT_eSPI();
SoftSPI SoftwareSpi(XPT2046_MOSI, XPT2046_MISO, XPT2046_CLK);
XPT2046_Touchscreen ts(XPT2046_CS, XPT2046_IRQ);

uint16_t touchScreenMinimumX = 200, touchScreenMaximumX = 3700, touchScreenMinimumY = 280, touchScreenMaximumY = 3800;

void my_touchpad_read(lv_indev_t *indev, lv_indev_data_t *data)
{
  if (ts.touched())
  {
    TS_Point p = ts.getPoint();
    data->point.x = map(p.x, touchScreenMinimumX, touchScreenMaximumX, 0, TFT_HOR_RES);
    data->point.y = map(p.y, touchScreenMinimumY, touchScreenMaximumY, 0, TFT_VER_RES);
    data->state = LV_INDEV_STATE_PRESSED;
  }
  else
  {
    data->state = LV_INDEV_STATE_RELEASED;
  }
}

static void slider_event_cb(lv_event_t *e)
{
  lv_obj_t *slider = (lv_obj_t *)lv_event_get_target(e);
  brightness = lv_slider_get_value(slider); // Update brightness value
  char buf[32];
  snprintf(buf, sizeof(buf), "Brightness: %d", brightness);
  lv_label_set_text(label, buf);
}

static void slider_released_cb(lv_event_t *e)
{
  setBrightness(brightness); // Apply brightness when released
}

void update_slider_from_variable()
{
  state = stateStore->get(myBulbId);
  int currentBrightness = state->getBrightness();

  // Sync the slider's position with the shared variable
  if (lv_slider_get_value(slider) != currentBrightness && ts.touched() == false)
  {
    lv_slider_set_value(slider, currentBrightness, LV_ANIM_ON);
    char buf[32];
    snprintf(buf, sizeof(buf), "Brightness: %d", currentBrightness);
    lv_label_set_text(label, buf); // Update the label to reflect the new value
    Serial.println("updating slider: " + String(currentBrightness) + " " + String(lv_slider_get_value(slider)));
  }
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

// Setup UI function
void initUi(lv_obj_t *parent)
{
  state = stateStore->get(myBulbId);
  int currentBrightness = state->getBrightness();
  label = lv_label_create(parent);
  char buf[32];
  snprintf(buf, sizeof(buf), "Brightness: %d", currentBrightness);
  lv_label_set_text(label, buf);
  lv_obj_set_style_text_color(label, lv_color_white(), 0); // Set text color to white
  lv_obj_align(label, LV_ALIGN_CENTER, 0, -20);

  slider = lv_slider_create(parent);
  lv_slider_set_range(slider, 0, 100);
  lv_slider_set_value(slider, currentBrightness, LV_ANIM_ON);
  lv_obj_set_width(slider, 220);
  lv_obj_align(slider, LV_ALIGN_CENTER, 0, 0);
  apply_slider_styles(slider);
  // Add event callbacks
  lv_obj_add_event_cb(slider, slider_event_cb, LV_EVENT_VALUE_CHANGED, NULL);
  lv_obj_add_event_cb(slider, slider_released_cb, LV_EVENT_RELEASED, NULL); // Runs when slider is released
}

// Hardware initialization function
void initCydHardware()
{

  // Start the SPI for the touch screen and init the TS library
  SoftwareSpi.setDataMode(SPI_MODE2);
  SoftwareSpi.setBitOrder(1);
  SoftwareSpi.setFrequency(2500000);
  SoftwareSpi.setClockDivider(SPI_CLOCK_DIV2);
  SoftwareSpi.begin();
  ts.begin(SoftwareSpi);
  ts.setRotation(2);

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
  uint32_t duty = (4095 / valueMax) * min(value, valueMax);
  ledcWrite(channel, duty);
}

int readLdr()
{
  return analogRead(LDR_PIN);
}

// LVGL setup
void setupUi()
{
  lv_init();
  uint8_t *draw_buf = new uint8_t[DRAW_BUF_SIZE];
  lv_display_t *disp = lv_tft_espi_create(TFT_HOR_RES, TFT_VER_RES, draw_buf, DRAW_BUF_SIZE);
  lv_display_set_rotation(disp, LV_DISPLAY_ROTATION_90);

  lv_obj_t *scr = lv_scr_act();
  lv_obj_set_style_bg_color(scr, lv_color_black(), 0);

  lv_indev_t *indev = lv_indev_create();
  lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
  lv_indev_set_read_cb(indev, my_touchpad_read);

  initUi(scr);
  tft.fillScreen(TFT_BLACK);
}

void loopBacklight()
{
  if (readLdr() < 300 || ts.touched() == true)
  {
    ledcAnalogWrite(LEDC_CHANNEL_0, 150);
  }
  else
  {
    ledcAnalogWrite(LEDC_CHANNEL_0, 0);
  }
}

uint32_t lastTick = 0;

void loopDisplay(void *param)
{
  for (;;)
  {
    lv_tick_inc(millis() - lastTick);
    lastTick = millis();
    lv_timer_handler();
    vTaskDelay(10 / portTICK_PERIOD_MS);
    update_slider_from_variable();
  }
}
