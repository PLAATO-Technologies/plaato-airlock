//#define SIMULATE_SENSORS        // DEBUG: define if you want to simulate reset switch, PCT2075 and STM8
#include "plaato_stm8.h"
#include "pct2075.h"

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

void setup() {
  stm8.setup();             // This sets up I2C.
  Serial.begin(115200);
  delay(100);               // Delay for I2C to work
  Serial.println("\n");     // 2 Println to make serial more readable
  
}

void loop() {
  delay(1000);
  // Wake up temp sensor
  temperature_sensor.wake_up();
  temperature_sensor.read_temperature();
  temperature_sensor.shut_down();

   // Check for reset and get bubblecount
  stm8.sync();

  // Set LED on STM8
  stm8.set_led(SLOWBREATHING);
  
  float temp = temperature_sensor.get_temperature();
  int bubble_count = stm8.get_count();
  Serial.print(temp); Serial.println("°C");
  Serial.print("BUB = "); Serial.println(bubble_count);
  
}
