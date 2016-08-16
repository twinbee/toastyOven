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

const int NUM_PROFILES = 3;
void (* profiles  [NUM_PROFILES]) () = { profile1, profile2, profile3 };

const int NUM_STAGES = 4;
int cookTemps[NUM_STAGES] = {0,0,0,0};
int cookTimes[NUM_STAGES] = {0,0,0,0};

// a buffer used for hysteresis / dampening.
//  within epsilon degrees is "good enough" 
//  to count time toward our goal time for 
//  that stage
// Use half the value that you actually want
// so if you want the range to be 10 degrees,
//  e.g. 500C +/-5 C, use 5.0f
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
cookTemps[0] = 160;
cookTimes[0] = 120;

cookTemps[1] = 220;
cookTimes[1] = 30;

cookTemps[2] = 160;
cookTimes[2] = 60;

cookTemps[3] = 40;
cookTimes[3] = 30;

}

void profile2()
{
   Serial.println("profile2: nothing");
   
cookTemps[0] = 160;
cookTimes[0] = 120;

cookTemps[1] = 220;
cookTimes[1] = 30;

cookTemps[2] = 160;
cookTimes[2] = 10;

cookTemps[3] = 40;
cookTimes[3] = 30;
}

void profile3()
{
   Serial.println("profile3: nothing");
      
cookTemps[0] = 160;
cookTimes[0] = 120;

cookTemps[1] = 220;
cookTimes[1] = 30;

cookTemps[2] = 160;
cookTimes[2] = 10;

cookTemps[3] = 40;
cookTimes[3] = 30;
}


/////////////////////////////////////////////////////////////////////////
// Base code part (no need to edit but go to town if you want to!)
/////////////////////////////////////////////////////////////////////////

int profileChosen = -1;
bool profileAcknowledged = false;

bool lastButton = false;
bool buttonState = false;
unsigned long timeButtonPressed = 0;

const int BUTTONPIN = 2;
const int LEDPIN = 3;
const int BOTTOMELEMENT = 12;
const int TOPELEMENT = 13;

//status of the environment
int currentTemp1 = 0;
int currentTemp2 = 0;
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
      outputData();
      controlElements();
      getTemp ();    // to be replaced with getting the temp from the thermocouple or RTD
      reactToTemp();
      advancePhase();
    }
    delay(10);
} //end main event loop

void controlElements()
{
  if (element1 == true)   
  { 
    digitalWrite(TOPELEMENT, HIGH);
  }
  else
  {
   digitalWrite(TOPELEMENT, LOW);
  }      
}

void getTemp ()
{
// if (element1) currentTemp += 2;
// else currentTemp -= 1;
 currentTemp1 = analogRead(sensorPin);  
}

void outputData()
{
  Serial.print(millis());
  Serial.print(",");
  Serial.print(element1);
  Serial.print(",");
  Serial.print(currentTemp1);
  Serial.print(",");
  Serial.print(profileChosen);
  Serial.print("[");
  Serial.print(profileStage);
  Serial.print("],");
  Serial.print(cookTemps[profileStage]);
  Serial.print(":");
  Serial.print(cookTimes[profileStage]);  
  Serial.print(",");
  Serial.print(timeAtGoal);  
  
    //overshoot condition!
  if ( currentTemp1 > (cookTemps[profileStage] + tempEpsilon) )
  {
   Serial.print(",!OVERSHOOT!  ");
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
 
 if ( currentTemp1 > (cookTemps[profileStage] - tempEpsilon)  
  //&& currentTemp1 < (cookTemps[profileStage] + tempEpsilon) 
  )
 {
  //first time that the temp was reached
  if (element1 && timeTempReached < 1 ) 
  {
    timeTempReached = millis();
  }
  else
  {
    timeAtGoal = (millis() - timeTempReached);
  }
 }

 if (!currentTemp1 > cookTemps[profileStage] )
  {  
   element1 = false;
  }
  else element1 = true;

}

void resetSoft()
{
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

 else if (timeAtGoal > 1000UL*cookTimes[profileStage])
 {
  profileStage += 1;
  timeAtGoal = 0;
 }
 
}
