
#define HEARTBEAT_PIN 13

// include the library code:
#include <HX711_ADC.h>

//HX711 constructor (dout pin, sck pin) https://github.com/olkal/HX711_ADC
HX711_ADC LoadCellA(11, 12);
HX711_ADC LoadCellB(9, 10);
HX711_ADC LoadCellC(7, 8);

#include <Wire.h> //for i2c-bus
#include <LiquidCrystal_I2C.h>

#define I2C_ADDR 0x27 // Display's i2c-address. Use i2c-scanner to search the correct one...
#define BACKLIGHT_PIN  3
#define En_pin  2
#define Rw_pin  1
#define Rs_pin  0
#define D4_pin  4
#define D5_pin  5
#define D6_pin  6
#define D7_pin 7

//display
LiquidCrystal_I2C lcd(I2C_ADDR,En_pin,Rw_pin,Rs_pin,D4_pin,D5_pin,D6_pin,D7_pin); 

unsigned int lcd_min_update_interval_ms = 500; //to acoid flickering etc

//lcd update timer (to prevent updating too often, f=0.5 Hz)
unsigned int t_prev_lcd_update = 0;
String prevL1, prevL2; //previous lines printed
byte prevLine;

String buf;

long measure_time;


int dispSensor = 4;

float calFactorA = 897.38;
float calFactorB = 953.85;
float calFactorC = 1171.47;

float distance = 350;
float offset = 30;

float calFactorDif = 0;

void setup() {

  lcd.begin (16,2); //  chars, lines
  lcd.setBacklightPin(3,POSITIVE);
  lcd.home (); 
  
  //splash 
  print2lcd(1,"CG TriScale");

  lcd.setBacklight(HIGH);
  print2lcd(2,"Init...");

  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }
  //Serial.println("Init...");

  
  long stabilisingtime = 5000; // tare preciscion can be improved by adding a few seconds of stabilising time


  LoadCellA.begin();
  LoadCellA.start(stabilisingtime);
  LoadCellA.setCalFactor(calFactorA); // user set calibration factor (float)

  LoadCellB.begin();
  LoadCellB.start(stabilisingtime);
  LoadCellB.setCalFactor(calFactorB); // user set calibration factor (float)
  
  LoadCellC.begin();
  LoadCellC.start(stabilisingtime);
  LoadCellC.setCalFactor(calFactorC); // user set calibration factor (float)
  // initialize digital pin 13 as an output. heartbeat HEARTBEAT_PIN
  pinMode(HEARTBEAT_PIN, OUTPUT);
  
  //Serial.println("Go!");

  
}

void loop() {

  //update() should be called at least as often as HX711 sample rate; >10Hz@10SPS, >80Hz@80SPS
  //longer delay in scetch will reduce effective sample rate (be carefull with delay() in loop)
  
  LoadCellA.update();
  LoadCellB.update();
  LoadCellC.update();

  //get smoothed value from data set + current calibration factor
  if (millis() > measure_time + 125) {



    if (Serial.available() > 0) {
      int inByte = Serial.read();
      switch (inByte) {
          case 'a':    
            dispSensor = 1;
            break;
          case 'b':    
            dispSensor = 2;
            break;
          case 'c':    
            dispSensor = 3;
            break;
          case 'g':    
            dispSensor = 4;
            break;            
          case '1':    
            calFactorDif = -100;
            break;
          case '2':    
            calFactorDif = 100;
            break;
          case '3':    
            calFactorDif = -10;
            break;
          case '4':    
            calFactorDif = 10;
            break;
          case '5':    
            calFactorDif = -1;
            break;
          case '6':    
            calFactorDif = 1;
            break;
          case '7':    
            calFactorDif = -0.1;
            break;
          case '8':    
            calFactorDif = 0.1;
            break;
          case '9':    
            calFactorDif = -0.01;
            break;
          case '0':    
            calFactorDif = 0.01;
            break;
        }

        switch (dispSensor) {
          case 1:    
            calFactorA = calFactorA + calFactorDif;
            break;
          case 2:    
            calFactorB = calFactorB + calFactorDif;
            break;
          case 3:    
            calFactorC = calFactorC + calFactorDif;
            break;
        }
        calFactorDif = 0;

        LoadCellA.setCalFactor(calFactorA);
        LoadCellB.setCalFactor(calFactorB);
        LoadCellC.setCalFactor(calFactorC);        
        
    }

    float a = LoadCellA.getData();
    float b = LoadCellB.getData();
    float c = LoadCellC.getData();

    switch (dispSensor) {
      case 1:    
        Serial.print("A=");
        Serial.print(a);
        Serial.print(" , cal=");
        Serial.print(calFactorA);
        break;
      case 2:    
        Serial.print(" B=");
        Serial.print(b);
        Serial.print(" , cal=");
        Serial.print(calFactorB);
        break;
      case 3:    
        Serial.print(" C=");
        Serial.print(c);
        Serial.print(" , cal=");
        Serial.print(calFactorC);
        break;
      case 4:
        float total = a + b + c;
        if( total > 50 ) {
          lcd.setBacklight(HIGH);     // Backlight on
          float grav = offset + (distance * (c/total));
          //Serial.print(" grav=");
          //Serial.print(grav);
          buf = "";
          buf += F("weight: ");
          buf += String(total, 1);
          buf += F("g");
          print2lcd(1,buf);
          
          buf = "";
          buf += F("CG: ");
          buf += String(grav, 1);
          buf += F("mm");         
          print2lcd(2,buf);
          
        } else {
          lcd.setBacklight(LOW);      // Backlight off
          //Serial.print(" empty");
          print2lcd(1,"empty");
          print2lcd(2,"...");
        }
        
//        Serial.print(" total=");
//        Serial.print(total);
//        Serial.print(" A=");
//        Serial.print(a);
//        Serial.print(" B=");
//        Serial.print(b);
//        Serial.print(" C=");
//        Serial.print(c);
        break;
    }
    
    //Serial.println(" ...");
        
    measure_time = millis();

    digitalWrite(HEARTBEAT_PIN, !digitalRead(HEARTBEAT_PIN));
  }

  
}

//-----------------------------------------------------------------
//Functions
//printing to LCD
void print2lcd(int line_1_2,String text)
  {
  //to avoid updating too often
  bool update = false;
  if (line_1_2 != prevLine) update = true;
  if (line_1_2 == prevLine && ((unsigned int)millis()-t_prev_lcd_update) > lcd_min_update_interval_ms) update = true;

  //check if the contents is changed.. if not --> do not update LCD
  if (line_1_2 == 1 && text == prevL1) update = false;
  if (line_1_2 == 2 && text == prevL2) update = false;
  
  
  if (update == true)
    {
    //clear line
    lcd.setCursor(0,line_1_2-1);
    lcd.print("                "); //..stupid way

    //print mess
    lcd.setCursor(0,line_1_2-1);
    lcd.print(text);  
 
    //update timestanp, line number and such
    prevLine = line_1_2;
    t_prev_lcd_update = (unsigned int)millis();

    if (line_1_2 == 1) prevL1 = text;
    if (line_1_2 == 2) prevL2 = text;
    }
  }




