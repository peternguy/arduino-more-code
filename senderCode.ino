/*
name, netid:
William Nguyen, wnguy4
Peter Nguyen, pnguy9
Owen Neal, oneal7
Nguyen Duc Minh Luong, nluong3

project name:
Morse Code Project

abstract:
We will have two sets of arduino boards, two senders and two receivers, sending morse code messages between each pair. 
The morse code message will be sent through the use of three buttons, one for dashes, dots, and an end of letter symbol ($). 
The receiver will look up the alphabet translation from a morse code sequence after a '$' is read. After the message is decoded 
and an end of word symbol is received ('\n'), then the message is displayed to the top of an LCD screen for five seconds, along 
with other methods of displaying the message to each pair.
*/

#include <SoftwareSerial.h>

//button pins
const int DOT_BUTTON_PIN = 2;
const int DASH_BUTTON_PIN = 3;
const int END_LETTER_BUTTON_PIN = 4;

//photoresistor variables
const int PHOTORESISTOR_PIN = A1;
bool is_ph_covered = false;
unsigned long ph_cover_start_time = 0;
bool ph_sent = false;
const int light_amount_threshold = 200;
const unsigned long dot_max_duration = 800;
const unsigned long dash_min_duration = 800;

const int POTENTIOMETER_PIN = A0;

//led pins
const int MORSE_LED_PIN = 5;
const int END_LETTER_LED_PIN = 6;
const int INACTIVITY_LED_PIN = 7;

//variables for keeping led on for a set amount of time
unsigned long led_duration = 500;
unsigned long dot_dash_led_time = 0;
unsigned long end_letter_led_time = 0;
unsigned long inactivity_led_time = 0;
bool dot_dash_led_active = false;
bool end_letter_led_active = false;
bool inactivity_led_active = false;

//debounce variables for each button
bool last_dot_button_state = false;
bool last_dash_button_state = false;
bool last_end_letter_button_state = false;
unsigned long last_dot_debounce_time = 0;
unsigned long last_dash_debounce_time = 0;
unsigned long last_end_letter_debounce_time = 0;
const unsigned long debounce_delay = 25; //debounce time -> can be adjusted if needed

bool dot_pressed = false;
bool dash_pressed = false;
bool end_letter_pressed = false;

unsigned long last_button_press_time = 0; //tracks last button press
unsigned long inactivity_timeout = 10000; //20 seconds for inactivity change to smaller number if want to late less inbetween words
bool inactivity_signal_sent = false; //keeps track if we've sent the "\n"

SoftwareSerial softSerial(0, 1); //for serial communication

void setup() {
  pinMode(DOT_BUTTON_PIN, INPUT);
  pinMode(DASH_BUTTON_PIN, INPUT);
  pinMode(END_LETTER_BUTTON_PIN, INPUT);

  pinMode(MORSE_LED_PIN, OUTPUT);
  pinMode(END_LETTER_LED_PIN, OUTPUT);
  pinMode(INACTIVITY_LED_PIN, OUTPUT);

  softSerial.begin(9600);
  Serial.begin(9600); //debug messages
}

void loop() {
  unsigned long current_time = millis();

  //turns off the current led if it is on after 2 seconds
  //resets the active led to no active led
  handleLEDDuration(current_time);

  adjustTimeoutInterval(); //changes timeout interval dynamically with the potentiometer

  //handle the three buttons
  processButton(DOT_BUTTON_PIN, last_dot_button_state, last_dot_debounce_time, dot_pressed, '.', current_time, MORSE_LED_PIN);
  processButton(DASH_BUTTON_PIN, last_dash_button_state, last_dash_debounce_time, dash_pressed, '-', current_time, MORSE_LED_PIN);
  processButton(END_LETTER_BUTTON_PIN, last_end_letter_button_state, last_end_letter_debounce_time, end_letter_pressed, '$', current_time, END_LETTER_LED_PIN);
  handlePhotoresistor(current_time);
  //check for inactivity and send "\n" if needed
  handleInactivity(current_time);

}

void handleLEDDuration(unsigned long current_time) {
  if (dot_dash_led_active && (current_time - dot_dash_led_time >= led_duration)) {
    digitalWrite(MORSE_LED_PIN, LOW);
    dot_dash_led_active = false;
  }

  if (end_letter_led_active && (current_time - end_letter_led_time >= led_duration)) {
    digitalWrite(END_LETTER_LED_PIN, LOW);
    end_letter_led_active = false;
  }

  if (inactivity_led_active && (current_time - inactivity_led_time >= led_duration)) {
    digitalWrite(INACTIVITY_LED_PIN, LOW);
    inactivity_led_active = false;
  }
}

//function to turn on an led given a pin number
//sets the active_led global variable to the given pin number
void turnOnLED(int pin) {
  if (pin == MORSE_LED_PIN) {
    dot_dash_led_active = true;
    dot_dash_led_time = millis();
  } else if (pin == END_LETTER_LED_PIN) {
    end_letter_led_active = true;
    end_letter_led_time = millis();
  } else if (pin == INACTIVITY_LED_PIN) {
    inactivity_led_active = true;
    inactivity_led_time = millis();
  }

  digitalWrite(pin, HIGH);
}

//one function for all the buttons, just changing parameters because they all do the same thing
void processButton(int button_pin, bool &last_state, unsigned long &debounce_time, bool &pressed, char signal, unsigned long current_time, int led_pin) {
  int buttonState = digitalRead(button_pin);

  //simple debounce
  if (buttonState != last_state) {
    debounce_time = current_time;
  }

  if ((current_time - debounce_time) > debounce_delay) {
    if (buttonState == HIGH && !pressed) {
      pressed = true;
      softSerial.write(signal); //send the character
      Serial.print(signal);
      Serial.println(" sent");
      last_button_press_time = current_time; //reset inactivity timer
      inactivity_signal_sent = false; //reset inactivity flag
      turnOnLED(led_pin);
      //sendMorseSymbol(signal, led_pin);
    } else if (buttonState == LOW) {
      pressed = false;
    }
  }

  last_state = buttonState;
}

void sendMorseSymbol(char signal, int led_pin) {
  softSerial.write(signal);
  Serial.print(signal);
  Serial.println(" sent");
  last_button_press_time = millis();
  inactivity_signal_sent = false;
  turnOnLED(led_pin);
}


//after a user set amount of time we send a newline character to indicate to the reciever that is the end of the current word
void handleInactivity(unsigned long current_time) {
  if ((current_time - last_button_press_time > inactivity_timeout) && !inactivity_signal_sent) {
    softSerial.write('\n'); //wend newline character
    Serial.println("Newline sent (timeout)");
    inactivity_signal_sent = true; //mark that we've sent the signal
    turnOnLED(INACTIVITY_LED_PIN);
  }
}

//allows the user to adjust the interval for inactivity and sending the end of word symbol dynamically between 5 and 60 seconds
void adjustTimeoutInterval() {
  static unsigned long last_timeout = inactivity_timeout; //store the last set timeout
  int potentiometer_val = analogRead(POTENTIOMETER_PIN); //read the potentiometer
  unsigned long new_timeout_interval = map(potentiometer_val, 0, 1023, 5000, 60000); //map to 5-60 seconds

  //only want to update if the difference is significant (e.g., more than 1 second)
  if (abs((long)new_timeout_interval - (long)last_timeout) > 1000) {
    inactivity_timeout = new_timeout_interval;
    last_timeout = new_timeout_interval;
    Serial.print("Inactivity timeout updated to: ");
    Serial.print(inactivity_timeout/1000);
    Serial.println(" seconds");
  }
}

void handlePhotoresistor(unsigned long current_time) {
  int light_val = analogRead(PHOTORESISTOR_PIN);

  if (light_val < light_amount_threshold) {  // Light is covered
    if (!is_ph_covered) {
      // Mark the light as covered and start timing
      is_ph_covered = true;
      ph_cover_start_time = current_time;
      ph_sent = false;  // Reset the sent flag
    }
  } else {  // Light is not covered
    if (is_ph_covered) {
      // Mark the light as uncovered and calculate the duration
      is_ph_covered = false;
      unsigned long duration = current_time - ph_cover_start_time;

      // Only send the Morse symbol once if it's not already sent
      if (!ph_sent) {
        if (duration < dot_max_duration) {
          softSerial.write('.'); //send the character
          Serial.print('.');
          Serial.println(" sent");
          last_button_press_time = current_time; //reset inactivity timer
          inactivity_signal_sent = false; //reset inactivity flag
          turnOnLED(MORSE_LED_PIN);
        } else if (duration >= dash_min_duration) {
          softSerial.write('-'); //send the character
          Serial.print('-');
          Serial.println(" sent");
          last_button_press_time = current_time; //reset inactivity timer
          inactivity_signal_sent = false; //reset inactivity flag
          turnOnLED(MORSE_LED_PIN);
        }
        ph_sent = true;  //mark as sent to avoid sending again
      }
    }
  }
}


