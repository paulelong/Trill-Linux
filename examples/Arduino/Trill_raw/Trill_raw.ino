
#include <Trill.h>

Trill slider(0x30); // for Trill Craft

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  
  if(!slider.begin())
    Serial.println("failed to initialise slider");
   
  slider.setMode(Trill::DIFF);
  //delay(10);
  //slider.setIDACValue(160);
}

void loop() {
  // put your main code here, to run repeatedly:

  delay(100);
  slider.requestRawData();
   
  if(slider.rawDataAvailable() > 0) {
    while(slider.rawDataAvailable() > 0) {
      int data = slider.rawDataRead();
      Serial.print(data);
      Serial.print(" ");
    }
    Serial.println("");
  }
}
