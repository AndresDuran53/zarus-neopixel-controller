#include <FastLED.h>
#include "IoTController.h"

#define DEBUG_FLAG false
#define DATA_PIN     4  // RGB Strip datapin
#define NUM_LEDS    33  // Total of leds

#define PATTERN_STATIC  1
#define PATTERN_RAINBOW 2
#define PATTERN_BREATHE 3
#define PATTERN_BLINK   4
#define PATTERN_WIPE    5
#define PATTERN_PULSE   6

String deviceType = "rgb-light";  // device type name
String deviceToken = "ifm644";    // device unique token
short consoleLevel = 3;           // debug level

//Neopixeles
CRGB leds[NUM_LEDS];        // array with the color of each LED
CRGB leds_aux[NUM_LEDS];    // auxiliary array with the previous colors of each LED
CRGB color_aux;             // auxiliary color
int display_mode = 0;       // number of the current animation
uint16_t display_step = 0;  // current step of the animation
uint16_t max_steps = 0;     // maximum number of steps of the animation

//Counters
unsigned long ul_PreviousMillis = 0UL;  // counter with the last update in ms
unsigned long ul_Interval = 50UL;       // milliseconds between each step of the animation

void setColorFromHex(String hexValue) {
  Logger::log("Setting Hex Value: " + hexValue, Logger::DEBUG_LOG);
  int number = (int) strtol( &hexValue[1], NULL, 16);
  int r = number >> 16;
  int g = number >> 8 & 0xFF;
  int b = number & 0xFF;
  color_aux = CRGB(r,g,b);
} //setColorFromHex()

void setLightsPattern(String newStatus) {
  Logger::log("Setting New rgb display mode: " + newStatus, Logger::INFO_LOG);
  display_mode = newStatus.toInt();
  cambiarPatron(display_mode);
} //setLightsPattern()

/*
 * We initialize and configure the libraries
 */
void setup() {
  IoTController::setup(deviceType,consoleLevel,deviceToken);
  IoTController::createStoredData("rgb_pattern", "rgbp", 2, "0", "String", [](String newStatus) {setLightsPattern(newStatus);});
  IoTController::createStoredData("rgb_color", "rgbc", 2, "0", "String", [](String hexValue) {setColorFromHex(hexValue);});
  IoTController::init();
  initLedFunctions();   // configuring LED functions
} //setup()


/*
 * main loop
 */
void loop() {
  IoTController::loop();
  stepAnimation();  // execute a step in the animations 
}

/*
 * Function that adjusts the configuration of the NeoPixels
 */
void initLedFunctions() {
  pinMode(DATA_PIN,OUTPUT);
  FastLED.addLeds<NEOPIXEL, DATA_PIN>(leds, NUM_LEDS); // configuring FastLED
  color_aux = CRGB(0, 255, 150);      // assigning the auxiliary color
  cambiarPatron(PATTERN_RAINBOW);     // assigning the current animation
} //initLedFunctions()

/*
 * Function that executes step by step the animations when necessary
 */
void stepAnimation() {
  unsigned long ul_CurrentMillis = millis();                // get the current ms
  if (ul_CurrentMillis - ul_PreviousMillis > ul_Interval) { // check if the waiting time has already passed
    ul_PreviousMillis = millis();                           // update the time of the last execution

    switch (display_mode) {
      case PATTERN_STATIC:
        stepChangeLed(color_aux);
        break;
      case PATTERN_RAINBOW:
        rainbow();
        break;
      case PATTERN_BREATHE:
        breathe();
        break;
      case PATTERN_BLINK:
        blinkLed(color_aux);
        break;   
      case PATTERN_WIPE:
        colorWipe(color_aux);
        break;
      case PATTERN_PULSE:
        pulseColor();
        break;
      default:
        cambiarPatron(PATTERN_STATIC);
        break;
    }
    display_step++;
  }
  if(display_step > max_steps){
    display_step=0;
  }
} //stepAnimation()


//------------LED_Patterns----------------//

/*
 * @params int pPatron; number that represents the new pattern
 * function that performs all necessary changes when updating
 * the current pattern
 */
void cambiarPatron(int pPatron){
  if(DEBUG_FLAG){
   Serial.print("Patron: ");
   Serial.println(pPatron);
  }
  display_mode = pPatron;   // cambia el patron actual por nuevo
  switch (display_mode) {
    case PATTERN_STATIC:
      max_steps = NUM_LEDS;
      ul_Interval = 50UL;
      break;

    case PATTERN_RAINBOW:
      max_steps = 255;
      ul_Interval = 50UL;
      break;

    case PATTERN_BREATHE:
      max_steps = (101*4)-1;
      ul_Interval = 10UL;
      for (uint16_t i = 0; i < NUM_LEDS; i++) leds_aux[i] = leds[i];
      break;

    case PATTERN_BLINK:
      max_steps = 5;
      ul_Interval = 500UL;
      break;

    case PATTERN_WIPE:
      max_steps = (NUM_LEDS*2)*2;
      ul_Interval = 50UL;
      break;

    case PATTERN_PULSE:
      max_steps = (101*4);
      ul_Interval = 2UL;
      break;

    default:
      max_steps = NUM_LEDS;
      ul_Interval = 50UL;
      break;
  }
  display_step = 0;
  ul_PreviousMillis = 0UL;
}

/*
 * Funcion que apaga todos los leds
 */
void turnOffLeds(){
  for (uint16_t i = 0; i < NUM_LEDS; i++) leds[i] = CRGB(0, 0, 0);
  FastLED.show();
}

/*
 * @params byte WheelPos; un valor de 0 a 255 que representa
 * la posicion actual en el ciclo.
 * Funcion que retorna un valor CRGB (color)
 */
CRGB Wheel(byte WheelPos) {
  WheelPos = 255 - WheelPos;
  if (WheelPos < 85) {
    return CRGB(255 - WheelPos * 3, 0, WheelPos * 3);
  }
  if (WheelPos < 170) {
    WheelPos -= 85;
    return CRGB(0, WheelPos * 3, 255 - WheelPos * 3);
  }
  WheelPos -= 170;
  return CRGB(WheelPos * 3, 255 - WheelPos * 3, 0);
} //Wheel()

// run the single color up the bar one LED at a time
/*
 * @params CRGB pColor; color con el que se rellenaran los LEDS
 * Funcion que va encendiendo los leds uno por uno con el color
 * designado
 */
void stepChangeLed(CRGB pColor) {
  leds[display_step % NUM_LEDS] = pColor;
  FastLED.show();
}

/*
 * Funcion que a un led especifico le asigna un color especifico
 * siguiendo un orden dado por la funcion Wheel()
 */
void rainbow() {
  leds[display_step % NUM_LEDS] = Wheel(display_step % 255);
  FastLED.show();
}

/*
 * Funcion que atenua el brillo de los colores actuales con un patron
 * simulando una respiracion
 */
void breathe() {
  int porcentaje = (int)(display_step%101);
  if(display_step%202>100){
    porcentaje = 100-porcentaje;
  }
  
  for (uint16_t i = 0; i < NUM_LEDS; i++) {
    leds[i] = CRGB( (leds_aux[i].red * porcentaje / 100),
                    (leds_aux[i].green * porcentaje / 100),
                    (leds_aux[i].blue * porcentaje / 100));
  }
  FastLED.show();
}

/*
 * @params CRGB pColor; color con el haran blink los leds
 * Funcion que logra hacer un Blink de un color solido
 * sobre los colores actuales sin modificar el patron de color existente
 */
void blinkLed(CRGB pColor) {
  //int indice_aux = display_step % NUM_LEDS;
  if (((display_step + (display_step / NUM_LEDS)) % 2) == 0) {
    for (uint16_t i = 0; i < NUM_LEDS; i++){
      leds_aux[i] = leds[i];
      leds[i] = pColor;
    }
  } else {
    for (uint16_t i = 0; i < NUM_LEDS; i++){
      leds[i] = leds_aux[i];
    }
  }
  FastLED.show();
}

void colorWipe(CRGB pColor) {
  int indice_aux = display_step%(NUM_LEDS*2);
  if (indice_aux < NUM_LEDS) {
    leds[indice_aux] = CRGB(0,0,0);
  }
  else {
    leds[indice_aux - NUM_LEDS] = pColor;
  }
  FastLED.show();
}

void pulseColor() {
  int porcentaje = (int)(display_step%101);  
  for (uint16_t i = 0; i < NUM_LEDS; i++) {
    leds[i] = CRGB( (leds_aux[i].red * porcentaje / 100),
                    (leds_aux[i].green * porcentaje / 100),
                    (leds_aux[i].blue * porcentaje / 100));
  }
  FastLED.show();
}

