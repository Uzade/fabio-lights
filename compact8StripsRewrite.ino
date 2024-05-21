#include <DMXSerial.h>
#include <Adafruit_NeoPixel.h>

#define SMARTSERIAL

#define numberOfStripes 2
#define numberOfEffects 2
#define ledsPerStripe 100

int adressOffset;

unsigned char outputpins[numberOfStripes] = {4, 5};

struct LED {
  unsigned char red;
  unsigned char green;
  unsigned char blue;
};

struct InputValue {
  unsigned char master;
  unsigned char red;
  unsigned char green;
  unsigned char blue;
};

typedef void (* ReducerFunction)(LED*, InputValue*, unsigned char);

struct LED stripes[numberOfStripes][ledsPerStripe];
struct InputValue inputValues[numberOfStripes];
struct Adafruit_NeoPixel actualStripes[numberOfStripes];

unsigned char effectValue;
unsigned char effectSpeed;
unsigned int updateRate;

unsigned long lastExecution;

//-------------- reducer functions --------------

void passthrough(LED* leds, InputValue* inputValue, unsigned char _speed) {
  double multiplier = inputValue->master/255.0;

  for(int i = 0; i < ledsPerStripe; i++){
    leds->red = inputValue->red * multiplier;
    leds->green = inputValue->green * multiplier;
    leds->blue = inputValue->blue * multiplier;
    leds++;
  }
}

void doNothing(LED* leds, InputValue* inputValue, unsigned char _speed) {
  //Just for testing purposes
  return;
}

//-------------- register reducers --------------
ReducerFunction reducers[numberOfEffects] = {
  &passthrough,
  &doNothing
};

//-------------- start framework implementation --------------

void setup() {
  DMXSerial.init(DMXReceiver);
  readDipSwitches();
  initializeStripes();
}

void loop() {
  lastExecution = millis();
  if(DMXSerial.dataUpdated()){
    readDMX();
  }
  calculateValues();
  updateLEDs();
  waitForNextLoop();
}

void readDipSwitches(){
  //TODO actually implement this
  adressOffset = 0;
}

void initializeStripes() {
  for(int i = 0; i < numberOfStripes; i++){
    actualStripes[i] = Adafruit_NeoPixel(ledsPerStripe, outputpins[i], NEO_GRB + NEO_KHZ800);
    actualStripes[i].begin();
  }
}

void readDMX(){
  /*
  inputValues[0].master = 128;
  inputValues[0].red = 255;
  inputValues[0].green = 0;
  inputValues[0].blue = 255;

  inputValues[1].master = 128;
  inputValues[1].red = 0;
  inputValues[1].green = 255;
  inputValues[1].blue = 0;

  effectValue = 20;
  effectSpeed = 0;
  */
  DMXSerial.resetUpdated();

  int baseChannel;
  for(int i = 0; i < numberOfStripes; i++){
    baseChannel = adressOffset + (i * 4) + 1;
    inputValues[i].master = DMXSerial.read(baseChannel + 0);
    inputValues[i].red = DMXSerial.read(baseChannel + 1);
    inputValues[i].green = DMXSerial.read(baseChannel + 2);
    inputValues[i].blue = DMXSerial.read(baseChannel + 3);
  }

  //Is 4+5 correct or does it have to be 5+6?
  effectValue = DMXSerial.read(baseChannel + 4);
  effectSpeed = DMXSerial.read(baseChannel + 5);
  updateRate = DMXSerial.read(baseChannel + 6);
  updateRate = (updateRate ? updateRate : 1) * 2
}

void calculateValues(){
  unsigned char effectIndex = effectValue / (256 / numberOfEffects);
  for(int i = 0; i < numberOfStripes; i++){
    //LED lastLeds[ledsPerStripe];
    //memcpy(stripes[i], lastLeds, sizeof(stripes[i]));
    //while(!stripeNeedsUpdate(lastLeds, stripes[i])){
      (*reducers[effectIndex])(stripes[i], &inputValues[i], effectSpeed);
    //}
  }
}

void updateLEDs(){
  for(int i = 0; i < numberOfStripes; i++){
    for(int j = 0; j < ledsPerStripe; j++){
      actualStripes[i].setPixelColor(
        j,
        stripes[i][j].red,
        stripes[i][j].green,
        stripes[i][j].blue
      );
    }
    actualStripes[i].show();
  }
}

void waitForNextLoop(){
  while(millis() - lastExecution < updateRate);
}

boolean stripeNeedsUpdate(LED* l1, LED* l2){
  boolean output = false;
  for(int i = 0; i < ledsPerStripe; i++){
    if(l1->red != l2->red || l1->green != l2->green || l1->blue != l2->blue){
      return true;
    }
    l1++;
    l2++;
  }
  return false;
}
