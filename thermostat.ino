#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Keypad.h>
#include <DHT.h>

// Define I2C pins
#define I2C_SCL 2  // or 22
#define I2C_SDA 42 // or 21

// Define DHT sensor type
#define DHTPIN 7       // Pin where the DHT sensor is connected
#define DHTTYPE DHT22  // DHT 22 (AM2302)

// Define other pin mappings
#define PUSH_BUTTON_PIN 4
#define ONBOARD_LED_PIN 9
#define BUZZER_PIN 19
#define FAN_A_PIN 35
#define FAN_B_PIN 38

// RGB LED Pins
#define RED_PIN 16
#define GREEN_PIN 33
#define BLUE_PIN 19

// OLED display width and height
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

// Create an OLED display object
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// Keypad setup
const byte ROW_NUM = 4;    // 4 rows
const byte COLUMN_NUM = 4; // 4 columns

char keys[ROW_NUM][COLUMN_NUM] = {
  {'D', '#', '0', '*'},
  {'C', '9', '8', '7'},
  {'B', '6', '5', '4'},
  {'A', '3', '2', '1'}
};

byte pin_rows[ROW_NUM] = {1, 10, 9, 11}; // GPIO1, GPIO10, GPIO9, GPIO11 connect to the row pins
byte pin_column[COLUMN_NUM] = {12, 14, 21, 34};  // GPIO12, GPIO14, GPIO21, GPIO34 connect to the column pins

Keypad keypad = Keypad(makeKeymap(keys), pin_rows, pin_column, ROW_NUM, COLUMN_NUM);

DHT dht(DHTPIN, DHTTYPE); // Initialize DHT sensor

float targetTemperature = 0.0; // User-defined target temperature
bool fanAState = false; // State of Fan A
bool fanBState = false; // State of Fan B

void setup() {
  // Start the serial communication
  Serial.begin(9600);
  Serial.println("Starting setup...");

  // Initialize I2C communication with specified pins
  Wire.begin(I2C_SDA, I2C_SCL);

  // Initialize the display
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {  // 0x3C is the default I2C address
    Serial.println("SSD1306 OLED initialization failed. Check wiring!");
    while (true); // Stop the program here if the display initialization fails
  } else {
    Serial.println("OLED initialized successfully.");
  }

  // Initialize DHT sensor
  dht.begin();

  // Set pin modes
  pinMode(PUSH_BUTTON_PIN, INPUT);
  pinMode(ONBOARD_LED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(FAN_A_PIN, OUTPUT);
  pinMode(FAN_B_PIN, OUTPUT);
  pinMode(RED_PIN, OUTPUT); // Set RED_PIN mode for LED

  // Clear the display and set initial display settings
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.display();
}

void loop() {
  char key = keypad.getKey();

  // Read temperature and humidity
  float currentTemperature = dht.readTemperature();
  float currentHumidity = dht.readHumidity();

  // Check for invalid readings
  if (isnan(currentTemperature) || isnan(currentHumidity)) {
    displayError("DHT Error!");
    return; // Skip the rest of loop if sensor error
  }

  // Handle keypad input
  if (key) {
    // Produce a beep sound when any key is pressed
    tone(BUZZER_PIN, 1000);  // Frequency of 1000 Hz
    delay(100);  // Short delay for the beep sound
    noTone(BUZZER_PIN);  // Stop the tone after the delay

    Serial.print("Key Pressed: ");
    Serial.println(key);

    if (key == '*') {
      // Clear the target temperature
      targetTemperature = 0.0;  // Reset target temperature
      Serial.println("Target temperature cleared.");
    } else if (key == '#') {
      // Display current temperature and humidity
      updateDisplay(currentTemperature, currentHumidity);
    } else if (key == 'A' || key == 'B' || key == 'C' || key == 'D') {
      // Store the operator based on key press (for example, A for setting temperature)
      if (key == 'A') {
        // Example: set target temperature
        targetTemperature = currentTemperature; // Replace with actual input handling if needed
        Serial.print("Target temperature set to: ");
        Serial.println(targetTemperature);
      }
    } else {
      // Handle numeric input to set the target temperature
      targetTemperature = (targetTemperature * 10) + (key - '0'); // Simplified target setting
      Serial.print("Current target temperature: ");
      Serial.println(targetTemperature);
    }

    // Control fans, LED, and buzzer based on the target temperature
    controlFansLEDAndBuzzer(currentTemperature);
  }

  // Update the display continuously
  updateDisplay(currentTemperature, currentHumidity);
}

void updateDisplay(float currentTemperature, float currentHumidity) {
  display.clearDisplay();
  display.setCursor(0, 0);
  display.print("Temp: ");
  display.print(currentTemperature);
  display.println(" C");
  display.print("Hum: ");
  display.print(currentHumidity);
  display.println(" %");
  display.print("Target: ");
  display.print(targetTemperature);
  display.println(" C");
  display.display();
}

void controlFansLEDAndBuzzer(float currentTemperature) {
  // Control Fan A and LED based on the current temperature
  if (currentTemperature > targetTemperature) {
    digitalWrite(FAN_A_PIN, HIGH); // Turn on Fan A
    digitalWrite(RED_PIN, HIGH);    // Turn on the LED
    fanAState = true;
    Serial.println("Fan A turned ON");
  } else {
    digitalWrite(FAN_A_PIN, LOW); // Turn off Fan A
    digitalWrite(RED_PIN, LOW);    // Turn off the LED
    fanAState = false;
    Serial.println("Fan A turned OFF");
  }

  // Control Fan B (example: turn on if humidity is above certain level)
  if (currentTemperature > 26.0) { // Example threshold for Fan B
    digitalWrite(FAN_B_PIN, HIGH); // Turn on Fan B
    fanBState = true;
    Serial.println("Fan B turned ON");
  } else {
    digitalWrite(FAN_B_PIN, LOW); // Turn off Fan B
    fanBState = false;
    Serial.println("Fan B turned OFF");
  }

  // Buzzer Alert for High Temperature
  if (currentTemperature >= targetTemperature + 5.0) { // Buzzer activates at 5°C above the target
    tone(BUZZER_PIN, 1000); // Continuous tone at 1000 Hz
    Serial.println("Buzzer ON: Temperature too high!");
  } else {
    noTone(BUZZER_PIN); // Stop the tone
    Serial.println("Buzzer OFF");
  }
}

void displayError(String errorMessage) {
  display.clearDisplay();
  display.setCursor(0, 0);
  display.setTextSize(2);
  display.print(errorMessage);
  display.display();
  delay(2000);
  display.clearDisplay();
}
