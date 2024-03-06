#ifndef CLICKWHEEL_H
#define CLICKWHEEL_H

#include <Arduino.h>

////////Start Click Wheel
#define CENTER_BUTTON_BIT 7
#define LEFT_BUTTON_BIT 9
#define RIGHT_BUTTON_BIT 8
#define UP_BUTTON_BIT 11
#define DOWN_BUTTON_BIT 10
#define WHEEL_TOUCH_BIT 29
#define BUFFER_SIZE 3
#define BUTTON_INDEX 0
#define BUTTON_STATE_INDEX 1
#define WHEEL_POSITION_INDEX 2
#define CLOCK_PIN 4                    // SCL purple
#define DATA_PIN 3                     // SDA grey
#define BUTTON_PIN_BITMASK 0x200000000 // 2^33 in hex
#define BIT_COUNT 32

bool ClickWheelClicked = false;
int ClickWheelScrollPrev[5];
int ClickWheelNoiseFilterAmt = 4; // three works for me!

volatile bool CLOCK_RISING = false;
volatile bool DATA_EDGE = false;

// Click Wheel Data Line Pin Bitmask
int wheel_position;
int last_wheel_position;
int last_button;
int last_interaction;
bool wheel_scroll_lift = false;
int WheelSensitivity = 0;

unsigned long crashtick = 0;

volatile boolean click_wheel_packets_received = false;
volatile int HapticFeebackDue = 0;

// used to store the current packet
uint32_t bits = 0;
// used to store the previous full packet
uint32_t lastBits = 0;
uint8_t bitIndex = 0;
uint8_t oneCount = 0;
uint8_t recording = 0;
// indicates whether the data pin is high or low
uint8_t dataBit = 1;
uint8_t lastPosition = 255;
int hapticWaveId = -1;

char buttons[] = {CENTER_BUTTON_BIT, LEFT_BUTTON_BIT, RIGHT_BUTTON_BIT,
                  UP_BUTTON_BIT,     DOWN_BUTTON_BIT, WHEEL_TOUCH_BIT};

const uint32_t PACKET_START = 0b01101;
char buffer[BUFFER_SIZE];
char prev_buffer[BUFFER_SIZE];
volatile bool InputReceived = false;

#endif 