#define USE_NODE_MCU_BOARD        // Telling Blynk what board we are using
#define APP_DEBUG                 // DEBUG: Comment this out to disable debug prints
#define BLYNK_DEBUG
#define BLYNK_PRINT Serial        // DEBUG: Comment this out to disable debug prints
//#define SIMULATE_SENSORS        // DEBUG: define if you want to simulate reset switch, PCT2075 and STM8

#define ESP_LED_PIN  2

#include "plaato_stm8.h"
#include "pct2075.h"

extern "C" {
  #include "user_interface.h"           // Needed for system_get_rst_info()
}

// From BlynkState.h
bool powerup = false;
enum Led_mode {
  NO_MODE,
  COUNTING1,
  COUNTING2,
  SLOWBREATHING,
  FASTBREATHING,
  SLOWFLASH,
  FASTFLASH,
  ALLOFF,
  ALLON,
  BOT,
  MID,
  TOP,
  SLOWDOWN,
  FASTDOWN,
  SLOWUP,
  FASTUP,
  BOTSLOWFLASH,
  BOTFASTFLASH,
  MIDSLOWFLASH,
  MIDFASTFLASH,
  TOPSLOWFLASH,
  TOPFASTFLASH,
  BOTONBUBBLE
};

// From Settings.h
#if defined(APP_DEBUG)
  #define DEBUG_PRINT(...) Serial.println(__VA_ARGS__)
#else
  #define DEBUG_PRINT(...)
#endif

// ERROR GLOBAL VARIABLES
String error_msg = "";
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
  Serial.println("test1");
}

void loop() {
  delay(1000);
  // Wake up temp sensor
  if (!temperature_sensor.wake_up()) {
    pct_error = true;
    error();
  }
 
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

  DEBUG_PRINT( String("Sleep counter = ") + String(*sleep_counter) );
  
}

void error() {
  pinMode(ESP_LED_PIN, OUTPUT);
  digitalWrite(ESP_LED_PIN, LOW);
}
