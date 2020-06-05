/*

		 D12 -> LCD 14
		 D11 -> LCD 13
		 D10 -> LCD 12
		 D9  -> LCD 11
		 D8  -> LCD 6
		 D7  -> LCD 4
		 
		 D6  -> plus button
		 D5  -> select button
		 D4	 -> minus button
		 
		 D3  -> trigger signal, 5V max
		 D2  -> external trigger signal (30/10 div)
		 
		 A0  -> voltmeter (100\10 div)
		 A1  -> current sensor
		 A2  -> tensometer
		 A3  -> tensometer
*/

#include <LiquidCrystal.h>
#include "HX711.h"

#define ADC_VREF_TYPE (1 << REFS0)

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

// inputs wiring
#define OPTO 3
#define EXT 2
#define LED 13
#define VIN A0
#define CIN A1

// buttons wiring
#define DECR 12
#define INC 11
#define OPT A4
#define SEL 10

HX711 scale;
LiquidCrystal lcd(RS, EN, D4, D5, D6, D7);

//  VOLATILE DATA TYPE TO STORE REVOLUTIONS
volatile int REV;

// Settings
byte mode = 0, bladesCount = 2, screen = 1;

//  DEFINE RPM AND MAXIMUM RPM
float rpm, thrust, maxThrust, lastMaxThrust, maxCurrent, lastMaxCurrent;
int maxRPM, lastMaxRPM;

unsigned long time; //  DEFINE TIME TAKEN TO COVER ONE REVOLUTION

int RPMlen, prevRPM; //  INTEGERS TO STORE LED VALUE AND CURRENT RPM AND PREVIOUS RPM

byte led;
byte flag = 0; //  A VARIABLE TO DETERMINE WHETHER THE LCD NEEDS TO BE CLEARED OR NOT

unsigned long prevtime = 0; //  STORE IDLE TIME TO TOGGLE MENU

byte lastIncState = 1;
byte lastDecState = 1;
byte lastSelState = 1;
byte lastOptState = 1;

long Adc[2];

float voltage = 0.0;
float current = 0.0;

float R1 = 100000.0; // resistance of R1 (100K) -see text!
float R2 = 10600.0;  // resistance of R2 (10K) - see text!

const int motorTypesCount = 2;
const String motorTypes[motorTypesCount] = {"12S14P", "9S12P"};
const int motorTypeIdxs[motorTypesCount] = {7, 6};
int motorType = 0;

void MotorTypeSelectScreen()
{
  lcd.clear();
  lcd.print("Motor type: ");
  lcd.setCursor(0, 1);
  lcd.print("<--  ");
  lcd.print(motorTypes[motorType]);
  lcd.print("  -->");
}

void MotorTypeSelectLoop()
{
  int readingInc = digitalRead(INC);
  int readingDec = digitalRead(DECR);
  int readingSel = digitalRead(SEL);

  if (readingInc != lastIncState && readingInc == LOW && motorType < motorTypesCount - 1)
  {
    motorType++;
    MotorTypeSelectScreen();
  }
  else if (readingDec != lastDecState && readingDec == LOW && motorType >= motorTypesCount - 1)
  {
    motorType--;
    MotorTypeSelectScreen();
  }
  else if (readingSel != lastSelState && readingSel == LOW)
  {
    bladesCount = motorTypeIdxs[motorType];

    screen = 0;
    MainScreen();

    if (mode > 1)
      attachInterrupt(digitalPinToInterrupt(EXT), RPMCount, RISING); //  ADD A HIGH PRIORITY ACTION ( AN INTERRUPT)  WHEN THE SENSOR GOES FROM LOW TO HIGH
    else
      attachInterrupt(digitalPinToInterrupt(OPTO), RPMCount, RISING); //  ADD A HIGH PRIORITY ACTION ( AN INTERRUPT)  WHEN THE SENSOR GOES FROM HIGH TO LOW
  }
  // save the reading. Next time through the loop, it'll be the lastButtonState:
  lastIncState = readingInc;
  lastDecState = readingDec;
  lastSelState = readingSel;
}

void BladesSelectScreen()
{
  lcd.clear();
  lcd.print("Blades count: ");
  lcd.setCursor(0, 1);
  lcd.print("<--  ");
  lcd.print(bladesCount, DEC);
  lcd.print("  -->");
}

void BladesSelectLoop()
{
  //setup blades
  int readingInc = digitalRead(INC);
  int readingDec = digitalRead(DECR);
  int readingSel = digitalRead(SEL);

  if (readingInc != lastIncState && readingInc == LOW)
  {
    bladesCount++;
    BladesSelectScreen();
  }
  else if (readingDec != lastDecState && readingDec == LOW && bladesCount > 1)
  {
    bladesCount--;
    BladesSelectScreen();
  }
  else if (readingSel != lastSelState && readingSel == LOW)
  {
    screen = 0;
    MainScreen();

    if (mode > 1)
      attachInterrupt(digitalPinToInterrupt(EXT), RPMCount, RISING); //  ADD A HIGH PRIORITY ACTION ( AN INTERRUPT)  WHEN THE SENSOR GOES FROM LOW TO HIGH
    else
      attachInterrupt(digitalPinToInterrupt(OPTO), RPMCount, RISING); //  ADD A HIGH PRIORITY ACTION ( AN INTERRUPT)  WHEN THE SENSOR GOES FROM HIGH TO LOW
  }

  // save the reading. Next time through the loop, it'll be the lastButtonState:
  lastIncState = readingInc;
  lastDecState = readingDec;
  lastSelState = readingSel;
}

void ModeSelectScreen()
{
  lcd.clear();
  lcd.print("Select mode: ");
  lcd.setCursor(0, 1);
  switch (mode)
  {
  case 0:
    lcd.print("<--   OPTO   -->");
    break;
  case 1:
    lcd.print("<--  PHASE   -->");
    break;
  case 2:
    lcd.print("<--   ESC    -->");
    break;
  }
}

void ModeSelectLoop()
{
  int readingInc = digitalRead(INC);
  int readingDec = digitalRead(DECR);
  int readingSel = digitalRead(SEL);

  if (readingInc != lastIncState && readingInc == LOW && mode < 2)
  {
    mode++;
    ModeSelectScreen();
  }
  else if (readingDec != lastDecState && readingDec == LOW && mode >= 2)
  {
    mode--;
    ModeSelectScreen();
  }
  else if (readingSel != lastSelState && readingSel == LOW)
  {
    screen++;

    switch (mode)
    {
    case 0:
      bladesCount = 2;
      BladesSelectScreen();
      break;
    case 1:
      bladesCount = 7;
      MotorTypeSelectScreen();
      break;
    case 2:
      bladesCount = 7;
      MotorTypeSelectScreen();
      break;
    }
  }

  // save the reading. Next time through the loop, it'll be the lastButtonState:
  lastIncState = readingInc;
  lastDecState = readingDec;
  lastSelState = readingSel;
}

void ModeSelectStartupLoop()
{  
  int readingInc = digitalRead(INC);
  int readingOpt = digitalRead(OPT);

  if (readingOpt != lastOptState && readingOpt == LOW && readingInc != lastIncState && readingInc == LOW)
  {
    screen = 3;
    mode = 1;
    MotorTypeSelectScreen();
  }
  else if (readingOpt != lastOptState && readingOpt == LOW)
  {
    screen = 2;
    mode = 0;
    BladesSelectScreen();
  }
  else if (readingInc != lastIncState && readingInc == LOW)
  {
    screen = 3;
    mode = 2;
    MotorTypeSelectScreen();
  }
  else if(readingOpt == HIGH && readingInc == HIGH)
  {
    ModeSelectScreen();
  }
  
  // save the reading. Next time through the loop, it'll be the lastButtonState:  
  lastOptState = readingOpt;
  lastIncState = readingInc;
}

void MainScreen()
{
  lcd.clear();
  lcd.print("RPM:  VOLT: KV:");
  lcd.setCursor(0, 1);

  int _rpm = (int)rpm;
  lcd.print(_rpm, DEC);
  lcd.setCursor(6, 1);
  lcd.print(voltage, 1);
  lcd.setCursor(12, 1);

  int _kv = (int)(rpm / voltage);
  lcd.print(_kv, DEC);

  delay(500);

  //second screen
  lcd.clear();
  lcd.print("THRUST:  A:");
  lcd.setCursor(0, 1);
  lcd.print(thrust, 1);
  lcd.setCursor(9, 1);
  lcd.print(current, 2);
  delay(500);
}

void IdleScreen()
{
  lcd.clear();
  lcd.print("RPM:  THR:  A:");
  lcd.setCursor(0, 1);
  lcd.print(lastMaxRPM, DEC);

  lcd.setCursor(6, 1);
  lcd.print(lastMaxThrust, 1);

  lcd.setCursor(12, 1);
  lcd.print(lastMaxCurrent, 1);

  delay(2000);
  lcd.clear();

  lcd.print("IDLE STATE");
  lcd.setCursor(0, 1);
  lcd.print("READY TO MEASURE");
  delay(2000);
}

void MainScreenLoop()
{
  long currtime = millis(); // GET CURRENT TIME
  if (REV > 5 * bladesCount)
  {
    int rev = REV;
    REV = 0;

    getADC();

    thrust = abs(scale.get_units(5));

    if (thrust > maxThrust)
        maxThrust = thrust; //  GET THE MAX thrust THROUGHOUT THE RUN

    current = (2.5 - (5.0 / 2047.0) * Adc[1] - 0.066) / 0.014;
    if (current < 0.09)
      current = 0.0;

    if (current > maxCurrent)
        maxCurrent = current; //  GET THE MAX current THROUGHOUT THE RUN

    voltage = ((Adc[0] * 5.0) / 2047.0) / (R2 / (R1 + R2));

    //statement to quash undesired reading !
    if (voltage < 0.09)
      voltage = 0.0;

    rpm = (60000 / (millis() - time)) * (rev / bladesCount);

    if (rpm > maxRPM)
      maxRPM = (int)rpm; //  GET THE MAX RPM THROUGHOUT THE RUN

    time = millis();

    MainScreen();

    prevtime = currtime; // RESET IDLETIME
  }
  else if ((currtime - prevtime) > 5000 || REV < 5 * bladesCount) //  IF THERE ARE NO READING FOR 5 SEC , THE SCREEN WILL SHOW MAX RPM
  {
    if (maxRPM > 0)
    {
      lastMaxRPM = maxRPM;
      maxRPM = 0;
    }

    if (maxThrust > 0)
    {
      lastMaxThrust = maxThrust;
      maxThrust = 0.0;
    }

    if (maxCurrent > 0)
    {
      lastMaxCurrent = maxCurrent;
      maxCurrent = 0.0;
    }
    IdleScreen();
    prevtime = currtime;
  }
}

void getADC()
{
  for (uint8_t adc_input = 0; adc_input < 2; adc_input++)
  {
    //need at least 2 conversions for more reliable result
    ADMUX = adc_input | ADC_VREF_TYPE;

    // Start the AD conversion
    ADCSRA |= 1 << ADSC;
    // Wait for the AD conversion to complete
    while
      bit_is_set(ADCSRA, ADSC);
    Adc[adc_input] = ADC;

    // Start the second AD conversion
    ADCSRA |= 1 << ADSC;
    // Wait for the AD conversion to complete
    while
      bit_is_set(ADCSRA, ADSC);
    Adc[adc_input] += ADC;
  }
}

void RPMCount()
{
  REV++; // INCREASE REVOLUTIONS

  if (led == LOW)
  {
    led = HIGH; //  TOGGLE STATUS LED
  }
  else
  {
    led = LOW;
  }

  digitalWrite(LED, led);
}

void setup()
{
  //Serial.begin(9600);   // GET VALUES USING SERIAL MONITOR

  // INITIATE LCD
  lcd.begin(16, 2);

  // INITIATE LOADCELL
  scale.begin(DOUT, SCK);

  // buttons
  pinMode(DECR, INPUT);
  pinMode(INC, INPUT);
  pinMode(SEL, INPUT);
  pinMode(OPT, INPUT);

  // INPUTS
  pinMode(VIN, INPUT);
  pinMode(CIN, INPUT);
  pinMode(EXT, INPUT);
  pinMode(OPTO, INPUT);

  pinMode(LED, OUTPUT);

  digitalWrite(DECR, HIGH);
  digitalWrite(INC, HIGH);
  digitalWrite(SEL, HIGH);
  digitalWrite(OPT, HIGH);

  //ADC setup
  ADMUX = ADC_VREF_TYPE;
  ADCSRA = (1 << ADEN) | (1 << ADPS2); // ADC enabled, prescaler division=16 (no interrupt, no auto-triggering)

  // Scale initialization and tare
  scale.read_average(5);
  scale.set_scale(FACTOR); // this value is obtained by calibrating the scale with known weights; see the README for details
  scale.tare();            // reset the scale to 0

  REV = 0; //  START ALL THE VARIABLES FROM 0
  rpm = 0;
  time = 0;

  
  // ModeSelectScreen();
  ModeSelectStartupLoop();
  delay(50);
  ModeSelectStartupLoop();
}

void loop()
{
  switch (screen)
  {
    case 0:
      MainScreenLoop();
      break;
    case 1:
      ModeSelectLoop();
      break;
    case 2:
      BladesSelectLoop();
      break;
    case 3:
      MotorTypeSelectLoop();
      break;
  }
}
