/*
Profile 1: heat a board for 60 seconds with preheat
  modified 8 May 2014
  by Scott Fitzgerald
 */
const int BOTTOMELEMENT = 12;
const int TOPELEMENT = 13;

int currentTemp = 30;
bool element1 = false;
int goalTemp = 500;

int timeAtGoal = 0;
int cookTime = 6000;

//pot used for simulating temperature
int sensorPin = A0; 

// the setup function runs once when you press reset or power the board
void setup() {
  Serial.begin(9600);

  pinMode(BOTTOMELEMENT, OUTPUT);

  pinMode(TOPELEMENT, OUTPUT);
  
  
}

// the loop function runs over and over again forevero
void loop() {
  Serial.print(millis());
  Serial.print(",");
  Serial.print(element1);
  Serial.print(",");
  Serial.println(currentTemp);
    
  if (element1 == true)   
  { 
    digitalWrite(TOPELEMENT, HIGH);
  }
  else
  {
   digitalWrite(TOPELEMENT, LOW);
  }      
  
  simTemp ();    
  reactToTemp();
      
  delay(100);       
}

void simTemp ()
{
// if (element1) currentTemp += 2;
// else currentTemp -= 1;
 
 currentTemp = analogRead(sensorPin);  
}

void reactToTemp ()
{
  if (timeAtGoal > cookTime)
  {
     digitalWrite(TOPELEMENT, LOW);
    while (true)
   {
      delay(1000); 
   }
  }
  
 if ( currentTemp > goalTemp )
 {
  element1 = false;
  timeAtGoal++;
 }
 else element1 = true;
}

