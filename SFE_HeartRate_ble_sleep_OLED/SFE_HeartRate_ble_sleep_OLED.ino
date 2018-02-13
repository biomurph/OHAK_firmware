/*
   Optical Heart Rate Detection (PBA Algorithm) using the MAX30105 Breakout
   By: Nathan Seidle @ SparkFun Electronics
   Date: October 2nd, 2016
   https://github.com/sparkfun/MAX30105_Breakout

   This is a demo to show the reading of heart rate or beats per minute (BPM) using
   a Penpheral Beat Amplitude (PBA) algorithm.

   It is best to attach the sensor to your finger using a rubber band or other tightening
   device. Humans are generally bad at applying constant pressure to a thing. When you
   press your finger against the sensor it varies enough to cause the blood in your
   finger to flow differently which causes the sensor readings to go wonky.

   Hardware Connections (Breakoutboard to Arduino):
   -5V = 5V (3.3V is allowed)
   -GND = GND
   -SDA = A4 (or SDA)
   -SCL = A5 (or SCL)
   -INT = Not connected

   The MAX30105 Breakout can handle 5V or 3.3V I2C logic. We recommend powering the board with 5V
   but it will also run at 3.3V.


   0.38925 = mAh running
   0.475 = mAh sleeping


*/
//#define DEBUG 1
#include "OHAK_Definitions.h"

#include <Wire.h>
#include "MAX30105.h"
#include <OpenHAK_MicroOLED.h>
#include "heartRate.h"
#include <BMI160Gen.h>

#include "QuickStats.h"
#include <TimeLib.h>
#include <Lazarus.h>
Lazarus Lazarus;

#include <ota_bootloader.h>
#include <SimbleeBLE.h>





MicroOLED oled(OLED_RESET, DC);    // reset pin, I2C address
String bpm = "";

MAX30105 particleSensor;

QuickStats stats; //initialize an instance of stats class

const byte RATE_SIZE = 4; //Increase this for more averaging. 4 is good.
byte rates[RATE_SIZE]; //Array of heart rates
byte rateSpot = 0;
long lastBeat = 0; //Time at which the last beat occurred
long lastTime;
long awakeTime;
#ifndef DEBUG
long interval = 30000; //30000 this is how long we capture hr data
int sleepTime = 600; //600 is production
#else
long interval = 15000; //30000 this is how long we capture hr data
int sleepTime = 60; //600 is production
#endif
float beatsPerMinute;
int beatAvg;
uint8_t lastBeatAvg;
uint8_t aveBeatsAve[255];
uint8_t aveCounter = 0;
bool tapFlag = false;


uint8_t mode = 10;
bool bConnected = false;

// interval between advertisement transmissions ms (range is 20ms to 10.24s) - default 20ms
int bleInterval = 675;  // 675 ms between advertisement transmissions longer time = better battery but slower scan/connect

/* DATA structures
   Sample data every 10 minutes
   Each sample
   Steps since Midnight (uint16_t) 0 - 65535
   Last BPM - rolling average of 30 seconds of caputre (uint8_t) 0 - 255
   Time ()

*/
typedef struct {
  uint32_t epoch;
  uint16_t steps;
  uint8_t hr;
  uint8_t hrDev;
  uint8_t battery;
  uint8_t aux1;
  uint8_t aux2;
  uint8_t aux3;
} Payload;

Payload samples[512];

uint16_t currentSample = 0;

uint8_t advdata[14] =
{
  13, // length // 0
  0x09, // complete local name type // 1
  0x4f, // 'O' // 2
  0x70, // 'p' // 3
  0x65, // 'e' // 4
  0x6e, // 'n' // 5
  0x48, // 'H' // 6
  0x41, // 'A' // 7
  0x4b, // 'K' // 8
  0x2D, // '-' // 9
  0x54, // 'T' // 10
  0x41, // 'A' // 11
  0x43, // 'C' // 12
  0x4f, // 'O' // 13
};

void setup()
{
  String stringy =  String(getDeviceIdLow(), HEX);
  advdata[10] = (uint8_t)stringy.charAt(0);
  advdata[11] = (uint8_t)stringy.charAt(1);
  advdata[12] = (uint8_t)stringy.charAt(2);
  advdata[13] = (uint8_t)stringy.charAt(3);
  SimbleeBLE_advdata = advdata;
  SimbleeBLE_advdata_len = sizeof(advdata);
  SimbleeBLE.advertisementData = "OpenHAK";
  // Device Information Service strings
  SimbleeBLE.manufacturerName = "openhak";
  SimbleeBLE.hardwareRevision = "0.3";
  SimbleeBLE.softwareRevision = "0.0.4";
  Wire.beginOnPins(SCL_PIN, SDA_PIN);
  // change the advertisement interval
  SimbleeBLE.advertisementInterval = bleInterval;
  SimbleeBLE.begin();
  pinMode(RED, OUTPUT);
  digitalWrite(RED, HIGH);
  pinMode(BLU, OUTPUT);
  digitalWrite(BLU, HIGH);
  pinMode(GRN, OUTPUT);
  digitalWrite(GRN, HIGH);

  //Setup all the devices
  BMI160.begin(0, -1); // use BMI_INT1 for internal interrupt, but we're handling the interrupt so using -1
  BMI160.attachInterrupt(NULL); // use bmi160_intr for internal interrupt callback, but we're handling the interrupt so NULL
  //BMI160.setIntTapEnabled(true);
  BMI160.setIntDoubleTapEnabled(true);
  BMI160.setStepDetectionMode(BMI160_STEP_MODE_NORMAL);
  BMI160.setStepCountEnabled(true);
  pinMode(BMI_INT1, INPUT); // set BMI interrupt pin
  Simblee_pinWake(BMI_INT1, LOW); // use this to wake the MCU if its sleeping

  const unsigned long DEFAULT_TIME = 1357041600; // Jan 1 2013
  setTime(DEFAULT_TIME);
#ifdef DEBUG
  Serial.begin(9600);
  Serial.println("Initializing...");
#endif

  // Initialize sensor
  if (!particleSensor.begin(Wire, 400)) //Use default I2C port, 400kHz speed
  {
    //Serial.println("MAX30105 was not found. Please check wiring/power. ");
    while (1);
  }

  particleSensor.setup(); //Configure sensor with default settings
  //particleSensor.setPulseAmplitudeRed(0x0A); //Turn Red LED to low to indicate sensor is running
  //particleSensor.setPulseAmplitudeGreen(0); //Turn off Green LED

  //Blink the startup pattern
  digitalWrite(RED, LOW);
  delay(400);
  digitalWrite(GRN, LOW);
  digitalWrite(RED, HIGH);
  delay(400);
  digitalWrite(GRN, HIGH);
  digitalWrite(BLU, LOW);
  delay(400);
  digitalWrite(BLU, HIGH);
#ifdef DEBUG
  digitalWrite(BLU, LOW);
  delay(400);
  digitalWrite(BLU, HIGH);
  delay(400);
  digitalWrite(BLU, LOW);
  delay(400);
  digitalWrite(BLU, HIGH);
#endif

  oled.begin();    // Initialize the OLED
  oled.flipHorizontal(true);
  oled.flipVertical(true);
  oled.clear(ALL); // Clear the display's internal memory
  oled.clear(PAGE); // Clear the buffer.
  oled.setFontType(2);
  oled.setCursor(0, 0);
  oled.print("09:30");
  oled.setCursor(0, 24);
  oled.setFontType(0);
  oled.println("text from");
  oled.print("Leif");
  oled.display();

  lastTime = millis();
  delay(5000);
}
void bmi160_intr(void)
{
  tapFlag = true;
  //Lazarus.ariseLazarus();
  //byte int_status = BMI160.getIntStatus0();
  // Serial.print("Steps ");
  // Serial.print(BMI160.getStepCount());
  // Serial.print(" Single ");
  // Serial.print(bitRead(int_status,5));
  // Serial.print(" Double ");
  // Serial.print(bitRead(int_status,4));
  // Serial.print(" REG ");
  // Serial.print(int_status,BIN);
  // Serial.println(" BMI160 interrupt: TAP! ");
}

void loop()
{
  if (Lazarus.lazarusArising()) {
    digitalWrite(BLU, LOW);
#ifdef DEBUG
    Serial.println("Lazarus has awakened!");
#endif
    // Serial.println("");
  }
  if (Simblee_pinWoke(BMI_INT1))
  {
    byte int_status = BMI160.getIntStatus0();
#ifdef DEBUG
    Serial.println("TAP has awakened!");
#endif
    Simblee_resetPinWake(BMI_INT1);
    digitalWrite(RED, LOW);
    delay(100);
    digitalWrite(RED, HIGH);
  }
  particleSensor.wakeUp();
  particleSensor.setup();
  long lastTime;
  int sleepTimeNow;
  uint32_t startTime;
  //SimbleeBLE.send(0);
  //lastBeatAvg = 0;
  //beatAvg = 0;
  //digitalWrite(RED,LOW);
  // if (timeStatus() != timeNotSet) {
  //         String printString = digitalClockDisplay();
  // }
  switch (mode) {
    case 0:
#ifdef DEBUG
      Serial.println("Enter mode 1");
#endif
      lastTime = millis();
      samples[currentSample].epoch = now();
      samples[currentSample].steps = BMI160.getStepCount();
      memset(aveBeatsAve, 0, sizeof(aveBeatsAve));
      aveCounter = 0;
#ifdef DEBUG
      Serial.println("Starting HR capture");
#endif
      printOLED("Measuring Heart Rate",true);
      startTime = millis();
      while (captureHR(startTime)) {
        ;
      }
      // uint16_t hrAveTotal=0;
      // byte min = 255;
      // byte max = 0;
      // for(byte i=0;i<aveCounter;i++){
      //   hrAveTotal += aveBeatsAve[i];
      // }
      //
      // uint8_t sampleHR = hrAveTotal/aveCounter;
      samples[currentSample].hr = stats.median(aveBeatsAve, aveCounter);
      samples[currentSample].hrDev = stats.stdev(aveBeatsAve, aveCounter);
      samples[currentSample].battery = getBatteryVoltage();
      //                samples[currentSample].aux1 = analogRead(PIN_2);
      //                samples[currentSample].aux2 = analogRead(PIN_3);
      //                samples[currentSample].aux3 = analogRead(PIN_4);

      bpm += String(samples[currentSample].hr);
      bpm += " BPM";
      printOLED(bpm,true);
      delay(5000);
      digitalWrite(OLED_RESET,LOW);
      if (bConnected) {
        sendSamples(samples[currentSample]);
      }
      if (currentSample < 511) {
        currentSample++;
      } else {
        //TODO: Check and see if this works!
        for (int k = currentSample; k > 0; k--) {
          samples[k] = samples[k - 1];
        }
        currentSample--;
        // for(uint16_t t=0;t<512;t++){
        //   memcpy(&samples[1], &samples[0], (512-1)*sizeof(*samples));
        // }
      }
#ifdef DEBUG
      Serial.print("Samples Captured: ");
      Serial.println(currentSample);
#endif
  
      awakeTime = millis() - lastTime;
      sleepTimeNow = sleepTime - (interval / 1000);
      sleepNow(sleepTimeNow);
      break;
    case 1:
#ifdef DEBUG
      Serial.println("Enter mode 1");
#endif
      startTime = millis();
      captureHR(startTime);
      break;
    case 2:
#ifdef DEBUG
      Serial.println("Enter mode 2");
#endif
      mode = 0;
      sleepTimeNow = sleepTime - (interval / 1000);
      sleepNow(sleepTimeNow);
      break;
    case 3:
#ifdef DEBUG
      Serial.println("Enter mode 3");
#endif
      transferSamples();
      break;
    case 10:
#ifdef DEBUG
      Serial.println("Enter mode 10");
#endif
      //mode = 0;
      digitalWrite(RED, LOW);
      printOLED("Not Connected",false);
      delay(600);
      sleepNow(10);
      break;
  }
  //Serial.print("IR=");
  //Serial.print(irValue);
  //Serial.print(", BPM=");
  //Serial.print(beatsPerMinute);
  //Serial.print(", Avg BPM=");
  //Serial.print(beatAvg);

  //if (irValue < 50000)
  // Serial.print(" No finger?");

  //Serial.println();
}
void SimbleeBLE_onReceive(char *data, int len) {
  // if the first byte is 0x01 / on / true
  //Serial.print("Received data over BLE ");
  //Serial.println(len);
  Lazarus.ariseLazarus();
  mode = data[0];
  if (mode == 10) {
    if (len >= 5) {
      unsigned long myNum = (data[1] << 24) | (data[2] << 16) | (data[3] << 8) | data[4];
      setTime(myNum);
      mode = 2;
    }
  }

}
void transferSamples() {
#ifdef DEBUG
  Serial.println("Starting History transfer");
#endif
  for (int i = 0; i < currentSample; i++) {
    if (bConnected) {
      sendSamples(samples[i]);
    }
  }
  mode = 0;
}
void sendSamples(Payload sample) {
  char data[20];
  data[0] = (sample.epoch >> 24) & 0xFF;
  data[1] = (sample.epoch >> 16) & 0xFF;
  data[2] = (sample.epoch >> 8) & 0xFF;
  data[3] = sample.epoch & 0xFF;
  data[4] = (sample.steps >> 8) & 0xFF;
  data[5] = sample.steps & 0xFF;
  data[6] = sample.hr;
  data[7] = sample.hrDev;
  data[8] = sample.battery;
  data[9] = sample.aux1;
  data[10] = sample.aux2;
  data[11] = sample.aux3;
  // send is queued (the ble stack delays send to the start of the next tx window)
  while (!SimbleeBLE.send(data, 9))
    ; // all tx buffers in use (can't send - try again later)
}
void sleepNow(long timeNow) {
  digitalWrite(RED, HIGH);
  digitalWrite(GRN, HIGH);
  digitalWrite(BLU, HIGH);
  digitalWrite(OLED_RESET,LOW);
  particleSensor.setPulseAmplitudeIR(0); //Turn off IR LED
  particleSensor.setPulseAmplitudeRed(0); //Turn off Red LED
  particleSensor.setPulseAmplitudeGreen(0); //Turn off Green LED
  particleSensor.shutDown();
  //int sleepTimeNow = timeNow - (interval/1000);
  Simblee_ULPDelay(SECONDS(timeNow));
}
bool captureHR(uint32_t startTime) {
  if (millis() - startTime > interval) {
#ifdef DEBUG
    Serial.println("HR capture done");
#endif

    return (false);
  }
  //analogWrite(GRN,200);
  long irValue = particleSensor.getGreen();
  //digitalWrite(GRN,HIGH);
  //digitalWrite(RED,LOW);
  while (SimbleeBLE.radioActive) {
    ;
  }
  if (checkForBeat(irValue) == true)
  {
    analogWrite(BLU, 200);
    //We sensed a beat!
    //digitalWrite(GRN,LOW);
    long delta = millis() - lastBeat;
    lastBeat = millis();

    beatsPerMinute = 60 / (delta / 1000.0);

    if (beatsPerMinute < 255 && beatsPerMinute > 20)
    {
      rates[rateSpot++] = (byte)beatsPerMinute; //Store this reading in the array
      rateSpot %= RATE_SIZE; //Wrap variable

      //Take average of readings
      beatAvg = 0;
      for (byte x = 0; x < RATE_SIZE; x++)
        beatAvg += rates[x];
      beatAvg /= RATE_SIZE;
      // if(bConnected) {
      //         SimbleeBLE.send((uint8_t)beatAvg);
      // }
    }
  }
  digitalWrite(BLU, HIGH);
  //digitalWrite(GRN,HIGH);
  //Here is where you could send all the beats if you wanted

  if (beatAvg != lastBeatAvg) {
    aveBeatsAve[aveCounter] = (uint8_t)beatAvg;
    aveCounter++;
#ifdef DEBUG
    Serial.print("Beat Ave: ");
    Serial.println(beatAvg);
#endif
    // if(bConnected) {
    //         SimbleeBLE.sendInt(beatAvg);
    // }

  }
  lastBeatAvg = beatAvg;
  return (true);
}
void SimbleeBLE_onConnect()
{
  bConnected = true;
  //digitalWrite(BLU, LOW);
  //mode = 1;
  //Lazarus.ariseLazarus(); // Tell Lazarus to arise.
  //analogWrite(BLU,10);
}

void SimbleeBLE_onDisconnect()
{
  bConnected = false;
  //mode = 2;
  //digitalWrite(BLU, HIGH);
  //analogWrite(BLU,255);
  //pinMode(BLU,INPUT);
}

String digitalClockDisplay() {
  // digital clock display of the time
  String dataString = "";
  dataString += year();
  dataString += "-";
  dataString += month();
  dataString += "-";
  dataString += day();
  dataString += " ";
  dataString += hour();
  dataString += ":";
  //Serial.print(hour());
  if (minute() < 10) {
    dataString += "0";
  }
  dataString += minute();
  dataString += ":";
  //Serial.print(hour());
  if (second() < 10) {
    dataString += "0";
  }
  dataString += second();
  return dataString;
}
uint8_t getBatteryVoltage() {
  int counts = analogRead(V_SENSE);
  // Serial.print(counts); Serial.print("\t");
  float volts = float(counts) * (3.3 / 1023.0);
  // Serial.print(volts,3); Serial.print("\t");
  volts *= 2;
  // Serial.println(volts,3);

  return volts / BATT_VOLT_CONST;
}

void printOLED(String inString, boolean printTime) {
  
  
  oled.begin();    // Initialize the OLED
  oled.flipHorizontal(true);
  oled.flipVertical(true);
  oled.clear(ALL); // Clear the display's internal memory
  oled.clear(PAGE); // Clear the buffer.
  oled.setFontType(2);
  oled.setCursor(0, 0);
  if(printTime){
    String timeString = "";
    timeString += hour();
    timeString += ":";
    if (minute() < 10) {
      timeString += "0";
    }
    timeString += minute();
    oled.print(timeString);
  }
  oled.setCursor(0, 24);
  oled.setFontType(0);
  oled.println(inString);
  oled.display();
}
