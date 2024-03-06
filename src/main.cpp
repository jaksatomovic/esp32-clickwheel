#include "main_header.h"

// put function declarations here:
void setupDisplay();
void scrollUp(void);
void scrollDown(void);

IRAM_ATTR void onClockEdge(void);
IRAM_ATTR void onDataEdge(void);
IRAM_ATTR int setBit(int n, int k);
IRAM_ATTR int clearBit(int n, int k);
IRAM_ATTR void clickWheelEvents();
IRAM_ATTR void wheelScroll(int increment);
IRAM_ATTR void processClickWheel(int button, int button_state, int position);

volatile bool START_STOP_FLAG = false;
volatile bool PLAY_PAUSE_FLAG = false;
volatile bool NEXT_SONG_FLAG = false;
volatile bool PREVIOUS_SONG_FLAG = false;
volatile bool GO_BACK_FLAG = false;
volatile bool SCROLL_DOWN_FLAG = false;
volatile bool SCROLL_UP_FLAG = false;

static int scroll_value = 1;

TFT_eSPI tft = TFT_eSPI();

void setupDisplay()
{
    // Initialise the TFT
    tft.begin();
    tft.setRotation(3);
    tft.fillScreen(TFT_RED);
}

void scrollUp()
{
	if (scroll_value <= 0)
	{
		return;
	}

	if (scroll_value > 100)
	{
		scroll_value = 100;
	}

	if (scroll_value > -1)
	{
		scroll_value--;
	}

	SCROLL_UP_FLAG = false;
}

void scrollDown()
{
	if (scroll_value >= 100)
	{ // 255
		return;
	}

	if (scroll_value <= 100)
	{
		scroll_value++;

		if (scroll_value > 100)
		{
			scroll_value = 256;
		}
	}

	SCROLL_DOWN_FLAG = false;
}

IRAM_ATTR void onDataEdge()
{
	dataBit = (digitalRead(DATA_PIN));
}

IRAM_ATTR void onClockEdge()
{
	if (dataBit == 0)
	{
		recording = 1;
		oneCount = 0;
	}
	else
	{
		// 32 1's in a row means we're definitely not in the middle of a packet
		if (++oneCount >= BIT_COUNT)
		{
			recording = 0;
			bitIndex = 0;
		}
	}
	// in the middle of the packet
	if (recording == 1)
	{
		if (dataBit)
		{
			bits = setBit(bits, bitIndex);
		}
		else
		{
			bits = clearBit(bits, bitIndex);
		}
		// we've collected the whole packet
		if (++bitIndex == 32)
		{
			bitIndex = 0;
			// click_wheel_packets_received = true;
			clickWheelEvents(); // lets not call this inside the interuptloop
		}
	}
}

ICACHE_RAM_ATTR void clickWheelEvents()
{
	// parse packet and broadcast data
	if ((bits & PACKET_START) != PACKET_START)
	{
		return;
	}
	for (size_t i = 0; i < BUFFER_SIZE; i++)
	{
		buffer[i] = -1;
	}

	for (size_t i = 0; i < sizeof(buttons); i++)
	{
		char buttonIndex = buttons[i];
		if ((bits >> buttonIndex) & 1 && !((lastBits >> buttonIndex) & 1))
		{
			buffer[BUTTON_INDEX] = buttonIndex;
			buffer[BUTTON_STATE_INDEX] = 1;
		}
		else if (!((bits >> buttonIndex) & 1) && (lastBits >> buttonIndex) & 1)
		{
			buffer[BUTTON_INDEX] = buttonIndex;
			buffer[BUTTON_STATE_INDEX] = 0;
		}
	}

	uint8_t wheelPosition = (bits >> 16) & 0xFF;

	buffer[WHEEL_POSITION_INDEX] = wheelPosition;
	if (memcmp(prev_buffer, buffer, BUFFER_SIZE) == 0)
	{
		return;
	}

	lastBits = bits;

	processClickWheel((int)buffer[BUTTON_INDEX], (int)buffer[BUTTON_STATE_INDEX],
					  (int)buffer[WHEEL_POSITION_INDEX]);

	memcpy(prev_buffer, buffer, BUFFER_SIZE);
}

// Function to set the kth bit of n
ICACHE_RAM_ATTR int setBit(int n, int k) { return (n | (1 << (k - 1))); }

// Function to clear the kth bit of n
ICACHE_RAM_ATTR int clearBit(int n, int k) { return (n & (~(1 << (k - 1)))); }

ICACHE_RAM_ATTR void processClickWheel(int button, int button_state, int position)
{

	if (button == WHEEL_TOUCH_BIT && button_state == 0)
	{
		// Finger has left scroll wheel
		wheel_scroll_lift = true;
		// Reset to circular buffer of wheels scroll events to reset as scroll directions as the finger has left the wheel
		ClickWheelScrollPrev[5] = 0;
		ClickWheelScrollPrev[4] = 0;
		ClickWheelScrollPrev[3] = 0;
		ClickWheelScrollPrev[2] = 0;
		ClickWheelScrollPrev[1] = 0;
		ClickWheelScrollPrev[0] = 0;
	}

	if (button == WHEEL_TOUCH_BIT && button_state == 1)
	{
		// Finger has Been presented to touch wheel
		wheel_scroll_lift = false;
		return;
	}

	if (button == 255 && button_state == 255)
	{
		// Finger is scrolling wheel
		if (ClickWheelClicked)
		{
			ClickWheelClicked = false; // ignore an initial touch on wheel just after a click
			return;
		}

		// Click wheel events run from 1-48 so i use this to managed the
		// transitions fro 48 to 0 or 0 to 48
		if (last_wheel_position > 45 && position < 5)
		{
			wheelScroll(-1);
			SCROLL_DOWN_FLAG = true;
		}
		else if (last_wheel_position < 5 && position > 45)
		{
			wheelScroll(1);
			SCROLL_UP_FLAG = true;
		}
		else if (last_wheel_position > position)
		{
			wheelScroll(-1);
			SCROLL_DOWN_FLAG = true;
		}
		else
		{
			wheelScroll(1);
			SCROLL_UP_FLAG = true;
		}
		last_wheel_position = position;
		InputReceived = true; // InputSleepCheck
		return;
	}

	if (button_state == 0 && button != 255)
	{
		// We have a button click event
		ClickWheelClicked = true;
		switch (button)
		{
		case CENTER_BUTTON_BIT:
			InputReceived = true; // InputSleepCheck
			HapticFeebackDue = 1;
			START_STOP_FLAG = true;
			delay(250);
			break;
		case LEFT_BUTTON_BIT:
			InputReceived = true; // InputSleepCheck
			HapticFeebackDue = 1;
			PREVIOUS_SONG_FLAG = true;
			delay(250);
			break;
		case RIGHT_BUTTON_BIT:
			InputReceived = true; // InputSleepCheck
			HapticFeebackDue = 1;
			NEXT_SONG_FLAG = true;
			delay(250);
			break;
		case UP_BUTTON_BIT:
			InputReceived = true; // InputSleepCheck
			HapticFeebackDue = 1;
			GO_BACK_FLAG = true;
			delay(250);
			break;
		case DOWN_BUTTON_BIT:
			InputReceived = true; // InputSleepCheck
			HapticFeebackDue = 1;
			PLAY_PAUSE_FLAG = true;
			delay(250);
			break;
		}
	}
}

ICACHE_RAM_ATTR void wheelScroll(int increment)
{
	// Very simple and dirty smoothing function as I seem to be getting random
	// reverse events on scrolling
	ClickWheelScrollPrev[5] = ClickWheelScrollPrev[4];
	ClickWheelScrollPrev[4] = ClickWheelScrollPrev[3];
	ClickWheelScrollPrev[3] = ClickWheelScrollPrev[2];
	ClickWheelScrollPrev[2] = ClickWheelScrollPrev[1];
	ClickWheelScrollPrev[1] = ClickWheelScrollPrev[0];
	ClickWheelScrollPrev[0] = increment;

	int NumMirrorResutsinhistory = 0;

	for (int i = 0; i < 5; i++)
	{
		// check scroll history and count number of times the current
		// direction has been recorded
		if (ClickWheelScrollPrev[i] == increment)
		{
			NumMirrorResutsinhistory++;
		}
	}

	// ignore the current wheel event if not simalar to recent
	if (NumMirrorResutsinhistory < ClickWheelNoiseFilterAmt)
	{
		return;
	}

	WheelSensitivity = 0;
	HapticFeebackDue = 1;
}

//***********************************************
//
// 					SETUP
//
//***********************************************

void setup()
{
	delay(3000);

	setupDisplay();

	// Initiaise the Apple Click Wheel
	// Ipod Classic 4th generation click wheel,
	// configure pins and input interupts
	pinMode(CLOCK_PIN, INPUT_PULLUP);
	pinMode(DATA_PIN, INPUT_PULLUP);
	attachInterrupt(digitalPinToInterrupt(CLOCK_PIN), onClockEdge, RISING);
	attachInterrupt(digitalPinToInterrupt(DATA_PIN), onDataEdge, CHANGE);

	Serial.println("Setup done");
}

//***********************************************
//
// 					LOOP
//
//***********************************************

void loop()
{

	if (START_STOP_FLAG)
	{
		Serial.println("Middle button pressed");
		START_STOP_FLAG = false;
	}

	if (PLAY_PAUSE_FLAG)
	{
		Serial.println("Down button pressed");
		PLAY_PAUSE_FLAG = false;
	}

	if (NEXT_SONG_FLAG)
	{
		Serial.println("Right button pressed");
		NEXT_SONG_FLAG = false;
	}

	if (PREVIOUS_SONG_FLAG)
	{
		Serial.println("Left button pressed");
		PREVIOUS_SONG_FLAG = false;
	}

	if (GO_BACK_FLAG)
	{
		Serial.println("Top button pressed");
		GO_BACK_FLAG = false;
	}

	if (SCROLL_UP_FLAG)
	{
		scrollDown();
	}

	if (SCROLL_DOWN_FLAG)
	{
		scrollUp();
	}
}
