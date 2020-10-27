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

float getTemp (void)
{
  static unsigned long last_time = 0;
  unsigned long time_now = millis();

  if (time_now - last_time >= MIN_READ_INTERVAL){
    lastSampleTime = sampleTime;
    lastSample = currentTemp1;
    sampleTime = last_time = time_now;
    currentTemp1 =  thermocouple.readCelsius();
  }
    return(currentTemp1);
}
