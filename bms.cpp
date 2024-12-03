#include <Wire.h>
#include <hd44780.h> // Main hd44780 header
#include <hd44780ioClass/hd44780_I2Cexp.h> // I2C expander I/O class header


// Create an LCD object
hd44780_I2Cexp lcd(0x27); // Adjust address if necessary


// Constants
const int resolution = 1023;          // 10-bit ADC resolution
const float R1 = 14830.0;             // Resistance R1 (in ohms) in the voltage divider
const float R2 = 2700.0;              // Resistance R2 (in ohms) in the voltage divider


// Calibration factors
const float calibrationFactor = 1.047;  // Adjust this value to calibrate individual cell voltages
const float totalCalibrationFactor = 0.99; // Adjust this value to calibrate total voltage


// Analog pin assignments
const int pin1 = A0;  // Pin for cell 1
const int pin2 = A1;  // Pin for cell 2
const int pin3 = A2;  // Pin for cell 3
const int pin4 = A3;  // Pin for cell 4


// Push button pins
const int buttonPin = 13;    // Button to toggle display mode


// Voltage range for percentage calculation
const float maxVoltage = 4.2;  // 100% at 4.2V (battery full)
const float minVoltage = 2.7;   // 0% at 2.7V (battery empty)


const int numReadings = 50; // Number of readings for averaging


int displayMode = 0;              // 0: Voltages, 1: Percentages, 2: Total voltage and total percentage
bool lastButtonState = HIGH;       // Store the last state of the display mode button


// Alert pins for low voltage
const int alertPinCell1 = 10;      // Alert pin for cell 1
const int alertPinCell2 = 9;       // Alert pin for cell 2
const int alertPinCell3 = 8;       // Alert pin for cell 3
const int alertPinCell4 = 7;       // Alert pin for cell 4


// Total voltage alert
const int alertPinTotalVoltage = 2; // Pin for total voltage alert


unsigned long lastUpdateTime = 0;  // To track the last update time
const unsigned long updateInterval = 2000; // Update every 2 seconds


// Declare voltage variables
float cell1Voltage = 0.0;
float cell2Voltage = 0.0;
float cell3Voltage = 0.0;
float cell4Voltage = 0.0;
float totalVoltage = 0.0;


void setup() {
    // Start serial communication
    Serial.begin(9600);
   
    // Initialize LCD with number of columns and rows
    int status = lcd.begin(16, 4); // 16 columns and 4 rows
    if (status) {
        status = -status; // Convert negative status value to positive number
        hd44780::fatalError(status); // Does not return
    }
    lcd.backlight(); // Turn on the backlight
   
    // Initialize button pins
    pinMode(buttonPin, INPUT_PULLUP);
   
    // Initialize alert pins
    pinMode(alertPinCell1, OUTPUT);
    pinMode(alertPinCell2, OUTPUT);
    pinMode(alertPinCell3, OUTPUT);
    pinMode(alertPinCell4, OUTPUT);
   
    // Ensure all alert outputs are off at the start
    digitalWrite(alertPinCell1, LOW);
    digitalWrite(alertPinCell2, LOW);
    digitalWrite(alertPinCell3, LOW);
    digitalWrite(alertPinCell4, LOW);
}


// Function to calculate the actual voltage after the voltage divider
float calculateVin(float vout) {
    return vout * (R1 + R2) / R2 * calibrationFactor; // Apply calibration factor
}


// Function to calculate the percentage of the voltage
float calculatePercentage(float voltage) {
    float percentage = (voltage - minVoltage) / (maxVoltage - minVoltage) * 100.0;
    if (percentage < 0) percentage = 0;    
    if (percentage > 100) percentage = 100;
    return percentage;
}


// Function to average ADC readings
int averageADC(int pin) {
    long sum = 0;
    for (int i = 0; i < numReadings; i++) {
        sum += analogRead(pin);
    }
    return sum / numReadings;
}


void loop() {
    // Check if it's time to update the readings
    if (millis() - lastUpdateTime >= updateInterval) {
        // Read analog values from each pin with averaging
        int rawValue1 = averageADC(pin1);
        int rawValue2 = averageADC(pin2);
        int rawValue3 = averageADC(pin3);
        int rawValue4 = averageADC(pin4);
       
        // Convert raw values to voltages at the analog pins
        float vout1 = (rawValue1 * 5.0) / resolution;
        float vout2 = (rawValue2 * 5.0) / resolution;
        float vout3 = (rawValue3 * 5.0) / resolution;
        float vout4 = (rawValue4 * 5.0) / resolution;


        // Calculate the actual voltages using the voltage divider formula
        cell1Voltage = calculateVin(vout1);
        cell2Voltage = calculateVin(vout2) - cell1Voltage; // Subtract previous cell voltage
        cell3Voltage = calculateVin(vout3) - (cell1Voltage + cell2Voltage); // Subtract cumulative previous cell voltages
        cell4Voltage = calculateVin(vout4) - (cell1Voltage + cell2Voltage + cell3Voltage); // Subtract cumulative previous cell voltages
       
        // Total voltage calculation with calibration
        totalVoltage = (cell1Voltage + cell2Voltage + cell3Voltage + cell4Voltage) * totalCalibrationFactor;


        // Calculate percentages
        float cell1Percentage = calculatePercentage(cell1Voltage);
        float cell2Percentage = calculatePercentage(cell2Voltage);
        float cell3Percentage = calculatePercentage(cell3Voltage);
        float cell4Percentage = calculatePercentage(cell4Voltage);
        float totalPercentage = (cell1Percentage + cell2Percentage + cell3Percentage + cell4Percentage) / 4.0;


        // Display on the LCD based on the current mode
        lcd.setCursor(0, 0); // Set cursor to the first line
        if (displayMode == 0) {
            lcd.print("B1:"); lcd.print(cell1Voltage, 2); lcd.print("V ");
            lcd.print("B2:"); lcd.print(cell2Voltage, 2); lcd.print("V");
           
            lcd.setCursor(0, 1); // Move to the second line
            lcd.print("B3:"); lcd.print(cell3Voltage, 2); lcd.print("V ");
            lcd.print("B4:"); lcd.print(cell4Voltage, 2); lcd.print("V");
        } else if (displayMode == 1) {
            lcd.print("B1:"); lcd.print(cell1Percentage, 0); lcd.print("% ");
            lcd.print("B2:"); lcd.print(cell2Percentage, 0); lcd.print("%");
           
            lcd.setCursor(0, 1);
            lcd.print("B3:"); lcd.print(cell3Percentage, 0); lcd.print("% ");
            lcd.print("B4:"); lcd.print(cell4Percentage, 0); lcd.print("%");
        } else if (displayMode == 2) {
            lcd.print("Total = ");
            lcd.print(totalVoltage, 2);
            lcd.print(" V");
           
            lcd.setCursor(0, 1);
            lcd.print("Battery % = ");
            lcd.print(totalPercentage, 0);
            lcd.print("%");
        }


        lastUpdateTime = millis(); // Update the last update time
    }


    // Button press logic to cycle display modes (INVERTED)
    bool buttonState = digitalRead(buttonPin);
    if (buttonState == HIGH && lastButtonState == LOW) { // Inverted logic
        displayMode = (displayMode + 1) % 3;  // Cycle through display modes 0, 1, 2
        delay(50); // Debounce delay
        lcd.clear(); // Clear only when changing mode
    }
    lastButtonState = buttonState;  // Save button state


    // Turn on alert pins if cell voltage is below set levels
    digitalWrite(alertPinCell1, cell1Voltage < 2.9 ? HIGH : LOW);
    digitalWrite(alertPinCell2, cell2Voltage < 3.9 ? HIGH : LOW);
    digitalWrite(alertPinCell3, cell3Voltage < 2.9 ? HIGH : LOW);
    digitalWrite(alertPinCell4, cell4Voltage < 3.9 ? HIGH : LOW);


    // Critical low voltage alarm
    digitalWrite(alertPinTotalVoltage, (cell1Voltage < 2.6 || cell2Voltage < 2.6 || cell3Voltage < 2.6 || cell4Voltage < 2.6) ? HIGH : LOW);
}
