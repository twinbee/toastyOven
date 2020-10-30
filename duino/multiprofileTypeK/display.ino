#include "ssd1306.h"
#include "nano_gfx.h"

// Use short version of serial data out or long.
// Long provides a lot of info about the PID system
#define SERIAL_DATA_SHORT 1

#if SERIAL_DATA_SHORT
/*
 * Don't want to swamp the serial output, so display 1 per second. Can be adjusted 
 * by changing the define for PERIOD 1K is 1 second timing, 500 2x a second 2K is 
 * 2 second timing.
 */
#define PERIOD 1000

void outputSerialData(void)
{
  static unsigned long last_time = 0;
  static byte displayHeader = 0;
  unsigned long time_now = millis();

  if (!displayHeader){
    Serial.println("Seconds,\tSet,\tTemp,\tPid,\t\tDuty,\t\tName");
    displayHeader = 1;
  }
  
  if (time_now - last_time >= PERIOD){
    last_time =time_now;
    Serial.print(((millis()-totalRunTime)/1000));
    Serial.print(",\t\t");
    Serial.print(profiles[profileChosen].setPoints[profileStage]);
    Serial.print(",\t");
    Serial.print(currentTemp1);
    Serial.print(",\t");
    Serial.print(pidOut,8);
    Serial.print(",\t");
    if (duty == 0){
      Serial.print("00000000");
    }else{
      Serial.print(duty, BIN); // print in base 2
    }
    Serial.print(",\t");
    Serial.println(MessagesReflowStatus[profileStage+1]);
  }  
}
#elif
/*
 * Not really sure why all this info is needed but the gent that wrote the oven 
 * control was out putting this info, I assume it made it easier to tune the oven.
 * If you need this info, flip the if SERIAL_DATA_SHORT to 0 and this will compile in.
 */
void outputSerialData(void)
{
  static byte displayHeader = 0;

  if (!displayHeader){
    Serial.println("Seconds, Temp, El1, El2, TempPoint, Duration, time@Goal, Profile in use, stage, duty, error, deriv, pidOut, Over? ");
    displayHeader = 1;
  }
  
  Serial.print(((millis()-totalRunTime)/1000));
  Serial.print(",");
  Serial.print(currentTemp1);
  Serial.print(",On?");
  Serial.print(element1);
  Serial.print(":");
  Serial.print(element2);
  Serial.print(",");
  Serial.print(profiles[profileChosen].setPoints[profileStage]);
  Serial.print(",");
  Serial.print(profiles[profileChosen].setPointDurations[profileStage]);  
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
  if ( currentTemp1 > (profiles[profileChosen].setPoints[profileStage] + tempEpsilon) )
  {
   Serial.print(",!OVER!");
  }

  Serial.println ();
}
#endif

/*
 * The OLED hardware for the below display is based on "0.96 Inch 
 * OLED Module 12864 128x64 Yellow Blue SSD1306 Driver I2C Serial 
 * Self-Luminous Display Board" the Module Size: 27mmx 27mm x 4mm.
 * 
 * You could use one the same size that use the SPI interfaces but 
 * you would have to set up SPI wiring and share it with the thermocouple,
 * doable but why bother.
 * 
 * Lots of different ones sold on Amazon, just pick one.  They even 
 * have differnt color schemes, all white, all blue.  In short volume (4-5)
 * around ~$5 USD in cost. Single unit cost a bit higher ~$7 USD. 
 * 
 * Attaching one to an Arduino NANO, I2C is on the Analog pins A4 (SCL) 
 * & A5 (SDA) not digital pins.
 * 
 */

// Needed for the OLED have to convert floats to char for display
char temp[20];

void outputOLEDSplash(void)
{
  // Start-up splash
  ssd1306_setFixedFont(ssd1306xled_font8x16);
  ssd1306_128x64_i2c_init();
  ssd1306_clearScreen();
  ssd1306_printFixed(0,  4, "Designed by THEM", STYLE_NORMAL);
  ssd1306_printFixed(0, 22, "Reflow Oven", STYLE_NORMAL);
  ssd1306_printFixed(0, 36, "V0.2", STYLE_NORMAL);
  ssd1306_printFixed(0, 50, "10-26-20", STYLE_NORMAL);
  delay(2000);
  ssd1306_clearScreen();
}

void outputOLEDStatus(void)
{
  // Clear OLED
  //ssd1306_clearScreen();

  // Print current system state
  ssd1306_printFixed(0, 4, "Profile:", STYLE_BOLD);
  ssd1306_printFixed(70, 4, profiles[profileChosen].name, STYLE_NORMAL);

  ssd1306_printFixed(0, 20, "State:", STYLE_BOLD);
  if (!profileAcknowledged)
  {
    ssd1306_printFixed(70, 20, MessagesReflowStatus[profileStage], STYLE_NORMAL);
  }
  else
  {
    ssd1306_printFixed(70, 20, MessagesReflowStatus[profileStage+1], STYLE_NORMAL);
  }
  ssd1306_printFixed(0, 37, "Temp:", STYLE_BOLD);
  dtostrf(currentTemp1, 5, 1, temp);
  ssd1306_printFixed(70, 37, temp, STYLE_NORMAL);
  ssd1306_printFixed(112, 37, "C", STYLE_NORMAL);
}
