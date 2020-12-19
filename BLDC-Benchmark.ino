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
		 D2  -> nc
		 
		 A0  -> voltmeter (100\10 div)
		 A1  -> current sensor
		 A2  -> tensometer
		 A3  -> tensometer
*/

#include <LiquidCrystal.h>
#include <Adafruit_TiCoServo.h>
#include "HX711.h"

#define ADC_VREF_TYPE (1 << REFS0)

// HX711 circuit wiring
#define DOUT    A3
#define SCK     A2
#define FACTOR  -361.5

//ESC output
#define ESC 10
// 1602 LCD wiring
#define RS 9
#define EN 8
#define D4 7
#define D5 6
#define D6 5
#define D7 4

// inputs wiring
#define EXT 3
#define LED 13
#define VIN A0
#define CIN A1

// buttons wiring
#define DECR 12
#define INC 11
#define OPT A4
#define SEL A5

#define SERVO_MIN 1000 // 1 ms pulse
#define SERVO_MAX 2000 // 2 ms pulse

Adafruit_TiCoServo  servo; 
HX711 scale;
LiquidCrystal lcd(RS, EN, D4, D5, D6, D7);

int throttle = 0;    // variable to store the servo position

//  VOLATILE DATA TYPE TO STORE REVOLUTIONS
volatile int REV;

// Settings
byte mode = 0, bladesCount = 2, screen = 1;

//  DEFINE RPM AND MAXIMUM RPM
float rpm, thrust;

unsigned long time; //  DEFINE TIME TAKEN TO COVER ONE REVOLUTION

unsigned long lastUIUpdate;
unsigned long lastReadingsUpdate;

byte uiPage = 0;

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
    StartMain();
  }
  // save the reading. Next time through the loop, it'll be the lastButtonState:
  lastIncState = readingInc;
  lastDecState = readingDec;
  lastSelState = readingSel;
}

void StartMain()
{
  if(screen > 0)
  {
    screen = 0;

    attachInterrupt(digitalPinToInterrupt(EXT), RPMCount, RISING); //  ADD A HIGH PRIORITY ACTION ( AN INTERRUPT)  WHEN THE SENSOR GOES FROM LOW TO HIGH
  }
}

void MainScreen()
{
  int _rpm = (int)rpm;
  int _kv = (int)(rpm / voltage);
  float _voltage = voltage;
  float _thrust = thrust;
  float _current = current;
  int _throttle = throttle;

  // print to serial
  Serial.print(";G");
  Serial.print(_throttle);
  Serial.print(";R");
  Serial.print(_rpm);
  Serial.print(";V");
  Serial.print(_voltage);
  Serial.print(";A");
  Serial.print(_current);
  Serial.print(";T");
  Serial.print(_thrust);
  Serial.print(";K");
  Serial.print(_kv);
  Serial.println();
  
  lcd.clear();

  if(uiPage == 0)
  {
    lcd.print("RPM:  VOLT: KV:");
  
    lcd.setCursor(0, 1);
  
    lcd.print(_rpm, DEC);
    lcd.setCursor(6, 1);
    lcd.print(_voltage, 1);
    lcd.setCursor(12, 1);
    lcd.print(_kv, DEC);

  }
  else if(uiPage == 1)
  {

    //second screen
    lcd.clear();
    lcd.print("THRUST:  A:");
    lcd.setCursor(0, 1);
    lcd.print(_thrust, 1);
    lcd.setCursor(9, 1);
    lcd.print(_current, 2);
  }
  else
  {
    lcd.clear();
    lcd.print("Throttle pos:");
    lcd.setCursor(0, 1);
    lcd.print(throttle, 1);
  }
}

void GetReadings()
{
  long currtime = millis(); // GET CURRENT TIME
  if (REV > 5 * bladesCount)
  {
    int rev = REV;
    REV = 0;

    getADC();

    thrust = abs(scale.get_units(5));

    current = (2.5 - (5.0 / 2047.0) * Adc[1] - 0.066) / 0.014;
    if (current < 0.09)
      current = 0.0;

    
    voltage = ((Adc[0] * 5.0) / 2047.0) / (R2 / (R1 + R2));

    //statement to quash undesired reading !
    if (voltage < 0.09)
      voltage = 0.0;

    unsigned long delta = millis() - time;
    rpm = (60000 / delta) * (rev / bladesCount);
    
    time = millis();

    prevtime = currtime; // RESET IDLETIME
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
  servo.attach(ESC, SERVO_MIN, SERVO_MAX);  
  
  Serial.begin(38400);   // GET VALUES USING SERIAL MONITOR

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

  
  MotorTypeSelectScreen();
  MotorTypeSelectLoop();
  delay(50);
  MotorTypeSelectLoop();
}

void loop()
{
  if (screen == 0)
  {
    unsigned long currentTime = millis();
    if(millis() - lastReadingsUpdate > 300)
    {
        GetReadings();
        lastReadingsUpdate = millis();
    }
    if(millis() - lastUIUpdate > 600)
    {
      MainScreen();
      lastUIUpdate = millis();
      uiPage++;
      if(uiPage > 2)
      {
        uiPage = 0;
      }
    }    
  }
  else
  {
    MotorTypeSelectLoop();
  }
}

void serialEvent() 
{
  int input = Serial.read();
  
  // set poles count command
  if(input == 101)
  {
    Serial.write(101);
    input = Serial.read();
    if(input > 2 && input % 2 == 0)
    {
      bladesCount = (int)(input/2);
      StartMain();
    }
  }
  
  if(input < 0 || input > 100)
  {
    input = 0;
  }

  if(input != throttle)
  {
    throttle = input;
    servo.write(map(throttle, 0, 100, 0, 180));
  }
}
