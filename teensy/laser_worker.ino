// include the library code:
#include <LiquidCrystal.h>
#include <EEPROM.h>
#include "EEPROMAnything.h"

// Pins for LCD
#define PIN_LCD_D7 12
#define PIN_LCD_D6 13
#define PIN_LCD_D5 14
#define PIN_LCD_D4 15
#define PIN_LCD_RS 21
#define PIN_LCD_EN 20

#define MAX_DISPLAY_WIDTH 16
#define MAX_DISPLAY_HEIGHT 2

// Input pin from laser cutter mobo
#define LASER_PIN 0

// Button connected here will reset the counter
#define RESET_PIN 4

// Relay to enable or disable the laser
#define ENABLE_PIN 16

// 5v from laser power supply
#define LASER_POWER_PIN 17

#define RFID_CODE_LENGTH 10
#define RFID_CODE_START 0x02
#define RFID_CODE_END 0x03

// ID-12LA RFID reader connected via the UART
HardwareSerial Uart = HardwareSerial();

// initialize the library with the numbers of the interface pins
LiquidCrystal lcd(PIN_LCD_RS, PIN_LCD_EN, PIN_LCD_D4, PIN_LCD_D5, PIN_LCD_D6, PIN_LCD_D7);

int lastPos;
unsigned long time_total=0;
unsigned long time_start=0;
unsigned long time_last=0;

void setup() {
  Serial.begin(9600);
  Uart.begin(9600);

  pinMode(ENABLE_PIN, OUTPUT);
  digitalWrite(ENABLE_PIN, LOW);
  
  // set up the LCD's number of columns and rows: 
  lcd.begin(MAX_DISPLAY_WIDTH, MAX_DISPLAY_HEIGHT);
  // Print a message to the LCD.
  lcd.print("  Please  Wait  ");
  
  pinMode(LASER_PIN, INPUT);
  digitalWrite(LASER_PIN, HIGH); // enable internal pullup
  
  pinMode(LASER_POWER_PIN, INPUT);
  digitalWrite(LASER_POWER_PIN, HIGH);
  
  pinMode(RESET_PIN, INPUT);
  digitalWrite(RESET_PIN, HIGH);
  
  lastPos = EEPROM.read(0);
  if(lastPos == 0x11) {
    EEPROM_readAnything(0x11, time_total);
  } else if(lastPos == 0x22) {
    EEPROM_readAnything(0x22, time_total);
  } else {
    lastPos = 0x22;
    time_total = 0;
  }
}

void loop() {
  static unsigned char rfidByteIndex = 0;
  static unsigned char rfid[RFID_CODE_LENGTH];
  unsigned char rfidByte;
  
  static unsigned char serialCommand = 0;
  static unsigned char serialDisplayBuffer[MAX_DISPLAY_WIDTH + 1];
  static unsigned char serialDisplayBufferIndex = 0;
  unsigned char serialByte;
  
  if (Serial.available() > 0) {
    serialByte = Serial.read();
   
    switch (serialCommand) {
      case 'e':
          // enable laser
          if (serialByte == '\n') {
            digitalWrite(ENABLE_PIN, HIGH);
            serialCommand = 0;
          }
          break;
      case 'u':
          // enable laser until odometer value
          if (serialByte == '\n') {
            serialCommand = 0;
          }
          break;
      case 'd':
          // disable laser
          if (serialByte == '\n') {
            digitalWrite(ENABLE_PIN, LOW);
            serialCommand = 0;
          }
          break;
      case 'p':
          // display a message
          if (serialByte == '\n') {
            serialDisplayBuffer[serialDisplayBufferIndex] = 0;
            
            lcd.setCursor(0, 0);
            lcd.print((char*)serialDisplayBuffer);
            
            while (serialDisplayBufferIndex++ < MAX_DISPLAY_WIDTH)
              lcd.print(" ");
            
            serialDisplayBufferIndex = 0;
            serialCommand = 0;
          } else {
              if (serialDisplayBufferIndex < MAX_DISPLAY_WIDTH)
                serialDisplayBuffer[serialDisplayBufferIndex++] = serialByte;
          }
          break;
      case 'o':
          // report odometer
          if (serialByte == '\n') {
            Serial.print("o");
            Serial.print(time_total / 1000);
            Serial.print("\n");
            serialCommand = 0;
          }
          break;
      case 0:
      default:
        // ignore unknown commands
        serialCommand = serialByte;
        break;
    }
   
  }
  
  if (Uart.available() > 0) {
    rfidByte = Uart.read();
    if (rfidByte == RFID_CODE_START) {
      rfidByteIndex = 0;
    } else {
      if (rfidByteIndex < RFID_CODE_LENGTH) {
        rfid[rfidByteIndex++] = rfidByte;
      } else {
        if (rfidByte == RFID_CODE_END) {
          // done reading RFID
          // send odometer and rfid code to the host
          Serial.print("r");
          Serial.print(time_total / 1000);
          Serial.print(",");
          Serial.write(rfid, RFID_CODE_LENGTH);
          Serial.print("\n");
        }
      }
    }
  }
  
  
  // set the cursor to column 0, line 1
  // (note: line 1 is the second row, since counting begins with 0):
  lcd.setCursor(0, 1);
  // print the number of seconds since reset:
  
  if(digitalRead(RESET_PIN) == 0) {
    time_total = 11400000L;
    EEPROM_writeAnything(0x11, time_total);
    EEPROM_writeAnything(0x22, time_total);
  }
  
  unsigned long time_tmp = time_total;
  if(time_last > 0 && time_start > 0) {
    time_tmp += (time_last - time_start);
  }    
  
  if((digitalRead(LASER_PIN) == 0) &&
     (digitalRead(LASER_POWER_PIN) == 1)) { // 0 --> laser is firing
    if(time_start == 0) {
      time_start = time_last = millis();
    } else {
      time_last = millis();
    }
  } else if(time_start > 0) {
    if(millis() - time_last > 10000) {
      time_total += time_last - time_start;
      time_last = time_start = 0;
    } else if(millis() - time_last > 1000) {      
      if(lastPos == 0x11) {
        EEPROM_writeAnything(0x22, time_tmp);
        EEPROM.write(0, 0x22);
        lastPos = 0x22;
      } else {
        EEPROM_writeAnything(0x11, time_tmp);
        EEPROM.write(0, 0x11);
        lastPos = 0x11;
      }
    }
  }
  

  lcd.print(time_tmp/1000);
  
  lcd.setCursor(14,1);
  if(time_start > 0) {
    switch((time_tmp/200) % 4) {
      case 0: lcd.print('|'); break;
      case 1: lcd.print('/'); break;
      case 2: lcd.print('-'); break;
      case 3: lcd.print('/'); break;
    }
  } else {
    lcd.print(" ");
  }
}
