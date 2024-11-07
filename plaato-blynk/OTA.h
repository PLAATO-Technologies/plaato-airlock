//#include <ESP8266HTTPClient.h>
#include <plaato_ESP8266httpUpdate.h>
#include "plaato_stm8.h"
#include <time.h>

const String overTheAirServer = ""; // Add URL to OTA endpoint
const String overTheAirCheck = "/check.php";
const String overTheAirDownload = "/download.php";
//const char* fingerprint = "08:3B:71:72:02:43:6E:CA:ED:42:86:93:BA:7E:DF:81:C4:BC:62:30"; // *herokuapp.com
const char* fingerprint = "AA:AA:AA:AA:AA:AA:AA:AA:AA:AA:AA:AA:AA:AA:AA:AA:AA:AA:AA:AA"; // *wrong fingerprint.

String error_msg = "";

void enterOTA() {
  BlynkState::set(MODE_OTA_UPGRADE);
  // Disconnect, not to interfere with OTA process
  
  DEBUG_PRINT(String("Checking for firmware update. Device type: ") + PRODUCT_NUMBER + String(" Current firmware = ") + BOARD_FIRMWARE_VERSION + String(" Error msg = ") + error_msg);
  DEBUG_PRINT(String("Firmware check URL: ") + overTheAirServer+overTheAirCheck);
  DEBUG_PRINT(String("Free heap: ") + String(ESP.getFreeHeap()));

  ESPhttpUpdate.rebootOnUpdate(false); // Prevents from reboot after update. We want to do that ourselves. 

   // Synchronize time useing SNTP. This is necessary to verify that
  // the TLS certificates offered by the server are currently valid.
  DEBUG_PRINT("Setting time using SNTP. Current time: ");
  configTime(8 * 3600, 0, "pool.ntp.org", "time.nist.gov");
  time_t now = time(nullptr);
  int i = 0;
  while (now < 60000) { // Quickfix (60000) of SNTP bug from esp core 2.4.0. https://github.com/esp8266/Arduino/issues/3905
    if (i > 10) {
      DEBUG_PRINT("ERROR: Could not connect to SNTP");
      break;
    }
    i++;
    delay(200);
    now = time(nullptr);
  }
  struct tm timeinfo;
  gmtime_r(&now, &timeinfo);
  DEBUG_PRINT(asctime(&timeinfo));

  switch(ESPhttpUpdate.getMD5(overTheAirServer, 443, overTheAirCheck, BOARD_FIRMWARE_VERSION, PRODUCT_NUMBER, fingerprint, error_msg)) { // HTTPS
    case HTTP_UPDATE_FAILED:
      DEBUG_PRINT(String("Firmware check: Firmware update failed (error ") + ESPhttpUpdate.getLastError() + "): " + ESPhttpUpdate.getLastErrorString());
      BlynkState::set(MODE_CONNECTING_CLOUD); 
      break;
    case HTTP_UPDATE_NO_UPDATES:
      DEBUG_PRINT("Firmware check: Firware is up to date.");
      BlynkState::set(MODE_CONNECTING_CLOUD);
      break;
    case HTTP_UPDATE_OK: {
      stm8.set_led(FASTDOWN);
      String md5_signature = ESPhttpUpdate.getLastMD5();
      DEBUG_PRINT(String("Firmware check: New firmware excists. MD5 is ") + String(md5_signature) + String(". Downloading firmware from /download.php"));
      switch(ESPhttpUpdate.update(overTheAirServer, md5_signature, 80, overTheAirDownload, BOARD_FIRMWARE_VERSION, PRODUCT_NUMBER)) { // HTTP
        case HTTP_UPDATE_FAILED:
          DEBUG_PRINT(String("Firmware download: Firmware update failed (error ") + ESPhttpUpdate.getLastError() + "): " + ESPhttpUpdate.getLastErrorString());
          BlynkState::set(MODE_CONNECTING_CLOUD);
          break;
        case HTTP_UPDATE_NO_UPDATES:
          DEBUG_PRINT("Firmware download: Firware is up to date.");
          BlynkState::set(MODE_CONNECTING_CLOUD);
          break;
        case HTTP_UPDATE_OK: {
          DEBUG_PRINT("Firmware download: HTTP_UPDATE_OK");
          ESP.restart();
          delay(1000);
        }
      }
     break;
   }
  }
}

