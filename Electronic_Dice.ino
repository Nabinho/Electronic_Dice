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
const uint8_t all_dices = 8;
uint8_t dices[all_dices][2] = { { 4, 0 },
                                { 6, 0 },
                                { 8, 0 },
                                { 10, 0 },
                                { 10, 0 },
                                { 12, 0 },
                                { 20, 0 },
                                { 100, 0 } };
int results = 0;
int sum_results = 0;
int last_sum_results = 0;
int single_roll = 0;

// Display on timeout variables
unsigned long last_update = 0;
int TIMEOUT = 10000;

// Buzzer variables
const uint8_t PIN_BUZZER = 7;
const int FREQUENCY = 250;

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
          virtual_position = 7;
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
        if (virtual_position > 7) {
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

// Notes frequency for startup animation
#define NOTE_B0 31
#define NOTE_C1 33
#define NOTE_CS1 35
#define NOTE_D1 37
#define NOTE_DS1 39
#define NOTE_E1 41
#define NOTE_F1 44
#define NOTE_FS1 46
#define NOTE_G1 49
#define NOTE_GS1 52
#define NOTE_A1 55
#define NOTE_AS1 58
#define NOTE_B1 62
#define NOTE_C2 65
#define NOTE_CS2 69
#define NOTE_D2 73
#define NOTE_DS2 78
#define NOTE_E2 82
#define NOTE_F2 87
#define NOTE_FS2 93
#define NOTE_G2 98
#define NOTE_GS2 104
#define NOTE_A2 110
#define NOTE_AS2 117
#define NOTE_B2 123
#define NOTE_C3 131
#define NOTE_CS3 139
#define NOTE_D3 147
#define NOTE_DS3 156
#define NOTE_E3 165
#define NOTE_F3 175
#define NOTE_FS3 185
#define NOTE_G3 196
#define NOTE_GS3 208
#define NOTE_A3 220
#define NOTE_AS3 233
#define NOTE_B3 247
#define NOTE_C4 262
#define NOTE_CS4 277
#define NOTE_D4 294
#define NOTE_DS4 311
#define NOTE_E4 330
#define NOTE_F4 349
#define NOTE_FS4 370
#define NOTE_G4 392
#define NOTE_GS4 415
#define NOTE_A4 440
#define NOTE_AS4 466
#define NOTE_B4 494
#define NOTE_C5 523
#define NOTE_CS5 554
#define NOTE_D5 587
#define NOTE_DS5 622
#define NOTE_E5 659
#define NOTE_F5 698
#define NOTE_FS5 740
#define NOTE_G5 784
#define NOTE_GS5 831
#define NOTE_A5 880
#define NOTE_AS5 932
#define NOTE_B5 988
#define NOTE_C6 1047
#define NOTE_CS6 1109
#define NOTE_D6 1175
#define NOTE_DS6 1245
#define NOTE_E6 1319
#define NOTE_F6 1397
#define NOTE_FS6 1480
#define NOTE_G6 1568
#define NOTE_GS6 1661
#define NOTE_A6 1760
#define NOTE_AS6 1865
#define NOTE_B6 1976
#define NOTE_C7 2093
#define NOTE_CS7 2217
#define NOTE_D7 2349
#define NOTE_DS7 2489
#define NOTE_E7 2637
#define NOTE_F7 2794
#define NOTE_FS7 2960
#define NOTE_G7 3136
#define NOTE_GS7 3322
#define NOTE_A7 3520
#define NOTE_AS7 3729
#define NOTE_B7 3951
#define NOTE_C8 4186
#define NOTE_CS8 4435
#define NOTE_D8 4699
#define NOTE_DS8 4978
#define REST 0

// Change this to make the song slower or faster
int timing = 120;

// notes of the moledy followed by the duration.
// a 4 means a quarter note, 8 an eighteenth , 16 sixteenth, so on
// !!negative numbers are used to represent dotted notes,
// so -4 means a dotted quarter note, that is, a quarter plus an eighteenth!!
int melody[] = {

  // Dart Vader theme (Imperial March) - Star wars
  // Score available at https://musescore.com/user/202909/scores/1141521
  // The tenor saxophone part was used

  NOTE_A5,
  4,
  NOTE_A4,
  -8,
  NOTE_A4,
  16,
  NOTE_A5,
  4,
  NOTE_GS5,
  -8,
  NOTE_G5,
  16,  //7
  NOTE_DS5,
  16,
  NOTE_D5,
  16,
  NOTE_DS5,
  8,
  REST,
  8,
  NOTE_A4,
  8,
  NOTE_DS5,
  4,
  NOTE_D5,
  -8,
  NOTE_CS5,
  16,

  NOTE_C5,
  16,
  NOTE_B4,
  16,
  NOTE_C5,
  16,
  REST,
  8,
  NOTE_F4,
  8,
  NOTE_GS4,
  4,
  NOTE_F4,
  -8,
  NOTE_A4,
  -16,  //9
  NOTE_C5,
  4,
  NOTE_A4,
  -8,
  NOTE_C5,
  16,
  NOTE_E5,
  2,

};

// sizeof gives the number of bytes, each int value is composed of two bytes (16 bits)
// there are two values per note (pitch and duration), so for each note there are four bytes
int notes = sizeof(melody) / sizeof(melody[0]) / 2;

// this calculates the duration of a whole note in ms
int wholenote = (60000 * 4) / timing;
int divider = 0;
int noteDuration = 0;

// Bitmap for startup animation
const uint8_t Soba[] PROGMEM = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x80,
  0x09, 0x00, 0x00, 0x00, 0xc0, 0x0c, 0x00, 0x00,
  0x00, 0x60, 0x06, 0x00, 0x00, 0x00, 0x30, 0x03,
  0x00, 0x00, 0x00, 0x98, 0x01, 0x00, 0xf8, 0x00,
  0xcc, 0x00, 0x00, 0xde, 0x03, 0x66, 0x00, 0x80,
  0x07, 0x0f, 0x33, 0x00, 0xc0, 0x01, 0x9c, 0x19,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xe0, 0xff,
  0xff, 0xff, 0x07, 0xe0, 0xff, 0xff, 0xff, 0x07,
  0xe0, 0xff, 0xff, 0xff, 0x07, 0xe0, 0xff, 0xff,
  0xff, 0x07, 0xe0, 0xff, 0xff, 0xff, 0x07, 0xc0,
  0xff, 0xff, 0xff, 0x03, 0xc0, 0xff, 0xff, 0xff,
  0x03, 0x80, 0xff, 0xff, 0xff, 0x01, 0x80, 0xff,
  0xff, 0xff, 0x01, 0x00, 0xff, 0xff, 0xff, 0x00,
  0x00, 0xfe, 0xff, 0x7f, 0x00, 0x00, 0xfc, 0xff,
  0x3f, 0x00, 0x00, 0xf8, 0xff, 0x1f, 0x00, 0x00,
  0xf0, 0xff, 0x0f, 0x00, 0x00, 0xc0, 0xff, 0x03,
  0x00, 0x00, 0x80, 0xff, 0x01, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

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

  // =========================================================================
  // Startup animation
  display.drawBitmap1(0, 0, 128, 64, Soba);

  // iterate over the notes of the melody.
  // Remember, the array is twice the number of notes (notes + durations)
  for (int thisNote = 0; thisNote < notes * 2; thisNote = thisNote + 2) {

    // calculates the duration of each note
    divider = melody[thisNote + 1];

    if (divider > 0) {
      // regular note, just proceed
      noteDuration = (wholenote) / divider;
    } else if (divider < 0) {
      // dotted notes are represented with negative durations!!
      noteDuration = (wholenote) / abs(divider);
      noteDuration *= 1.5;  // increases the duration in half for dotted notes
    }

    // we only play the note for 90% of the duration, leaving 10% as a pause
    tone(PIN_BUZZER, melody[thisNote], noteDuration * 0.9);

    // Wait for the specief duration before playing the next note.
    delay(noteDuration);

    // stop the waveform generation before the next note.
    noTone(PIN_BUZZER);
  }
  display.clear();
  delay(500);
  // =========================================================================
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
        display.printFixed(0, 10, "*", STYLE_NORMAL);
        display.setTextCursor(15, 10);
        display.print(single_roll);

        display.setFixedFont(ssd1306xled_font6x8);
        display.printFixed(0, 35, "Result all dices:", STYLE_NORMAL);
        display.setFixedFont(ssd1306xled_font8x16);
        display.printFixed(0, 40, "*", STYLE_NORMAL);
        display.setTextCursor(15, 40);
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
          if (i == 4) {
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
      display.printFixed(0, 10, "*", STYLE_NORMAL);
      display.setTextCursor(15, 10);
      display.print(single_roll);

      display.setFixedFont(ssd1306xled_font6x8);
      display.printFixed(0, 35, "Result all dices:", STYLE_NORMAL);
      display.setFixedFont(ssd1306xled_font8x16);
      display.printFixed(0, 40, "*", STYLE_NORMAL);
      display.setTextCursor(15, 40);
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

  } else { // If in configuration mode
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
          virtual_position = 0;
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
      display.printFixed(0, 10, "D", STYLE_NORMAL);
      display.setTextCursor(15, 10);
      display.print(dices[virtual_position][0]);
      if (virtual_position == 4) {
        display.printFixed(35, 10, "%", STYLE_NORMAL);
      }
      display.setFixedFont(ssd1306xled_font6x8);
      display.printFixed(0, 35, "Number of dices:", STYLE_NORMAL);
      display.setFixedFont(ssd1306xled_font8x16);
      display.printFixed(0, 40, "*", STYLE_NORMAL);
      display.setTextCursor(15, 40);
      display.print(dices[virtual_position][1]);

    }

    // If selecting number os dices
    if (number_dice) {

      display.setFixedFont(ssd1306xled_font6x8);
      display.printFixed(0, 0, "Configuring dice:", STYLE_NORMAL);
      display.setFixedFont(ssd1306xled_font8x16);
      display.printFixed(0, 10, "D", STYLE_NORMAL);
      display.setTextCursor(15, 10);
      display.print(dices[last_virtual_position][0]);
      if (last_virtual_position == 4) {
        display.printFixed(35, 10, "%", STYLE_NORMAL);
      }
      display.setFixedFont(ssd1306xled_font6x8);
      display.printFixed(0, 35, "Number of dices:", STYLE_NORMAL);
      display.setFixedFont(ssd1306xled_font8x16);
      display.printFixed(0, 40, "*", STYLE_NORMAL);
      display.setTextCursor(15, 40);
      display.print(virtual_position);

    }
    delay(500);
  }
}
// -----------------------------------------------------------------------------------------------
