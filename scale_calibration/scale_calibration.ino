#include <LiquidCrystal.h>
#include "HX711.h"


// HX711 circuit wiring
#define DOUT    A3
#define SCK     A2
#define FACTOR  -361.5

// 1602 LCD wiring
#define RS 9
#define EN 8
#define D4 7
#define D5 6
#define D6 5
#define D7 4

HX711 scale;
LiquidCrystal lcd(RS, EN, D4, D5, D6, D7);

long time = 0;

float thrust;

void setup()
{
  // INITIATE LOADCELL
  scale.begin(DOUT, SCK);

  // Scale initialization and tare
  scale.read_average(5);
  scale.set_scale(FACTOR); // this value is obtained by calibrating the scale with known weights; see the README for details
    scale.tare();            // reset the scale to 0

    time = 0;
  }

void loop()
{
 if(millis() - time > 150)
 {
  thrust = abs(scale.get_units(3));
 
    if(thrust < 0)
      thrust = 0;

  time = millis();
  lcd.clear();
  lcd.print(thrust, 1);
 }
}
