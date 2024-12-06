#include <SoftwareSerial.h>
#include <LiquidCrystal_I2C.h>
#include <map>
#include <SevSeg.h>

LiquidCrystal_I2C lcd(0x27, 16, 2);
SevSeg sevseg;
//morse code translation dictionary
std::map<String, char> morse_dict = {
  {".-", 'A'}, {"-...", 'B'}, {"-.-.", 'C'}, {"-..", 'D'}, {".", 'E'},
  {"..-.", 'F'}, {"--.", 'G'}, {"....", 'H'}, {"..", 'I'}, {".---", 'J'},
  {"-.-", 'K'}, {".-..", 'L'}, {"--", 'M'}, {"-.", 'N'}, {"---", 'O'},
  {".--.", 'P'}, {"--.-", 'Q'}, {".-.", 'R'}, {"...", 'S'}, {"-", 'T'},
  {"..-", 'U'}, {"...-", 'V'}, {".--", 'W'}, {"-..-", 'X'}, {"-.--", 'Y'},
  {"--..", 'Z'}, {".----", '1'}, {"..---", '2'}, {"...--", '3'}, {"....", '4'},
  {".....", '5'}, {"-....", '6'}, {"--...", '7'}, {"---..", '8'}, {"----.", '9'},
  {"-----", '0'}
};


const int buffer_size = 64;
char letter_buffer[buffer_size];
char message_buffer[buffer_size];
int letter_index = 0;
int message_index = 0;

SoftwareSerial softSerial(0, 1); // RX, TX

// timer variables for showing complete message
unsigned long display_start_time = 0;
bool displaying_message = false;

//alarm System Variables
const int SPEAKER_PIN = 12;

//buzzer variables
const int BUZZER_PIN = 13;
bool buzzer_active = false;
unsigned long buzzer_start_time = 0;
const int tone_duration = 200;

bool alarm_active = false;
bool alarm_state = false; //current state of the alarm (ON/OFF)
unsigned long last_alarm_toggle_time = 0;
const unsigned long alarm_interval = 500; //interval for alarm toggling in ms

unsigned long alarm_start_time = 0; //time when alarm was activated
const unsigned long alarm_duration = 5000; //alarm duration in milliseconds (5 seconds)

const String distress_signals[] = {"SOS", "HELP", "FIRE", "EMERG", "DANGER", "MEDIC", "911"};
const int distress_signals_count = sizeof(distress_signals) / sizeof(distress_signals[0]);

void setup() {
  softSerial.begin(9600);
  Serial.begin(9600); // Debugging
  lcd.init();
  lcd.backlight();
  lcd.begin(16, 2);
  lcd.print("Waiting...");

  byte numDigits = 1; //single digit for the 7-segment display
  byte digitPins[] = {}; //no digit control pins, single digit
  byte segmentPins[] = {6, 5, 2, 3, 4, 7, 8, 9}; //pins for each segment
  bool resistorsOnSegments = true; //resistors are on the segment lines
  byte hardwareConfig = COMMON_CATHODE;

  sevseg.begin(hardwareConfig, numDigits, digitPins, segmentPins, resistorsOnSegments);
  sevseg.setBrightness(90);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(SPEAKER_PIN, OUTPUT);
  digitalWrite(SPEAKER_PIN, LOW);
}

void loop() {
  //process incoming characters
  if (softSerial.available()) {
    char received = softSerial.read();
    processReceivedChar(received);
  }
  sevseg.refreshDisplay();
  //makes sure to keep full message on lcd for 5 sec
  handleDisplayTimeout();
  handleBuzzer();
  //handles the alarm if it is active
  handleAlarm();
}

void processReceivedChar(char received) {
  if (received == '$') {
    handleEndOfLetter();
  } else if (received == '\n') {
    handleEndOfMessage();
  } else if (received == '.' || received == '-') {
    appendToLetterBuffer(received);
    playBuzzerSound();
  }
}

void playBuzzerSound() {
  buzzer_active = true;
  buzzer_start_time = millis();
  digitalWrite(BUZZER_PIN, HIGH);
}

void handleBuzzer() {
  if (buzzer_active && (millis() - buzzer_start_time >= tone_duration)) {
    digitalWrite(BUZZER_PIN, LOW);
    buzzer_active = false;
  }
}

//end of letter signal is '$' -> translates the letter in the letter buffer to an english character and add
//to the message buffer, also clear letter buffer
void handleEndOfLetter() {
  char letter = translateMorseToChar(letter_buffer);
  if (letter != '?') {
    addToMessageBuffer(letter);
    displayLastLetter(letter);
  }
  clearBuffer(letter_buffer, letter_index);
}

//calls the function to show complete message to the lcd, starts a 5 second timer and tells other function
//handleDisplayTimeout that the message is being displayed
void handleEndOfMessage() {
  displayMessage();
  display_start_time = millis(); //start the 5-second display timer -> works in tandem with handleDisplayTimeout
  displaying_message = true;    //indicate message is being displayed

  // Check if the message is a distress signal
  String message_str = String(message_buffer);
  message_str.toUpperCase(); // Makes sure comparison is case-insensitive

  for (int i = 0; i < distress_signals_count; i++) {
    if (message_str.indexOf(distress_signals[i]) != -1) {
      activateAlarm(); // Activate alarm if distress signal is found
      break;
    }
  }
}

//clear lcd and reset message buffer after 5 seconds
void handleDisplayTimeout() {
  // if (displaying_message && (millis() - display_start_time >= 5000)) {
  //   lcd.clear();
  //   lcd.print("Waiting...");
  //   displaying_message = false; // Reset display state
  //   clearBuffer(message_buffer, message_index); // Clear the message buffer
  // }
    static int count = 0;
    static unsigned long lastUpdate = 0;

    if (displaying_message) {
        if (count <= 5 && (millis() - lastUpdate >= 1000)) {
            sevseg.setNumber(count); //display the current number
            sevseg.refreshDisplay(); 
            lastUpdate = millis(); //update the last update time
            count++; 
        }

        if (count > 5) {
            lcd.clear();
            lcd.print("Waiting...");
            displaying_message = false; //reset display state
            clearBuffer(message_buffer, message_index); //clear the message buffer
            count = 0; //reset the counter
        }
    }
}

//adds the morse code character just recieved to the buffer for morse code in current word interval
void appendToLetterBuffer(char received) {
  if (letter_index < buffer_size - 1) {
    letter_buffer[letter_index++] = received;
    letter_buffer[letter_index] = '\0';
  }
}

//adds a translated letter to the buffer of the complete message that will be dispayed to the top of the lcd
void addToMessageBuffer(char letter) {
  if (message_index < buffer_size - 1) {
    message_buffer[message_index++] = letter;
    message_buffer[message_index] = '\0';
  }
}

//shows reciever the last letter they recieved
void displayLastLetter(char letter) {
  lcd.setCursor(0, 1);
  lcd.print(letter);
  Serial.print("Letter received: ");
  Serial.println(letter);
}

//displays the complete message recieved before the '\n'
void displayMessage() {
  lcd.clear();
  lcd.print(message_buffer);
  Serial.print("Message received: ");
  Serial.println(message_buffer);
}

//uses the defined dictionary to get the alphabet translation of the morse code
char translateMorseToChar(const char* morse) {
  String morse_str = String(morse);
  if (morse_dict.count(morse_str)) {
    return morse_dict[morse_str];
  }
  return '?'; // Unknown Morse code
}

void clearBuffer(char* buffer, int& index) {
  memset(buffer, 0, buffer_size);
  index = 0;
}

//alarm activation function
//similar logic to keeping the message on the lcd screen
void activateAlarm() {
  if (!alarm_active) {
    alarm_active = true;
    alarm_state = false; //start with speaker off
    last_alarm_toggle_time = millis(); //initialize toggle timer
    alarm_start_time = millis(); //initialize alarm start timer
    Serial.println("Distress signal detected! Alarm activated.");
  }
}

//alarm Handling Function - switches the speaker on and off to produce multiple sounds
//like an alarm
void handleAlarm() {
  if (alarm_active) {
    unsigned long current_time = millis();

    //check if alarm duration has been reached
    if (current_time - alarm_start_time >= alarm_duration) {
      deactivateAlarm(); //deactivate alarm after duration
      return; //exit the function to prevent further processing
    }

    //toggle alarm state at specified intervals
    if (current_time - last_alarm_toggle_time >= alarm_interval) {
      last_alarm_toggle_time = current_time;
      alarm_state = !alarm_state;

      if (alarm_state) {
        digitalWrite(SPEAKER_PIN, HIGH);
      } else {
        digitalWrite(SPEAKER_PIN, LOW); 
      }
    }
  }
}

void deactivateAlarm() {
  alarm_active = false;
  alarm_state = false;
  digitalWrite(SPEAKER_PIN, LOW);
  Serial.println("Alarm deactivated after 5 seconds.");
}
