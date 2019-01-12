


bool captureHR(uint32_t startTime) {
  if (millis() - startTime > interval) {
#ifdef DEBUG
    Serial.println("HR capture done");
#endif
    return (false);
  }
  // Use the sum of Red and Green to create the sample
  redSample = particleSensor.getRed();
  irSample = particleSensor.getIR();
  sumSample = redSample + irSample;
  sumSample = irSample + redSample;
  // Run the sample through a simple EMA highpass filter https://www.norwegiancreations.com/2016/03/arduino-tutorial-simple-high-pass-band-pass-and-band-stop-filtering/
  sumEMA_S = (EMA_a*sumSample) + ((1-EMA_a)*sumEMA_S);  //run the EMA
  sumHighpass = sumSample - sumEMA_S;                   //calculate the high-pass signal
#ifdef DEBUG
    unsigned long activeTime = micros();
#endif

  while (SimbleeBLE.radioActive) {
    ;
  }
  
#ifdef DEBUG
    Serial.print("radioActive uS ");          // use to test Simblee radio active time in serial monitor
    Serial.println(micros() - activeTime);
int plottable = constrain(sumHighpass,-500,500);
    Serial.println(plottable);              // use to test pulse waveform in serial plotter
#endif
  if (checkForPulse(sumHighpass) == true)
  {
    unsigned  long thisMillis = millis();
    analogWrite(RED, 200);
    //We sensed a beat!
    if(firstBeat){
      firstBeat = false;
      lastBeatTime = thisMillis;
      return(true);
    }
    delta = float(thisMillis - lastBeatTime);
    lastBeatTime = thisMillis;

    beatsPerMinute = 60 / (delta / 1000.0);

    if (beatsPerMinute < 200 && beatsPerMinute > 50)  // bpm 200 = 0.3 seconds/beat, bpm 50 = 1.2 seconds/beat
    {
        beat = beatsPerMinute;
        arrayBeats[beatCounter] = (uint8_t)beat;
        beatCounter++;
        if(beatCounter > 255){ beatCounter = 0; }
    }
//#ifdef DEBUG
    Serial.print("Instantaneous BPM: ");  // use to test instantaneous bpm in serial monitor
    Serial.println(beat);
//#endif
  }
  digitalWrite(RED, HIGH);

  return (true);
}


bool checkForPulse(int sample){

boolean beatDetected = false;

if(!rising){
  if (sample < T){
      T = sample;    // keep track of lowest point
    }else{
      amp = getAmplitude();
      rising = true;
      if(amp>90 && amp<3000){
        beatDetected = true;  // detect beat on lowest point
      }
    }
  }else{
    if (sample > P){
      P = sample;     // keep track of highest point
    }else{
      amp = getAmplitude();
      rising = false;
    }
  }
  return beatDetected;
}

int getAmplitude(){
    int a = P-T;
    if(rising){
      T = P;
    }else{
      P = T;
    }
    return a;
}
