#include <EEPROM.h>     // We are going to read and write Tag's UIDs from/to EEPROM
#include <MFRC522.h>
#include <LiquidCrystal_I2C.h>
#include <SoftwareSerial.h>
#include <Servo.h>
#include <SPI.h>
#include <Wire.h>

// Create instances
SoftwareSerial SIM900(3, 4); // SoftwareSerial SIM900(Rx, Tx)
MFRC522 mfrc522(10, 9); // MFRC522 mfrc522(SS_PIN, RST_PIN)
LiquidCrystal_I2C lcd(0x27, 16, 2); // Set the LCD address to 0x27 for a 16 chars and 2 line display
Servo myServo;  // create servo object to control a servo

// Initialize Pins for led's, servo, buzzer and button
constexpr uint8_t greenLed = 7;
constexpr uint8_t blueLed = 6;
constexpr uint8_t redLed = 5;
constexpr uint8_t ServoPin = 8;
constexpr uint8_t BuzzerPin = 2;
constexpr uint8_t wipeB = 1;     // Button pin for WipeMode

boolean match = false;          // initialize card match to false
boolean programMode = false;  // initialize programming mode to false
boolean replaceMaster = false;
boolean RFIDMode = true;

uint8_t successRead;    // Variable integer to keep if we have Successful Read from Reader

byte storedCard[4];   // Stores an ID read from EEPROM
byte readCard[4];   // Stores scanned ID read from RFID Module
byte masterCard[4];   // Stores master card's ID read from EEPROM


///////////////////////////////////////// Setup ///////////////////////////////////
void setup() {
  //Protocol Configuration
  lcd.init();  // initialize the LCD
  lcd.backlight();
  SPI.begin();           // MFRC522 Hardware uses SPI protocol
  mfrc522.PCD_Init();    // Initialize MFRC522 Hardware
  myServo.attach(ServoPin);   // attaches the servo on pin 8 to the servo object
  myServo.write(10);   // Initial Position
  // Arduino communicates with SIM900 GSM shield at a baud rate of 19200
  // Make sure that corresponds to the baud rate of your module
  SIM900.begin(19200);

  //Arduino Pin Configuration
  pinMode(redLed, OUTPUT);
  pinMode(greenLed, OUTPUT);
  pinMode(blueLed, OUTPUT);
  pinMode(BuzzerPin, OUTPUT);
  pinMode(wipeB, INPUT_PULLUP);   // Enable pin's pull up resistor

  // Make sure led's are off
  digitalWrite(redLed, LOW);
  digitalWrite(greenLed, LOW);
  digitalWrite(blueLed, LOW);

  //If you set Antenna Gain to Max it will increase reading distance
  //mfrc522.PCD_SetAntennaGain(mfrc522.RxGain_max);

  ShowReaderDetails();  // Show details of PCD - MFRC522 Card Reader details

  //Wipe Code - If the Button (wipeB) Pressed while setup run (powered on) it wipes EEPROM
  if (digitalRead(wipeB) == LOW) {  // when button pressed pin should get low, button connected to ground
    digitalWrite(redLed, HIGH); // Red Led stays on to inform user we are going to wipe

    lcd.setCursor(0, 0);
    lcd.print("RST Button Pressed");
    digitalWrite(BuzzerPin, HIGH);
    delay(1000);
    digitalWrite(BuzzerPin, LOW);

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("This will remove");
    lcd.setCursor(0, 1);
    lcd.print("all records");
    delay(2000);

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("You have 10 ");
    lcd.setCursor(0, 1);
    lcd.print("secs to Cancel");
    delay(2000);

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Unpres to cancel");
    lcd.setCursor(0, 1);
    lcd.print("Counting: ");

    bool buttonState = monitorWipeButton(10000); // Give user enough time to cancel operation
    if (buttonState == true && digitalRead(wipeB) == LOW) {    // If button still be pressed, wipe EEPROM
      lcd.clear();
      lcd.print("Wiping EEPROM...");
      for (uint16_t x = 0; x < EEPROM.length(); x = x + 1) {    //Loop end of EEPROM address
        if (EEPROM.read(x) == 0) {              //If EEPROM address 0
          // do nothing, already clear, go to the next address in order to save time and reduce writes to EEPROM
        }
        else {
          EEPROM.write(x, 0);       // if not write 0 to clear, it takes 3.3mS
        }
      }
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Wiping Done");

      // visualize a successful wipe
      digitalWrite(redLed, LOW);
      digitalWrite(BuzzerPin, HIGH);
      delay(200);
      digitalWrite(redLed, HIGH);
      digitalWrite(BuzzerPin, LOW);
      delay(200);
      digitalWrite(redLed, LOW);
      digitalWrite(BuzzerPin, HIGH);
      delay(200);
      digitalWrite(redLed, HIGH);
      digitalWrite(BuzzerPin, LOW);
      delay(200);
      digitalWrite(redLed, LOW);
    }
    else {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Wiping Cancelled"); // Show some feedback that the wipe button did not pressed for 10 seconds
      digitalWrite(redLed, LOW);
    }
  }

  // Check if master card defined, if not let user choose a master card
  // This also useful to just redefine the Master Card
  // You can keep other EEPROM records just write other than 143 to EEPROM address 1
  // EEPROM address 1 should hold magical number which is '143'
  if (EEPROM.read(1) != 143) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("No Admin Card ");
    lcd.setCursor(0, 1);
    lcd.print("Defined");
    delay(2000);

    lcd.setCursor(0, 0);
    lcd.print("Scan A ID to ");
    lcd.setCursor(0, 1);
    lcd.print("Define as Admin");

    do {
      successRead = getID();            // sets successRead to 1 when we get read from reader otherwise 0

      // Visualize Master Card need to be defined
      digitalWrite(blueLed, HIGH);
      digitalWrite(BuzzerPin, HIGH);
      delay(200);
      digitalWrite(BuzzerPin, LOW);
      digitalWrite(blueLed, LOW);
      delay(200);
    }
    while (!successRead);                  // Program will not go further while you not get a successful read
    for ( uint8_t j = 0; j < 4; j++ ) {        // Loop 4 times
      EEPROM.write( 2 + j, readCard[j] );  // Write scanned Tag's UID to EEPROM, start from address 3
    }
    EEPROM.write(1, 143);                  // Write to EEPROM we defined Master Card.
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Admin Defined");
    delay(2000);
  }

  for ( uint8_t i = 0; i < 4; i++ ) {          // Read Master Card's UID from EEPROM
    masterCard[i] = EEPROM.read(2 + i);    // Write it to masterCard
  }

  ShowOnLCD();
  cycleLeds();    // Everything ready lets give user some feedback by cycling leds

  // AT command to set SIM900 to SMS mode
  SIM900.println("AT+CMGF=1");
  delay(1000);
  // Set module to send SMS data to serial out upon receipt
  SIM900.println("AT+CNMI=2,2,0,0,0\r");
  delay(100);
}


///////////////////////////////////////// Main Loop ///////////////////////////////////
void loop () {
  // System will first check the mode
  if (RFIDMode == false) {  // If the RFID Mode is false then it will only look for master tag and messages
    // Function to receive message
    receive_message();

    // Function to get tag's UID
    getID();
    if ( isMaster(readCard) ) { // If Scanned tag is master tag
      RFIDMode = true; // Go back to RFID mode
    }
  }

  else if (RFIDMode == true) { // If RFID Mode is true then it will look for all tags and messages
    do {
      // Function to receive message
      receive_message();

      successRead = getID();  // sets successRead to 1 when we get read from reader otherwise 0

      if (programMode) {
        cycleLeds();              // Program Mode cycles through Red Green Blue waiting to read a new card
      }
      else {
        normalModeOn();     // Normal mode, blue Power LED is on, all others are off
      }
    }
    while (!successRead && RFIDMode == true);   // If we get a tag and RFID mode is true (RFID Mode will become false on getting 'close' message)
    if (programMode) {
      if ( isMaster(readCard) ) { //When in program mode check First If master card scanned again to exit program mode
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Exiting Program Mode");
        digitalWrite(BuzzerPin, HIGH);
        delay(1000);
        digitalWrite(BuzzerPin, LOW);
        ShowOnLCD();
        programMode = false;
        return;
      }

      else {
        if ( findID(readCard) ) { // If scanned card is known delete it
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("Already there");
          deleteID(readCard);

          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("ID to ADD/REM");
          lcd.setCursor(0, 1);
          lcd.print("Admin to Exit");
        }

        else {                    // If scanned card is not known add it
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("New ID,adding...");
          writeID(readCard);

          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("Scan to ADD/REM");
          lcd.setCursor(0, 1);
          lcd.print("Admin to Exit");
        }
      }
    }

    else {
      if ( isMaster(readCard)) {    // If scanned card's ID matches Master Card's ID - enter program mode
        programMode = true;

        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Program Mode");

        uint8_t count = EEPROM.read(0);   // Read the first Byte of EEPROM that stores the number of ID's in EEPROM
        lcd.setCursor(0, 1);
        lcd.print("I have ");
        lcd.print(count);
        lcd.print(" records");

        digitalWrite(BuzzerPin, HIGH);
        delay(2000);
        digitalWrite(BuzzerPin, LOW);

        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Scan a ID to ");
        lcd.setCursor(0, 1);
        lcd.print("ADD/REMOVE");
      }

      else {
        if ( findID(readCard) ) { // If not, see if the card is in the EEPROM
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("Access Granted");
          granted();         // Open the door lock
          send_message("Barrier Opened \n If it wasn't valid person, type 'close'");
          myServo.write(10);
          ShowOnLCD();
        }

        else {      // If not, show that the Access is denied
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("Access Denied");
          denied();
          send_message("One wrong ID Passed");
          ShowOnLCD();
        }
      }
    }
  }
}

/////////////////////////////////////////  Access Granted    ///////////////////////////////////
void granted () {
  digitalWrite(blueLed, LOW);   // Turn off blue LED
  digitalWrite(redLed, LOW);  // Turn off red LED
  digitalWrite(greenLed, HIGH);   // Turn on green LED
  myServo.write(90);
  delay(3000);
}

///////////////////////////////////////// Access Denied  ///////////////////////////////////
void denied() {
  digitalWrite(greenLed, LOW);  // Make sure green LED is off
  digitalWrite(blueLed, LOW);   // Make sure blue LED is off
  digitalWrite(redLed, HIGH);   // Turn on red LED
  digitalWrite(BuzzerPin, HIGH);
  delay(1000);
  digitalWrite(BuzzerPin, LOW);
}


///////////////////////////////////////// Get Tag's UID ///////////////////////////////////
uint8_t getID() {
  // Getting ready for Reading Tags
  if ( ! mfrc522.PICC_IsNewCardPresent()) { //If a new Tag placed to RFID reader continue
    return 0;
  }
  if ( ! mfrc522.PICC_ReadCardSerial()) {   //Since a Tag placed get Serial and continue
    return 0;
  }
  // There are Mifare Tags which have 4 byte or 7 byte UID care if you use 7 byte Tag
  // I think we should assume every Tag as they have 4 byte UID
  // Until we support 7 byte Tags
  for ( uint8_t i = 0; i < 4; i++) {  //
    readCard[i] = mfrc522.uid.uidByte[i];
  }
  mfrc522.PICC_HaltA(); // Stop reading
  return 1;
}

/////////////////////// Check if RFID Reader is correctly initialized or not /////////////////////
void ShowReaderDetails() {
  // Get the MFRC522 software version
  byte v = mfrc522.PCD_ReadRegister(mfrc522.VersionReg);

  // When 0x00 or 0xFF is returned, communication probably failed
  if ((v == 0x00) || (v == 0xFF)) {
    lcd.setCursor(0, 0);
    lcd.print("Communication Failure");
    lcd.setCursor(0, 1);
    lcd.print("Check Connections");
    digitalWrite(BuzzerPin, HIGH);
    delay(2000);
    // Visualize system is halted
    digitalWrite(greenLed, LOW);  // Make sure green LED is off
    digitalWrite(blueLed, LOW);   // Make sure blue LED is off
    digitalWrite(redLed, HIGH);   // Turn on red LED
    digitalWrite(BuzzerPin, LOW);
    while (true); // do not go further
  }
}

///////////////////////////////////////// Cycle Leds (Program Mode) ///////////////////////////////////
void cycleLeds() {
  digitalWrite(redLed, LOW);  // Make sure red LED is off
  digitalWrite(greenLed, HIGH);   // Make sure green LED is on
  digitalWrite(blueLed, LOW);   // Make sure blue LED is off
  delay(200);
  digitalWrite(redLed, LOW);  // Make sure red LED is off
  digitalWrite(greenLed, LOW);  // Make sure green LED is off
  digitalWrite(blueLed, HIGH);  // Make sure blue LED is on
  delay(200);
  digitalWrite(redLed, HIGH);   // Make sure red LED is on
  digitalWrite(greenLed, LOW);  // Make sure green LED is off
  digitalWrite(blueLed, LOW);   // Make sure blue LED is off
  delay(200);
}

//////////////////////////////////////// Normal Mode Led  ///////////////////////////////////
void normalModeOn () {
  digitalWrite(blueLed, HIGH);  // Blue LED ON and ready to read card
  digitalWrite(redLed, LOW);  // Make sure Red LED is off
  digitalWrite(greenLed, LOW);  // Make sure Green LED is off
}

//////////////////////////////////////// Read an ID from EEPROM //////////////////////////////
void readID( uint8_t number ) {
  uint8_t start = (number * 4 ) + 2;    // Figure out starting position
  for ( uint8_t i = 0; i < 4; i++ ) {     // Loop 4 times to get the 4 Bytes
    storedCard[i] = EEPROM.read(start + i);   // Assign values read from EEPROM to array
  }
}

///////////////////////////////////////// Add ID to EEPROM   ///////////////////////////////////
void writeID( byte a[] ) {
  if ( !findID( a ) ) {     // Before we write to the EEPROM, check to see if we have seen this card before!
    uint8_t num = EEPROM.read(0);     // Get the numer of used spaces, position 0 stores the number of ID cards
    uint8_t start = ( num * 4 ) + 6;  // Figure out where the next slot starts
    num++;                // Increment the counter by one
    EEPROM.write( 0, num );     // Write the new count to the counter
    for ( uint8_t j = 0; j < 4; j++ ) {   // Loop 4 times
      EEPROM.write( start + j, a[j] );  // Write the array values to EEPROM in the right position
    }
    BlinkLEDS(greenLed);
    lcd.setCursor(0, 1);
    lcd.print("Added");
    delay(1000);
  }
  else {
    BlinkLEDS(redLed);
    lcd.setCursor(0, 0);
    lcd.print("Failed!");
    lcd.setCursor(0, 1);
    lcd.print("wrong ID or bad EEPROM");
    delay(2000);
  }
}

///////////////////////////////////////// Remove ID from EEPROM   ///////////////////////////////////
void deleteID( byte a[] ) {
  if ( !findID( a ) ) {     // Before we delete from the EEPROM, check to see if we have this card!
    BlinkLEDS(redLed);      // If not
    lcd.setCursor(0, 0);
    lcd.print("Failed!");
    lcd.setCursor(0, 1);
    lcd.print("wrong ID or bad EEPROM");
    delay(2000);
  }
  else {
    uint8_t num = EEPROM.read(0);   // Get the numer of used spaces, position 0 stores the number of ID cards
    uint8_t slot;       // Figure out the slot number of the card
    uint8_t start;      // = ( num * 4 ) + 6; // Figure out where the next slot starts
    uint8_t looping;    // The number of times the loop repeats
    uint8_t j;
    uint8_t count = EEPROM.read(0); // Read the first Byte of EEPROM that stores number of cards
    slot = findIDSLOT( a );   // Figure out the slot number of the card to delete
    start = (slot * 4) + 2;
    looping = ((num - slot) * 4);
    num--;      // Decrement the counter by one
    EEPROM.write( 0, num );   // Write the new count to the counter
    for ( j = 0; j < looping; j++ ) {         // Loop the card shift times
      EEPROM.write( start + j, EEPROM.read(start + 4 + j));   // Shift the array values to 4 places earlier in the EEPROM
    }
    for ( uint8_t k = 0; k < 4; k++ ) {         // Shifting loop
      EEPROM.write( start + j + k, 0);
    }
    BlinkLEDS(blueLed);
    lcd.setCursor(0, 1);
    lcd.print("Removed");
    delay(1000);
  }
}

///////////////////////////////////////// Check Bytes   ///////////////////////////////////
boolean checkTwo ( byte a[], byte b[] ) {
  if ( a[0] != 0 )      // Make sure there is something in the array first
    match = true;       // Assume they match at first
  for ( uint8_t k = 0; k < 4; k++ ) {   // Loop 4 times
    if ( a[k] != b[k] )     // IF a != b then set match = false, one fails, all fail
      match = false;
  }
  if ( match ) {      // Check to see if if match is still true
    return true;      // Return true
  }
  else  {
    return false;       // Return false
  }
}

///////////////////////////////////////// Find Slot   ///////////////////////////////////
uint8_t findIDSLOT( byte find[] ) {
  uint8_t count = EEPROM.read(0);       // Read the first Byte of EEPROM that
  for ( uint8_t i = 1; i <= count; i++ ) {    // Loop once for each EEPROM entry
    readID(i);                // Read an ID from EEPROM, it is stored in storedCard[4]
    if ( checkTwo( find, storedCard ) ) {   // Check to see if the storedCard read from EEPROM
      // is the same as the find[] ID card passed
      return i;         // The slot number of the card
      break;          // Stop looking we found it
    }
  }
}

///////////////////////////////////////// Find ID From EEPROM   ///////////////////////////////////
boolean findID( byte find[] ) {
  uint8_t count = EEPROM.read(0);     // Read the first Byte of EEPROM that
  for ( uint8_t i = 1; i <= count; i++ ) {    // Loop once for each EEPROM entry
    readID(i);          // Read an ID from EEPROM, it is stored in storedCard[4]
    if ( checkTwo( find, storedCard ) ) {   // Check to see if the storedCard read from EEPROM
      return true;
      break;  // Stop looking we found it
    }
    else {    // If not, return false
    }
  }
  return false;
}


///////////////////////////////////////// Blink LED's For Indication   ///////////////////////////////////
void BlinkLEDS(int led) {
  digitalWrite(blueLed, LOW);   // Make sure blue LED is off
  digitalWrite(redLed, LOW);  // Make sure red LED is off
  digitalWrite(greenLed, LOW);  // Make sure green LED is off
  digitalWrite(BuzzerPin, HIGH);
  delay(200);
  digitalWrite(led, HIGH);  // Make sure blue LED is on
  digitalWrite(BuzzerPin, LOW);
  delay(200);
  digitalWrite(led, LOW);   // Make sure blue LED is off
  digitalWrite(BuzzerPin, HIGH);
  delay(200);
  digitalWrite(led, HIGH);  // Make sure blue LED is on
  digitalWrite(BuzzerPin, LOW);
  delay(200);
  digitalWrite(led, LOW);   // Make sure blue LED is off
  digitalWrite(BuzzerPin, HIGH);
  delay(200);
  digitalWrite(led, HIGH);  // Make sure blue LED is on
  digitalWrite(BuzzerPin, LOW);
  delay(200);
}

////////////////////// Check readCard IF is masterCard   ///////////////////////////////////
// Check to see if the ID passed is the master programing card
boolean isMaster( byte test[] ) {
  if ( checkTwo( test, masterCard ) )
    return 1;
  else
    return 0;
}

/////////////////// Counter to check in reset/wipe button is pressed or not   /////////////////////
bool monitorWipeButton(uint32_t interval) {
  unsigned long currentMillis = millis(); // grab current time

  while (millis() - currentMillis < interval)  {
    int timeSpent = (millis() - currentMillis) / 1000;
    Serial.println(timeSpent);
    lcd.setCursor(10, 1);
    lcd.print(timeSpent);

    // check on every half a second
    if (((uint32_t)millis() % 10) == 0) {
      if (digitalRead(wipeB) != LOW) {
        return false;
      }
    }
  }
  return true;
}

////////////////////// Print Info on LCD   ///////////////////////////////////
void ShowOnLCD() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(" Access Control");
  lcd.setCursor(0, 1);
  lcd.print("   Scan a ID");
}

///////////////////////////// Receiving the message//////////////////////////////
void receive_message()
{
  char incoming_char = 0; //Variable to save incoming SMS characters
  String incomingData;   // for storing incoming serial data

  if (SIM900.available() > 0)
  {
    incomingData = SIM900.readString(); // Get the incoming data.
    delay(10);
  }

  // if received command is to open the door
  if (incomingData.indexOf("open") >= 0)
  {
    myServo.write(90);
    RFIDMode = true;
    send_message("Opened");
    delay(10000);
    myServo.write(0);
  }

  // if received command is to halt the system
  if (incomingData.indexOf("close") >= 0)
  {
    RFIDMode = false;
    send_message("Closed");
  }
  incomingData = "";
}

//////////// Send the message////////////////////////
void send_message(String message)
{
  SIM900.println("AT+CMGF=1");    //Set the GSM Module in Text Mode
  delay(1000);
  SIM900.println("AT+CMGS=\"+917021360692\"\r"); // Replace it with your mobile number
  delay(1000);
  SIM900.println(message);   // The SMS text you want to send
  delay(100);
  SIM900.println((char)26);  // ASCII code of CTRL+Z
  delay(1000);
  SIM900.println();
}
