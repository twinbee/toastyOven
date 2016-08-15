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

const int NUM_PROFILES = 3;
void (* profiles  [NUM_PROFILES]) () = { profile1, profile1, profile1};

int profileChosen = -1;
bool profileAcknowledged = false;

const int BUTTONPIN = 2;
const int LEDPIN = 3;
const int BOTTOMELEMENT = 12;
const int TOPELEMENT = 13;

//status of the environment
int currentTemp1 = 0;
int currentTemp2 = 0;
bool element1 = false;
bool element2 = false;

int timeTempReached = -1;
int profileStage = 0;
unsigned long timeAtGoal = 0;

// global vars used between functions in the event loop
const int NUM_STAGES = 4;
int cookTemps[NUM_STAGES];
int cookTimes[NUM_STAGES];

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
  collectButtonInfo();
}
 else   
    {
      outputData();
      controlElements();
      simTemp ();    // to be replaced with getting the temp from the thermocouple or RTD
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

void simTemp ()
{
// if (element1) currentTemp += 2;
// else currentTemp -= 1;
 currentTemp1 = analogRead(sensorPin);  
}

void outputData()
{
//  Serial.print(millis());
//  Serial.print(",");
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
  Serial.println(timeAtGoal);  
}

void profile1()
{
   Serial.println("profile1: MEMS mics");
// Preheat 160C
//Dwell 120s
//Ramp Up 1C/s for 98s
//Dwell  30s
//Ramp Down 1C/s for 98s
//Ramp Off
cookTemps[0] = 160;
cookTimes[0] = 10;

cookTemps[1] = 220;
cookTimes[1] = 10;

cookTemps[2] = 160;
cookTimes[2] = 10;

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

void collectButtonInfo()
{
  bool lastButton = false;
  
  bool buttonState = false;
  int timeButtonPressed = -1;

  buttonState = digitalRead(BUTTONPIN);

  while (!profileAcknowledged)
  {
    if (buttonState == HIGH) 
  {
    if (lastButton != buttonState) 
    {
       Serial.println("B-on");
       timeButtonPressed = millis();
    }
  } 
  else 
  {
    if (lastButton != buttonState) 
    {
      if (millis() - timeButtonPressed < 3000)
      {
       profileChosen++;
       profileChosen %= NUM_PROFILES;
       blinkOutProfileChosen();
       Serial.print("Selected profile: ");
       Serial.println(profileChosen);
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

}

void blinkOutProfileChosen()
{
     for (int i = 1; i <= profileChosen; i++)
     {
     digitalWrite(LEDPIN, HIGH);
     delay (1000);
     
     digitalWrite(LEDPIN, LOW);
     delay (500);
     }
     digitalWrite(LEDPIN, LOW);
}

void blinkOutAck()
{
     for (int i = 1; i <= 10; i++)
     {
     digitalWrite(LEDPIN, HIGH);
     delay (100);
     
     digitalWrite(LEDPIN, LOW);
     delay (100);
     }
}



void reactToTemp ()
{

 if ( currentTemp1 > cookTemps[profileStage] )
 {
  if (element1 && timeTempReached < 1 ) 
  {
    timeTempReached = millis();
  }
  else
  {
    timeAtGoal = (millis() - timeTempReached);
  }
   element1 = false;
 }
 else element1 = true;
 
}

void advancePhase ()
{
 if (profileStage >= NUM_STAGES) 
 { 
   Serial.println("Resetting");
   profileStage = 0;
   profileChosen = -1;
   profileAcknowledged = false;
 }

 else if (timeAtGoal > 1000UL*cookTimes[profileStage])
 {
  profileStage += 1;
  timeAtGoal = 0;
 }
 
}
