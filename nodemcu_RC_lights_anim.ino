/*

 Remote Controlled NeoPixel lights

 This program will run a strand of NeoPixel lights, and turn them on and off at
 at a set time of day and night. Time is updated via NTP over wifi.

 NTP functions from ESP example, by Michael Margolis/Tom Igoe/Ivan Grokhotkov.

 NeoPixel functions are from the NeoPixelBus library, for the ESP8266/NodeMCU.

 */

#include <ESP8266WiFi.h>
#include "esp_ntp.h"

#define INCLUDE_NEO_KHZ400_SUPPORT
#include <NeoPixelBus.h>
#include <NeoPixelAnimator.h>

////////////////// Function Prototypes ////////////////////
// NeoPixels//
RgbColor Wheel(byte WheelPos);
void rainbowCycle(uint8_t wait);
// inits //
void initWifi();
void DrawInitialRainbow();
void LoopAnimUpdate(const AnimationParam& param);

//////////////////// Global Variables //////////////////////
// NTP //
char ssid[] = "ultragenesis"; //  your network SSID (name)
char pass[] = "privatewifi2240"; // your network password

// A UDP instance to let us send and receive packets over UDP
//extern WiFiUDP udp;

/////////////////////////////////////////////////////////////

///////////////////// Neopixels setup ///////////////////////
//Neopixel setup
const uint16_t PixelCount = 144*2;
const uint8_t PixelPin = D9; // For RX0 / DMADriven branch
const uint16_t AnimCount = 1; // we only need one

NeoPixelBus<NeoGrbFeature, NeoEsp8266Dma400KbpsMethod> strip(PixelCount, PixelPin);
//NeoPixelBus strip = NeoPixelBus(pixelCount, pixelPin, NEO_GRB | NEO_KHZ400);
//NeoPixelBus strip = NeoPixelBus(pixelCount, pixelPin);
//NeoPixelBus strip = NeoPixelBus(pixelCount, pixelPin, NEO_RGB); //Alternate color order
NeoPixelAnimator animations(AnimCount); // NeoPixel animation management object

//Some pre-defined colors
#define colorSaturation 128
RgbColor red = RgbColor(colorSaturation, 0, 0);
RgbColor green = RgbColor(0, colorSaturation, 0);
RgbColor blue = RgbColor(0, 0, colorSaturation);
RgbColor white = RgbColor(colorSaturation);
RgbColor black = RgbColor(0);

//HslColor hslRed(red);
//HslColor hslGreen(green);
//HslColor hslBlue(blue);
//HslColor hslWhite(white);
//HslColor hslBlack(black);
////////////////////////////////////////////////////////////

bool lightsOn = false;
bool ntpValid = false;

void setup() {
    Serial.begin(115200);
    Serial.println();
    Serial.println();

    pinMode(BUILTIN_LED, OUTPUT); // Initialize the BUILTIN_LED pin as an output
    digitalWrite(BUILTIN_LED, HIGH); //Board LED on then off, to indicate bootup
    delay(500);
    digitalWrite(BUILTIN_LED, LOW);

    initWifi(); //wifi start & connect
    ntpStart(); //Setup NTP functions

    strip.Begin(); //Start the NeoPixel strip

    Serial.println("\n\nSetting strip to all RED ...\n\n");
    // this resets all the neopixels to an red state, to show startup
    for (int i = 0; i < PixelCount; i++) {
        strip.SetPixelColor(i, red);
    }
    //strip.ClearTo(red); //this might be causing problems
    strip.Show();
    delay(1000); //1 sec delay on all-red bootup, as indicator

    ntpValid = doNTPupdate(); //Do an NTP update on startup

    // we use the index 0 animation to time how often we rotate all the pixels
    animations.StartAnimation(0, 66, LoopAnimUpdate); 
}

void initWifi(){
    // We start by connecting to a WiFi network
    Serial.print("Connecting to ");
    Serial.println(ssid);
    WiFi.begin(ssid, pass);

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("");

    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
}

//On and Off times for lights
clockTime onTime = {.hours = (6 + 12), .minutes = 01, .seconds = 0}; //6:01 PM
clockTime offTime = {.hours = 1, .minutes = 30, .seconds = 0}; //1:30 AM



void loop() {
    //Serial.println("Starting loop()");
    
    if(lightsOn){
      animations.UpdateAnimations();
      Serial.println("Did UpdateAnimations()");
      strip.Show();
      Serial.println("showed strip");
    }

    clockTime lastNTP;
    getCurrTime(&lastNTP);

    int32_t millis_since_update = millis() - lastNTP.updateTimeMillis;
    if( millis_since_update > 30000 ){
      Serial.println("Doing NTP update ...");
      ntpValid = doNTPupdate();
    }

    if (ntpValid) { // Only check time if the percieved time is valid

        //Convert to seconds from midnight for easy comparisons
        unsigned long onTimeAsSeconds = ct_to_day_seconds(onTime);
        unsigned long offTimeAsSeconds = ct_to_day_seconds(offTime);
        unsigned long currTimeAsSeconds = ct_to_day_seconds(lastNTP);
    
        //For debugging start/stop condition:
        //Serial.print("onTimeAsSeconds = ");
        //Serial.println(onTimeAsSeconds);
        //Serial.print("offTimeAsSeconds = ");
        //Serial.println(offTimeAsSeconds);
        //Serial.print("currTimeAsSeconds = ");
        //Serial.println(currTimeAsSeconds);
      
        if ((currTimeAsSeconds <= offTimeAsSeconds) || (currTimeAsSeconds >= onTimeAsSeconds)) {
            Serial.println("LIGHTS ON");
            if(lightsOn == false){  //If transitioning from OFF state...
              DrawInitialRainbow(); //...redraw the initial rainbow colors
            }
            Serial.println("Did initial rainbow draw.");
            lightsOn = true;
            //animation update continues in superloop
        } else {
            Serial.println("LIGHTS OFF");
            lightsOn = false;
            strip.ClearTo(black);
            strip.Show();
            //animation update will NOT continue in superloop
        }
    }

    //Old
    //Pseudocode:
    // check time
    // if time is on time --> turn on
    // if time is off time --> turn off
    // if on --> rainbowcycle
    // if off --> wait 30 seconds
    //Serial.println("finishing loop()");
}



///////////// Neopixels functions ///////////////
// Slightly different, this makes the rainbow equally distributed throughout
// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
RgbColor Wheel(byte WheelPos) {
    WheelPos = 255 - WheelPos;

    if (WheelPos < 85) {
        return RgbColor(255 - WheelPos * 3, 0, WheelPos * 3);
    }
    if (WheelPos < 170) {
        WheelPos -= 85;
        return RgbColor(0, WheelPos * 3, 255 - WheelPos * 3);
    }
    WheelPos -= 170;
    return RgbColor(WheelPos * 3, 255 - WheelPos * 3, 0);
}

//Draws static rainbow, using Wheel()
void DrawInitialRainbow()
{
    uint16_t i;

    for (i = 0; i < strip.PixelCount(); i++) {
        RgbColor thisCol = Wheel((i * 256 / strip.PixelCount()) & 255);
        //thisCol.Darken(128);
        //thisCol.R /= 8;
        //thisCol.G /= 8;
        //thisCol.B /= 8;
        //strip.SetPixelColor(i, thisCol.R, thisCol.G, thisCol.B);
        strip.SetPixelColor(i, thisCol);
    }
}

void LoopAnimUpdate(const AnimationParam& param)
{
    // wait for this animation to complete,
    // we are using it as a timer of sorts
    if (param.state == AnimationState_Completed)
    {
        Serial.println("anim complete, restarting");
        // done, time to restart this position tracking animation/timer
        animations.RestartAnimation(param.index);

        // rotate the complete strip one pixel to the right on every update
        strip.RotateRight(1);
        //strip.RotateLeft(1);
    }
}

