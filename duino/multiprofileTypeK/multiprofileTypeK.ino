/**
ToastyOven arduino code by Matthew A Bennett, 2016
Licensed under the Mozilla Public License v 2.0

  A very minimal toaster oven reflow oven controller using 
   arduino, SSRs, thermocouples/amps, and 
   
   To use, fill out a 4-stage profile functor and add to the array "profiles"
    Use profile1() as an example
   When you turn on the toaster, a short button press changes profiles
   The LED will blink a number of times to indicate the profile setting
   When you have the one you want, hold the button for three seconds.
   
   Serial port will spit out diagnostic data you can use for profiling
    in a CSV format of: time,temp,TopOn,BottomOn
*/
/*
 * Modified by David Mandala, with my changes also Licensed 
 * under the Mozilla Public License v 2.0
 * 
 * Moved the profiles into an array, added a second button and added an 
 * OLED display to the mix.  Seperated the functions into several files
 * for ease of keeping track of what I'm doing.
 */

///////////////////////////////////////////////////////////////////////
// API part (e.g. make your own profile)
///////////////////////////////////////////////////////////////////////

// ***** INCLUDES *****
#include <SPI.h>
#include <Wire.h>

// Pins to control or read devices
#define SWITCH_STOP_START_BUTTON_PIN 3
#define SWITCH_SELECT_PROFILE_BUTTON_PIN 2
#define LED_PIN LED_BUILTIN
#define BOTTOM_ELEMENT 6
#define TOP_ELEMENT 5

typedef enum SWITCH
{
  SWITCH_NONE,
  SWITCH_STOP_START,
  SWITCH_SELECT_PROFILE
} switch_t;

// PID controller constants
const float k_p = 1.0f;
const float k_i = 100.0f;
const float k_d = 200.0f;

// a buffer used for hysteresis / dampening.
//  within epsilon degrees is "good enough" 
//  to count time toward our goal time for 
//  that stage
// Use half the value that you actually want
// so if you want the range to be 10 degrees,
//  e.g. 500C +/-5C, use 5.0C
// Want no more then 5 degree swing but 5 went over by 10+
int tempEpsilon = 1; 

// ***** MESSAGES *****
const char* MessagesReflowStatus[] = {
  "Ready ",
  "Pre   ",
  "Soak  ",
  "Reflow",
  "R Cool",
  "Cool  ",
  "Done! ",
  "Hot!  ",
  "Error "
};

const int NUM_STAGES = 5;

struct tprofile{
  char name[10];
  int setPoints[NUM_STAGES];
  int setPointDurations[NUM_STAGES];
};
//            Pre,  soak, reflow, rcool, cool, ps,  ss,   sref,  scool, cool
tprofile profiles[] = {
  {"Lead   ", 180,   200,   220,  180,   50,   1,    60,   20,   10,    60},
  {"No Lead", 200,   220,   245,  220,   50,   1,   100,   25,   25,    60},
  {"Test   ", 150,   180,   210,  180,   50,   1,    70,   20,   20,    60}
};

/////////////////////////////////////////////////////////////////////////
// Base code part (no need to edit but go to town if you want to!)
/////////////////////////////////////////////////////////////////////////
// Get the count of how many profiles are stored in the program.
// looks a little strange because of how sizeof works on arrays of structs
#define NUM_PROFILES sizeof profiles / sizeof profiles[0]

float error = 0.0f;
float integral = 0.0f;
float derivative = 0.0f;

float pidOut = 0.0f;

float lastSample = 0.0f;
unsigned long lastSampleTime = millis();
unsigned long sampleTime = millis();
unsigned long totalRunTime;

// in duty cycles, 010101 means on, off, on, off, on, off when going right to left
#define DUTY_CYCLE_0    0b00000000
#define DUTY_CYCLE_25   0b10001000
#define DUTY_CYCLE_50   0b10101010
#define DUTY_CYCLE_625  0b10101110
#define DUTY_CYCLE_75   0b11101110
#define DUTY_CYCLE_875  0b11111110
#define DUTY_CYCLE_100  0b11111111

byte duty = DUTY_CYCLE_0;
byte dutyCycleCounter = 0;

int profileChosen = 0; // First profile in memory.
bool profileAcknowledged = false;

//status of the environment
float currentTemp1 = 0;
bool element1 = false;
bool element2 = false;

unsigned long timeTempReached = 0;
unsigned long timeAtGoal = 0;

int profileStage = 0;

// All the functions in the other files.
float getTemp (void);
void initButtons(void);
byte switchJustPressed(void);
void outputOLEDSplash(void);
void outputOLEDStatus(void);

// the setup function runs once when you press reset or power the board
void setup(void) {
  Serial.begin(115200);

  getTemp ();
  // Set pin modes
  pinMode(BOTTOM_ELEMENT, OUTPUT);
  pinMode(TOP_ELEMENT, OUTPUT);
  pinMode(LED_PIN, OUTPUT);
  initButtons();

  switchJustPressed();
  outputOLEDSplash();
  Serial.print("Profiles in memory: ");
  Serial.println(NUM_PROFILES);
  Serial.print("Current Profile: ");
  Serial.println(profiles[profileChosen].name);
  
  digitalWrite(LED_PIN, HIGH);
}

// the loop function runs over and over again forever
void loop(void) {
  if (!profileAcknowledged)
  {
    selectProfile();
    getTemp();
    outputOLEDStatus();
  }
  else   
  {
    getTemp ();
    checkButtonForResetEvent();
    outputSerialData();
    outputOLEDStatus();
    controlElements();
    calculatePid();
    reactToTemp();
    advancePhase();
    delay(500);// Why 1/2 second delay here?
  }
} //end main event loop

void calculatePid(void)
{
 error = profiles[profileChosen].setPoints[profileStage] - currentTemp1;
 derivative = (lastSample - currentTemp1)/(lastSampleTime - sampleTime + 1);
 integral += error/(lastSampleTime - sampleTime);
 
 pidOut = k_p*error + k_i*integral + k_d*derivative;
}

void controlElements(void)
{
 dutyCycleCounter = (dutyCycleCounter + 1)%8;
 
 //counterth  bit of dutyCycle
 element1 = (duty & ( 1 << dutyCycleCounter%8 )) >> dutyCycleCounter%8;
 element2 = (duty & ( 1 << (dutyCycleCounter+1)%8 )) >> (dutyCycleCounter+1)%8;
 
 if (element1)
 {
  digitalWrite(TOP_ELEMENT, true);
 }
 else //if (!element1)
 {
  digitalWrite(TOP_ELEMENT, false);
 }
 
 if (element2)
 {
  digitalWrite(BOTTOM_ELEMENT, true);
 }
 else //if (!element2)
 {
  digitalWrite(BOTTOM_ELEMENT, false);
 }
}

void reactToTemp (void)
{

  if ( currentTemp1 > profiles[profileChosen].setPoints[profileStage] - tempEpsilon )
  {
    //first time that the temp was reached
    if (
      timeTempReached < 1 ) 
    {
      timeTempReached = millis();
    }
    else
    {
      timeAtGoal = (millis() - timeTempReached);
    }
  }

  //Part deux: take care of the duty cycle
  // TODO: add reaction as PID control

  // Case 1: so many overshoot!
  //if (currentTemp1 > profiles[profileChosen].setPoints[profileStage] + tempEpsilon ) 
  if (pidOut < -2.0*tempEpsilon)
  {
    duty = DUTY_CYCLE_0;
  }
  //Case 2: such overshoot!
  //else if (currentTemp1 > profiles[profileChosen].setPoints[profileStage] )
  else if (pidOut < -tempEpsilon)
  {  
    duty = DUTY_CYCLE_50;
  }
  //else if (currentTemp1 > profiles[profileChosen].setPoints[profileStage] - tempEpsilon )
  else if (pidOut < 0.0 )
  {
    duty = DUTY_CYCLE_625;
  }
  //else if (currentTemp1 < profiles[profileChosen].setPoints[profileStage] - 2*tempEpsilon )
  else if (pidOut < tempEpsilon )
  {
    duty = DUTY_CYCLE_75;
  }
  else if (pidOut < 2.0*tempEpsilon )
  {
    duty = DUTY_CYCLE_875;
  }
  else if (pidOut < 3.0*tempEpsilon )
  {
    duty = DUTY_CYCLE_100;
  }
  else //a long, long way to go and many ob-stackles in your path
    //  --the oracle from o brother, where art thou?
  {
    duty = DUTY_CYCLE_100;
  }
  
}

void advancePhase (void)
{
 if (profileStage >= NUM_STAGES) 
 { 
   Serial.println("End of profile");
   resetSoft();
 }
 else if (timeAtGoal > 1000UL*profiles[profileChosen].setPointDurations[profileStage])
 {
  profileStage += 1;
  timeAtGoal = 0UL;
  timeTempReached = 0UL;
 }
} 

void selectProfile(void)
{
  byte buttonPressed = switchJustPressed();
  if (buttonPressed == SWITCH_SELECT_PROFILE)
  {
    profileChosen++;
    profileChosen %= NUM_PROFILES;
    Serial.print("Selected profile: ");
    Serial.println(profiles[profileChosen].name);
  }
  else if (buttonPressed == SWITCH_STOP_START)
  {
    profileAcknowledged = true;
    totalRunTime = millis();
  }
}

void checkButtonForResetEvent(void)
{
  byte buttonPressed = switchJustPressed();
  if (buttonPressed == SWITCH_STOP_START)
  {
    resetSoft();
  }
  return;
}

void resetSoft(void)
{
  digitalWrite(TOP_ELEMENT, 0);
  digitalWrite(BOTTOM_ELEMENT, 0);
  element1 = false;
  element2 = false;
  duty = 
   profileStage = 0;
   profileAcknowledged = false;
   return;
}
