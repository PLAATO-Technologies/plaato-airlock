#define USE_NODE_MCU_BOARD        // Telling Blynk what board we are using
#define APP_DEBUG                 // DEBUG: Comment this out to disable debug prints
#define BLYNK_DEBUG
#define BLYNK_PRINT Serial        // DEBUG: Comment this out to disable debug prints
//#define SIMULATE_SENSORS        // DEBUG: define if you want to simulate reset switch, PCT2075 and STM8

#define ESP_LED_PIN  2

#include "BlynkProvisioning.h"
#include "plaato_stm8.h"
#include "pct2075.h"

extern "C" {
  #include "user_interface.h"           // Needed for system_get_rst_info()
}

// ERROR GLOBAL VARIABLES
bool pct_error = false;
bool stm8_error = false;

// SLEEP GLOBAL VARIABLES
uint32_t *sleep_counter = (uint32_t *)0x60001240;
unsigned long start;
const int sleep_time_s = 290; // Should be 290
bool goodnight = false;
unsigned long transmit_time_stamp = 0;
const int transmit_delay = 4000;
const int provisioning_delay = 30000;

void setup() {
  start = micros();
  stm8.setup();             // This sets up I2C.
  Serial.begin(115200);
  delay(100);               // Delay for I2C to work
  Serial.println("\n");         // 2 Println to make serial more readable
  
  // Wake up temp sensor
  if (!temperature_sensor.wake_up()) {
    pct_error = true;
    error();
  }

  delay(100);
 
  // Measure temperature
  if (!temperature_sensor.read_temperature()) {
    pct_error = true;
    error();
  } else {    
    // Check temperature range 
    if ((temperature_sensor.get_temperature() < -20) || temperature_sensor.get_temperature() > 50) {
      pct_error = true;
      error();
    }
  }

  // Shut down
  if (!temperature_sensor.shut_down()) {
    pct_error = true;
    error();
  }

   // Check for reset and get bubblecount
  if (!stm8.sync()) {
    stm8_error = true;
    error();
  }

  // Set LED on STM8
  if (!stm8.set_led(SLOWBREATHING)) {
    stm8_error = true;
    error();
  }

  if (system_get_rst_info()->reason != REASON_DEEP_SLEEP_AWAKE) {
    DEBUG_PRINT("POWER-UP");
    powerup = true;
    *sleep_counter = 0;
  } else {
    DEBUG_PRINT("WAKE-UP");
    (*sleep_counter)++;
  }

  if (pct_error || stm8_error) {
    if (pct_error && !stm8_error) {
      error_msg = "E1";
      Serial.println("E1 - PCT2075 ERROR");
    } else if (!pct_error && stm8_error) {
      error_msg = "E2";
      Serial.println("E2 - STM8 ERROR");
    } else {
      error_msg = "E12";
      Serial.println("E1 - PCT2075 ERROR");
      Serial.println("E2 - STM8 ERROR");
    }
  } else {
    error_msg = String(*sleep_counter);
  }
    
  
  Serial.print(temperature_sensor.get_temperature()); Serial.println("°C");
  Serial.print("BUB = "); Serial.println(stm8.get_count());

  // Print serial number and firmware version
  Serial.print("serial_number="); Serial.print(PRODUCT_NUMBER); Serial.print("-"); Serial.print(WiFi.macAddress()); 
  Serial.print("\t firmware="); Serial.println(BOARD_FIRMWARE_VERSION);

  
  BlynkProvisioning.begin();
  
  // Check for reset flag
  if (stm8.get_reset_flag()) {
    DEBUG_PRINT("RESET FLAG detected. Resetting WiFi Config");
    config_reset();
  }

  DEBUG_PRINT( String("Sleep counter = ") + String(*sleep_counter) );
  
}

void loop() {
  // This handles the network and cloud connection. If ERROR (false) is thrown it is handled here.  
  if (!BlynkProvisioning.run()) {
    if (provisioning) {
      DEBUG_PRINT("Error from blynk.run(). Rebooting ESP since PROVISIONING.");
      delay(10);
      ESP.restart();
    } else {
      DEBUG_PRINT("Error from blynk.run(). Deep_sleep since not provisioning.");
      delay(10);
      if (!stm8.set_led(COUNTING2)) {
        Serial.println("ERROR - STM8: Can not set LED mode");
        error();
      }
      deep_sleep();
    }
  }

  if (goodnight) {
    if (provisioning) {
      if (millis() - transmit_time_stamp > provisioning_delay) {
        DEBUG_PRINT("Provisioning delay over");
        if (!stm8.set_led(COUNTING1)) {
          Serial.println("ERROR - STM8: Can not set LED mode");
          error();
        }
        deep_sleep();
      }
    } else {
      if (millis() - transmit_time_stamp > transmit_delay) {
        DEBUG_PRINT("Transmit delay over");
        if (!stm8.set_led(COUNTING1)) {
          Serial.println("ERROR - STM8: Can not set LED mode");
          error();
        }
        deep_sleep();
      }
    }
  }
}

BLYNK_CONNECTED() {
  if (!stm8_error) {
    Blynk.virtualWrite(V100, stm8.get_count());
  }
  if (!pct_error) {
    Blynk.virtualWrite(V101, temperature_sensor.get_temperature());   // Todo: Send temp first to make tempcorrection in back-end easier? 
  }
  Blynk.virtualWrite(V99, error_msg);
  
  goodnight = true;
  transmit_time_stamp = millis();
  IPAddress ip = WiFi.localIP();
  Serial.println(ip);
  DEBUG_PRINT("Data transmitted to cloud. Going to sleep after transmit/provisioning-delay");
}

// Deep sleep function. Uses global variable "unsigned long start" for debugging
void deep_sleep() {
  int sec = (micros()-start)/1000;
  Blynk.disconnect();
  DEBUG_PRINT( String("Been awake for ") + String(sec) + String(" ms. Going to sleep for ") + String(sleep_time_s) + String(" seconds. ") );
  ESP.deepSleep(sleep_time_s * 1000000, WAKE_RFCAL);
  delay(100);
}

void error() {
  pinMode(ESP_LED_PIN, OUTPUT);
  digitalWrite(ESP_LED_PIN, LOW);
}
