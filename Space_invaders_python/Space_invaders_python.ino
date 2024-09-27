#include "BluetoothSerial.h"
#include <driver/adc.h>

BluetoothSerial SerialBT;

// EMG sensor and accelerometer variables
int emgPin = 36;  // EMG analog input pin
int xPin = 33;    // Accelerometer X axis pin
int yPin = 32;    // Accelerometer Y axis pin
int zPin = 39;    // Accelerometer Z axis pin

// Calibration variables
float calibration_mean = 0.0;
int sumEMG = 0;
int Calibration_len = 1000;

void calibration() {
  Serial.println("Calibration starts in 3 seconds...");
  delay(3000);
  sumEMG = 0;
  for (int i = 1; i <= Calibration_len; i++) {
    int EMG = analogRead(emgPin);  // Read EMG sensor
    sumEMG += EMG;
    delay(10);
  }
  calibration_mean = sumEMG / (float)Calibration_len;
  Serial.println("Calibration done!");
  Serial.print("Calibration Mean: ");
  Serial.println(calibration_mean);
}

void setup() {
  Serial.begin(115200);    // For debug purposes
  SerialBT.begin("Space_invaders");  // Start Bluetooth Serial

  delay(5000);  // Let sensors stabilize
  calibration();  // Calibrate EMG sensor
}

void loop() {
  // Read EMG and accelerometer data
  int EMG = analogRead(emgPin);
  int x = analogRead(xPin);
  int y = analogRead(yPin);
  int z = analogRead(zPin);

  // Send data via Bluetooth
  SerialBT.print(EMG);
  SerialBT.print(",");
  SerialBT.print(x);
  SerialBT.print(",");
  SerialBT.print(y);
  SerialBT.print(",");
  SerialBT.println(z);
  Serial.print(EMG);
  Serial.print(",");
  Serial.print(x);
  Serial.print(",");
  Serial.print(y);
  Serial.print(",");
  Serial.println(z);

  delay(50);  // Send data every 50 ms
}
