# arduino-morse-code
Two-way communication system using four Arduino boards and breadboards, enabling serial communication between sender and receiver pairs.

The main idea of this project is to let two people communicate to each other in morse code using two sets of two Arduino boards, where they are paired together so that one is used as a receiver and one is used as a sender. To send a message we are using three buttons, one to send a dash (‘-’), one to send a dot (‘.’), and an end of letter button to send a ‘$’ so the receiver knows the sequence is over. There will also be a running interval that resets after each button is pressed and if long enough goes by without a button press then the sender will send a ‘\n’ to signal the receiver that the sequence it just received is a word and it’s done being transmitted.

The receiving side will read the dots and dashes and store them into a buffer and once the end of letter symbol (‘$’) is received then it will translate the Morse code sequence into an alphabetical character through the use of a dictionary where each morse sequence is mapped to its corresponding alphabetical character. After translating the letter it is displayed on the bottom line of an lcd and the letter is stored in a message buffer while it waits for the end of word signal (‘\n’). Once the ‘\n’ is received then the receiver will display the message inside of the buffer of translated letters to the top line of an lcd.


![milestone 8](https://github.com/user-attachments/assets/da9277ba-6664-4da7-afde-48d413fd2b2e)
