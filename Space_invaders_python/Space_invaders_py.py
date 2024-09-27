import serial
import time
from pynput.keyboard import Controller, Key

# Configure the Bluetooth serial port
bt_port = "COM14"  # Replace with your Bluetooth serial port
baud_rate = 115200
ser = serial.Serial(bt_port, baud_rate)

# Set up the keyboard controller
keyboard = Controller()

# Define thresholds for detecting movement and sharp EMG variations
x_right_threshold = 190
x_left_threshold = 150

# EMG variables for sharp variation detection
emg_last_value = 0  # Holds the last EMG value to compute differences
sharp_variation_threshold = 25  # Threshold for detecting sharp EMG variations (adjust as needed)

# Function to press the space bar
def press_space_bar():
    keyboard.press(Key.space)
    time.sleep(0.05)
    keyboard.release(Key.space)
    print("Space bar pressed")

# Function to press the right arrow key
def press_right_arrow():
    keyboard.press(Key.right)
    time.sleep(0.05)
    keyboard.release(Key.right)
    print("Right arrow pressed")

# Function to press the left arrow key
def press_left_arrow():
    keyboard.press(Key.left)
    time.sleep(0.05)
    keyboard.release(Key.left)
    print("Left arrow pressed")

# Main loop to read data and emulate keypresses
def main():
    global emg_last_value
    try:
        while True:
            # Read data from the Bluetooth serial connection
            if ser.in_waiting > 0:
                data = ser.readline().decode('utf-8').strip()
                emg_value, x_value, y_value, z_value = map(int, data.split(','))

                # Print data for debugging
                

                # Detect arm twist to the right
                if x_value > x_right_threshold:
                    press_right_arrow()
                    # pass
                    print("                             Right")

                # Detect arm twist to the left
                elif x_value < x_left_threshold:
                    press_left_arrow()
                    # pass
                    print("                             Left")

                # Detect sharp variation in EMG value
                emg_variation = abs(emg_value - emg_last_value)
                print(f"EMG: {emg_value}, X: {x_value}, var: {emg_variation}") 

                if emg_variation > sharp_variation_threshold:
                    # Trigger action on sharp variation (e.g., muscle contraction)
                    press_space_bar()
                    print("                             ZAP")

                # Update the last EMG value for the next comparison
                emg_last_value = emg_value

            # time.sleep(0.05)  # Run every 50 ms (to match ESP32) 

    except KeyboardInterrupt:
        print("Program interrupted")

if __name__ == "__main__":
    main()
