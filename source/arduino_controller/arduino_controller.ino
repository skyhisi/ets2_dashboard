/*
Copyright (c) 2013, Silas Parker
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

    Redistributions of source code must retain the above copyright notice, this
    list of conditions and the following disclaimer.
    Redistributions in binary form must reproduce the above copyright notice,
    this list of conditions and the following disclaimer in the documentation
    and/or other materials provided with the distribution.
    The name of Silas Parker may not be used to endorse or promote products
    derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <LiquidCrystal.h>
#include <Servo.h>

const int SPEEDO_PIN      = A1;
const int RPM_PIN         = A0;
const int LEFT_INDICATOR  = A2;
const int RIGHT_INDICATOR = A3;
const int PARKING_BREAK   = A4;
const int FUEL_WARNING    = A5;

// Servo variables
Servo speedo;
Servo rpm;



#define PACKET_SYNC 0xFF
#define PACKET_VER  2

#define SERVO_DIR_NORMAL false
#define SERVO_DIR_INVERT true

int serial_byte;

LiquidCrystal lcd(12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2);

void setup()
{
  Serial.begin(115200);
  
  lcd.begin(16, 2);
  lcd.print("Self Test");
  
  // Initialise servos
  speedo.attach(SPEEDO_PIN);
  speedo.write(180);
  
  rpm.attach(RPM_PIN);
  rpm.write(180);
  
  // Initialise LEDs
  pinMode(LEFT_INDICATOR, OUTPUT);
  pinMode(RIGHT_INDICATOR, OUTPUT);
  pinMode(PARKING_BREAK, OUTPUT);
  pinMode(FUEL_WARNING, OUTPUT);
  
  digitalWrite(LEFT_INDICATOR, 0);
  digitalWrite(RIGHT_INDICATOR, 0);
  digitalWrite(PARKING_BREAK, 0);
  digitalWrite(FUEL_WARNING, 0);
  
  
  delay(500);
  
  speedo.write(0);
  rpm.write(0);
  digitalWrite(LEFT_INDICATOR, 1);
  digitalWrite(RIGHT_INDICATOR, 1);
  digitalWrite(PARKING_BREAK, 1);
  digitalWrite(FUEL_WARNING, 1);
  
  
  delay(500);
  
  speedo.write(180);
  rpm.write(180);
  digitalWrite(LEFT_INDICATOR, 0);
  digitalWrite(RIGHT_INDICATOR, 0);
  digitalWrite(PARKING_BREAK, 0);
  digitalWrite(FUEL_WARNING, 0);
  
  
  lcd.clear();
  lcd.print("Wait");
  
  // Wait a second to ensure serial data isn't from re-programming 
  delay(1000);
  lcd.clear();
  lcd.print("Ready");
}



void read_serial_byte_set_servo(Servo& servo, bool invert)
{
  serial_byte = Serial.read();
  serial_byte = (serial_byte < 0) ? 0 : ((serial_byte > 180) ? 180 : serial_byte);
  if (invert)
    servo.write(180 - serial_byte);
  else
    servo.write(serial_byte);
}

void skip_serial_byte()
{
  (void)Serial.read();
}

void digitalWriteFromBit(int port, int value, int shift)
{
  digitalWrite(port, (value >> shift) & 0x01);
}

void loop()
{
  if (Serial.available() < 16)
    return;
  
  serial_byte = Serial.read();
  if (serial_byte != PACKET_SYNC)
    return;
    
  serial_byte = Serial.read();
  if (serial_byte != PACKET_VER)
  {
    lcd.clear();
    lcd.print("PROTOCOL VERSION ERROR");
    return;
  }
  
  read_serial_byte_set_servo(speedo, SERVO_DIR_INVERT); // Speed  
  read_serial_byte_set_servo(rpm, SERVO_DIR_INVERT); // RPM
  
  skip_serial_byte(); // Brake air pressure
  skip_serial_byte(); // Brake temperature
  skip_serial_byte(); // Fuel ratio
  skip_serial_byte(); // Oil pressure
  skip_serial_byte(); // Oil temperature
  skip_serial_byte(); // Water temperature
  skip_serial_byte(); // Battery voltage
    
  
  // Truck lights byte
  serial_byte = Serial.read();
  digitalWriteFromBit(LEFT_INDICATOR,  serial_byte, 5);  
  digitalWriteFromBit(RIGHT_INDICATOR, serial_byte, 4);
  
  // Warning lights bytes

  serial_byte = Serial.read();  
  digitalWriteFromBit(PARKING_BREAK, serial_byte, 7);
  digitalWriteFromBit(FUEL_WARNING, serial_byte, 3);  
 
  // Enabled flags
  serial_byte = Serial.read();
  
  // Text length
  int text_len = Serial.read();
  
  // Followed by text
  if (0 < text_len && text_len < 127)
  {
    lcd.clear();
    for (int i = 0; i < text_len; ++i)
    {
      while (Serial.available() == 0) // Wait for data if slow
      {
        delay(2);
      }
      serial_byte = Serial.read();
      if (serial_byte < 0 && serial_byte > 127)
        return;
      
      if (serial_byte == '\n')
        lcd.setCursor(0, 1);
      else
        lcd.print(char(serial_byte));
//      delay(2);
    }
  }
  
}

