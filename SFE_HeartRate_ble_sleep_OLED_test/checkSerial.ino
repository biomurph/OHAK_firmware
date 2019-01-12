
void checkSerial(){
  if(Serial.available()){
    char inChar = Serial.read();

    switch(inChar){
      case 'h':
        graphBeats(true);
        return;
      default:
        return;
    }
  }
}


void graphBeats(boolean ws){
    if(ws){
      particleSensor.wakeUp();
      particleSensor.setup(ledBrightness, sampleAverage, ledMode, sampleRate, pulseWidth, adcRange);
    }
//    Serial.println("HR");
    memset(arrayBeats, 0, sizeof(arrayBeats));
    firstBeat = true;
    beat = lastBeat = beatCounter = 0;
    delay(1500);
    sumEMA_S = particleSensor.getIR() + particleSensor.getRed();
    startTime = millis();
    while (captureHR(startTime)) {
      ;
    }
}

