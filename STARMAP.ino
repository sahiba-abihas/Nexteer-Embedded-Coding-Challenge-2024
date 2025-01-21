#include "NexteerWifiCmdInterface.h" // downloaded from STARMAP site
#include <LiquidCrystal.h> //Comes with IDE
#include <IRremote.h> //Download linked in submission, from the old 2009 version
#include <IRremoteInt.h>
#include <dht11.h> //Came with the CD ROM (downloaded the tutorials from ELEGOO website and dragged DHT library to my library folder)
#include <Servo.h> //Comes with IDE
#include <EEPROM.h>

//Pin 0 & 1 left empty for day-of competition, and digital pin 8 is open right now. All other digital & analog pins are used.

// Phase 0, ESP-01 & Communication
NexteerWifiCmdInterface wifi(2, 3); // RX -> 3, TX -> 2! The other way doesn't work, refer to UART section in specs
//Global ip&mac adress so can read in setup and print on LCD loop once push button has been pressed 
String ipAdress;
String macAdress;

//IR Remote:
int receiver = 7; // Signal Pin of IR receiver module
IRrecv irrecv(receiver);     // create instance of 'irrecv'
decode_results results;      // create instance of 'decode_results'
String id = "";

// incase IR Remote doesn't work, ID_input is for the serial monitor
String ID_input = "0000";

//Phase 1 Temp & Humidity Sensor:
#define dataPin A0
dht11 DHT11;

//push button + debounce
const int buttonPin = 4; // arduino digital pin number
int buttonState;            // the current reading from the input pin
int lastButtonState = LOW;  // the previous reading from the input pin
unsigned long lastDebounceTime = 0;  // the last time the output pin was toggled
unsigned long debounceDelay = 50;    // the debounce time; increase if the output flickers
float buttonCounter = 0.0; 
//float because had to increase by 0.5 instead of 1 because each press was counted a two state changes

//LCD screen
const int rs = 6, en = 5, d4 = 10, d5 = 11, d6 = 12, d7 = 13;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);
bool printedPh0 = false;

// Comma parsing (defining globally because used multiple times in void loop and want to create an instance here once)
int firstComma;
int secondComma;


//Phase 2: Servo for angle
bool servoAtt = false; //To track if the servo has been attached or not to the arduino pin
//I experiences the servo keep re-initializing and moving randomly after setup prior to it's use in phase 2
// To combat this, I only attach when I need it in phase 2 so the IR receiver module doesn't get noise
Servo myservo;
int servoPos = 5;

//Phase 3: Joystick:
const int X_pin = A2; // analog pin connected to X output
const int Y_pin = A1; // analog pin connected to Y output
bool directionsPrinted = false; //To only print directions once on LCD instead of in a loop
bool p3InputSent = false;
String directions = "";
String p3Answer = ""; //To store the answer to submit (ex: 512+1023?)
int joySteps = 0; //To figure out when to stop waiting for joystick inputs based on direction string length


//Phase 4:
String guess = "123456";
String feedback = "000000";
bool possible_digits[6][10];
bool digit_used[10];

//Phase 5: LED light-up Using Shift Register: 74HC595
#define dataPin A3
#define latchPin A4  
#define clockPin A5 
byte leds = 0;
String code = "";
int addr = 0;

// Logic for Simulation to prevent looping forever in void loop()
bool started = false; 
bool goToPhase1 = false;
bool phase1Done = false;
bool goToPhase2 = false;
bool phase2Done = false;
//For example, I run through the code in phase 3 multiple times UNTIL the joystick values are done
//which striggers phase3Done to be true
bool goToPhase3 = false;
bool phase3Done = false;
bool goToPhase4 = false;
bool phase4Done = false;
bool goToPhase5 = false;
bool phase5Done = false;
bool done = false;

// Function for phase 5 (Shift Register)
void updateShiftRegister(byte num) {
  digitalWrite(latchPin, LOW);  // drop latch pin to GND
  shiftOut(dataPin, clockPin, LSBFIRST, num); // Write data
  digitalWrite(latchPin, HIGH); // Push data to outputs
}

// Function for IR Remote Values
void translateIR() // takes action based on IR code received
{
  switch(results.value) {
  case 0xFFA25D: Serial.println("POWER"); id = ""; Serial.println("id reset!"); break;
  case 0xFFE21D: Serial.println("FUNC/STOP"); break;
  case 0xFF629D: Serial.println("VOL+"); break;
  case 0xFF22DD: Serial.println("FAST BACK");    break;
  case 0xFF02FD: Serial.println("PAUSE");    break;
  case 0xFFC23D: Serial.println("FAST FORWARD");   break;
  case 0xFFE01F: Serial.println("DOWN");    break;
  case 0xFFA857: Serial.println("VOL-");    break;
  case 0xFF906F: Serial.println("UP");    break;
  case 0xFF9867: Serial.println("EQ");    break;
  case 0xFFB04F: Serial.println("ST/REPT");    break;
  case 0xFF6897: Serial.println("0");  id+="0";  break;
  case 0xFF30CF: Serial.println("1");  id+="1";  break;
  case 0xFF18E7: Serial.println("2"); id+="2";   break;
  case 0xFF7A85: Serial.println("3"); id+="3";   break;
  case 0xFF10EF: Serial.println("4"); id+="4";   break;
  case 0xFF38C7: Serial.println("5"); id+="5";   break;
  case 0xFF5AA5: Serial.println("6");  id+="6";  break;
  case 0xFF42BD: Serial.println("7"); id+="7";   break;
  case 0xFF4AB5: Serial.println("8"); id+="8";   break;
  case 0xFF52AD: Serial.println("9"); id+="9";   break;
  case 0xFFFFFFFF: Serial.println(" REPEAT");    break;  

  default: 
    Serial.println(" other button : ");
    Serial.println(results.value);

  } // End Case
  delay(500); // Do not get immediate repeat
}

void update_possible_digits() {
  //Guess is a String with 6 letters
    for (int i = 0; i < 6; i++) {
        int digit = guess.charAt(i) - '0'; // 'char' -> digit (ex: ASCII of '3' - ASCII of '0' = 3)
        if (feedback.charAt(i) == 'X') { // X -> not in number, not a possible digit in any location
            for (int j = 0; j < 6; j++) {
                possible_digits[j][digit] = false; //digit used in index of 2D array which needs to be integer
            }
        } else if (feedback.charAt(i) == 'Z') { // Z -> in number in right spot
            for (int j = 0; j < 6; j++) {
                if (j != i) {
                    possible_digits[j][digit] = false; // Thus, make the possible index for this digit false in OTHER locations
                }
            }
            digit_used[digit] = true; // We have now used this digit, will NOT try again in future turns
        } else if (feedback.charAt(i) == 'Y') {  // Y -> In number in wrong index
            possible_digits[i][digit] = false;   // flag this specific index as wrong for this digit
        }
    }
}

void generate_next_guess() {
    bool new_digit_used[10]; // Copy digit_used[] into new_digit_used to make new guesses 
    //only want to switch digit)used when feedback is 'Z' but this new_digit_used is temporary and only for the current guess
    // to not use repeat digits like '223456' in the same guess
    for (int i = 0; i < 10; i++) {
        new_digit_used[i] = digit_used[i];
    }

    for (int i = 0; i < 6; i++) {
        if (feedback.charAt(i) != 'Z') { // Only change for locations not 'Z' ('X' or 'Y')
            for (int digit = 1; digit <= 9; digit++) { 
                if (!new_digit_used[digit] && possible_digits[i][digit]) { 
                    guess.setCharAt(i, '0' + digit); // Make sure it's in String guess as the 'char' value not int
                    new_digit_used[digit] = true;
                    break;
                }
            }
        }
    }
}

void setup() {
  Serial.begin(115200);
  delay(100);
  //Serial.setTimeout(10);

  // Found best to clear both lCD before & after beginning (sometimes random numbers & characters appeared on start)
  lcd.clear();
  lcd.begin(16, 2);
  lcd.clear();
  lcd.print("Sahiba 2025");

  // On wifi (ESP) commands, find best to use small delays both before & after, otherwise the String instance stores the previous response
  delay(100);
  wifi.Setup();
  delay(1000);

  // Finish initalizing other phase requirements before sending wifi commands
  irrecv.enableIRIn(); //IR remote
  pinMode(buttonPin, INPUT); // push button


  // For Phase 4: Initialize possible_digits array and digit_used array
  for (int i = 0; i < 6; i++) {
    for (int j = 0; j < 10; j++) {
        possible_digits[i][j] = true; 
    }
  }

  for (int i = 0; i < 10; i++) {
    digit_used[i] = false;
  }

  // For phase 5: Needed Shift Register because ran out of pins
  pinMode(dataPin, OUTPUT);
  pinMode(latchPin, OUTPUT);
  pinMode(clockPin, OUTPUT);
  leds = 0;
  updateShiftRegister(leds);


  //Connect ESP-01 to my home wifi
  String connectingWifi; 
  connectingWifi.reserve(80);
  wifi.SendCommand("NXTR10:NXTRVISITOR_|_", connectingWifi); 
  delay(1000);

  //String instaces are declared globably above to print on LCD in loop()
  ipAdress.reserve(80);
  wifi.SendCommand("NXTR13", ipAdress);
  delay(1000);

  macAdress.reserve(80);
  wifi.SendCommand("NXTR14", macAdress);


  delay(1000);
  //printing on Serial for debugging purposes
  Serial.println(ipAdress.c_str());
  Serial.println(macAdress.c_str());
  p3Answer.reserve(100);
}

void loop() {
  if(!done) {
    //Debounce required because with just simple digitalRead(buttonPin) logic, one press can be read many times
    // A simple delay works too (as used later in phase 3 instead of debounce)
    // during phase 0, I wanted to set up this debounce to practice and incase needed later (like for mystery phase)
    int reading = digitalRead(buttonPin);
    if(reading!= lastButtonState) {
      lastDebounceTime= millis();
    }
    if((millis()-lastDebounceTime)>debounceDelay) {
      if(reading!=buttonState) {
        buttonState=reading;
        buttonCounter+=0.5; //Each button state was causing a button press to increase by '2' 
      }
    }
    lastButtonState = reading;

    if(buttonCounter == 1.0) { // equally fine to try buttonCounter == 2 and buttonCounter += 1 earlier
    //The 'float' idea workes fine here, and allows for better tracking on the Serial Monitor & for later phases
      started = true;
    }
    if(buttonCounter == 2.0) {
      goToPhase5 = true;
    }

    if(started) {  // Only start once the button is pressed
      if (!printedPh0) { // if(!prentedPh0) -> if (not printed on the LCD screen in phase 0)
        //print graduation date
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("ipAdress");
        lcd.setCursor(0, 1);
        lcd.print(1969);
        printedPh0 = true; // This just prevents me from overwriting my lcd screen many times
      }

        // For Serial Monitor inputs
      while (Serial.available() > 0) { 
        ID_input = Serial.readString();
      }
      if(ID_input.toInt() == 3981) {
        goToPhase1 = true;
      }

        // For IR remote inputs. Sometimes, noise can come in and cause a number to be misread, so use 'Power' as a reset button
      if (irrecv.decode(&results)){ // have we received an IR signal?
        translateIR(); 
        irrecv.resume(); // receive the next value
      }  
      if(id == "3981") {

        goToPhase1 = true;
      }
      if(buttonCounter == 3.0) {
        Serial.println(id);
        Serial.println(buttonCounter);
        String p5Output;
        wifi.SendCommand("NXTR30:starmap.generationnextcoding.com_|_80_|_/3981/a5/" + id, p5Output);
        delay(1000);
        done = true;
        }      
    }
      
    
    if(goToPhase1 && !phase1Done) {
      delay(100);
      String p1Input;
      wifi.SendCommand("NXTR30:starmap.generationnextcoding.com_|_80_|_/3981/p1/", p1Input);
      delay(1000);
      Serial.println(p1Input);
      
    
      // DHT data for when at nexteer:
      
      int t = 20; // naturally in degrees celcius. 
      int h = 49; 

      // Parse server response (no guanrantee which numbers will be +/- day of competition, so use commas to distuinguish next value)
      firstComma = p1Input.indexOf(',');
      secondComma = p1Input.indexOf(',', firstComma + 1);

      // // Extract the substrings and convert them to integers
      int tempOffset = p1Input.substring(2, firstComma).toInt();
      int humidityOffset = p1Input.substring(firstComma + 1, secondComma).toInt();
      int scale = p1Input.substring(secondComma + 1).toInt();
      Serial.println(tempOffset);
      Serial.println(humidityOffset);
      Serial.println(scale);

      if(scale == 1) {
        t = 1.8*t+32;
      } else if(scale == 2) {
        t += 273.15;
      }

      t += tempOffset;
      h += humidityOffset;

      //for debugging:
      Serial.println(t);
      Serial.println(h);

      String ending = "" + String(t) + "," + String(h);
      Serial.println(ending);
      String p1Output;
      delay(100);
      wifi.SendCommand("NXTR30:starmap.generationnextcoding.com_|_80_|_/3981/a1/" + ending, p1Output);
      delay(1000);
      code+=p1Output.substring(2);
      Serial.println(code);
      goToPhase2 = true;
      phase1Done = true;

    }

    if(goToPhase2 && !phase2Done) {
      String p2Input;
      wifi.SendCommand("NXTR30:starmap.generationnextcoding.com_|_80_|_/3981/p2/", p2Input);
      delay(1000);
      Serial.println(p2Input);
      firstComma = p2Input.indexOf(',');
      secondComma = p2Input.indexOf(',', firstComma + 1);

      // // Extract the substrings and convert them to integers
      int encryptedAngle = p2Input.substring(2, firstComma).toInt();
      int encryptionKey = p2Input.substring(firstComma + 1, secondComma).toInt();
      int numRounds = p2Input.substring(secondComma + 1).toInt();
      int encryptionRange = 181;

      Serial.println(encryptedAngle);
      Serial.println(encryptionKey);
      Serial.println(numRounds);
      Serial.println(encryptionRange);

      Serial.println("decrypting...");
      
      // Decrypting... (following figure 4 visual)
      for(int i=0; i<numRounds; i++) {
        encryptedAngle = (encryptedAngle + encryptionRange - encryptionKey) % encryptionRange;
      }

          
      Serial.println("Decrypted Angle: " + encryptedAngle);

      String offset;
      if(encryptedAngle == 0) {
        offset = "0";
      } else if(encryptedAngle == 30) {
        offset = "1";
      } else if(encryptedAngle == 60) {
        offset = "2";
      } else if(encryptedAngle == 90) {
        offset = "3";
      } else if(encryptedAngle == 120) {
        offset = "4";
      } else if(encryptedAngle == 150) {
        offset = "5";
      } else if(encryptedAngle == 180) {
        offset = "6";
      }

      Serial.println("offset: " + offset);
      String p2Output;
      delay(1000);
      wifi.SendCommand("NXTR30:starmap.generationnextcoding.com_|_80_|_/3981/a2/" + offset, p2Output);
      delay(100);
      code+=p2Output.substring(2,4);
      Serial.println(offset);
      Serial.println(encryptedAngle);
      Serial.println(code);

      myservo.attach(9);
      for(servoPos = 5; servoPos <= encryptedAngle; servoPos += 1) { // goes from 0 degrees to 180 degrees
        myservo.write(servoPos);              // tell servo to go to position in variable 'pos'
        delay(15);                       // waits 15ms for the servo to reach the position
      }
      myservo.detach();



      goToPhase3 = true;
      phase2Done = true;
      delay(5000);
    }
    

    if(goToPhase3 && !phase3Done) {
      
      if(!p3InputSent) {
        String p3Input;
        wifi.SendCommand("NXTR30:starmap.generationnextcoding.com_|_80_|_/3981/p3/", p3Input);
        delay(100);
        p3InputSent = true;
        Serial.println(directions);
      }

      // Change String directions to match the up/down/left/right corresponding to number input
      for(int i =0; i<directions.length(); i++) {
        if(directions[i]=='0') directions[i] = 'U';
        else if(directions[i]=='1') directions[i] = 'D';
        else if(directions[i]=='2') directions[i] = 'L';
        else if(directions[i]=='3') directions[i] = 'R';
      }

      if(!directionsPrinted) { //Once again, preventing from overloading LCD screen
        lcd.clear();
        lcd.print(directions);
        directionsPrinted = true;
      }



      //use the push button on breadboard to log the joystick's values
      if (digitalRead(buttonPin) == HIGH) { //Check if the pushbutton is pressed
        int xValue = analogRead(X_pin); // Read the X value of the joystick
        int yValue = analogRead(Y_pin); // Read the Y value of the joystick

        /* Note: 
        I noticed the STARMAP tolerance was within this range anyways, so to account for any 
        //lower tolerance on the day-of competition, I'm offsetting my 'X' and 'Y' joy values
        since I know sometimes I get values within these ranges for the 512 end of X/Y, the 1023 end is fine 
        */
        
        if(yValue < 520 && yValue > 488) {
          yValue = 512;
        } 
        if(xValue < 520 && xValue > 488) {
          xValue = 512;
        } 
        
        //p3Answer += String(xValue) + "+" + String(yValue) + "?"; //String manipulation to input phase 3 answer to servers
        String joyinput = String(xValue) + "+" + String(yValue) + "?";
        p3Answer+=joyinput;
        Serial.println(p3Answer);

        delay(1000); // Debounce delay (here, delay worked just fine for me)
        
        joySteps++; // increment how many times I have inputted a joystick value
      }

      if(joySteps == directions.length()) { //Stop collecting joystick inputs once String direction is fully read (otherwise void loop() would make this keep going)
        delay(1000);
        Serial.println("we're done with joystick inputs");
        Serial.println("removing the ?");
        delay(500);
        String p3out;
        p3out.reserve(100);
        p3out = p3Answer.substring(0, p3Answer.length() - 1);
      
        Serial.println("");
        Serial.println(p3out);
        delay(1000);



        wifi.SendCommand("NXTR30:starmap.generationnextcoding.com_|_80_|_/3981/a3/" + p3out, p3out);
        delay(1000);
        code+=p3out.substring(2);
        Serial.println(code);
        buttonCounter = 1.0; //Resetting back to 1
        phase3Done = true;
        goToPhase4 = true;
        
        
      }
      
    }

    if(goToPhase4 && !phase4Done) {
      Serial.print("Current Guess: ");
      Serial.println(guess);


      // Example feedback string from server (replace with actual server communication)
      delay(1000);
      String p4Response;
      wifi.SendCommand("NXTR30:starmap.generationnextcoding.com_|_80_|_/3981/a4/" + guess, p4Response);
      delay(100);
      feedback = p4Response.substring(7);

      // Print feedback
      Serial.print("Feedback: ");
      Serial.println(feedback);

      // Check if the guess is correct or not
      bool correct = true;
      for (int i = 0; i < 6; i++) {
        if (feedback.charAt(i) != 'Z') {
          correct = false;
          break;
        }
      }

      if(correct) {
        phase4Done = true;
        Serial.println("code: " + p4Response.substring(0,1));
        code+=p4Response.substring(2,4);
        delay(1000);

        Serial.println(code);



      } else {
        update_possible_digits(); //based on feedback
        generate_next_guess();
        delay(1000);
      }
    }

    if(goToPhase5 && !phase5Done) {
      code = "24216231";
      // To start, Red & 3 Yellow LEDs on
      bitSet(leds, 6);
      bitSet(leds, 1);
      bitSet(leds, 2);
      bitSet(leds, 3);
      updateShiftRegister(leds);
      delay(2000);

      // Process each digit in the 8-digit code string using a for loop
      for (int i = 0; i < code.length(); i++) {
        leds = 0;
        bitSet(leds, 4);
        updateShiftRegister(leds);
        delay(500); // Adjust delay as needed for alignment
        char digit = code[i];

        // Turn on yellow LEDs based on the current digit
        // bits 0, 1, 2 in specs wired from right to left as directed, and respectivelly are connected to bits/pins 1, 2, 3 of Shift Register
        switch (digit) { //Representing digit in 'binary' with 1 = on, 0 = off
          case '1':
            bitSet(leds, 1); // 1 -> 001
            break;
          case '2':
            bitSet(leds, 2); // 2 -> 010
            break;
          case '3':
            bitSet(leds, 1); // 3 -> 011
            bitSet(leds, 2); 
            break;
          case '4':
            bitSet(leds, 3); // 4 -> 100
            break;
          case '5':
            bitSet(leds, 1); // 5 -> 101
            bitSet(leds, 3); // 
            break;
          case '6':
            bitSet(leds, 2); // 6 -> 110
            bitSet(leds, 3); 
            break;
          case 'F':
            bitSet(leds, 1); // 7 -> 111
            bitSet(leds, 2); 
            bitSet(leds, 3); 
            break;
          default:
            break;
        }
        updateShiftRegister(leds);
        delay(2000);

        // Switch blue LED (4) to green (5)
        bitClear(leds, 4); // Turn off blue LED
        bitSet(leds, 5); // Turn on green LED
        updateShiftRegister(leds);
        delay(2000); // Can adjust delay as needed for green LED confirmation
      }

      // Finished processing all digits in the code
      
      leds=0;
      updateShiftRegister(leds);
      phase5Done = true;
      done=true;


    }

  }

}


