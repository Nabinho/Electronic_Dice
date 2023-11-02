/***********************************************************************************************************************
 *
 * Written by Nabinho - 2023
 *
 *  Call reseedRandom once in setup to start random on a new sequence.  Uses 
 *  four bytes of EEPROM.
 *
 *  Reference: https://forum.arduino.cc/t/the-reliable-but-not-very-sexy-way-to-seed-random/65872/53
 *
 *  The reseedRandom raw value can be initialized allowing different 
 *  applications or instances to have different random sequences.
 * 
 *  https://www.random.org/cgi-bin/randbyte?nbytes=4&format=h
 *  https://www.fourmilab.ch/cgi-bin/Hotbits?nbytes=4&fmt=c&npass=1&lpass=8&pwtype=3
 *
 **********************************************************************************************************************/

// Project libraries
#include <Wire.h>
#include <lcdgfx.h>
#include <avr/eeprom.h>
#include <lcdgfx_gui.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_MPU6050.h>

// MPU6050 object
Adafruit_MPU6050 MPU6050OBJ;

// OLED display object
DisplaySH1106_128x64_I2C display(-1);

// -----------------------------------------------------------------------------------------------
// EEPROM random seed function
void reseedRandom(uint32_t* address) {
  static const uint32_t HappyPrime = 127807 /*937*/;
  uint32_t raw;
  unsigned long seed;

  // Read the previous raw value from EEPROM
  raw = eeprom_read_dword(address);

  // Loop until a seed within the valid range is found
  do {
    // Incrementing by a prime (except 2) every possible raw value is visited
    raw += HappyPrime;

    // Park-Miller is only 31 bits so ignore the most significant bit
    seed = raw & 0x7FFFFFFF;
  } while ((seed < 1) || (seed > 2147483646));

  // Seed the random number generator with the next value in the sequence
  srandom(seed);

  // Save the new raw value for next time
  eeprom_write_dword(address, raw);
}
uint32_t reseedRandomSeed EEMEM = 0xFFFFFFFF;
// -----------------------------------------------------------------------------------------------

// Dices type variables
const uint8_t all_dices = 14;
uint8_t dices[all_dices][2] = { { 3, 0 },
                                { 4, 0 },
                                { 5, 0 },
                                { 6, 0 },
                                { 8, 0 },
                                { 10, 0 },
                                { 10, 0 },
                                { 12, 0 },
                                { 16, 0 },
                                { 20, 0 },
                                { 24, 0 },
                                { 48, 0 },
                                { 100, 0 },
                                { 120, 0 } };
int results = 0;
long sum_results = 0;
int last_sum_results = 0;
int single_roll = 0;

// Display on timeout variables
unsigned long last_update = 0;
int TIMEOUT = 30000;

// Buzzer variables
const uint8_t PIN_BUZZER = 7;
const int FREQUENCY = 100;

// Touch sensor pin
const uint8_t PIN_TOUCH = A3;

// Encoder variables
const uint8_t PinB = 2;
const uint8_t PinA = 3;
const uint8_t SW = 4;
bool configuration = true;
bool select_dice = true;
bool number_dice = false;

// Updated by the ISR
volatile int virtual_position = 0;
int last_virtual_position = 0;

// -----------------------------------------------------------------------------------------------
// Encoder reading function
void isr() {
  static unsigned long lastInterruptTime = 0;
  unsigned long interruptTime = millis();

  // If interrupts come faster than 5ms, assume it's a bounce and ignore
  if (interruptTime - lastInterruptTime > 5) {
    if (digitalRead(PinB) == LOW) {
      virtual_position--;
      if (select_dice) {
        if (virtual_position < 0) {
          virtual_position = all_dices - 1;
        }
      }
      if (number_dice) {
        if (virtual_position < 0) {
          virtual_position = 10;
        }
      }
    } else {
      virtual_position++;
      if (select_dice) {
        if (virtual_position > all_dices - 1) {
          virtual_position = 0;
        }
      }
      if (number_dice) {
        if (virtual_position > 10) {
          virtual_position = 0;
        }
      }
    }

    // Keep track of when we were here last (no more than every 5ms)
    lastInterruptTime = interruptTime;
  }
}

// -----------------------------------------------------------------------------------------------
// Code setup
void setup(void) {

  // Serial monitor initialization for debuging
  Serial.begin(9600);
  Serial.println("Starting...");

  // LED built-in initialization
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);

  // Touch sensor initialization
  pinMode(PIN_TOUCH, INPUT);

  // Buzzer initialization
  pinMode(PIN_BUZZER, OUTPUT);
  noTone(PIN_BUZZER);

  // Rotary encoder initialization
  pinMode(PinA, INPUT);
  pinMode(PinB, INPUT);
  pinMode(SW, INPUT);

  // Attach the routine to service the interrupts
  attachInterrupt(digitalPinToInterrupt(PinA), isr, LOW);

  // Example that reseeds the random number generator each time the application starts
  // Uses EEMEM to determine the EEPROM address
  // Most common use
  reseedRandom(&reseedRandomSeed);

  // Setupt motion detection
  Serial.println("Starting MPU6050...");
  MPU6050OBJ.begin();
  MPU6050OBJ.setHighPassFilter(MPU6050_HIGHPASS_0_63_HZ);
  MPU6050OBJ.setMotionDetectionThreshold(1);
  MPU6050OBJ.setMotionDetectionDuration(20);
  MPU6050OBJ.setInterruptPinLatch(true);  // Keep it latched.  Will turn off when reinitialized.
  MPU6050OBJ.setInterruptPinPolarity(true);
  MPU6050OBJ.setMotionInterrupt(true);
  Serial.println("Done.");
  delay(100);

  // OLED initialization
  Serial.println("Starting OLED...");
  display.begin();
  display.fill(0x00);
  display.clear();
  Serial.println("Done.");
  delay(100);

}

// -----------------------------------------------------------------------------------------------
// Code repetition
void loop(void) {

  if (!configuration) {  // Runs dice rolling routine

    // If rotary encoder button is pressed enters configuration mode
    if (digitalRead(SW) == LOW) {
      delay(500);
      if (digitalRead(SW) == LOW) {
        configuration = true;
        select_dice = true;
        number_dice = false;
        tone(PIN_BUZZER, 1000);
        delay(50);
        tone(PIN_BUZZER, 750);
        delay(50);
        tone(PIN_BUZZER, 500);
        delay(50);
        noTone(PIN_BUZZER);
        while (digitalRead(SW) == LOW)
          ;
        virtual_position = 0;
      }
    }

    // Check if touched capacitive sensor
    else if (digitalRead(PIN_TOUCH) == HIGH) {
      delay(100);
      if (digitalRead(PIN_TOUCH) == HIGH) {
        display.clear();

        // Single roll D20
        single_roll = random(1, 21);
        Serial.print("Result single D20: ");
        Serial.println(single_roll);
        Serial.print("\n");

        // Beeps Buzzer
        tone(PIN_BUZZER, FREQUENCY);
        delay(50);
        noTone(PIN_BUZZER);

        display.setFixedFont(ssd1306xled_font6x8);
        display.printFixed(0, 0, "Result single D20:", STYLE_NORMAL);
        display.setFixedFont(ssd1306xled_font8x16);
        display.printFixed(0, 15, "*", STYLE_NORMAL);
        display.setTextCursor(12, 8);
        display.print(single_roll);

        display.setFixedFont(ssd1306xled_font6x8);
        display.printFixed(0, 35, "Result all dices:", STYLE_NORMAL);
        display.setFixedFont(ssd1306xled_font8x16);
        display.printFixed(0, 45, "*", STYLE_NORMAL);
        display.setTextCursor(12, 40);
        display.print(last_sum_results);

        // Does nothing until sensor is released
        while (digitalRead(PIN_TOUCH) == HIGH)
          ;

        // Updates last dice update
        last_update = millis();
      }
    }

    // Wait for motion
    if (MPU6050OBJ.getMotionInterruptStatus()) {
      display.clear();
      digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));

      // Generate random dice rolls
      for (uint8_t i = 0; i < all_dices; i++) {
        Serial.print("D");
        Serial.print(dices[i][0]);
        for (uint8_t j = 0; j < dices[i][1]; j++) {
          if (i == 6) {
            results = random(1, dices[i][0] + 1);
            results = results * 10;
          } else {
            results = random(1, dices[i][0] + 1);
          }

          Serial.print(" : ");
          Serial.print(results);
          sum_results += results;
          delay(5);
        }
        Serial.print("\n");
        // Beeps Buzzer
        tone(PIN_BUZZER, FREQUENCY);
        delay(10);
        noTone(PIN_BUZZER);
      }

      // Saves last result
      last_sum_results = sum_results;

      // Prints results
      Serial.print("Results all dices: ");
      Serial.println(last_sum_results);
      Serial.print("\n");

      display.setFixedFont(ssd1306xled_font6x8);
      display.printFixed(0, 0, "Result single D20:", STYLE_NORMAL);
      display.setFixedFont(ssd1306xled_font8x16);
      display.printFixed(0, 15, "*", STYLE_NORMAL);
      display.setTextCursor(12, 8);
      display.print(single_roll);

      display.setFixedFont(ssd1306xled_font6x8);
      display.printFixed(0, 35, "Result all dices:", STYLE_NORMAL);
      display.setFixedFont(ssd1306xled_font8x16);
      display.printFixed(0, 45, "*", STYLE_NORMAL);
      display.setTextCursor(12, 40);
      display.print(last_sum_results);

      // Resets sum of results
      sum_results = 0;

      // Updates last dice update
      last_update = millis();
    }

    // Checks if display timed out to keep it clear
    if ((millis() - last_update) > TIMEOUT) {
      display.clear();
    }
    delay(100);

  } else {  // If in configuration mode
    display.clear();

    // Checks if rotary encoder button is pressed to change configuration being made
    if (digitalRead(SW) == LOW) {
      delay(100);
      if (digitalRead(SW) == LOW) {
        if (select_dice) {
          last_virtual_position = virtual_position;
          virtual_position = dices[last_virtual_position][1];
        }
        if (number_dice) {
          dices[last_virtual_position][1] = virtual_position;
          virtual_position = last_virtual_position;
        }
        select_dice = !select_dice;
        number_dice = !select_dice;
        tone(PIN_BUZZER, FREQUENCY);
        delay(10);
        noTone(PIN_BUZZER);
        while (digitalRead(SW) == LOW)
          ;
      }
    }

    // If touch sensor is pressed exit configuration mode
    else if (digitalRead(PIN_TOUCH) == HIGH) {
      delay(500);
      if (digitalRead(PIN_TOUCH) == HIGH) {
        configuration = false;
        select_dice = false;
        number_dice = false;
        tone(PIN_BUZZER, 500);
        delay(50);
        tone(PIN_BUZZER, 750);
        delay(50);
        tone(PIN_BUZZER, 1000);
        delay(50);
        noTone(PIN_BUZZER);
        while (digitalRead(PIN_TOUCH) == HIGH)
          ;
      }
    }

    // If selecting dices
    if (select_dice) {

      display.setFixedFont(ssd1306xled_font6x8);
      display.printFixed(0, 0, "Configuring dice:", STYLE_NORMAL);
      display.setFixedFont(ssd1306xled_font8x16);
      display.printFixed(0, 15, "D", STYLE_NORMAL);
      display.setTextCursor(12, 8);
      display.print(dices[virtual_position][0]);
      if (virtual_position == 6) {
        display.printFixed(32, 15, "%", STYLE_NORMAL);
      }
      display.setFixedFont(ssd1306xled_font6x8);
      display.printFixed(0, 35, "Number of dices:", STYLE_NORMAL);
      display.setFixedFont(ssd1306xled_font8x16);
      display.printFixed(0, 45, "*", STYLE_NORMAL);
      display.setTextCursor(12, 40);
      display.print(dices[virtual_position][1]);
    }

    // If selecting number os dices
    if (number_dice) {

      display.setFixedFont(ssd1306xled_font6x8);
      display.printFixed(0, 0, "Configuring dice:", STYLE_NORMAL);
      display.setFixedFont(ssd1306xled_font8x16);
      display.printFixed(0, 15, "D", STYLE_NORMAL);
      display.setTextCursor(12, 8);
      display.print(dices[last_virtual_position][0]);
      if (last_virtual_position == 6) {
        display.printFixed(32, 15, "%", STYLE_NORMAL);
      }
      display.setFixedFont(ssd1306xled_font6x8);
      display.printFixed(0, 35, "Number of dices:", STYLE_NORMAL);
      display.setFixedFont(ssd1306xled_font8x16);
      display.printFixed(0, 45, "*", STYLE_NORMAL);
      display.setTextCursor(12, 40);
      display.print(virtual_position);
    }
    delay(250);
  }
}
// -----------------------------------------------------------------------------------------------
