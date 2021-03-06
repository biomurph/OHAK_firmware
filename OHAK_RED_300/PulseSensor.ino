/*
 *  THIS CODE LIFTED FROM THE PULSESENSOR PLAYGROUND LIBRARY
 */


#define FADE_LEVEL_PER_SAMPLE 12
#define MAX_FADE_LEVEL 255
#define MIN_IBI 300

    int FadePin = RED;            // pin to fade on beat

    // Pulse detection output variables.
    int BPM;                // int that holds Beats Per Minute value
    int IBI;                // int that holds the time interval (ms) between beats! Must be seeded!
    boolean Pulse;          // "True" when User's live heartbeat is detected. "False" when not a "live beat".
    boolean QS;             // The start of beat has been detected and not read by the Sketch.
    int FadeLevel;          // brightness of the FadePin, in scaled PWM units. See FADE_SCALE
    int threshSetting;      // used to seed and reset the thresh variable
    int amp;                         // used to hold amplitude of pulse waveform, seeded (sample value)
    unsigned long lastBeatTime;      // used to find IBI. Time (sampleCounter) of the previous detected beat start.
    unsigned long sampleIntervalMs = 10;  // expected time between calls to readSensor(), in milliseconds.
    int rate[10];                    // array to hold last ten IBI values (ms)
    unsigned long pulseSampleCounter;     // used to determine pulse timing. Milliseconds since we started.
    int P;                           // used to find peak in pulse wave, seeded (sample value)
    int T;                           // used to find trough in pulse wave, seeded (sample value)
    int thresh;                      // used to find instant moment of heart beat, seeded (sample value)
    boolean firstBeat;               // used to seed rate array so we startup with reasonable BPM
    boolean secondBeat;              // used to seed rate array so we startup with reasonable BPM


void resetVariables(){
  for (int i = 0; i < 10; ++i) {
    rate[i] = 0;
  }
  QS = false;
  BPM = 0;
  IBI = 750;                  // 750ms per beat = 80 Beats Per Minute (BPM)
  Pulse = false;
  pulseSampleCounter = 0;
  lastBeatTime = 0;
  P = 0;                     
  T = 0;                    
  threshSetting = 0;        // used to seed and reset the thresh variable
  thresh = 0;     // threshold a little above the trough
  amp = 100;                  // beat amplitude 1/10 of input range.
  firstBeat = true;           // looking for the first beat
  secondBeat = false;         // not yet looking for the second beat in a row
  FadeLevel = 0; // LED is dark.
}


void findBeat(float signal){  // this takes 120uS max
//  thatTestTime = micros();  // USE TO TIME BEAT FINDER
  pulseSampleCounter += sampleIntervalMs;         // keep track of the time in mS with this variable
  int N = pulseSampleCounter - lastBeatTime;      // monitor the time since the last beat to avoid noise

  // Fade the Fading LED
  FadeLevel = FadeLevel + FADE_LEVEL_PER_SAMPLE;
  FadeLevel = constrain(FadeLevel, 0, MAX_FADE_LEVEL);
  FadeHeartbeatLED();

  //  find the peak and trough of the pulse wave
  if (signal > thresh && N > (IBI / 5) * 3) { // avoid dichrotic noise by waiting 3/5 of last IBI
    if (signal > P) {                        // P is the peak
      P = signal;                            // keep track of highest point in pulse wave
    }
  }

  if (signal < thresh && signal < T) {       // thresh condition helps avoid noise
    T = signal;                              // T is the trough
  }                                          // keep track of lowest point in pulse wave

  //  NOW IT'S TIME TO LOOK FOR THE HEART BEAT
  // signal surges down in value every time there is a pulse
  if (N > 250) {                             // avoid high frequency noise
    if ( (signal < thresh) && (Pulse == false) && (N > (IBI / 5) * 3) ) { // && (amp > 50)) {
      IBI = pulseSampleCounter - lastBeatTime;    // measure time between beats in mS
      if(IBI < MIN_IBI){ return; }
      Pulse = true;                          // set the Pulse flag when we think there is a pulse
      lastBeatTime = pulseSampleCounter;          // keep track of time for next pulse

      if (secondBeat) {                      // if this is the second beat, if secondBeat == TRUE
        secondBeat = false;                  // clear secondBeat flag
        for (int i = 0; i <= 9; i++) {       // seed the running total to get a realisitic BPM at startup
          rate[i] = IBI;
        }
      }

      if (firstBeat) {                       // if it's the first time we found a beat, if firstBeat == TRUE
        firstBeat = false;                   // clear firstBeat flag
        secondBeat = true;                   // set the second beat flag
        // IBI value is unreliable so discard it
        return;
      }


      // keep a running total of the last 10 IBI values
//      word runningTotal = 0;                  // clear the runningTotal variable
//
//      for (int i = 0; i <= 8; i++) {          // shift data in the rate array
//        rate[i] = rate[i + 1];                // and drop the oldest IBI value
//        runningTotal += rate[i];              // add up the 9 oldest IBI values
//      }

//      rate[9] = IBI;                          // add the latest IBI to the rate array
//      runningTotal += rate[9];                // add the latest IBI to runningTotal
//      runningTotal /= 10;                     // average the last 10 IBI values
      BPM = 60000 / IBI; //runningTotal;             // how many beats can fit into a minute? that's BPM!
      QS = true;                              // set Quantified Self flag (we detected a beat)
      FadeLevel = 0;                          // re-light that LED.
    }
  }

  if (signal > thresh && Pulse == true) {  // when the values are going down, the beat is over
    Pulse = false;                         // reset the Pulse flag so we can do it again
    amp = P - T;                           // get amplitude of the pulse wave
    thresh = amp/2 + T;                  // set thresh at 50% of the amplitude
    P = thresh;                            // reset these for next time
    T = thresh;
  }

  if (N > 2500) {                          // if 2.5 seconds go by without a beat
    thresh = threshSetting;                // set thresh default
    P = 0;                                 // set P default
    T = 0;                                 // set T default
    lastBeatTime = pulseSampleCounter;     // bring the lastBeatTime up to date
    firstBeat = true;                      // set these to avoid noise
    secondBeat = false;                    // when we get the heartbeat back
    QS = false;
    BPM = 0;
    IBI = 600;                  // 600ms per beat = 100 Beats Per Minute (BPM)
    Pulse = false;
    amp = 100;                 
  }
//      thisTestTime = micros(); Serial.println(thisTestTime - thatTestTime);
}

void FadeHeartbeatLED(){
  analogWrite(FadePin, FadeLevel);
}

void printBeatData(){
  Serial.print("BPM: "); Serial.print(BPM); printTab();
  Serial.print("IBI: "); Serial.print(IBI); printTab();
  Serial.print("thresh: "); Serial.print(thresh); printTab();
  Serial.print("amp: "); Serial.print(amp); printTab();

  Serial.println();
}

boolean checkQS(){
  if(QS){ 
    QS = false;
    return true;
  }
  return false;
}
