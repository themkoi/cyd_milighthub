#ifndef DEFINES_H
#define DEFINES_H

#include <Arduino.h>
#define LV_CONF_INCLUDE_SIMPLE


#include "milightIncludes.h"
#include "lightControl.h"

#define TOUCH_CS 33 // just to get rid of warning

#include <SPI.h>
#include <SoftSPI.h>


#include <XPT2046_Touchscreen.h>
// A library for interfacing with the touch screen

#include <TFT_eSPI.h>
// A library for interfacing with LCD displays

// backlight 
#define LCD_BACK_LIGHT_PIN 21

// use first channel of 16 channels (started from zero)
#define LEDC_CHANNEL_0     0

// use 12 bit precission for LEDC timer
#define LEDC_TIMER_12_BIT  12

// use 5000 Hz as a LEDC base frequency
#define LEDC_BASE_FREQ     5000

// light sensor
#define LDR_PIN 34

// The CYD touch uses some non default
// SPI pins

#define XPT2046_IRQ 36
#define XPT2046_MOSI 32
#define XPT2046_MISO 39
#define XPT2046_CLK 25
#define XPT2046_CS 33

#include "cyd.h"

// mmWave sensor stuff

#include <s3km1110.h>

#define RX_GPIO 35
#define TX_GPIO 22

#define MMWAVE_GPIO 27

#include "mmWave.h"

void milightTask(void *param);

#endif