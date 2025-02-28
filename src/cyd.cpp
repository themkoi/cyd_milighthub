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
  if (second() != lastSecond)
  {
  char timeBuffer[32];
  snprintf(timeBuffer, sizeof(timeBuffer), "%02d:%02d:%02d", hour(), minute(), second());
  lv_label_set_text(timeLabel, timeBuffer);
  lastSecond = second();
  }
}

void my_touchpad_read(lv_indev_t *indev, lv_indev_data_t *data)
{
  static unsigned long lastValidTouchTime = 0;
  static unsigned long lastTouchStartTime = 0;
  static const unsigned long noiseThreshold = 50;  // Minimum duration (ms) to consider as a valid touch
  static const unsigned long debounceDelay = 50;   // Debounce time (ms)
  static bool lastTouchState = false;

  bool isTouched = ts.touched();

  if (isTouched)
  {
    unsigned long currentTime = millis();

    // If a new touch is detected
    if (!lastTouchState)
    {
      lastTouchStartTime = currentTime; // Record when the touch started
      lastTouchState = true;
    }

    // Check if the touch lasts longer than the noise threshold
    if (currentTime - lastTouchStartTime >= noiseThreshold)
    {
      lastValidTouchTime = currentTime;

      // Read touch coordinates
      TS_Point p = ts.getPoint();
      Serial.print("X: ");
      Serial.print(p.x);
      Serial.print(", Y: ");
      Serial.println(p.y);
      data->point.x = map(p.x, touchScreenMinimumX, touchScreenMaximumX, 0, TFT_HOR_RES);
      data->point.y = map(p.y, touchScreenMinimumY, touchScreenMaximumY, 0, TFT_VER_RES);
      data->state = LV_INDEV_STATE_PRESSED;
    }
    else
    {
      // If still within the noise threshold, ignore the touch
      data->state = LV_INDEV_STATE_RELEASED;
    }
  }
  else
  {
    // Handle touch release
    unsigned long currentTime = millis();

    if (lastTouchState && (currentTime - lastValidTouchTime >= debounceDelay))
    {
      lastTouchState = false;
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
  lv_obj_clear_state(Switch, LV_STATE_CHECKED);
  if (variable)
  {
    lv_obj_add_state(Switch, LV_STATE_CHECKED);
  }
}

void apply_switch_styles(lv_obj_t *sw)
{
  lv_obj_set_style_bg_color(sw, lv_color_hex(0xca9ee6), LV_PART_INDICATOR | LV_STATE_CHECKED);
}

// Modify the UI setup function to include the switch
void initUi(lv_obj_t *parent)
{
  state = stateStore->get(myBulbId);
  int currentBrightness = state->getBrightness();

  brightnessLabel = lv_label_create(parent);
  char buf[32];
  snprintf(buf, sizeof(buf), "Brightness: %d", currentBrightness);
  lv_label_set_text(brightnessLabel, buf);
  lv_obj_set_style_text_color(brightnessLabel, lv_color_white(), 0);
  lv_obj_align(brightnessLabel, LV_ALIGN_CENTER, 0, -20);

  slider = lv_slider_create(parent);
  lv_slider_set_range(slider, 0, 100);
  lv_slider_set_value(slider, currentBrightness, LV_ANIM_ON);
  lv_obj_set_width(slider, 250);
  lv_obj_align(slider, LV_ALIGN_CENTER, 0, 0);
  apply_slider_styles(slider);

  lv_obj_add_event_cb(slider, slider_event_cb, LV_EVENT_VALUE_CHANGED, NULL);
  lv_obj_add_event_cb(slider, slider_released_cb, LV_EVENT_RELEASED, NULL);

  lightSwitch = lv_switch_create(parent);
  apply_switch_styles(lightSwitch);
  lv_obj_align(lightSwitch, LV_ALIGN_BOTTOM_LEFT, 50, -50); // Position in bottom-left corner
  lv_obj_add_event_cb(lightSwitch, light_switch_event_cb, LV_EVENT_VALUE_CHANGED, NULL);
  lightLabel = lv_label_create(parent);
  lv_label_set_text(lightLabel, "Light");
  lv_obj_set_style_text_color(lightLabel, lv_color_white(), 0);
  lv_obj_set_style_text_align(lightLabel, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN);
  lv_obj_align(lightLabel, LV_ALIGN_BOTTOM_LEFT, 56, -30);

  mmwaveSwitch = lv_switch_create(parent);
  apply_switch_styles(mmwaveSwitch);
  lv_obj_align(mmwaveSwitch, LV_ALIGN_BOTTOM_RIGHT, -50, -50); // Position in bottom-right corner
  lv_obj_add_event_cb(mmwaveSwitch, mmwave_switch_event_cb, LV_EVENT_VALUE_CHANGED, NULL);
  mmwaveLabel = lv_label_create(parent);
  lv_label_set_text(mmwaveLabel, "mmWave");
  lv_obj_set_style_text_color(mmwaveLabel, lv_color_white(), 0);
  lv_obj_set_style_text_align(mmwaveLabel, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN);
  lv_obj_align(mmwaveLabel, LV_ALIGN_BOTTOM_RIGHT, -41, -30);

  char ipBuffer[32];
  String ipString = WiFi.localIP().toString();
  snprintf(ipBuffer, sizeof(ipBuffer), "%s", ipString.c_str());

  ipLabel = lv_label_create(parent);
  lv_label_set_text(ipLabel, ipBuffer);
  lv_obj_set_style_text_color(ipLabel, lv_color_white(), 0);
  lv_obj_align(ipLabel, LV_ALIGN_TOP_MID, 0, 10);

  timeLabel = lv_label_create(parent);
  lv_label_set_text(timeLabel, ipBuffer);
  lv_obj_set_style_text_color(timeLabel, lv_color_white(), 0);
  lv_obj_align(timeLabel, LV_ALIGN_TOP_MID, 0, 30);
  updateTimeLabel();

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
}

unsigned long backlightTimeout = 0;
bool backlightActive = false;

unsigned long touchStartTime = 0;
bool touchDetected = false;

float smoothedLdrValue = 0; // Variable to store the smoothened LDR value
const float alpha = 0.1;    // Smoothing factor (adjust for more/less smoothening)

void loopBacklight()
{
  if (ts.touched())
  {
    if (!touchDetected)
    {
      touchStartTime = millis(); // Record the time when touch starts
      touchDetected = true;
    }
    else if (millis() - touchStartTime >= 30)
    {                                      // Check if touch has been active for 300ms
      backlightTimeout = millis() + 20000; // Set timeout to 20 seconds
      backlightActive = true;
    }
  }
  else
  {
    touchDetected = false; // Reset if touch is no longer detected
  }

  if (backlightActive)
  {
    ledcAnalogWrite(LEDC_CHANNEL_0, 150);
    if (millis() > backlightTimeout)
    {
      backlightActive = false; // Disable backlight after timeout
    }
  }
  else
  {
    int ldrValue = readLdr();
    smoothedLdrValue = (alpha * ldrValue) + ((1 - alpha) * smoothedLdrValue); // Apply EMA
    int brightness = map(smoothedLdrValue, 400, 0, 0, 150);                  // Map smoothed LDR values to brightness
    brightness = constrain(brightness, 0, 150);                              // Ensure brightness stays within range
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
    isTouched = ts.touched();
    lv_tick_inc(millis() - lastTick);
    lastTick = millis();
    lv_timer_handler();
    vTaskDelay(10 / portTICK_PERIOD_MS);
    state = stateStore->get(myBulbId);
    bool isOn = state->isOn();
    if (millis() - previousMillis >= interval) {
        previousMillis = millis();

        update_switch_from_variable(lightSwitch, isOn);
        update_slider_from_variable();
        updateTimeLabel();
    }
  }
}
