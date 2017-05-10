const String MESSAGING = "com.google.android.apps.messaging";
const String EMAIL = "com.google.android.gm";
const String CALL = "com.android.dialer";
const String MISSED_CALL = "com.android.server.telecom";
const String VOICEMAIL = "com.motorola.appdirectedsmsproxy";

#include <SoftwareSerial.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>


//Declares the OLED display
#define OLED_RESET 4
Adafruit_SSD1306 display(OLED_RESET);

//Sets up SoftwareSerial for the bluetooth module
SoftwareSerial BT(10, 11); 
// creates a "virtual" serial port/UART
// connect BT module TX to D10
// connect BT module RX to D11
// connect BT Vcc to 5V, GND to GND

// stores incoming character from other device
char a; 

//Variable to keep track of the info received via bluetooth
String package;

//Variables to set based on the received info
String recdTime;
String notifLine1;
String notifLine2;

//Declare states for the SM
enum SW_States {SW_Start, SW_s0, SW_s1} SW_State;

//Initialize flags for state transitions
boolean notifRecd = false;
boolean notifClrd = false;
boolean timeRecd = false;

//Extra flag for managing the buzzer
boolean soundAlarm = false;

//Define the SM logic
void tickFct_Watch() {

  //Check if new BT messages have been pushed
  getBluetooth();

  //How to change between the states
  switch(SW_State) { //Transitions
    case SW_Start:   //Initial Transition
      SW_State = SW_s0;
      break;

    //This is the time display state
    case SW_s0:
      //Change states if notif posted
      if (notifRecd) {
        soundAlarm = true;
        SW_State = SW_s1;
      }
      //Stay here if notif cleared
      else if (notifClrd) {
        SW_State = SW_s0;
      }
      //Stay here if new time posted
      else if (timeRecd) {
        SW_State = SW_s0;
      }
      break;

    //This is the notification display state
    case SW_s1:
      //Stay here if notif posted
      if (notifRecd) {
        soundAlarm = true;
        SW_State = SW_s1;
      }
      //Stay here if new time posted
      else if (timeRecd) {
        SW_State = SW_s1;
      }
      //Go back to time state if notif cleared
      else if (notifClrd) {
        SW_State = SW_s0;
      }
      break;
  }

  //These are the actions inside the states
  switch(SW_State) {  //State Actions
    //This is the time display state
    case SW_s0:
      //Display the time
      displayTime();
      //Reset the flags
      timeRecd = false;
      notifClrd = false;
      break;

    //This is the notification display state
    case SW_s1:
      //Display the notif
      displayNotif();
      //Reset the flag
      notifRecd = false;
      break;
  }
}

//Method to display the time based on the most recent time sent via BT
void displayTime() {
  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setCursor(0,0);
  display.setTextSize(3);
  display.print("  " + recdTime);
  display.display();
}

//Method to display notif's based on the most recent notif sent via BT
void displayNotif() {
  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setCursor(0,0);
  display.setTextSize(2);
  display.println(notifLine1);
  display.println(notifLine2);
  display.display();
  //If the soundAlarm flag is high, play the tone then set it low
  //This prevents the tone from looping infinitely
  if (soundAlarm) {
    tone(7, 1000, 1000);
    soundAlarm = false;
  }
}

//Method to receive the BT data using the HC-06
void getBluetooth() {
  //Only enter if there is something to read
    if (BT.available()) {
      //While we're receiving
      while (BT.available()) {
        delay(10);
        // if text arrived in from BT serial...
        a=(BT.read());
  
        //end of transmission
        if (a == '#') {
          break;
        }
        //Append the chars onto a string
        package += a;
      }

      //Once we've received the whole string
      if (package.length() > 0) {

     //This ladder checks which type of notification
     //has been received and what should be displayed
        if (package == MESSAGING) {
          notifRecd = true;
          notifLine1 = "   New";
          notifLine2 = "  Message!";
        }
        else if (package == EMAIL) {
          notifRecd = true;
          notifLine1 = "   New";
          notifLine2 = "  Email!";
        }
        else if (package == CALL) {
          notifRecd = true;
          notifLine1 = "  Incoming";
          notifLine2 = "   Call!";
        }
        else if (package == MISSED_CALL) {
          notifRecd = true;
          notifLine1 = "  Missed";
          notifLine2 = "   Call!";
        }
        else if (package == VOICEMAIL) {
          notifRecd = true;
          notifLine1 = "   New";
          notifLine2 = "  Voicemail!";
        }

      //If the package didn't match a notification,
      //it must have been a time value
        else {
          timeRecd = true;
          recdTime = package;
        }
        
        //Reset the package so we can receive another
        package = "";
    }
  }
}

//Setup method to activate the SoftwareSerial, display, and set the initial SM state
void setup() {
  BT.begin(9600);
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);  // initialize with the I2C addr 0x3C (for the 128x32)
  display.display();
  display.clearDisplay();
  display.display();
  //Attach an interrupt to pin 2 on the rising edge
  attachInterrupt(digitalPinToInterrupt(2), ISR_CLR, RISING);
  SW_State = SW_Start;
}

//In the loop method, just tick the state machine
void loop() {
  tickFct_Watch();
}

//ISR for the interrupt on pin 2, the piezo tap sensor
void ISR_CLR() {
  notifClrd = true;
}
