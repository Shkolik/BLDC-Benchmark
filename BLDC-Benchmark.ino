#include <LiquidCrystal.h>

// initialize the library by associating any needed LCD interface pin
// with the arduino pin number it is connected to
const int rs = 12, en = 11, d4 = 7, d5 = 6, d6 = 5, d7 = 4;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

// input pins
const int photo_pin = 3, ext_trigger = 2, ledPin = 8, dc_in = A3;
// menu buttons
const int minus = A0, menu = A1, plus = A2;

//  VOLATILE DATA TYPE TO STORE REVOLUTIONS
volatile int REV;     

// Settings  
byte mode = 0, bladesCount = 1, screen = 1;

//  DEFINE RPM AND MAXIMUM RPM
float rpm, maxRPM;  


unsigned long time;         //  DEFINE TIME TAKEN TO COVER ONE REVOLUTION
  
int RPMlen , prevRPM;  //  INTEGERS TO STORE LED VALUE AND CURRENT RPM AND PREVIOUS RPM
 
byte flag = 0, led = 0;             //  A VARIABLE TO DETERMINE WHETHER THE LCD NEEDS TO BE CLEARED OR NOT

unsigned long prevtime = 0;       //  STORE IDLE TIME TO TOGGLE MENU

byte lastPlusState = 1, readingPlus = 1;
byte lastMinusState = 1, readingMinus = 1;
byte lastMenuState = 1, readingMenu = 1;

float vout = 0.0;
float vin = 0.0;
float R1 = 100000.0; // resistance of R1 (100K) -see text!
float R2 = 10850.0; // resistance of R2 (10K) - see text!

void BladesSelectScreen()
{
  lcd.clear();
  lcd.print("Blades count: ");
  lcd.setCursor(0, 1);
  lcd.print("<--  ");
  lcd.print(bladesCount,DEC);
  lcd.print("  -->");  
}

void BladesSelectLoop()
{
	//setup blades
	int readingPlus = digitalRead(plus);
	int readingMinus = digitalRead(minus);
	int readingMenu = digitalRead(menu);
  
    if (readingPlus != lastPlusState && readingPlus == LOW) 
    {
		bladesCount++;
		BladesSelectScreen();
    }
	else if(readingMinus != lastMinusState && readingMinus == LOW && bladesCount > 1) 
    {
		bladesCount--;
		BladesSelectScreen();
    } 
	else if(readingMenu != lastMenuState && readingMenu == LOW)
	{
		screen = 0;
    MainScreen(0, 0.0);
    		
		if(mode > 0)
			attachInterrupt(digitalPinToInterrupt(ext_trigger), RPMCount, RISING);     	//  ADD A HIGH PRIORITY ACTION ( AN INTERRUPT)  WHEN THE SENSOR GOES FROM LOW TO HIGH
        else
			attachInterrupt(digitalPinToInterrupt(photo_pin), RPMCount, RISING);     	//  ADD A HIGH PRIORITY ACTION ( AN INTERRUPT)  WHEN THE SENSOR GOES FROM LOW TO HIGH
	}

	// save the reading. Next time through the loop, it'll be the lastButtonState:
	lastPlusState = readingPlus;
	lastMinusState = readingMinus;
	lastMenuState = readingMenu;
}

void ModeSelectScreen()
{
  lcd.clear();
  lcd.print("Select mode: ");
  lcd.setCursor(0, 1);  
  switch(mode)
  {
    case 0:
      lcd.print("<-- Optical -->");
      break;
    case 1:
      lcd.print("<--  Stand  -->");
      break;
  }
}

void ModeSelectLoop()
{
	//setup blades
	int readingPlus = digitalRead(plus);
	int readingMinus = digitalRead(minus);
	int readingMenu = digitalRead(menu);
  
    if (readingPlus != lastPlusState && readingPlus == LOW && mode < 1) 
    {
		mode++;
		ModeSelectScreen();
    }
	else if(readingMinus != lastMinusState && readingMinus == LOW && mode >= 1) 
    {
		mode--;
		ModeSelectScreen();
    } 
	else if(readingMenu != lastMenuState && readingMenu == LOW)
	{
		screen++;
		BladesSelectScreen();		
	}

	// save the reading. Next time through the loop, it'll be the lastButtonState:
	lastPlusState = readingPlus;
	lastMinusState = readingMinus;
	lastMenuState = readingMenu;
}

void MainScreen(int rpm, float voltage)
{
  lcd.clear();
  lcd.print("RPM:  VOLT: KV:");
  lcd.setCursor(0, 1);
  lcd.print(rpm,DEC);
  lcd.setCursor(6, 1);
  lcd.print(voltage,1);
  lcd.setCursor(12, 1);
  lcd.print((int)(rpm/voltage),DEC);
}
  
void MainScreenLoop()
{
  MainScreen((int)rpm, vin);
  delay(200);
    
	long currtime = millis();                 // GET CURRENT TIME
  
	long idletime = currtime - prevtime - 200;        //  CALCULATE IDLE TIME
    
	if(REV >= 5 )                  //  IT WILL UPDATE AFETR EVERY 10 READINGS
	{   
		int rev = REV;
		REV = 0;    
             
		rpm = 60000/(millis() - time);
     
		rpm = rpm*(rev/bladesCount);
     
     
		if(rpm > maxRPM)
			maxRPM = rpm;                             //  GET THE MAX RPM THROUGHOUT THE RUN
    
		time = millis();                            
     		
		vout = (analogRead(dc_in) * 5.0) / 1024.0; // see text
		vin = vout / (R2/(R1+R2)); 
		if (vin < 0.09) 
			vin = 0.0;				//statement to quash undesired reading !
     
		MainScreen((int)rpm, vin);
		delay(500);

		prevtime = currtime;                        // RESET IDLETIME
	}
   
	/*if(idletime > 5000 )                      //  IF THERE ARE NO READING FOR 5 SEC , THE SCREEN WILL SHOW MAX RPM
	{
		lcd.clear();
		lcd.print("MAXIMUM RPM");
		lcd.setCursor(0, 1);
		lcd.print((int)maxRPM,DEC);                     // DISPLAY MAX RPM
		lcd.print("   RPM");
		delay(2000);
		lcd.clear();
		lcd.print("IDLE STATE");
		lcd.setCursor(0, 1);
		lcd.print("READY TO MEASURE");
		delay(2000);
		prevtime = currtime;
	}*/	
}
  
void RPMCount()                                
{
   REV++;                                        // INCREASE REVOLUTIONS
   
   if (led == LOW)
   {     
     led = HIGH;                                 //  TOGGLE STATUS LED
   }
   else
   {
     led = LOW;
   }
   
   digitalWrite(ledPin, led);
}

void setup() 
{
	//Serial.begin(9600);   // GET VALUES USING SERIAL MONITOR
	
	lcd.begin(16, 2);     // INITIATE LCD

    pinMode(minus, INPUT);
	pinMode(plus, INPUT);
	pinMode(menu, INPUT);
	pinMode(dc_in, INPUT);

	digitalWrite(minus, HIGH);
	digitalWrite(plus, HIGH);
	digitalWrite(menu, HIGH);
	
	pinMode(ledPin, OUTPUT);

    REV = 0;      //  START ALL THE VARIABLES FROM 0 
	rpm = 0;
	time = 0;
     
    ModeSelectScreen();
}

void loop() 
{
 
	switch(screen)
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
	}


}