#ifndef CYD_H
#define CYD_H

#include "defines.h"


extern SoftSPI SoftwareSpi;
extern XPT2046_Touchscreen ts;

extern TFT_eSPI tft;

extern bool mmwaveState;

void loopDisplay(void *param);
void loopBacklight(void *param);

void manageBacklight();

void initCydHardware();

void ledcAnalogWrite(uint8_t channel, uint32_t value, uint32_t valueMax = 255);

int readLdr();

void apply_slider_styles(lv_obj_t *slider);
void update_label_text(lv_obj_t *label, char symbol, int number);

void setupUi();

#endif