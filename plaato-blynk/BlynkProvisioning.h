#include "Settings.h"
#include <BlynkSimpleEsp8266.h>
#include "BlynkState.h"
#include "ConfigStore.h"
#include "ConfigMode.h"
#include "OTA.h"
#include "plaato_stm8.h"

uint8_t last_led_mode = 0;

inline
void BlynkState::set(State m) {
  if (state != m) {
    DEBUG_PRINT(String(StateStr[state]) + " => " + StateStr[m]);
    state = m;

    // Set LED
    uint8_t new_led_mode;
    
    if (state == MODE_WAIT_CONFIG) {
      new_led_mode = SLOWBREATHING; 
    } else if (state == MODE_ERROR) {
      new_led_mode = BOT; 
    } else if (state == MODE_CONFIGURING) {
      new_led_mode = MIDFASTFLASH;
    } else {
      new_led_mode = FASTFLASH;
    }

    if (last_led_mode != new_led_mode) {
      last_led_mode = new_led_mode;
      DEBUG_PRINT(String("Setting LED-mode: ") + String(new_led_mode));
      if (!stm8.set_led(new_led_mode)) {
        Serial.println("ERROR - STM8: Can not set LED mode");
      } 
    }
    
    
  }
}

class Provisioning {

public:
  void begin()
  {
    DEBUG_PRINT("");
    DEBUG_PRINT("Hardware v" + String(BOARD_HARDWARE_VERSION));
    DEBUG_PRINT("Firmware v" + String(BOARD_FIRMWARE_VERSION));

    randomSeed(ESP.getChipId());

    config_init();

    if (configStore.flagConfig) {
      provisioning = false;
      BlynkState::set(MODE_CONNECTING_NET);
    } else {
      provisioning = true;
      BlynkState::set(MODE_WAIT_CONFIG);
    }
  }

  bool run() {
    switch (BlynkState::get()) {
    case MODE_WAIT_CONFIG:       
    case MODE_CONFIGURING:       enterConfigMode();    return true;
    case MODE_CONNECTING_NET:    enterConnectNet();    return true;
    case MODE_CONNECTING_CLOUD:  enterConnectCloud();  return true;
    case MODE_RUNNING:           running();            return true;
    case MODE_OTA_UPGRADE:       enterOTA();           return true;
    case MODE_SWITCH_TO_STA:     enterSwitchToSTA();   return true;
    default:                     enterError();         return false;
    }
  }

};

Provisioning BlynkProvisioning;
