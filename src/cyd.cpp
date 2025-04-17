#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>
#include "cyd.h"

// LVGL display buffer and resolution
#define TFT_HOR_RES 240
#define TFT_VER_RES 320
#define DRAW_BUF_SIZE (TFT_HOR_RES * TFT_VER_RES / 10 * (LV_COLOR_DEPTH / 8))

lv_color_t whiteColor = lv_color_make(255, 255, 255);
lv_color_t mauveColor = lv_color_make(203, 166, 247);
lv_obj_t *slider;
lv_obj_t *brightnessLabel;

bool lightSwitchState = false; // Boolean state for the switch
lv_obj_t *lightSwitch;
lv_obj_t *lightLabel;

bool mmwaveState = true; // Boolean state for the switch
lv_obj_t *mmwaveSwitch;
lv_obj_t *mmwaveLabel;

lv_obj_t *ipLabel;
lv_obj_t *timeLabel;

bool isTouched = false;

TFT_eSPI tft = TFT_eSPI();
SoftSPI SoftwareSpi(XPT2046_MOSI, XPT2046_MISO, XPT2046_CLK);
XPT2046_Touchscreen ts(XPT2046_CS, XPT2046_IRQ);

uint16_t touchScreenMinimumX = 400, touchScreenMaximumX = 3836, touchScreenMinimumY = 350, touchScreenMaximumY = 3850;

uint8_t lastSecond = 0;

void updateTimeLabel()
{
  if (timeLabel == NULL)
    return;

  if (second() != lastSecond)
  {
    char timeBuffer[32];
    snprintf(timeBuffer, sizeof(timeBuffer), "%02d:%02d:%02d", hour(), minute(), second());
    lv_label_set_text(timeLabel, timeBuffer);
    lastSecond = second();
  }
}

static unsigned long lastValidTouchTime = 0;
static unsigned long lastTouchStartTime = 0;
static const unsigned long noiseThreshold = 50; // Minimum duration (ms) to consider as a valid touch
static const unsigned long debounceDelay = 50;  // Debounce time (ms)
static bool lastTouchState = false;

void my_touchpad_read(lv_indev_t *indev, lv_indev_data_t *data)
{
  bool touched = ts.touched();

  if (touched)
  {
    unsigned long currentTime = millis();

    if (!lastTouchState)
    {
      lastTouchStartTime = currentTime;
      lastTouchState = true;
    }

    if (currentTime - lastTouchStartTime >= noiseThreshold)
    {
      lastValidTouchTime = currentTime;

      TS_Point p = ts.getPoint();
      Serial.print("X: ");
      Serial.print(p.x);
      Serial.print(", Y: ");
      Serial.println(p.y);

      if (p.x >= touchScreenMinimumX && p.x <= touchScreenMaximumX &&
          p.y >= touchScreenMinimumY && p.y <= touchScreenMaximumY)
      {
        data->point.x = map(p.x, touchScreenMinimumX, touchScreenMaximumX, 0, TFT_HOR_RES);
        data->point.y = map(p.y, touchScreenMinimumY, touchScreenMaximumY, 0, TFT_VER_RES);
        isTouched = true;
        data->state = LV_INDEV_STATE_PRESSED;
      }
      else
      {
        isTouched = false;
        data->state = LV_INDEV_STATE_RELEASED;
      }
    }
    else
    {
      isTouched = false;
      data->state = LV_INDEV_STATE_RELEASED;
    }
  }
  else
  {
    unsigned long currentTime = millis();

    if (lastTouchState && (currentTime - lastValidTouchTime >= debounceDelay))
    {
      lastTouchState = false;
      isTouched = false;
      data->state = LV_INDEV_STATE_RELEASED;
    }
  }
}

static void slider_event_cb(lv_event_t *e)
{
  lv_obj_t *slider = (lv_obj_t *)lv_event_get_target(e);
  brightness = lv_slider_get_value(slider); // Update brightness value
  char buf[32];
  snprintf(buf, sizeof(buf), "Brightness: %d", brightness);
  lv_label_set_text(brightnessLabel, buf);
}

static void slider_released_cb(lv_event_t *e)
{
  setBrightness(brightness); // Apply brightness when released
}

void update_slider_from_variable()
{
  if (slider == NULL)
    return;
  state = stateStore->get(myBulbId);
  int currentBrightness = state->getBrightness();

  // Sync the slider's position with the shared variable
  if (lv_slider_get_value(slider) != currentBrightness && isTouched == false)
  {
    lv_slider_set_value(slider, currentBrightness, LV_ANIM_ON);
    char buf[32];
    snprintf(buf, sizeof(buf), "Brightness: %d", currentBrightness);
    lv_label_set_text(brightnessLabel, buf); // Update the label to reflect the new value
    Serial.println("updating slider: " + String(currentBrightness) + " " + String(lv_slider_get_value(slider)));
  }
}

// Helper functions
void apply_slider_styles(lv_obj_t *slider)
{
  lv_obj_set_style_bg_color(slider, whiteColor, LV_PART_MAIN);
  lv_obj_set_style_bg_color(slider, whiteColor, LV_PART_INDICATOR);
  lv_obj_set_style_bg_color(slider, lv_color_lighten(whiteColor, LV_OPA_30), LV_PART_KNOB);
}

void update_label_text(lv_obj_t *label, char symbol, int number)
{
  if (label == NULL)
    return;
  char buf[128];
  snprintf(buf, sizeof(buf), "%c: %d", symbol, number);
  lv_label_set_text(label, buf);
}

// Event callback for the switch
static void light_switch_event_cb(lv_event_t *e)
{
  lv_obj_t *sw = (lv_obj_t *)lv_event_get_target(e); // Explicit cast to lv_obj_t*
  lightSwitchState = lv_obj_has_state(sw, LV_STATE_CHECKED);
  Serial.println("Switch toggled: " + String(lightSwitchState));
  if (lightSwitchState)
  {
    milightClient->updateStatus(MiLightStatus::ON);
  }
  else
  {
    milightClient->updateStatus(MiLightStatus::OFF);
  }
}

static void mmwave_switch_event_cb(lv_event_t *e)
{
  lv_obj_t *sw = (lv_obj_t *)lv_event_get_target(e); // Explicit cast to lv_obj_t*
  mmwaveState = lv_obj_has_state(sw, LV_STATE_CHECKED);
  Serial.println("Switch toggled: " + String(mmwaveState));
}

// Update switch appearance based on state
void update_switch_from_variable(lv_obj_t *Switch, bool variable)
{
  if (Switch == NULL)
    return;
  lv_obj_clear_state(Switch, LV_STATE_CHECKED);
  if (variable)
  {
    lv_obj_add_state(Switch, LV_STATE_CHECKED);
  }
}

static void scene_minus_btn_event_cb(lv_event_t *e)
{
  Serial.println("Minus Button Clicked");
  previousMode();
}

static void scene_plus_btn_event_cb(lv_event_t *e)
{
  Serial.println("Plus Button Clicked");
  nextMode();
}

static void speed_minus_btn_event_cb(lv_event_t *e)
{
  Serial.println("Minus Button Clicked");
  speedDown();
}

static void speed_plus_btn_event_cb(lv_event_t *e)
{
  Serial.println("Plus Button Clicked");
  speedUp();
}

static void color_btn_event_cb(lv_event_t *e)
{
  Serial.println("Plus Button Clicked");
  setHue(NATURAL_HUE);
}

void apply_switch_styles(lv_obj_t *sw)
{
  lv_obj_set_style_bg_color(sw, lv_color_hex(0xca9ee6), LV_PART_INDICATOR | LV_STATE_CHECKED);
}

void initUi(lv_obj_t *parent)
{
  lv_obj_t *tabView = lv_tabview_create(lv_scr_act());
  lv_obj_set_size(tabView, LV_HOR_RES, LV_VER_RES);
  lv_obj_align(tabView, LV_ALIGN_CENTER, 0, 0);
  lv_tabview_set_tab_bar_size(tabView, 40);

  lv_obj_set_style_bg_color(tabView, lv_color_hex(TFT_BLACK), 0);
  lv_obj_t *tab = lv_tabview_add_tab(tabView, "Main Tab");
  lv_obj_t *othersTab = lv_tabview_add_tab(tabView, "Other Controls");

  lv_obj_t *content = lv_tabview_get_content(tabView);
  lv_obj_clear_flag(content, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t *tab_bar = lv_tabview_get_tab_bar(tabView);

  /******************************* button1 *******************************/
  lv_obj_t *button1 = lv_obj_get_child(tab_bar, 0);

  lv_obj_set_style_bg_opa(button1, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_CHECKED);
  lv_obj_set_style_bg_color(button1, lv_color_hex(0xca9ee6), LV_PART_MAIN | LV_STATE_CHECKED);
  lv_obj_set_style_border_width(button1, 2, LV_PART_MAIN | LV_STATE_CHECKED);
  lv_obj_set_style_border_color(button1, lv_color_make(0, 0, 0), LV_PART_MAIN | LV_STATE_CHECKED);
  lv_obj_set_style_border_side(button1, LV_BORDER_SIDE_LEFT, LV_PART_MAIN | LV_STATE_CHECKED);
  lv_obj_set_style_text_color(button1, lv_color_hex(0x1e1e2e), LV_PART_MAIN | LV_STATE_CHECKED);

  lv_obj_set_style_border_color(button1, lv_color_make(0, 0, 0), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_opa(button1, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_width(button1, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_side(button1, LV_BORDER_SIDE_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_color(button1, lv_color_black(), LV_PART_MAIN | LV_STATE_DEFAULT);

  /******************************* button2 *******************************/
  lv_obj_t *button2 = lv_obj_get_child(tab_bar, 1);
  lv_obj_set_style_bg_opa(button2, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_CHECKED);
  lv_obj_set_style_bg_color(button2, lv_color_hex(0xca9ee6), LV_PART_MAIN | LV_STATE_CHECKED);
  lv_obj_set_style_border_width(button2, 2, LV_PART_MAIN | LV_STATE_CHECKED);
  lv_obj_set_style_border_side(button2, LV_BORDER_SIDE_LEFT, LV_PART_MAIN | LV_STATE_CHECKED);
  lv_obj_set_style_border_color(button2, lv_color_make(0, 0, 0), LV_PART_MAIN | LV_STATE_CHECKED);
  lv_obj_set_style_text_color(button2, lv_color_hex(0x1e1e2e), LV_PART_MAIN | LV_STATE_CHECKED);

  lv_obj_set_style_border_color(button2, lv_color_make(0, 0, 0), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_opa(button2, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_width(button2, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_border_side(button2, LV_BORDER_SIDE_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_color(button2, lv_color_black(), LV_PART_MAIN | LV_STATE_DEFAULT);

  state = stateStore->get(myBulbId);
  int currentBrightness = state->getBrightness();

  slider = lv_slider_create(tab);
  lv_slider_set_range(slider, 0, 100);
  lv_slider_set_value(slider, currentBrightness, LV_ANIM_ON);
  lv_obj_set_width(slider, 250);
  lv_obj_align(slider, LV_ALIGN_CENTER, 0, -5);
  apply_slider_styles(slider);

  lv_obj_add_event_cb(slider, slider_event_cb, LV_EVENT_VALUE_CHANGED, NULL);
  lv_obj_add_event_cb(slider, slider_released_cb, LV_EVENT_RELEASED, NULL);

  brightnessLabel = lv_label_create(tab);
  char buf[32];
  snprintf(buf, sizeof(buf), "Brightness: %d", currentBrightness);
  lv_label_set_text(brightnessLabel, buf);
  lv_obj_set_style_text_color(brightnessLabel, lv_color_white(), 0);
  lv_obj_align_to(brightnessLabel, slider, LV_ALIGN_CENTER, 0, -20);

  lightSwitch = lv_switch_create(tab);
  apply_switch_styles(lightSwitch);
  lv_obj_align(lightSwitch, LV_ALIGN_BOTTOM_LEFT, 50, -30);
  lv_obj_add_event_cb(lightSwitch, light_switch_event_cb, LV_EVENT_VALUE_CHANGED, NULL);

  lightLabel = lv_label_create(tab);
  lv_label_set_text(lightLabel, "Light");
  lv_obj_set_style_text_color(lightLabel, lv_color_white(), 0);
  lv_obj_set_style_text_align(lightLabel, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN);
  lv_obj_align_to(lightLabel, lightSwitch, LV_ALIGN_BOTTOM_MID, 0, 20);

  mmwaveSwitch = lv_switch_create(tab);
  apply_switch_styles(mmwaveSwitch);
  lv_obj_align(mmwaveSwitch, LV_ALIGN_BOTTOM_RIGHT, -50, -30);
  lv_obj_add_event_cb(mmwaveSwitch, mmwave_switch_event_cb, LV_EVENT_VALUE_CHANGED, NULL);

  mmwaveLabel = lv_label_create(tab);
  lv_label_set_text(mmwaveLabel, "mmWave");
  lv_obj_set_style_text_color(mmwaveLabel, lv_color_white(), 0);
  lv_obj_set_style_text_align(mmwaveLabel, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN);
  lv_obj_align_to(mmwaveLabel, mmwaveSwitch, LV_ALIGN_BOTTOM_MID, 0, 20);

  char ipBuffer[32];
  String ipString = WiFi.localIP().toString();
  snprintf(ipBuffer, sizeof(ipBuffer), "%s", ipString.c_str());

  ipLabel = lv_label_create(parent);
  lv_label_set_text(ipLabel, ipBuffer);
  lv_obj_set_style_text_color(ipLabel, lv_color_white(), 0);
  lv_obj_align(ipLabel, LV_ALIGN_TOP_MID, 0, 44);

  timeLabel = lv_label_create(parent);
  lv_label_set_text(timeLabel, ipBuffer);
  lv_obj_set_style_text_color(timeLabel, lv_color_white(), 0);
  lv_obj_align(timeLabel, LV_ALIGN_TOP_MID, 0, 54 + 7);
  updateTimeLabel();

  // Parent container to hold everything
  lv_obj_t *sceneBack = lv_obj_create(othersTab);
  lv_obj_set_size(sceneBack, 135, 40);
  lv_obj_align(sceneBack, LV_ALIGN_BOTTOM_LEFT, 5, -20);
  lv_obj_set_style_bg_color(sceneBack, lv_color_hex(TFT_BLACK), LV_PART_MAIN); // Dark background
  lv_obj_set_style_border_width(sceneBack, 0, LV_PART_MAIN);
  lv_obj_set_style_pad_all(sceneBack, 0, LV_PART_MAIN); // Remove extra padding

  // Minus button (-)
  lv_obj_t *btn_minus_scene = lv_btn_create(sceneBack);
  lv_obj_set_size(btn_minus_scene, 35, 30); // Rectangular size
  lv_obj_align(btn_minus_scene, LV_ALIGN_RIGHT_MID, -5, 0);
  lv_obj_set_style_bg_color(btn_minus_scene, lv_color_hex(0xCA9EE6), LV_PART_MAIN);
  lv_obj_set_style_radius(btn_minus_scene, 20, LV_PART_MAIN); // Round only left side
  lv_obj_set_style_pad_all(btn_minus_scene, 0, LV_PART_MAIN);

  lv_obj_t *label_minus_scene = lv_label_create(btn_minus_scene);
  lv_label_set_text(label_minus_scene, "-");
  lv_obj_set_style_text_color(label_minus_scene, lv_color_hex(0x1e1e2e), LV_PART_MAIN);
  lv_obj_center(label_minus_scene);

  lv_obj_add_event_cb(btn_minus_scene, scene_minus_btn_event_cb, LV_EVENT_CLICKED, NULL);

  // Scene Label (Centered text)
  lv_obj_t *label_scene = lv_label_create(sceneBack);
  lv_label_set_text(label_scene, "Scene");
  lv_obj_align(label_scene, LV_ALIGN_CENTER, 0, 0);
  lv_obj_set_style_text_color(label_scene, lv_color_hex(0xFFFFFF), LV_PART_MAIN); // White text

  // Plus button (+)
  lv_obj_t *btn_plus_scene = lv_btn_create(sceneBack);
  lv_obj_set_size(btn_plus_scene, 35, 30); // Rectangular size
  lv_obj_align(btn_plus_scene, LV_ALIGN_LEFT_MID, 5, 0);
  lv_obj_set_style_bg_color(btn_plus_scene, lv_color_hex(0xCA9EE6), LV_PART_MAIN);
  lv_obj_set_style_radius(btn_plus_scene, 20, LV_PART_MAIN); // Round only right side
  lv_obj_set_style_pad_all(btn_plus_scene, 0, LV_PART_MAIN);

  lv_obj_t *label_plus_scene = lv_label_create(btn_plus_scene);
  lv_label_set_text(label_plus_scene, "+");
  lv_obj_set_style_text_color(label_plus_scene, lv_color_hex(0x1e1e2e), LV_PART_MAIN);
  lv_obj_center(label_plus_scene);

  lv_obj_add_event_cb(btn_plus_scene, scene_plus_btn_event_cb, LV_EVENT_CLICKED, NULL);

  // Parent container to hold everything
  lv_obj_t *speedBack = lv_obj_create(othersTab);
  lv_obj_set_size(speedBack, 135, 40);
  lv_obj_align(speedBack, LV_ALIGN_BOTTOM_RIGHT, -5, -20);
  lv_obj_set_style_bg_color(speedBack, lv_color_hex(TFT_BLACK), LV_PART_MAIN); // Dark background
  lv_obj_set_style_border_width(speedBack, 0, LV_PART_MAIN);
  lv_obj_set_style_pad_all(speedBack, 0, LV_PART_MAIN); // Remove extra padding

  // Minus button (-)
  lv_obj_t *btn_minus_speed = lv_btn_create(speedBack);
  lv_obj_set_size(btn_minus_speed, 35, 30); // Rectangular size
  lv_obj_align(btn_minus_speed, LV_ALIGN_RIGHT_MID, -5, 0);
  lv_obj_set_style_bg_color(btn_minus_speed, lv_color_hex(0xCA9EE6), LV_PART_MAIN);
  lv_obj_set_style_radius(btn_minus_speed, 20, LV_PART_MAIN); // Round only left side
  lv_obj_set_style_pad_all(btn_minus_speed, 0, LV_PART_MAIN);

  lv_obj_t *label_minus_speed = lv_label_create(btn_minus_speed);
  lv_label_set_text(label_minus_speed, "-");
  lv_obj_set_style_text_color(label_minus_speed, lv_color_hex(0x1e1e2e), LV_PART_MAIN);

  lv_obj_center(label_minus_speed);

  lv_obj_add_event_cb(btn_minus_speed, speed_minus_btn_event_cb, LV_EVENT_CLICKED, NULL);

  // speed Label (Centered text)
  lv_obj_t *label_speed = lv_label_create(speedBack);
  lv_label_set_text(label_speed, "speed");
  lv_obj_align(label_speed, LV_ALIGN_CENTER, 0, 0);
  lv_obj_set_style_text_color(label_speed, lv_color_hex(0xFFFFFF), LV_PART_MAIN); // White text

  // Plus button (+)
  lv_obj_t *btn_plus_speed = lv_btn_create(speedBack);
  lv_obj_set_size(btn_plus_speed, 35, 30); // Rectangular size
  lv_obj_align(btn_plus_speed, LV_ALIGN_LEFT_MID, 5, 0);
  lv_obj_set_style_bg_color(btn_plus_speed, lv_color_hex(0xCA9EE6), LV_PART_MAIN);
  lv_obj_set_style_radius(btn_plus_speed, 20, LV_PART_MAIN); // Round only right side
  lv_obj_set_style_pad_all(btn_plus_speed, 0, LV_PART_MAIN);

  lv_obj_t *label_plus_speed = lv_label_create(btn_plus_speed);
  lv_label_set_text(label_plus_speed, "+");
  lv_obj_set_style_text_color(label_plus_speed, lv_color_hex(0x1e1e2e), LV_PART_MAIN);
  lv_obj_center(label_plus_speed);

  lv_obj_add_event_cb(btn_plus_speed, speed_plus_btn_event_cb, LV_EVENT_CLICKED, NULL);

  // Minus button (-)
  lv_obj_t *color_button = lv_btn_create(othersTab);
  lv_obj_set_size(color_button, 100, 35); // Rectangular size
  lv_obj_align(color_button, LV_ALIGN_CENTER, 0, -15);
  lv_obj_set_style_bg_color(color_button, lv_color_hex(0xCA9EE6), LV_PART_MAIN);
  lv_obj_set_style_radius(color_button, 20, LV_PART_MAIN); // Round only left side
  lv_obj_set_style_pad_all(color_button, 0, LV_PART_MAIN);

  lv_obj_t *label_color = lv_label_create(color_button);
  lv_label_set_text(label_color, "def color");
  lv_obj_set_style_text_color(label_color, lv_color_hex(0x1e1e2e), LV_PART_MAIN);
  lv_obj_center(label_color);

  lv_obj_add_event_cb(color_button, color_btn_event_cb, LV_EVENT_CLICKED, NULL);

  // Initial state of the switch
  state = stateStore->get(myBulbId);
  bool isOn = state->isOn();
  update_switch_from_variable(lightSwitch, isOn);

  update_switch_from_variable(mmwaveSwitch, mmwaveState);
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
  tft.init();
  tft.initDMA();
  tft.setRotation(1); /* Landscape orientation */

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
lv_display_t *disp;

void lvgl_tick_task(void *param);
void lvgl_handler_task(void *param);

void my_log_cb(lv_log_level_t level, const char *buf)
{
  // Simply send the log message via serial
  Serial.print(level);
  Serial.println(buf); // Use Serial.println for output
}

typedef struct
{
  TFT_eSPI *tft;
} lv_tft_espi_t;

static void displayFlush(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map)
{
  lv_tft_espi_t *dsc = (lv_tft_espi_t *)lv_display_get_driver_data(disp);

  uint32_t w = (area->x2 - area->x1 + 1);
  uint32_t h = (area->y2 - area->y1 + 1);

  tft.startWrite();
  tft.setAddrWindow(area->x1, area->y1, w, h);
  tft.pushColors((uint16_t *)px_map, w * h, true);
  tft.endWrite();

  lv_display_flush_ready(disp);
}

lv_display_t *lv_tft_create(uint32_t hor_res, uint32_t ver_res, void *buf, uint32_t buf_size_bytes)
{
  lv_tft_espi_t *dsc = (lv_tft_espi_t *)lv_malloc_zeroed(sizeof(lv_tft_espi_t));
  LV_ASSERT_MALLOC(dsc);
  if (dsc == NULL)
    return NULL;

  lv_display_t *disp = lv_display_create(hor_res, ver_res);
  if (disp == NULL)
  {
    lv_free(dsc);
    return NULL;
  }
  lv_display_set_driver_data(disp, (void *)dsc);
  lv_display_set_flush_cb(disp, displayFlush);
  lv_display_set_buffers(disp, (void *)buf, NULL, buf_size_bytes, LV_DISPLAY_RENDER_MODE_PARTIAL);
  return disp;
}

// LVGL setup
void setupUi()
{
  lv_init();
  uint8_t *draw_buf = new uint8_t[DRAW_BUF_SIZE];
  disp = lv_tft_create(TFT_HOR_RES, TFT_VER_RES, draw_buf, DRAW_BUF_SIZE);
  tft.init();
  tft.initDMA();

  // Clear the screen before writing to it
  tft.fillScreen(TFT_BLACK);
  lv_display_set_rotation(disp, LV_DISPLAY_ROTATION_90);

  lv_obj_t *scr = lv_scr_act();
  lv_obj_set_style_bg_color(scr, lv_color_black(), 0);
  lv_screen_load(scr); // optional, if scr isn't active already

  lv_indev_t *indev = lv_indev_create();
  lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
  lv_indev_set_read_cb(indev, my_touchpad_read);

  initUi(scr);
}

unsigned long backlightTimeout = 0;
bool backlightActive = false;

unsigned long touchStartTime = 0;
bool touchDetected = false;

float smoothedLdrValue = 0; // Variable to store the smoothened LDR value
const float alpha = 0.1;    // Smoothing factor (adjust for more/less smoothening)

void checkTouched()
{
  bool touched = ts.touched();
  unsigned long currentTime = millis();

  if (touched)
  {
    if (!lastTouchState)
    {
      lastTouchStartTime = currentTime;
      lastTouchState = true;
    }

    if (currentTime - lastTouchStartTime >= noiseThreshold)
    {
      lastValidTouchTime = currentTime;
      TS_Point p = ts.getPoint();

      if (p.x >= touchScreenMinimumX && p.x <= touchScreenMaximumX &&
          p.y >= touchScreenMinimumY && p.y <= touchScreenMaximumY)
      {
        isTouched = true;
      }
      else
      {
        isTouched = false;
      }
    }
    else
    {
      isTouched = false;
    }
  }
  else
  {
    if (lastTouchState && (currentTime - lastValidTouchTime >= debounceDelay))
    {
      lastTouchState = false;
      isTouched = false;
    }
  }
}


void loopBacklight()
{
  checkTouched();
  if (isTouched)
  {
    backlightActive = true;
    backlightTimeout = millis() + 20000;
  }

  if (backlightActive)
  {
    ledcAnalogWrite(LEDC_CHANNEL_0, 150);
    if (millis() > backlightTimeout)
      backlightActive = false;
  }
  else
  {
    int ldrValue = readLdr();
    smoothedLdrValue = (alpha * ldrValue) + ((1 - alpha) * smoothedLdrValue);
    int brightness = map(smoothedLdrValue, 400, 0, 0, 150);
    brightness = constrain(brightness, 0, 150);
    ledcAnalogWrite(LEDC_CHANNEL_0, brightness);
  }
}


uint32_t lastTick = 0;

unsigned long previousMillis = 0;
const unsigned long interval = 1000;

void loopDisplay(void *param)
{
  for (;;)
  {
    lv_tick_inc(millis() - lastTick);
    lastTick = millis();
    lv_timer_handler();
    vTaskDelay(1 / portTICK_PERIOD_MS);
    if (millis() - previousMillis >= interval)
    {
      state = stateStore->get(myBulbId);
      bool isOn = state->isOn();
      previousMillis = millis();

      update_switch_from_variable(lightSwitch, isOn);
      update_slider_from_variable();
      updateTimeLabel();
    }
  }
}