/*
 **********************************************
 **      __        _______    _______        **
 **     |  |      |   ____|  |   __  \       **
 **     |  |      |  |____   |  |  \  \      **
 **     |  |      |   ____|  |  |   |  |     **
 **     |  |____  |  |____   |  |__/  /      **
 **     |_______| |_______|  |_______/       **
 **                                          **
 **********************************************
 *
 *
 *
 * 		HOW TO USE THE LIBRARY
 * 		1. Initialize the TIMER for the DIN of Addressable LED
 * 				i.	Set the GPIO
 * 				ii.	Set the Period value to 89
 * 				iii.Add the DMA request for that channel and set the direction as Memory to peripheral
 * 		2. Use 'LED_Strip_Init' function to create variable for a strip
 * 		3. Use 'Set_LED' function to fill in values of rgb and brightness
 * 		4. Initialize the nextState to 0 before changing the function
 * 		5. Call the function in the following way inside the while loop
 * 			now = HAL_GetTick();
 *	  	  	if(now - strip1->prev >= strip1->timeout){
 *		  	  	  glow_alternate(strip1, c1, c2);
 *		  	  	  Display(strip1);
 *		  	  	  strip1->prev = HAL_GetTick();
 *		  	}
 *
 */

#include "ws2812b.h"


/***
 * LED strip intialization
 * params:
 * 		int len - length of the strip
 * 		TIM_HandleTypeDef *htim - pointer to timer handle responsible for the strip's PWM input
 * 		uint32_t channel - channel of the timer connected to the LED strip
 * 		int timeout - timeout for the LED strip - controls the speed of display
 * 	return val:
 * 		LEDstrip* - the struct for the LED strip
 */
LEDstrip* LED_Strip_Init(int len, TIM_HandleTypeDef *htim, uint32_t channel,
		int timeout, int max_freq) {
	LEDstrip *strip;
	strip = (LEDstrip*) malloc(sizeof(LEDstrip));
	strip->htim = htim;
	strip->channel = channel;
	strip->len = len;
	strip->timeout = timeout;
	strip->period = (max_freq *1000000 / (htim->Init.Prescaler + 1)) / 800000;
	htim->Init.Period = strip->period - 1;
	strip->prev = HAL_GetTick();
	strip->nextState = 0;
	// default state
	for (int i = 0; i < len; i++)
		Set_LED(&(strip->led[i]), 0, 0, 0, 100);
	Display(strip);
	return strip;
}

/***
 * Deallocating the strip variable
 * params:
 * 		LEDstrip* strip - pointer to the strip variable to be deallocated
 */
void free_strip(LEDstrip *strip) {
	free(strip);
}

/***
 * Setting LED struct values
 * params:
 * 		LED *led - pointer to LED struct
 * 		uint8_t red, green, blue - 8 bit int values for RGB
 * 		int brightness - brightness percentage of the LED
 */
void Set_LED(LED *led, uint8_t red, uint8_t green, uint8_t blue, int brightness) {
	led->blue = blue * brightness / 100;
	led->red = red * brightness / 100;
	led->green = green * brightness / 100;
	led->color = led->blue + (led->red << 8) + (led->green << 16);
	led->brightness = brightness;
}

/***
 * Display the LED strip
 * params:
 * 		LEDstrip *strip - pointer to the LED strip to be displayed
 */
void Display(LEDstrip *strip) {
	int index = 0;
	for (int p = 0; p < strip->len; p++) {
		for (int i = 23; i >= 0; i--) {
#ifdef BLUEPILL
			if (strip->led[p].color & (1 << i))
				strip->dutyCycle[index] = 60; // 60;  // 2/3 of 90
			else
				strip->dutyCycle[index] = 30; // 30;  // 1/3 of 90
#else
			if (strip->led[p].color & (1 << i))
				strip->dutyCycle[index] =
						(strip->htim->Instance != TIM3) ? 105 : 73; // 60;  // 2/3 of 90
			else
				strip->dutyCycle[index] =
						(strip->htim->Instance != TIM3) ? 70 : 35; // 30;  // 1/3 of 90
#endif
			index++;
		}
	}
	// reset for >= 50us
	for (int i = 0; i < 50; i++)
		strip->dutyCycle[index++] = 0;

	HAL_TIM_PWM_Start_DMA(strip->htim, strip->channel,
			(uint32_t*) (strip->dutyCycle), index);
}


/***
 * Sets RGB values to 1 LED
 * params:
 * 		LEDstrip *strip - pointer to the LED strip to be displayed
 * 		LED color - the RGB values to be displayed
 * 		int no - LED number - 0 to len-1
 */
void Set_One(LEDstrip *strip, LED color, int no) {
	for (int j = 0; j < strip->len; j++) {
		if (j == no)
			Set_LED(&(strip->led[j]), color.red, color.green, color.blue,
					color.brightness);
		else
			Set_LED(&(strip->led[j]), 0, 0, 0, 0);
	}
}

/***
 * Sets RGB values to all LEDs
 * params:
 * 		LEDstrip *strip - pointer to the LED strip to be displayed
 * 		LED color - the RGB values to be displayed
 */
void All_ON(LEDstrip *strip, LED color) {
	for (int j = 0; j < strip->len; j++)
		Set_LED(&(strip->led[j]), color.red, color.green, color.blue,
				color.brightness);
}

/***
 * Resets all LEDs
 * params:
 * 		LEDstrip *strip - pointer to the LED strip to be displayed
 */
void All_OFF(LEDstrip *strip) {
	for (int j = 0; j < strip->len; j++)
		Set_LED(&(strip->led[j]), 0, 0, 0, 0);
}

/***
 * Toggles all LEDs
 * params:
 * 		LEDstrip *strip - pointer to the LED strip to be displayed
 * 		LED color - the RGB values to be displayed
 */
void All_Blink(LEDstrip *strip, LED color) {
	for (int j = 0; j < strip->len; j++)
		Set_LED(&(strip->led[j]), color.red, color.green, color.blue,
				(strip->nextState % 2) * 100);
	strip->nextState += 1;
}

/***
 * Points towards left - clockwise - moves by 1 frame
 * params:
 * 		LEDstrip *strip - pointer to the LED strip to be displayed
 * 		LED moving - LED variable for moving color
 * 		LED bg - LED variable for background color
 */
void moveLeft(LEDstrip *strip, LED moving, LED bg) {
	for (int j = 0; j < strip->len; j++) {
		if (j == strip->nextState)
			Set_LED(&(strip->led[j]), moving.red, moving.green, moving.blue,
					moving.brightness);
		else
			Set_LED(&(strip->led[j]), bg.red, bg.green, bg.blue, bg.brightness);
	}
	strip->nextState = (
			(strip->nextState == strip->len - 1) ? 0 : (strip->nextState + 1));
}

/***
 * Points towards right - counter clockwise - moves by 1 frame
 * params:
 * 		LEDstrip *strip - pointer to the LED strip to be displayed
 * 		LED moving - LED variable for moving color
 * 		LED bg - LED variable for background color
 */
void moveRight(LEDstrip *strip, LED moving, LED bg) {
	for (int j = 0; j < strip->len; j++) {
		if (j == strip->nextState)
			Set_LED(&(strip->led[j]), moving.red, moving.green, moving.blue,
					moving.brightness);
		else
			Set_LED(&(strip->led[j]), bg.red, bg.green, bg.blue, bg.brightness);
	}
	strip->nextState =
			((strip->nextState == 0) ? (strip->len - 1) : (strip->nextState - 1));
}

/***
 * Points towards centre - moves by 1 frame
 * params:
 * 		LEDstrip *strip - pointer to the LED strip to be displayed
 * 		LED moving - LED variable for moving color
 * 		LED bg - LED variable for background color
 */
void converge(LEDstrip *strip, LED moving, LED bg) {
	for (int j = 0; j < strip->len; j++) {
		if (j == strip->nextState || j == strip->len - 1 - strip->nextState)
			Set_LED(&(strip->led[j]), moving.red, moving.green, moving.blue,
					moving.brightness);
		else
			Set_LED(&(strip->led[j]), bg.red, bg.green, bg.blue, bg.brightness);
	}
	strip->nextState = (
			(strip->nextState == strip->len / 2) ? 0 : (strip->nextState + 1));
}

/***
 * Points away from centre - moves by 1 frame
 * params:
 * 		LEDstrip *strip - pointer to the LED strip to be displayed
 * 		LED moving - LED variable for moving color
 * 		LED bg - LED variable for background color
 */
void diverge(LEDstrip *strip, LED moving, LED bg) {
	for (int j = 0; j < strip->len; j++) {
		if (j == strip->nextState || j == strip->len - 1 - strip->nextState)
			Set_LED(&(strip->led[j]), moving.red, moving.green, moving.blue,
					moving.brightness);
		else
			Set_LED(&(strip->led[j]), bg.red, bg.green, bg.blue, bg.brightness);
	}
	strip->nextState =
			((strip->nextState == 0) ? (strip->len / 2) : (strip->nextState - 1));
}

/***
 * Fading effect - moves by 1 frame
 * Moves towards left with reducing brightness
 * params:
 * 		LEDstrip *strip - pointer to the LED strip to be displayed
 * 		LED moving - LED variable for moving color
 */
void fading_effect_left(LEDstrip *strip, LED moving) {
	for (int j = 0; j < strip->len; j++) {
		if (j == strip->nextState)
			Set_LED(&(strip->led[j]), moving.red, moving.green, moving.blue,
					100);
		else if (j == strip->nextState - 1
				|| (j + 1) % strip->len == strip->nextState)
			Set_LED(&(strip->led[j]), moving.red, moving.green, moving.blue,
					75);
		else if (j == strip->nextState - 2
				|| (j + 2) % strip->len == strip->nextState)
			Set_LED(&(strip->led[j]), moving.red, moving.green, moving.blue,
					50);
		else if (j == strip->nextState - 3
				|| (j + 3) % strip->len == strip->nextState)
			Set_LED(&(strip->led[j]), moving.red, moving.green, moving.blue,
					25);
		else
			Set_LED(&(strip->led[j]), moving.red, moving.green, moving.blue, 0);
	}
	strip->nextState = (
			(strip->nextState == strip->len - 1) ? 0 : (strip->nextState + 1));
}

/***
 * Fading effect - moves by 1 frame
 * Moves towards right with reducing brightness
 * params:
 * 		LEDstrip *strip - pointer to the LED strip to be displayed
 * 		LED moving - LED variable for moving color
 */
void fading_effect_right(LEDstrip *strip, LED moving) {
	for (int j = 0; j < strip->len; j++) {
		if (j == strip->nextState)
			Set_LED(&(strip->led[j]), moving.red, moving.green, moving.blue,
					100);
		else if (j == (strip->nextState + 1) % (strip->len))
			Set_LED(&(strip->led[j]), moving.red, moving.green, moving.blue,
					75);
		else if (j == (strip->nextState + 2) % (strip->len))
			Set_LED(&(strip->led[j]), moving.red, moving.green, moving.blue,
					50);
		else if (j == (strip->nextState + 3) % (strip->len))
			Set_LED(&(strip->led[j]), moving.red, moving.green, moving.blue,
					25);
		else
			Set_LED(&(strip->led[j]), moving.red, moving.green, moving.blue, 0);
	}
	strip->nextState = (
			(strip->nextState == 0) ? strip->len - 1 : (strip->nextState - 1));
}

LED rainbow_colors[12] = { { .red = 0xFF, .green = 0x00, .blue = 0x00 }, {
		.red = 0xD5, .green = 0x2A, .blue = 0x00 },
		// {.red=0xAB, .green=0x55, .blue=0x00},
		{ .red = 0xAB, .green = 0x7F, .blue = 0x00 }, { .red = 0xAB, .green =
				0xAB, .blue = 0x00 },
		{ .red = 0x56, .green = 0xD5, .blue = 0x00 }, { .red = 0x00, .green =
				0xFF, .blue = 0x00 },
		// {.red=0x00, .green=0xD5, .blue=0x2A},
		{ .red = 0x00, .green = 0xAB, .blue = 0x55 }, { .red = 0x00, .green =
				0x56, .blue = 0xAA },
		{ .red = 0x00, .green = 0x00, .blue = 0xFF },
		// {.red=0x2A, .green=0x00, .blue=0xD5},
		{ .red = 0x55, .green = 0x00, .blue = 0xAB }, { .red = 0x7F, .green =
				0x00, .blue = 0x81 },
		{ .red = 0XAB, .green = 0x00, .blue = 0x55 },
// {.red=0xD5, .green=0x00, .blue=0x2B},
		};

LED blue_shades[12] = { { .red = 0x87, .green = 0xCE, .blue = 0xFA }, { .red =
		0x00, .green = 0xBF, .blue = 0xFF }, { .red = 0x1E, .green = 0x90,
		.blue = 0xFF }, { .red = 0x00, .green = 0x00, .blue = 0xFF }, { .red =
		0x00, .green = 0x00, .blue = 0xCD }, { .red = 0x00, .green = 0x00,
		.blue = 0x8B }, { .red = 0x00, .green = 0x00, .blue = 0x8B }, { .red =
		0x00, .green = 0x00, .blue = 0xCD }, { .red = 0x00, .green = 0x00,
		.blue = 0xFF }, { .red = 0x1E, .green = 0x90, .blue = 0xFF }, { .red =
		0x00, .green = 0xBF, .blue = 0xFF }, { .red = 0x87, .green = 0xCE,
		.blue = 0xFA }, };

LED red_shades[12] = {

{ .red = 0xFF, .green = 0x40, .blue = 0x40 }, { .red = 0xFF, .green = 0x33,
		.blue = 0x33 }, { .red = 0xFF, .green = 0x26, .blue = 0x26 }, { .red =
		0xFF, .green = 0x19, .blue = 0x19 }, { .red = 0xFF, .green = 0x0D,
		.blue = 0x0D }, { .red = 0xFF, .green = 0x00, .blue = 0x00 },

{ .red = 0xFF, .green = 0x00, .blue = 0x00 }, { .red = 0xFF, .green = 0x0D,
		.blue = 0x0D }, { .red = 0xFF, .green = 0x19, .blue = 0x19 }, { .red =
		0xFF, .green = 0x26, .blue = 0x26 }, { .red = 0xFF, .green = 0x33,
		.blue = 0x33 }, { .red = 0xFF, .green = 0x40, .blue = 0x40 },

};

/***
 * Shows colors from the given range - moves by 1 frame
 * params:
 * 		LEDstrip *strip - pointer to the LED strip to be displayed
 * 		shade sh - range of colors - values can be found at enum shade
 */
void diffuser_effect(LEDstrip *strip, shade sh) {
	for (int j = 0; j < strip->len; j++) {
		int ind = (strip->nextState + j) % 12;
		if (sh == RAINBOW)
			Set_LED(&(strip->led[j]), rainbow_colors[ind].red,
					rainbow_colors[ind].green, rainbow_colors[ind].blue, 100);
		else if (sh == BLUE)
			Set_LED(&(strip->led[j]), blue_shades[ind].red,
					blue_shades[ind].green, blue_shades[ind].blue, 100);
		else if (sh == RED)
			Set_LED(&(strip->led[j]), red_shades[ind].red,
					red_shades[ind].green, red_shades[ind].blue, 100);
	}
	strip->nextState = strip->nextState + 1;
}

/***
 * Alternates between 2 colors for alternate pins - changes by 1 frame
 * params:
 * 		LEDstrip *strip - pointer to the LED strip to be displayed
 * 		LED color1, color2 - LED variable for color
 */
void glow_alternate(LEDstrip *strip, LED color1, LED color2) {
	for (int j = 0; j < strip->len; j++) {
		if (j % 2 == strip->nextState)
			Set_LED(&(strip->led[j]), color1.red, color1.green, color1.blue,
					color1.brightness);
		else
			Set_LED(&(strip->led[j]), color2.red, color2.green, color2.blue,
					color2.brightness);
	}
	strip->nextState = (strip->nextState) ? 0 : 1;
}

/***
 * Alternates between 2 colors for alternate pins - changes by 1 frame
 * params:
 * 		LEDstrip *strip - pointer to the LED strip to be displayed
 * 		LED color1, color2 - LED variable for color
 *
 * 		optimum:
 * 		timeout = 150, INC = 5
 */
int state = 0;
void pulsating_effect(LEDstrip *strip, LED color) {
	if (strip->nextState > 100 || strip->nextState < 0) {
		strip->pulse = (-1) * (strip->pulse);
		strip->nextState += strip->pulse;
	}
	if (strip->channel == 0)
		state = strip->nextState;
	for (int j = 0; j < strip->len; j++) {
		Set_LED(&(strip->led[j]), color.red, color.green, color.blue,
				strip->nextState);
	}
	strip->nextState += strip->pulse;
}


LED battery_color[5] = {
		{ .red = 0x00, .green = 0xFF, .blue = 0x00 },
		{ .red = 0xFF, .green = 0xFF, .blue = 0x00 },
		{ .red = 0xFF, .green = 0x80, .blue = 0x00 },
		{ .red = 0xFF, .green = 0x00, .blue = 0x00 },
		{ .red = 0xCC, .green = 0x00, .blue = 0x00 }
};

void level_indicator(LEDstrip *strip, int level) {
	int i = 0, j = 0;
	float inc = 0;
	int size = strip->len / 5 + (strip->len%5!=0 ? 1:0)  ;
	for(j = 0; j <= strip->len; j++, inc=(100*j/strip->len)) {
		if (level <= inc) {
			i = j;
			break;
		}
	}
	i = strip->len-1-i;
	LED color = battery_color[(i/size >= 4 ? 4:i/size)];
	for (j = strip->len - 1; j >= 0; j--) {
		if (j > i)			// j > i??
			Set_LED(&(strip->led[j]), color.red, color.green, color.blue, 100);
		else
			Set_LED(&(strip->led[j]), 0, 0, 0, 0);
	}
}

