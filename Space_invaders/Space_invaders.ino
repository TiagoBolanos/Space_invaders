#include "BluetoothSerial.h"
#include <driver/adc.h>
#include <BleKeyboard.h>
#include <math.h>

unsigned long lastZSentTime = 0;
const unsigned long zDelayDuration = 500;  // Delay in milliseconds (0.5 seconds)

float smoothingFactor = 0.1;  // Smoothing factor for accelerometer (0.0 - 1.0)
float x;
float y;
float z;
float smoothedX;
float smoothedY;
float smoothedZ;
// EMG parameters
float EMG = 0, lEMG = 0;
int sumEMG = 0, sumsqrEMG = 0;
int Calibration_len = 500;
float calibration_mean = 0.0;
float delta, mean, sum_of_squared_deviations, variance, tf;
int emgDebounceCount = 0;
int emgDebounceThreshold = 5;

int n = 0, j = 0, state_duration = 0;
int samples_above_fail = 1;       // Debounce muscle contractions
int active_state_duration = 2;    // Duration to consider muscle activation
int fail_size = 5;                // Fail size for filtering noise
float threshold = 2;            // EMG signal threshold
float thresholdMultiplier = 1.5;  // Multiplier to adjust threshold dynamically

bool onset = false;
bool alarms = false;
bool flag = false;

// Moving average filter parameters
const int filterSize = 10;
float emgFilter[filterSize];
int filterIndex = 0;
float emgFiltered = 0;

void addToFilter(float value) {
  emgFilter[filterIndex] = value;
  filterIndex = (filterIndex + 1) % filterSize;
}

float getFilteredValue() {
  float sum = 0;
  for (int i = 0; i < filterSize; i++) {
    sum += emgFilter[i];
  }
  return sum / filterSize;
}

void calibration() {
  Serial.println("Calibration starts in 3 seconds");
  delay(3000);
  sumEMG = 0;
  sumsqrEMG = 0;
  calibration_mean = 0;
  float i = 1;
  for (i = 1; i <= Calibration_len; i++) {
    EMG = analogRead(36);  // Read EMG sensor
    sumEMG += EMG;
    sumsqrEMG += EMG * EMG;
    calibration_mean = sumEMG / i;
    delta = EMG - calibration_mean;
    mean = mean + delta / (i + 1);
    sum_of_squared_deviations += delta * (EMG - mean);
    delay(10);
  }
  
  calibration_mean = sumEMG / Calibration_len;
  variance = ((sumsqrEMG / Calibration_len) - calibration_mean * calibration_mean) * (Calibration_len / (Calibration_len - 1));
  if (variance == 0) {
    variance = 1;  // Avoid divide by zero error
  }
  
  Serial.print("Calibration Mean: ");
  Serial.println(calibration_mean);
  Serial.print("Variance: ");
  Serial.println(variance);
  Serial.println("Calibration Successful!");
  delay(1000);
}

BleKeyboard bleKeyboard;
bool hci = true;

void setup() {
  Serial.begin(115200);
  Serial.println("Started");
  
  if (hci) {
    bleKeyboard.begin();  // Start BLE Keyboard
  }

  delay(5000);  // Short delay to settle sensors
  calibration();  // Perform calibration

  // Initialize EMG filter
  for (int i = 0; i < filterSize; i++) {
    emgFilter[i] = 0;
  }
}

void loop() {
  // Handle calibration command via Serial
  if (Serial.available() > 0) {
    char receivedChar = Serial.read();
    if (receivedChar == 'c') {
      calibration();  // Trigger recalibration
    }
  }

  // Read and filter EMG signal
  EMG = analogRead(36);  // EMG sensor read
  addToFilter(EMG);
  emgFiltered = getFilteredValue();

  // Calculate tf value for muscle activation
  tf = abs(1 / variance) * pow((emgFiltered - calibration_mean), 2);
  
  // Detect muscle contraction based on tf value
  if (onset) {
    if (alarms == false && tf < threshold) {
      alarms = true;
    } else if (alarms && tf < threshold) {
      state_duration += 1;
      if (j > 0) {
        n += 1;
        if (n == samples_above_fail) {
          n = 0;
          j = 0;
        }
      }
      if (state_duration == active_state_duration) {
        onset = false;
        alarms = false;
        flag = false;
        resetState();
      }
    } else {
      j++;
      if (j > fail_size) {
        resetState();
      }
    }
  } else {
    if (alarms == false && tf >= threshold) {
      alarms = true;
    } else if (alarms && tf >= threshold) {
      state_duration++;
      if (j > 0) {
        n++;
        if (n == samples_above_fail) {
          resetState();
        }
      }
      if (state_duration == active_state_duration) {
        flag = true;
        onset = true;
        resetState();
      }
    } else {
      j++;
      if (j > fail_size) {
        resetState();
      }
    }
  }

  // Read accelerometer values
  x = analogRead(33);
  y = analogRead(32);
  z = analogRead(39);

  // Apply smoothing to accelerometer values
  smoothedX = (x * smoothingFactor) + (smoothedX * (1 - smoothingFactor));
  smoothedY = (y * smoothingFactor) + (smoothedY * (1 - smoothingFactor));
  smoothedZ = (z * smoothingFactor) + (smoothedZ * (1 - smoothingFactor));

  // Detect left/right arm movement
  if (smoothedX > 190) {
    bleKeyboard.press(KEY_RIGHT_ARROW);
    Serial.println("Right");
  } else if (smoothedX < 150) {
    bleKeyboard.press(KEY_LEFT_ARROW);
    Serial.println("Left");
  } else {
    bleKeyboard.releaseAll();
  }

  // Detect muscle contraction to trigger space bar
  if (flag && (millis() - lastZSentTime >= zDelayDuration)) {
    bleKeyboard.press(KEY_LEFT_SHIFT);
    lastZSentTime = millis();
    delay(20);
    bleKeyboard.releaseAll();
  }

  // Debugging info
  Serial.print(smoothedX);
  Serial.print(", ");
  Serial.print(tf);
  Serial.print(", ");
  Serial.println(flag);

  delay(20);
}

void resetState() {
  alarms = false;
  n = 0;
  j = 0;
  state_duration = 0;
}
