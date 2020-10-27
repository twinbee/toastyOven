

/*
 * The design of this button input system is to catch any button press no matter what the
 * oven is doing, thus the ISR routines to watch the button presses on pins 2 and 3.  It will
 * only allow a single button to be pressed and lock out both until the press is read and cleared
 * this could be changed to allowing both buttons to be pressed but it would require seperate 
 * buttonPressTimes to debounce each button from when it was pressed and or'ing the responses 
 * into the buttonPressed var.  A little more complex but doable, just not needed for the oven 
 * control app.
 */

#define DEBOUNCE_BUTTON_TIME 300  // Test and adjust to not get more then 1 response per switch press

volatile byte buttonPressed = 0;
volatile byte buttonTimerStarted = 0;
volatile byte buttonWait;
unsigned long buttonPressTime;

void initButtons(void){
  pinMode (2, INPUT_PULLUP);
  pinMode (3, INPUT_PULLUP);
  buttonPressed=255;
  buttonWait=255;
  buttonPressTime = millis();
  attachInterrupt (digitalPinToInterrupt (2), button1ISR, FALLING);
  delay(1);
  attachInterrupt (digitalPinToInterrupt (3), button2ISR, FALLING);
  delay(1);
  buttonPressed=0;  
}

void button1ISR (void) {
  if (!buttonWait && !buttonPressed){// If a key was pressed or if buttonWait set, exit doing nothing 
    buttonPressed = buttonWait = 1;// Disable the interupt, and assign the button
    buttonTimerStarted = 0;
  }
}

void button2ISR (void) {
  if (!buttonWait && !buttonPressed){
    buttonPressed = buttonWait = 2;
    buttonTimerStarted = 0;
  }
}

byte switchJustPressed(void) {
  byte temp = 0;
  if (buttonPressed) {
    if (!buttonTimerStarted){
      buttonPressTime = millis();
      buttonTimerStarted = 1;
    }
    temp = buttonPressed;
    buttonPressed = 0;
  }
  debounceButton();
  return temp;
}

void debounceButton (void){
  int debounce = DEBOUNCE_BUTTON_TIME;// Time to wait before reenabling interupt routines

  if (buttonWait) {
    if (!buttonTimerStarted){
      buttonPressTime = millis();
      buttonTimerStarted = 1;
      return;
    }
    if (buttonPressTime +  (unsigned long) debounce > millis()){
      return;
    }
    buttonWait = 0;
    buttonPressed=0;
  }
  return;
}
