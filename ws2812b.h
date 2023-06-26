#include <stdlib.h>
#include <math.h>

#define MAX_LEN 30

#include "stm32f1xx_hal.h"


typedef struct{
	uint8_t blue;
	uint8_t red;
	uint8_t green;
	int color;
	int brightness;
}LED;

typedef struct{
	int len;
	LED led[MAX_LEN];
	uint16_t dutyCycle[MAX_LEN*24 + 50];
	TIM_HandleTypeDef *htim;
	uint32_t channel;
	int timeout;
	uint16_t period;
	uint32_t prev;
	int nextState;
	int pulse;		// for pulsating/breathing effect
}LEDstrip;

typedef enum diffuser_options{
	RAINBOW,
	BLUE,
	RED
}shade;

LEDstrip* LED_Strip_Init(int len, TIM_HandleTypeDef *htim, uint32_t channel, int timeout, int max_freq);
void free_strip(LEDstrip *strip);
void Set_LED(LED *led, uint8_t red, uint8_t green, uint8_t blue, int brightness);
void Display(LEDstrip *strip);
void Clear_Strip(LEDstrip *strip);
void Set_One(LEDstrip *strip, LED color, int no);
void All_ON(LEDstrip *strip, LED color);
void All_OFF(LEDstrip *strip);
void All_Blink(LEDstrip *strip, LED color);
void moveLeft(LEDstrip *strip, LED moving, LED bg);
void moveRight(LEDstrip *strip, LED moving, LED bg);
void converge(LEDstrip *strip, LED moving, LED bg);
void diverge(LEDstrip *strip, LED moving, LED bg);
void fading_effect_left(LEDstrip *strip, LED moving);
void fading_effect_right(LEDstrip *strip, LED moving);
void diffuser_effect(LEDstrip *strip, shade sh);
void glow_alternate(LEDstrip *strip, LED color1, LED color2);
void pulsating_effect(LEDstrip *strip, LED color);
void level_indicator(LEDstrip *strip, int level);
