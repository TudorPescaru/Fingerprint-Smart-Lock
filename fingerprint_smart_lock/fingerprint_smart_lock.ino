#include <Adafruit_Fingerprint.h>
#include <Servo.h>

#define RED_LED 11
#define GREEN_LED 10
#define BLUE_LED 9
#define SERVO 6
#define BUTTON 4
#define ADD 7
#define RESET 8
#define OUT_ARDU 3
#define IN_SENSOR 2

SoftwareSerial softwareSerial(IN_SENSOR, OUT_ARDU);
Adafruit_Fingerprint finger = Adafruit_Fingerprint(&softwareSerial);
Servo servo;
uint8_t nextId = 1;
boolean isEnrolled = false;
boolean doorOpen = false;
int buttonState = 0;
int lastButtonState = 0;
int addState = 0;
int lastAddState = 0;
int resetState = 0;
int lastResetState = 0;
int angle = 145;

void setup() {
  // Initialize serial
  Serial.begin(9600);

  // Set the data rate for the sensor serial port
  finger.begin(57600);

  // Check if sensor is connected
  if (finger.verifyPassword()) {
    Serial.println("Found fingerprint sensor!");
  } else {
    Serial.println("Did not find fingerprint sensor :(");
    while (1) { delay(1); }
  }

  // Get sensor parameters
  Serial.println(F("Reading sensor parameters"));
  finger.getParameters();
  Serial.print(F("Status: 0x")); Serial.println(finger.status_reg, HEX);
  Serial.print(F("Sys ID: 0x")); Serial.println(finger.system_id, HEX);
  Serial.print(F("Capacity: ")); Serial.println(finger.capacity);
  Serial.print(F("Security level: ")); Serial.println(finger.security_level);
  Serial.print(F("Device address: ")); Serial.println(finger.device_addr, HEX);
  Serial.print(F("Packet len: ")); Serial.println(finger.packet_len);
  Serial.print(F("Baud rate: ")); Serial.println(finger.baud_rate);

  // Get number of registered fingerprints
  finger.getTemplateCount();

  if (finger.templateCount == 0) {
    Serial.print("Sensor doesn't contain any fingerprint data.");
    isEnrolled = false;
  }
  else {
    Serial.print("Sensor contains "); 
    Serial.print(finger.templateCount); 
    Serial.println(" templates");
    isEnrolled = true;
  }

  // Init pins
  pinMode(RED_LED, OUTPUT);
  pinMode(GREEN_LED, OUTPUT);
  pinMode(BLUE_LED, OUTPUT);
  pinMode(BUTTON, INPUT);
  pinMode(ADD, INPUT);
  pinMode(RESET, INPUT);

  // Init servo
  servo.attach(SERVO);
  servo.write(angle);
}

void loop() {
  // Read button inputs
  readMainButton();
  readAddButton();
  readResetButton();

  // Display LED colors
  if (!isEnrolled) {
    RGB_color(255, 255, 0); // Yellow
    delay(500);
    RGB_color(0, 0, 0); // Black
    delay(500);
  } else if (!doorOpen) {
    RGB_color(255, 0, 0); // Red
  } else {
    RGB_color(0, 255, 0); // Green
  }
}

void readMainButton() {
  // read the pushbutton input pin:
  buttonState = digitalRead(BUTTON);

  // compare the buttonState to its previous state
  if (buttonState != lastButtonState) {
    // if the state has changed, increment the counter
    if (buttonState == HIGH) {
      // if the current state is HIGH then the button went from off to on:
      buttonPress();
    }
    // Delay a little bit to avoid bouncing
    delay(50);
  }
  // save the current state as the last state, for next time through the loop
  lastButtonState = buttonState;
}

void readAddButton() {
  // read the pushbutton input pin:
  addState = digitalRead(ADD);

  // compare the buttonState to its previous state
  if (addState != lastAddState) {
    // if the state has changed, increment the counter
    if (addState == HIGH) {
      // if the current state is HIGH then the button went from off to on:
      performEnroll();
    }
    // Delay a little bit to avoid bouncing
    delay(50);
  }
  // save the current state as the last state, for next time through the loop
  lastAddState = addState;
}

void readResetButton() {
  // read the pushbutton input pin:
  resetState = digitalRead(RESET);

  // compare the buttonState to its previous state
  if (resetState != lastResetState) {
    // if the state has changed, increment the counter
    if (resetState == HIGH) {
      // if the current state is HIGH then the button went from off to on:
      finger.emptyDatabase();
      isEnrolled = false;
      nextId = 1;
    }
    // Delay a little bit to avoid bouncing
    delay(50);
  }
  // save the current state as the last state, for next time through the loop
  lastResetState = resetState;
}

void buttonPress() {
  // Check which action to perform
  if (!isEnrolled) {
    performEnroll();
  } else if (!doorOpen) {
    openDoor();
  } else {
    closeDoor();
  }
}

void performEnroll() {
  Serial.println("Enrolling...");
  RGB_color(0, 0, 255); // Blue

  // Scan fingerprint
  while (!getFingerprintEnroll());
  isEnrolled = true;
  nextId++;

  RGB_color(0, 255, 0); // Green
  delay(2000);
}

void openDoor() {
  Serial.println("Verifying identity...");
  RGB_color(0, 0, 255); // Blue

  // Check fingerprint
  while (!getFingerprintID());
  doorOpen = true;
  
  RGB_color(0, 255, 0); // Green
  Serial.println("Opening door...");

  // Move servo
  for (int pos = angle; pos >= angle - 90; pos -= 1) {
    servo.write(pos);
    delay(15);
  }
  angle -= 90;
}

void closeDoor() {
  Serial.println("Closing door...");
  doorOpen = false;
  RGB_color(255, 0, 0); // Red

  // Move servo
  for (int pos = angle; pos <= angle + 90; pos += 1) {
    servo.write(pos);
    delay(15);
  }
  angle += 90;
}

uint8_t getFingerprintEnroll() {
  int p = -1;
  
  Serial.print("Waiting for valid finger to enroll as #");
  Serial.println(nextId);
  
  while (p != FINGERPRINT_OK) {
    p = finger.getImage();
    switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image taken");
      break;
    case FINGERPRINT_NOFINGER:
      Serial.println(".");
      break;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      break;
    case FINGERPRINT_IMAGEFAIL:
      Serial.println("Imaging error");
      break;
    default:
      Serial.println("Unknown error");
      break;
    }
  }

  // OK success!

  p = finger.image2Tz(1);
  switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image converted");
      break;
    case FINGERPRINT_IMAGEMESS:
      Serial.println("Image too messy");
      return false;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      return false;
    case FINGERPRINT_FEATUREFAIL:
      Serial.println("Could not find fingerprint features");
      return false;
    case FINGERPRINT_INVALIDIMAGE:
      Serial.println("Could not find fingerprint features");
      return false;
    default:
      Serial.println("Unknown error");
      return false;
  }

  Serial.println("Remove finger");
  delay(2000);
  p = 0;
  while (p != FINGERPRINT_NOFINGER) {
    p = finger.getImage();
  }
  Serial.print("ID "); Serial.println(nextId);
  p = -1;
  Serial.println("Place same finger again");
  while (p != FINGERPRINT_OK) {
    p = finger.getImage();
    switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image taken");
      break;
    case FINGERPRINT_NOFINGER:
      Serial.print(".");
      break;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      break;
    case FINGERPRINT_IMAGEFAIL:
      Serial.println("Imaging error");
      break;
    default:
      Serial.println("Unknown error");
      break;
    }
  }

  // OK success!

  p = finger.image2Tz(2);
  switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image converted");
      break;
    case FINGERPRINT_IMAGEMESS:
      Serial.println("Image too messy");
      return false;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      return false;
    case FINGERPRINT_FEATUREFAIL:
      Serial.println("Could not find fingerprint features");
      return false;
    case FINGERPRINT_INVALIDIMAGE:
      Serial.println("Could not find fingerprint features");
      return false;
    default:
      Serial.println("Unknown error");
      return false;
  }

  // OK converted!
  Serial.print("Creating model for #");  Serial.println(nextId);

  p = finger.createModel();
  if (p == FINGERPRINT_OK) {
    Serial.println("Prints matched!");
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    Serial.println("Communication error");
    return false;
  } else if (p == FINGERPRINT_ENROLLMISMATCH) {
    Serial.println("Fingerprints did not match");
    return false;
  } else {
    Serial.println("Unknown error");
    return false;
  }

  Serial.print("ID "); Serial.println(nextId);
  p = finger.storeModel(nextId);
  if (p == FINGERPRINT_OK) {
    Serial.println("Stored!");
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    Serial.println("Communication error");
    return false;
  } else if (p == FINGERPRINT_BADLOCATION) {
    Serial.println("Could not store in that location");
    return false;
  } else if (p == FINGERPRINT_FLASHERR) {
    Serial.println("Error writing to flash");
    return false;
  } else {
    Serial.println("Unknown error");
    return false;
  }

  return true;
}

uint8_t getFingerprintID() {
  uint8_t p = finger.getImage();
  switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image taken");
      break;
    case FINGERPRINT_NOFINGER:
      Serial.println("No finger detected");
      return false;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      return false;
    case FINGERPRINT_IMAGEFAIL:
      Serial.println("Imaging error");
      return false;
    default:
      Serial.println("Unknown error");
      return false;
  }

  // OK success!

  p = finger.image2Tz();
  switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image converted");
      break;
    case FINGERPRINT_IMAGEMESS:
      Serial.println("Image too messy");
      return false;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      return false;
    case FINGERPRINT_FEATUREFAIL:
      Serial.println("Could not find fingerprint features");
      return false;
    case FINGERPRINT_INVALIDIMAGE:
      Serial.println("Could not find fingerprint features");
      return false;
    default:
      Serial.println("Unknown error");
      return false;
  }

  // OK converted!
  p = finger.fingerSearch();
  if (p == FINGERPRINT_OK) {
    Serial.println("Found a print match!");
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    Serial.println("Communication error");
    return false;
  } else if (p == FINGERPRINT_NOTFOUND) {
    Serial.println("Did not find a match");
    return false;
  } else {
    Serial.println("Unknown error");
    return false;
  }

  // found a match!
  Serial.print("Found ID #"); Serial.print(finger.fingerID);
  Serial.print(" with confidence of "); Serial.println(finger.confidence);

  return true;
}

void RGB_color(int red_light_value, int green_light_value, int blue_light_value) {
  analogWrite(RED_LED, red_light_value);
  analogWrite(GREEN_LED, green_light_value);
  analogWrite(BLUE_LED, blue_light_value);
}
