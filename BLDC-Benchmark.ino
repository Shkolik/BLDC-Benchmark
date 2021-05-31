#include <LiquidCrystal.h>
#include <Adafruit_TiCoServo.h>
#include "HX711.h"

#include "menu.h"

#include <GyverFilters.h>
#include "GyverEncoder.h"

// HX711 circuit wiring
#define DOUT    A3
#define SCK     A2
#define FACTOR  -361.5 // scale factor

//ESC output
#define ESC 10

// 1602 LCD wiring
#define V0 5
#define RS 12
#define EN 11
#define D4 9
#define D5 8
#define D6 7
#define D7 6

// inputs wiring
#define EXT 3
#define LED 13
#define VIN A1
#define CIN A0

// buttons wiring
#define ENCL A5
#define ENCR A4
#define SEL 4

#define SERVO_MIN 1000 // 1 ms pulse
#define SERVO_MAX 2000 // 2 ms pulse

// current sensor scale
#define C_SCALE 0.066

#define R1 100000.0 // resistance of R1 (100K) -see text!
#define R2 10600.0  // resistance of R2 (10K) - see text!

#define VCC 5.0

#include "Tacho.h"

Tacho tacho;

Adafruit_TiCoServo  servo;
HX711 scale;
LiquidCrystal lcd(RS, EN, D4, D5, D6, D7);
Encoder encoder(ENCR, ENCL, SEL, TYPE1);

int throttle = -1;    // servo position

volatile byte elapsed = 0;

// Settings
byte mode = 0; // handheld by default.
byte polesCount = 2, screen = 1;

int rpm = 0;
float thrust = 0.0;
float voltage = 0.0;
float current = 0.0;

GMedian<3, float> voltageFilter;
GMedian<5, float> currentFilter;
GMedian<5, float> thrustFilter;

unsigned long time; //  DEFINE TIME TAKEN TO COVER ONE REVOLUTION

unsigned long lastUIUpdate;
unsigned long lastReadingsUpdate;

byte uiPage = 0;

byte led;
byte scaleEnable = 0;

byte lastSelState = 1;

const int motorTypesCount = 2;
const int motorTypeIdxs[motorTypesCount] = {7, 6};
byte motorType = 0;

bool encoderRight = false;
bool encoderLeft = false;
bool clicked = false;

void StartMain()
{
  if (screen > 0)
  {
    screen = 0;

    attachInterrupt(digitalPinToInterrupt(EXT), RPM_ISR, RISING); //  ADD A HIGH PRIORITY ACTION ( AN INTERRUPT)  WHEN THE SENSOR GOES FROM LOW TO HIGH
  }
}

void MainScreen()
{
  int _kv = (int)(rpm /voltage);
  
  // print to serial
  Serial.print(";G");
  Serial.print(throttle);
  Serial.print(";R");
  Serial.print(rpm);
  Serial.print(";V");
  Serial.print(voltage);
  Serial.print(";A");
  Serial.print(current);
  Serial.print(";T");
  Serial.print(thrust);
  Serial.print(";K");
  Serial.print(_kv);
  Serial.println();

  lcd.clear();

  switch(mode)
  {
    case 0:   // Handheld tachometer 
      lcd.print(readPgmString(&s_rpm[0]));
      
      printBlades();
      
      lcd.setCursor(0, 1);
      lcd.print(rpm, DEC);
      break;
    case 1:   // Connected to PC stand
      /*lcd.print(readPgmString(pgm_read_word(&l_headers[uiPage]))); // print header
      lcd.setCursor(0, 1);
      
      if (uiPage == 0)
      {
        lcd.print(rpm, DEC);
        lcd.setCursor(6, 1);
        lcd.print(voltage, 1);
        lcd.setCursor(12, 1);
        lcd.print(_kv, DEC);
      }
      else if (uiPage == 1)
      {
        lcd.print(thrust, 1);
        lcd.setCursor(9, 1);
        lcd.print(current, 2);
      }
      else
      {*/
        lcd.clear();
        // Thrust
        if(thrust > 999)
          lcd.print((int)thrust, DEC);
        else
          lcd.print(thrust, 1);
        lcd.print(F("g"));

        lcd.setCursor(7, 0);

        // voltage
        lcd.print(voltage, 1);
        lcd.print(F("v"));

        lcd.setCursor(12, 0);

        // KV
        lcd.print(_kv, DEC);

        lcd.setCursor(0, 1);
        
        //rpm
        lcd.print(rpm, DEC);
  
        lcd.setCursor(7, 1);

        // current
        lcd.print(current, 1);
        lcd.print(F("a"));

        lcd.setCursor(12, 1);

        // throttle
        lcd.print(throttle > 0 ? throttle : 0, 1);
        lcd.print(F("%"));
        //lcd.print(throttle > 0 ? throttle : 0, 1);
      //}
      break;
  }
}

bool clearLcd = true;
byte settingsScreen = 0; // 0 - select mode, 1 - select poles, 2 - scale enable
bool editMode = false;

void SettingsScreen()
{
  static byte mode_edited = mode;
  static byte enableScale_edited = scaleEnable;

  if(!editMode)
  {      
    if(encoderRight)
    {
      clearEncoderFlags();
      settingsScreen = cycleValue(settingsScreen, 0, 2);
      clearLcd = true;
    }

    if(encoderLeft)
    {
      clearEncoderFlags();
      settingsScreen = cycleValue(settingsScreen, 0, 2, 0, 1);
      clearLcd = true;
    }

    if(clicked)
    {
      clearEncoderFlags();
      editMode = true;
    }
  }
  
  switch(mode)
  {
    case 0: // Handheld tacho
      if(clearLcd)
      {
        lcd.clear();
        lcd.print(readPgmString(pgm_read_word(&l_settings_hh[settingsScreen]))); // header     
        clearLcd = false;    
      }
      
      if(settingsScreen == 0)
      {
        if(editMode)
        {
          if(encoderRight)
          {
            clearEncoderFlags();
            mode_edited = cycleValue(mode_edited, 0, 1);
          }

          if(encoderLeft)
          {
            clearEncoderFlags();
            mode_edited = cycleValue(mode_edited, 0, 1, 0, 1);
          }

          if(clicked)
          {
            clearEncoderFlags();
            editMode = false;
            clearLcd = true; // when switching from edit to view - clear lcd!
            mode = mode_edited; //apply selected mode
            polesCount = mode ? motorTypeIdxs[motorType] : polesCount;
          }

          printEditIndicator();
        }

        lcd.setCursor(3, 1);
        lcd.print(readPgmString(pgm_read_word(&l_modes[mode_edited]))); // print selected mode
      }
      else if(settingsScreen == 1)
      {
          if(editMode)
          {
            if(encoderRight)
            {
              clearEncoderFlags();
              polesCount = cycleValue(polesCount, 1, 4);
            }

            if(encoderLeft)
            {
              clearEncoderFlags();
              polesCount = cycleValue(polesCount, 1, 4, 0, 1);
            }

            if(clicked)
            {
              clearEncoderFlags();
              editMode = false;
              clearLcd = true; // when switching from edit to view - clear lcd!
            }

            printEditIndicator();
          }

          lcd.setCursor(7, 1);
          lcd.print(polesCount); // selected poles
      }
      else 
      {
          if(editMode)
          {
            if(encoderRight)
            {
              clearEncoderFlags();
              enableScale_edited = cycleValue(enableScale_edited, 0, 1);
            }

            if(encoderLeft)
            {
              clearEncoderFlags();
              enableScale_edited = cycleValue(enableScale_edited, 0, 1, 0, 1);
            }

            if(clicked)
            {
              clearEncoderFlags();
              editMode = false;
              clearLcd = true; // when switching from edit to view - clear lcd!
              scaleEnable = enableScale_edited > 0; //apply
            }

            printEditIndicator();
          }

          lcd.setCursor(3, 1);
          lcd.print(readPgmString(pgm_read_word(&l_enableScale_options[enableScale_edited]))); // selected scale mode
      }
      break;

    case 1: // Connected to PC stand
      if(clearLcd)
      {
        lcd.clear();
        lcd.print(readPgmString(pgm_read_word(&l_settings_stand[settingsScreen]))); // header     
        clearLcd = false;    
      }
      
      if(settingsScreen == 0)
      {
          if(editMode)
          {
            if(encoderRight)
            {
              clearEncoderFlags();
              mode_edited = cycleValue(mode, 0, 1);
            }

            if(encoderLeft)
            {
              clearEncoderFlags();
              mode_edited = cycleValue(mode, 0, 1, 0, 1);
            }

            if(clicked)
            {
              clearEncoderFlags();
              editMode = false;
              clearLcd = true; // when switching from edit to view - clear lcd!
              mode = mode_edited; //apply selected mode
              polesCount = mode ? polesCount : 2;
            }

            printEditIndicator();
          }

          lcd.setCursor(3, 1);
          lcd.print(readPgmString(pgm_read_word(&l_modes[mode_edited]))); // selected mode
      }
      else if(settingsScreen == 1)
      {
          if(editMode)
          {
            if(encoderRight)
            {
              clearEncoderFlags();
              motorType = cycleValue(motorType, 0, 1);
            }

            if(encoderLeft)
            {
              clearEncoderFlags();
              motorType = cycleValue(motorType, 0, 1, 0, 1);
            }

            if(clicked)
            {
              clearEncoderFlags();
              editMode = false;
              clearLcd = true; // when switching from edit to view - clear lcd!
              polesCount = motorTypeIdxs[motorType]; //apply selected motor type
            }

            printEditIndicator();
          }

          lcd.setCursor(4, 1);
          lcd.print(readPgmString(pgm_read_word(&l_motorTypes[motorType]))); // selected motor type
      }
      else 
      {
          if(editMode)
          {
            if(encoderRight)
            {
              clearEncoderFlags();
              enableScale_edited = cycleValue(enableScale_edited, 0, 1);
            }

            if(encoderLeft)
            {
              clearEncoderFlags();
              enableScale_edited = cycleValue(enableScale_edited, 0, 1, 0, 1);
            }

            if(clicked)
            {
              clearEncoderFlags();
              editMode = false;
              clearLcd = true; // when switching from edit to view - clear lcd!
              scaleEnable = enableScale_edited; //apply
            }

            printEditIndicator();
          }

          lcd.setCursor(3, 1);
          lcd.print(readPgmString(pgm_read_word(&l_enableScale_options[enableScale_edited]))); // selected scale mode
      }
      
      break;
  }
}

void printEditIndicator()
{
  lcd.setCursor(0, 1);
  lcd.print(readPgmString(&s_edit[0]));
}

void printBlades()
{
  lcd.setCursor(14, 0);
  if(polesCount == 1)
  {
    lcd.setCursor(15, 0);    
    lcd.write(byte(0));
  }
  if(polesCount == 2)
  {
    lcd.write(byte(6));
    lcd.write(byte(4));
    lcd.setCursor(14, 1);
    lcd.write(byte(2));
    lcd.write(byte(7));
  }
  if(polesCount == 3)
  {
    lcd.write(byte(1));
    lcd.write(byte(0));
    lcd.setCursor(14, 1);
    lcd.write(byte(2));
    lcd.write(byte(3));
  }
  if(polesCount == 4)
  {
    lcd.write(byte(5));
    lcd.write(byte(4));
    lcd.setCursor(14, 1);
    lcd.write(byte(2));
    lcd.write(byte(3));
  }
}

void GetReadings()
{
  long currtime = millis(); // GET CURRENT TIME
  
    float _thrust = 0.0;
    
    _thrust = scaleEnable ? abs(scale.get_units(3)) : 0.0;
    
    if (_thrust < 0)
      _thrust = 0;

    thrust = thrustFilter.filtered(_thrust);

    float _current = analogRead(CIN) * C_SCALE;
    if (_current < 0.06)
      _current = 0.0;

    current = currentFilter.filtered(_current);

    float _voltage = ((analogRead(VIN) * VCC) / 1023.0) / (R2 / (R1 + R2));

    //statement to quash undesired reading !
    if (_voltage < 0.09)
      _voltage = 0.0;

    voltage = voltageFilter.filtered(_voltage);

    rpm = tacho.getRPM();

    time = millis();
  
}

void RPM_ISR()
{
  // tick every 1 full turn
  if (++elapsed >= polesCount)
  {
    elapsed = 0;
    tacho.tick();

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
}

void initScale()
{
    // INITIATE LOADCELL
    scale.begin(DOUT, SCK);

    // Scale initialization and tare
    scale.read_average(5);
    scale.set_scale(FACTOR); // this value is obtained by calibrating the scale with known weights; see the README for details
    scale.tare();            // reset the scale to 0
}

void initLcd()
{
  // INITIATE LCD
  lcd.begin(16, 2);

  lcd.createChar(0, customChar0);
  lcd.createChar(1, customChar1);
  lcd.createChar(2, customChar2);
  lcd.createChar(3, customChar3);
  lcd.createChar(4, customChar4);
  lcd.createChar(5, customChar5);
  lcd.createChar(6, customChar6);
  lcd.createChar(7, customChar7);
}

void setup()
{
  servo.attach(ESC, SERVO_MIN, SERVO_MAX);

  Serial.begin(38400);   // GET VALUES USING SERIAL MONITOR
  Serial.setTimeout(100);
  
  pinMode(V0, OUTPUT);
  analogWrite(V0, 115);

  initLcd();
  

  // buttons  
  pinMode(SEL, INPUT);

  // INPUTS
  pinMode(VIN, INPUT);
  pinMode(CIN, INPUT);
  pinMode(EXT, INPUT);

  pinMode(LED, OUTPUT);

  digitalWrite(SEL, HIGH);
  
  if(scaleEnable)
  {
    initScale();
  }
  rpm = 0;
  time = 0;

  // send greetings to pc
  Serial.write(101);

  StartMain();
}

byte cycleValue(byte val, byte min, byte max)
{
  return cycleValue(val, min, max, 1, 1);
}

byte cycleValue(byte val, byte min, byte max, byte dir, byte step)
{
  if(dir) // increment
  {
    return val > max - step ? min : val + step;
  }
  return val - step < min  ? max : val - step;
}

void clearEncoderFlags()
{
  encoderRight = false;
  encoderLeft = false;
  clicked = false;
}

void loop()
{
  static unsigned long lastSettingsUpdate = 0;
  encoder.tick();

  // long press should open/close settings
  if(encoder.isHolded())
  {
    clearEncoderFlags();
    clearLcd = true;
    screen = cycleValue(screen, 0, 1);
  }

  if (screen == 0)
  {
    unsigned long currentTime = millis();
    if (millis() - lastReadingsUpdate > 250)
    {
      GetReadings();
      lastReadingsUpdate = millis();
    }
    if (millis() - lastUIUpdate > 500)
    {
      MainScreen();
      lastUIUpdate = millis();
      uiPage++;
      if (uiPage > 2)
      {
        uiPage = 0;
      }
    }

    // click to cycle blades count
    if(mode == 0 && encoder.isClick())
    {
      polesCount = cycleValue(polesCount, 1, 4);
    }
    
  }
  else
  {
    if(encoder.isRight())
    {
      clearEncoderFlags();
      encoderRight = true;
    }
  
    if(encoder.isLeft())
    {
      clearEncoderFlags();
      encoderLeft = true;
    }
  
    if(encoder.isClick())
    {
      clearEncoderFlags();
      clicked = true;
    }

    if(encoder.isHolded())
    {
      clearEncoderFlags();
    }
  
    if (millis() - lastSettingsUpdate > 200)
    {
      SettingsScreen();
      lastSettingsUpdate = millis();
    }
  }
}

void setThrottle(int percent)
{
  if (percent < 0 || percent > 100)
    {
      percent = 0;
    }

    //if (percent != throttle)
    //{
      throttle = percent;
      servo.write(map(throttle, 0, 100, 0, 180));
    //}
}

void serialEvent()
{ 
  /* PROTOCOL
  "101,[POLES COUNT],[SCALE ENABLE];"
  "102,[THROTTLE POSITION];"
  */

  char data[32];
  int bytesRead = Serial.readBytesUntil(';', data, 32);
  if(bytesRead > 1) // waiting for packet at least few bytes
  {
    data[bytesRead] = NULL;
  
    int content[5] = {0, 0, 0, 0, 0};
    int idx = 0;
    char* offset = data;
  
    while(true)
    {
      content[idx++] = atoi(offset);
      offset = strchr(offset, ','); // next delimiter
      if(offset)
        offset++;
      else
        break;
    }
  
    switch(content[0])
    {
      case 101: // settings
        mode = 1; // if settings received - then we are connected
        polesCount = (byte)(content[1] / 2);
        scaleEnable = (byte)content[2];
        if(scaleEnable)
        {
          initScale();
        }
        Serial.write(101); // confirm that we recieved data
        break;
      case 102: // regular packet
        setThrottle(content[1]);
        break;
    }
  }
}
