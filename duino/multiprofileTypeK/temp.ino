#include <max6675.h> 

/*
 * Using a 6675 Thermocouple AMP with a type K bead type thermocouple. There are both 
 * bead and solid mass type K thermocouples available on Amaxon, the bead type are faster 
 * to resond to temp change and easier to put into a board hole to monitor the board temp as
 * close to the board as possible, a good idea for a reflow oven.
 */
// Set no less then 250 (1/4 second) but can be longer
#define MIN_READ_INTERVAL 250

unsigned char thermocoupleCSPin = 11;
unsigned char thermocoupleD0Pin = 12;
unsigned char thermocoupleCLKPin = 13;

MAX6675 thermocouple(thermocoupleCLKPin, thermocoupleCSPin, thermocoupleD0Pin);

// Make sure we don't read this too fast, it can only be read every 250 millis
float readSensor(void)
{
  unsigned long time_now = millis();
  static unsigned long last_time = 0;
  static float currentTemp;

  if (time_now - last_time >= MIN_READ_INTERVAL){
    currentTemp =  thermocouple.readCelsius();
    last_time = time_now;
  }
  return(currentTemp);
}

float getTemp (void)
{
  lastSampleTime = sampleTime;
  sampleTime = millis();
  lastSample = currentTemp1;
  currentTemp1 = readSensor();
  return(currentTemp1);
}
