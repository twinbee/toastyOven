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

///////////////////////////////////////////////////////////////////////
// API part (e.g. make your own profile)
///////////////////////////////////////////////////////////////////////

#include <OneWire.h>
#include <DallasTemperature.h>

// PID controller constants
const float k_p = 1.0f;
const float k_i = 100.0f;
const float k_d = 100.0f;

// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)

const int BUTTONPIN = 4;
const int LEDPIN = 13;
const int BOTTOMELEMENT = 12;
const int TOPELEMENT = 11;

const int ONE_WIRE_BUS = 2;
OneWire oneWire(ONE_WIRE_BUS);

// Pass our oneWire reference to Dallas Temperature. 
DallasTemperature sensors(&oneWire);

// arrays to hold device address
DeviceAddress insideThermometer;


const int NUM_PROFILES = 3;
void (* profiles  [NUM_PROFILES]) () = { profile1, profile2, profile3 };

const int NUM_STAGES = 4;
int setPoints[NUM_STAGES] = {0,0,0,0};
int setPointDurations[NUM_STAGES] = {0,0,0,0};

// a buffer used for hysteresis / dampening.
//  within epsilon degrees is "good enough" 
//  to count time toward our goal time for 
//  that stage
// Use half the value that you actually want
// so if you want the range to be 10 degrees,
//  e.g. 500C +/-5C, use 5.0C
int tempEpsilon = 5; 

void profile1()
{
//times are in seconds
//temps are in degrees Centigrade

  Serial.println("profile1: MEMS mics");
// Preheat 160C
//Dwell 120s
//Ramp Up 1C/s for 98s
//Dwell  30s
//Ramp Down 1C/s for 98s
//Ramp Off
setPoints[0] = 160;
setPointDurations[0] = 120;

setPoints[1] = 220;
setPointDurations[1] = 30;

setPoints[2] = 160;
setPointDurations[2] = 60;

setPoints[3] = 40;
setPointDurations[3] = 30;

}

void profile2()
{
   Serial.println("profile2: nothing");
   
setPoints[0] = 160;
setPointDurations[0] = 120;

setPoints[1] = 220;
setPointDurations[1] = 30;

setPoints[2] = 160;
setPointDurations[2] = 10;

setPoints[3] = 40;
setPointDurations[3] = 30;
}

void profile3()
{
   Serial.println("profile3: nothing");
      
setPoints[0] = 160;
setPointDurations[0] = 120;

setPoints[1] = 220;
setPointDurations[1] = 30;

setPoints[2] = 160;
setPointDurations[2] = 10;

setPoints[3] = 40;
setPointDurations[3] = 30;
}


/////////////////////////////////////////////////////////////////////////
// Base code part (no need to edit but go to town if you want to!)
/////////////////////////////////////////////////////////////////////////
float error = 0.0f;
float integral = 0.0f;
float derivative = 0.0f;

float pidOut = 0.0f;

float lastSample = 0.0f;
unsigned long lastSampleTime = millis();
unsigned long sampleTime = millis();

// in duty cycles, 010101 means on, off, on, off, on, off when going right to left
const byte dutyCycle0 = 0b00000000;
const byte dutyCycle25 = 0b10001000;
const byte dutyCycle50 = 0b10101010;
const byte  dutyCycle625 = 0b10101110;
const byte  dutyCycle75 = 0b11101110;
const byte  dutyCycle875 = 0b11111110;
const byte dutyCycle100 = 0b11111111;

byte duty = dutyCycle0;

byte dutyCycleCounter = 0;

int profileChosen = -1;
bool profileAcknowledged = false;

bool lastButton = false;
bool buttonState = false;
unsigned long timeButtonPressed = 0;

//status of the environment
float currentTemp1 = 0;
float currentTemp2 = 0;
bool element1 = false;
bool element2 = false;

unsigned long timeTempReached = 0;
unsigned long timeAtGoal = 0;

int profileStage = 0;

//pot used for simulating temperature during development
int sensorPin = A0; 

// the setup function runs once when you press reset or power the board
void setup() {
  Serial.begin(9600);

  // locate devices on the bus
  Serial.print("Locating devices...");
  sensors.begin();
  Serial.print("Found ");
  Serial.print(sensors.getDeviceCount(), DEC);
  Serial.println(" devices.");

  // report parasite power requirements
  Serial.print("Parasite power is: "); 
  if (sensors.isParasitePowerMode()) Serial.println("ON");
  else Serial.println("OFF");
  
  // assign address manually.  the addresses below will beed to be changed
  // to valid device addresses on your bus.  device address can be retrieved
  // by using either oneWire.search(deviceAddress) or individually via
  // sensors.getAddress(deviceAddress, index)
  //insideThermometer = { 0x28, 0x1D, 0x39, 0x31, 0x2, 0x0, 0x0, 0xF0 };

  // Method 1:
  // search for devices on the bus and assign based on an index.  ideally,
  // you would do this to initially discover addresses on the bus and then 
  // use those addresses and manually assign them (see above) once you know 
  // the devices on your bus (and assuming they don't change).
  if (!sensors.getAddress(insideThermometer, 0)) Serial.println("Unable to find address for Device 0"); 
  
  // method 2: search()
  // search() looks for the next device. Returns 1 if a new address has been
  // returned. A zero might mean that the bus is shorted, there are no devices, 
  // or you have already retrieved all of them.  It might be a good idea to 
  // check the CRC to make sure you didn't get garbage.  The order is 
  // deterministic. You will always get the same devices in the same order
  //
  // Must be called before search()
  //oneWire.reset_search();
  // assigns the first address found to insideThermometer
  //if (!oneWire.search(insideThermometer)) Serial.println("Unable to find address for insideThermometer");

  // show the addresses we found on the bus
  Serial.print("Device 0 Address: ");
  printAddress(insideThermometer);
  Serial.println();

  // set the resolution to 9 bit (Each Dallas/Maxim device is capable of several different resolutions)
  sensors.setResolution(insideThermometer, 9);
 
  Serial.print("Device 0 Resolution: ");
  Serial.print(sensors.getResolution(insideThermometer), DEC); 
  Serial.println();
  
  pinMode(BOTTOMELEMENT, OUTPUT);
  pinMode(TOPELEMENT, OUTPUT);

  pinMode(LEDPIN, OUTPUT);

  pinMode(BUTTONPIN, INPUT);
  
  digitalWrite(LEDPIN, HIGH);
  
  blinkOutAck();

}

// the loop function runs over and over again forever
void loop() {
if (!profileAcknowledged)
{
  collectProfileSetting();
}
 else   
    {
      checkButtonForResetEvent();
      controlElements();
      getTemp ();    // to be replaced with getting the temp from the thermocouple or RTD
      calculatePid();
      outputData();
      reactToTemp();
      advancePhase();
    }
    delay(500);
} //end main event loop

void calculatePid()
{
 error = setPoints[profileStage] - currentTemp1;
 derivative = (lastSample - currentTemp1)/(lastSampleTime - sampleTime + 1);
 integral += error/(lastSampleTime - sampleTime);
 
 pidOut = k_p*error + k_i*integral + k_d*derivative;
}

void controlElements()
{
 dutyCycleCounter = (dutyCycleCounter + 1)%8;
 
 //counterth  bit of dutyCycle
 element1 = (duty & ( 1 << dutyCycleCounter%8 )) >> dutyCycleCounter%8;
 element2 = (duty & ( 1 << (dutyCycleCounter+1)%8 )) >> (dutyCycleCounter+1)%8;
 
 if (element1)
 {
  digitalWrite(TOPELEMENT, true);
 }
 else //if (!element1)
 {
  digitalWrite(TOPELEMENT, false);
 }
 
 if (element2)
 {
  digitalWrite(BOTTOMELEMENT, true);
 }
 else //if (!element2)
 {
  digitalWrite(BOTTOMELEMENT, false);
 }
}

void getTemp ()
{
  sensors.requestTemperatures();
  
  lastSampleTime = sampleTime;
  sampleTime = millis();
  
  lastSample = currentTemp1;

  //currentTemp1 = sensors.getTempC(insideThermometer);
  currentTemp1 = analogRead(sensorPin);  
}

void outputData()
{
  Serial.print(millis());
  Serial.print(",");
  Serial.print(currentTemp1);
  Serial.print(",On?");
  Serial.print(element1);
  Serial.print(":");
  Serial.print(element2);
  Serial.print(",");
  Serial.print(setPoints[profileStage]);
  Serial.print(",");
  Serial.print(setPointDurations[profileStage]);  
  Serial.print(",");
  Serial.print(timeAtGoal);  
  Serial.print(",");
  Serial.print(profileChosen);
  Serial.print("[");
  Serial.print(profileStage);
  Serial.print("],");  
  Serial.print(duty, 2); // print in base 2
  Serial.print(",");  
  Serial.print(error,4);
  Serial.print(",");  
  Serial.print(integral,8);
  Serial.print(",");  
  Serial.print(derivative,8);
  Serial.print(",");  
  Serial.print(pidOut,8);
  Serial.print(",");  
    
    //overshoot condition!
  if ( currentTemp1 > (setPoints[profileStage] + tempEpsilon) )
  {
   Serial.print(",!OVER!");
  }

  Serial.println ();
}


void collectProfileSetting()
{
  buttonState = digitalRead(BUTTONPIN);

  while (!profileAcknowledged)
  {
  if (buttonState == HIGH) 
  {
    if (lastButton != buttonState) 
    {
      timeButtonPressed = millis();
    }
  } 
  else //  if (buttonState == LOW) 
  {
    if (lastButton != buttonState && millis() - timeButtonPressed > 30) 
    {
      //Serial.print("B-off after ");
      //Serial.println(millis() - timeButtonPressed);
      
      if (millis() - timeButtonPressed < 1000)
      {
       profileChosen++;
       profileChosen %= NUM_PROFILES;
       Serial.print("Selected profile: ");
       Serial.println(profileChosen+1);
       blinkOutProfileChosen();
      }
      else 
      {
       profileAcknowledged = true;
       blinkOutAck();
       blinkOutProfileChosen();
       blinkOutAck();
       //do the settings for the given profile
       profiles[profileChosen](); 
       break;
      }      
    }
 }
 lastButton = buttonState;
 buttonState= digitalRead(BUTTONPIN);
} //end while

      timeButtonPressed = 0;
}

void  checkButtonForResetEvent()
{
  lastButton = buttonState;
  buttonState = digitalRead(BUTTONPIN);

   if (buttonState == HIGH) 
   {
        Serial.println("!RESET!");
        resetSoft();
   }      
}

void blinkOutProfileChosen()
{
     digitalWrite(LEDPIN, LOW);
     delay (200);
     
     for (int i = 1; i <= profileChosen+1; i++)
     {
     digitalWrite(LEDPIN, HIGH);
     delay (200);
     
     digitalWrite(LEDPIN, LOW);
     delay (100);
     }
     
     digitalWrite(LEDPIN, LOW);
     delay (100);
     
     digitalWrite(LEDPIN, LOW);
}

void blinkOutAck()
{
     for (int i = 1; i <= 4; i++)
     {
     digitalWrite(LEDPIN, HIGH);
     delay (50);
     
     digitalWrite(LEDPIN, LOW);
     delay (50);
     }
}

void reactToTemp ()
{
 if ( currentTemp1 > setPoints[profileStage] - tempEpsilon )
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
  //if (currentTemp1 > setPoints[profileStage] + tempEpsilon ) 
  if (pidOut < -2.0*tempEpsilon)
  {
   duty = dutyCycle0;
  }
  //Case 2: such overshoot!
  //else if (currentTemp1 > setPoints[profileStage] )
  else if (pidOut < -tempEpsilon)
  {  
   duty = dutyCycle25;
  }
  //else if (currentTemp1 > setPoints[profileStage] - tempEpsilon )
  else if (pidOut < 0.0 )
  {
    duty = dutyCycle50;
  }
  //else if (currentTemp1 < setPoints[profileStage] - 2*tempEpsilon )
  else if (pidOut < tempEpsilon )
  {
    duty = dutyCycle625;
  }
  else if (pidOut < 2.0*tempEpsilon )
  {
    duty = dutyCycle75;
  }
  else if (pidOut < 3.0*tempEpsilon )
  {
    duty = dutyCycle875;
  }
  else //a long, long way to go and many ob-stackles in your path
    //  --the oracle from o brother, where art thou?
  {
    duty = dutyCycle100;
  }
  
}

void resetSoft()
{
  digitalWrite(TOPELEMENT, 0);
  digitalWrite(BOTTOMELEMENT, 0);
  element1 = false;
  element2 = false;
  duty = 
   profileStage = 0;
   profileChosen = -1;
   profileAcknowledged = false;
}

void advancePhase ()
{
 if (profileStage >= NUM_STAGES) 
 { 
   Serial.println("Resetting");
   resetSoft();
 }

 else if (timeAtGoal > 1000UL*setPointDurations[profileStage])
 {
  profileStage += 1;
  timeAtGoal = 0UL;
  timeTempReached = 0UL;
 }
} 
 
 // function to print a device address
void printAddress(DeviceAddress deviceAddress)
{
  for (uint8_t i = 0; i < 8; i++)
  {
    if (deviceAddress[i] < 16) Serial.print("0");
    Serial.print(deviceAddress[i], HEX);
  }
}


