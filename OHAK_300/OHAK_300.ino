/*
  Pulse Patch
  This code targets a Simblee
  I2C Interface with MAX30102 Sp02 Sensor Module
  Also got a BOSH MEMS thing
  And it might actually be a MAX30101...?
*/

#include <Wire.h>
#include "OHAK_Definitions.h"

//This line added by Chip 2016-09-28 to enable plotting by Arduino Serial Plotter
//const int PRINT_ONLY_FOR_PLOTTER = 1;  //Set this to zero to return normal verbose print() statements

unsigned int LED_blinkTimer;
unsigned int LED_fadeTimer;
unsigned int rainbowTimer;
int LED_blinkTime = 300;
int LED_fadeTime = 50;
int rainbowTime = 50;
int LEDpin[] = {RED,GRN,BLU};
int LEDvalue[3];
boolean rising[] = {true,true,true};
boolean falling;
int fadeValue = 200;
int LEDcounter = 0;
int lightInPlay;
int colorWheelDegree = 0;
boolean rainbow = false;

volatile boolean MAX_interrupt = false;
short interruptSetting;
short interruptFlags;
float Celcius;
float Fahrenheit;
char sampleCounter = 0;
int REDvalue;
int IRvalue;
int GRNvalue;
char md = SPO2_MODE;  // SPO2_MODE or HR_MODE
float sampleRate = 100.0; // 
char readPointer;
char writePointer;
char ovfCounter;
int redAmp = 10;
int irAmp = 10;
int grnAmp = 10;


//  TESTING
unsigned int thisTestTime;
unsigned int thatTestTime;

boolean useFilter = false;
long filtered_value = 0;
int g = 10;
float HPfilterInputRED[NUM_SAMPLES];
float HPfilterOutputRED[NUM_SAMPLES];
float LPfilterInputRED[NUM_SAMPLES];
float LPfilterOutputRED[NUM_SAMPLES];
float HPfilterInputIR[NUM_SAMPLES];
float HPfilterOutputIR[NUM_SAMPLES];
float LPfilterInputIR[NUM_SAMPLES];
float LPfilterOutputIR[NUM_SAMPLES];


void setup(){

  Wire.beginOnPins(SCL_PIN,SDA_PIN);
  Serial.begin(230400);
  for(int i=0; i<3; i++){
    pinMode(LEDpin[i],OUTPUT); analogWrite(LEDpin[i],255); // Enable RGB and turn them off
  }
  
  LED_blinkTimer = LED_fadeTimer = millis();
  
//  pinMode(MAX_INT,INPUT);
//  attachPinInterrupt(MAX_INT,MAX_ISR,LOW);
//  serviceInterrupts();

  MAX_init(SR_100); // initialize MAX30102, specify sampleRate
  if (useFilter){ initFilter(); }
  if (DEBUG) {
    printAllRegisters();
    Serial.println("");
    printHelpToSerial();
    Serial.println("");
    Serial.println("OpenHAK v0.2.1");
    getBMI_chipID();    // should print 0xD1
    getMAXdeviceInfo(); // prints rev and device ID
  } else {
    //when configured for the Arduino Serial Plotter, start the system running right away
    enableMAX30102(true);
    thatTestTime = micros();
  }
  initFilter();
  useFilter = true;
  pulse(BLU);
  
}


void loop(){

  interruptFlags = MAX_readInterrupts();
  if(interruptFlags > 0){
    serveInterrupts(interruptFlags); // go see what woke us up, and do the work
    if(DEBUG){
      thisTestTime = micros();
      printTab(); printTab(); Serial.println(thisTestTime - thatTestTime);
      thatTestTime = thisTestTime;
    }
//    if(sampleCounter == 0x00){  // rolls over to 0 at 200
//      MAX30102_writeRegister(TEMP_CONFIG,0x01); // take temperature
//    }
  }


  if(rainbow){
    rainbowLEDs();
  }else{
    pulse(BLU);
  }

  eventSerial();
}

void eventSerial(){
  while(Serial.available()){
    char inByte = Serial.read();
    uint16_t intSource;
    switch(inByte){
      case 'h':
        printHelpToSerial();
        break;
      case 'b':
        if(DEBUG) Serial.println("start running");
        enableMAX30102(true);
        thatTestTime = micros();
        break;
      case 's':
        if(DEBUG) Serial.println("stop running");
        enableMAX30102(false);
        break;
      case 't':
        MAX30102_writeRegister(TEMP_CONFIG,0x01);
        break;
      case 'i':
        intSource = MAX30102_readShort(STATUS_1);
        if(DEBUG) Serial.print("intSource: 0x"); Serial.println(intSource,HEX);
        break;
      case 'v':
        getBMI_chipID();
        getMAXdeviceInfo();
        break;
      case '?':
        printAllRegisters();
        break;
      case 'f':
        useFilter = false;
        break;
      case 'F':
        useFilter = true;
        break;
      case '1':
        redAmp++; if(redAmp > 50){redAmp = 50;}
        setLEDamplitude(redAmp, irAmp, grnAmp);
        serialAmps();
        break;
      case '2':
        redAmp--; if(redAmp < 1){redAmp = 0;}
        setLEDamplitude(redAmp, irAmp, grnAmp);
        serialAmps();
        break;
      case '3':
        irAmp++; if(irAmp > 50){irAmp = 50;}
        setLEDamplitude(redAmp, irAmp, grnAmp);
        serialAmps();
        break;
      case '4':
        irAmp--; if(irAmp < 1){irAmp = 0;}
        setLEDamplitude(redAmp, irAmp, grnAmp);
        serialAmps();
        break;
      case 'r':
        LEDvalue[0] = 0; LEDvalue[1] = 255; LEDvalue[2] = 255;
        for(int i=0; i<3; i++){
          analogWrite(LEDpin[i],LEDvalue[i]);        
        }
        colorWheelDegree = 0;
        rainbow = true;
        break;
      case 'R':
        rainbow = false;
        break;
      case 'p':
        getBatteryVoltage();
        break;
      case 'o':
        if(DEBUG){ Serial.print("MAX_INT: "); Serial.println(digitalRead(MAX_INT)); }
        break;
      case 'x':
        MAX_init(SR_100);
        break;
      case 'q':
        serviceInterrupts();
        break;
      default:
        if(DEBUG){ Serial.print("Serial Event got: "); Serial.write(inByte); Serial.println(); }
        break;
      
    }
  }
}


void blinkLEDs(){
  if(millis() - LED_blinkTimer > LED_blinkTime){
    LED_blinkTimer = millis();
    LEDcounter++;
    if(LEDcounter>2){ LEDcounter = 0; }
    for(int i=0; i<3; i++){
      if(i == LEDcounter){
        analogWrite(LEDpin[i],200);
      }else{
        analogWrite(LEDpin[i],255);
      }
    }
  }
}





void rainbowLEDs(){
  if(millis()-rainbowTimer > rainbowTime){
    rainbowTimer = millis();
    colorWheelDegree++;
    
    if(colorWheelDegree > 360){ colorWheelDegree = 1; }
    
    if(colorWheelDegree > 0 && colorWheelDegree < 61){
      LEDvalue[1] = map(colorWheelDegree,1,60,254,0);
      writeLEDvalues(); // g fades up while r is up b is down
    } else if(colorWheelDegree > 60 && colorWheelDegree < 121){
      LEDvalue[0] = map(colorWheelDegree,61,120,0,254);
      writeLEDvalues(); // r fades down while g is up b is down
    } else if(colorWheelDegree > 120 && colorWheelDegree < 181){
      LEDvalue[2] = map(colorWheelDegree,121,180,254,0);
      writeLEDvalues(); // b fades up while g is up r is down
    } else if(colorWheelDegree > 180 && colorWheelDegree < 241){
      LEDvalue[1] = map(colorWheelDegree,181,240,0,254);
      writeLEDvalues(); // g fades down while b is up r is down
    } else if(colorWheelDegree > 240 && colorWheelDegree < 301){
      LEDvalue[0] = map(colorWheelDegree,241,300,254,0);
      writeLEDvalues(); // r fades up while b is up g is down
    } else if(colorWheelDegree > 300 && colorWheelDegree < 361){
      LEDvalue[2] = map(colorWheelDegree,301,360,0,254);
      writeLEDvalues(); // b fades down while r is up g is down
    }
   
  } 
}

void pulse(int led){  // needs help
  if(millis() - LED_fadeTimer > LED_fadeTime){
    if(falling){
      fadeValue-=10;
      if(fadeValue <= 0){
        falling = false;
        fadeValue = 2;
      }
    }else{
      fadeValue+=10;
      if(fadeValue >= 200){
        falling = true;
        fadeValue = 200;
      }
    }
    analogWrite(led,fadeValue);
    LED_fadeTimer = millis();
  }
}


void writeLEDvalues(){  
  for(int i=0; i<3; i++){
    analogWrite(LEDpin[i],LEDvalue[i]);
  }
}



//Print out all of the commands so that the user can see what to do
//Added: Chip 2016-09-28
void printHelpToSerial() {
  if(DEBUG) {
    Serial.println(F("Commands:"));
    Serial.println(F("   'h'  Print this help information on available commands"));
    Serial.println(F("   'b'  Start the thing running at the sample rate selected"));
    Serial.println(F("   's'  Stop the thing running"));
    Serial.println(F("   't'  Initiate a temperature conversion. This should work if 'b' is pressed or not"));
    Serial.println(F("   'i'  Query the interrupt flags register. Not really useful"));
    Serial.println(F("   'v'  Verify the device by querying the RevID and PartID registers (hex 6 and hex 15 respectively)"));
    Serial.println(F("   '1'  Increase red LED intensity"));
    Serial.println(F("   '2'  Decrease red LED intensity"));
    Serial.println(F("   '3'  Increase IR LED intensity"));
    Serial.println(F("   '4'  Decrease IR LED intensity"));
    Serial.println(F("   '?'  Print all registers"));
    Serial.println(F("   'F'  Turn on filters"));
    Serial.println(F("   'p'  Query battery Voltage"));
    Serial.println(F("   'o'  Read MAX_INT pin value"));
    Serial.println(F("   'x'  Soft Reset MAX30101"));
    Serial.println(F("   'q'  Service Interrupts"));
  }
}

int MAX_ISR(uint32_t dummyPin) { // gotta have a dummyPin...
  MAX_interrupt = true;
  return 0; // gotta return something, somehow...
}

void serialAmps(){
  if(DEBUG){
    Serial.print("PA\t");
    Serial.print(redAmp); printTab(); Serial.print(irAmp); printTab(); Serial.println(grnAmp);
  }
}

//float getBatteryVoltage(){
//    int thisCount, lastCount;
//    for(int i=0; i<100; i++){
//      lastCount = analogRead(V_SENSE);
//      delay(10);
//      thisCount = analogRead(V_SENSE);
//      if(DEBUG){
//        Serial.print(i); Serial.print("\t"); Serial.print(lastCount); Serial.print("\t"); Serial.println(thisCount);
//      }
//      if(thisCount >= lastCount){ break; }
//      delay(10);
//    }
//    volts = float(thisCount) * (3.0 / 1023.0);
//    volts *= 2.0;
//    if(DEBUG){
//      Serial.print(thisCount); Serial.print("\t"); Serial.println(volts,3);
//    }
//    return volts / BATT_VOLT_CONST; // why do this??
//}

float getBatteryVoltage(){
  int counts = analogRead(V_SENSE);
  if(DEBUG){ Serial.print(counts); Serial.print("\t"); }
  float volts = float(counts) * (3.0/1023.0);
  if(DEBUG) { Serial.print(volts,3); Serial.print("\t"); }
  volts *=2;
  if(DEBUG){ Serial.println(volts,3); }

  return volts;
}
