#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPUpdateServer.h>
#include <DNSServer.h>

extern void enterOTA();

bool provisioning = false;
unsigned int server_connect_fails = 0;

ESP8266WebServer server(WIFI_AP_CONFIG_PORT);
ESP8266HTTPUpdateServer httpUpdater;
DNSServer dnsServer;
const byte DNS_PORT = 53;
unsigned long sync_started = 0;

const char* config_form = R"html(
<!DOCTYPE HTML><html>
<form method='get' action='config'>
  <label>WiFi SSID: </label><input type="text" name="ssid" length=32 required="required"><br/>
  <label>Password:  </label><input type="text" name="pass" length=32><br/>
  <label>Auth token:</label><input type="text" name="blynk" placeholder="a0b1c2d..." pattern="[a-zA-Z0-9]{12,32}" maxlength="32" required="required"><br/>
  <label>Host: </label><input type="text" name="host" length=32><br/>
  <label>Port: </label><input type="number" name="port" value="80" min="1" max="65535"><br/>
  <input type='submit' value="Apply">
</form>
</html>
)html";

void restartMCU() {
  ESP.restart();
}

void enterConfigMode()
{
  provisioning = true;
  WiFi.disconnect();
  WiFi.mode(WIFI_OFF);
  delay(1000);
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(WIFI_AP_IP, WIFI_AP_IP, WIFI_AP_Subnet);
  String SSID_with_id = String(PRODUCT_WIFI_SSID) + "-" + String(ESP.getChipId(), HEX);
  WiFi.softAP(SSID_with_id.c_str());
  delay(500);
  IPAddress myIP = WiFi.softAPIP();
  DEBUG_PRINT(String("AP SSID: ") + SSID_with_id);
  DEBUG_PRINT(String("AP IP:   ") + myIP[0] + "." + myIP[1] + "." + myIP[2] + "." + myIP[3]);

  if (myIP == (uint32_t)0)
  {
    BlynkState::set(MODE_ERROR);
    return;
  }

  // Set up DNS Server
  dnsServer.setTTL(300); // Time-to-live 300s
  dnsServer.setErrorReplyCode(DNSReplyCode::ServerFailure); // Return code for non-accessible domains
#ifdef WIFI_CAPTIVE_PORTAL_ENABLE
  dnsServer.start(DNS_PORT, "*", WiFi.softAPIP()); // Point all to our IP
  server.onNotFound(handleRoot);
#else
  dnsServer.start(DNS_PORT, BOARD_CONFIG_AP_URL, WiFi.softAPIP());
  DEBUG_PRINT(String("AP URL:  ") + BOARD_CONFIG_AP_URL);
#endif

  httpUpdater.setup(&server);

  server.on("/", []() {
    server.send(200, "text/html", config_form);
  });
  server.on("/config", []() {
    String ssid = server.arg("ssid");
    String ssidManual = server.arg("ssidManual");
    String pass = server.arg("pass");
    if (ssidManual != "") {
      ssid = ssidManual;
    }
    String token = server.arg("blynk");
    String host  = server.arg("host");
    String port  = server.arg("port");

    String content;
    unsigned statusCode;

    DEBUG_PRINT(String("WiFi SSID: ") + ssid + " Pass: " + pass);
    DEBUG_PRINT(String("Blynk cloud: ") + token + " @ " + host + ":" + port);

    if (token.length() == 32 && ssid.length() > 0) {
      configStore.flagConfig = false;
      CopyString(ssid, configStore.wifiSSID);
      CopyString(pass, configStore.wifiPass);
      CopyString(token, configStore.cloudToken);
      if (host.length()) {
        CopyString(host,  configStore.cloudHost);
      }
      if (port.length()) {
        configStore.cloudPort = port.toInt();
      }

      content = R"json({"status":"ok","msg":"Configuration saved"})json";
      statusCode = 200;
      server.send(statusCode, "application/json", content);

      BlynkState::set(MODE_SWITCH_TO_STA);
    } else if (token == "sherbetlemon" && ssid.length() > 0) {
      DEBUG_PRINT("Plaato Backup OTA provisioning");
      content = R"json({"status":"success","msg":"Connecting to wifi and doing firmware update"})json";
      statusCode = 200;
      server.send(statusCode, "application/json", content);
      WiFi.begin(ssid.c_str(), pass.c_str());   //WiFi connection
      
      DEBUG_PRINT("Connecting to wifi network...");
      while (WiFi.status() != WL_CONNECTED) {  //Wait for the WiFI connection completion
        delay(500);
      }
      DEBUG_PRINT("Connected!");
      enterOTA();
      restartMCU();      
    }
    else {
      DEBUG_PRINT("Configuration invalid");
      content = R"json({"status":"error","msg":"Configuration invalid"})json";
      statusCode = 404;
      server.send(statusCode, "application/json", content);
    }
  });
  server.on("/board_info.json", []() {
    char buff[256];
    snprintf(buff, sizeof(buff),
      R"json({"board":"%s","vendor":"%s","tmpl_id":"%s","fw_ver":"%s","hw_ver":"%s"})json",
      BOARD_NAME,
      BOARD_VENDOR,
      BOARD_TEMPLATE_ID,
      BOARD_FIRMWARE_VERSION,
      BOARD_HARDWARE_VERSION
    );
    server.send(200, "application/json", buff);
  });
  server.on("/reset", []() {
    config_reset();
    server.send(200, "application/json", R"json({"status":"ok","msg":"Configuration reset"})json");
  });
  server.on("/reboot", []() {
    restartMCU();
  });

  server.begin();

  while (BlynkState::is(MODE_WAIT_CONFIG) || BlynkState::is(MODE_CONFIGURING)) {
    dnsServer.processNextRequest();
    server.handleClient();
    if (BlynkState::is(MODE_WAIT_CONFIG) && WiFi.softAPgetStationNum() > 0) {
      BlynkState::set(MODE_CONFIGURING);
    } else if (BlynkState::is(MODE_CONFIGURING) && WiFi.softAPgetStationNum() == 0) {
      BlynkState::set(MODE_WAIT_CONFIG);
    }
  }

  server.stop();
}

void enterConnectNet() {
  BlynkState::set(MODE_CONNECTING_NET);
  DEBUG_PRINT(String("Connecting to WiFi: ") + configStore.wifiSSID);
  
  WiFi.mode(WIFI_STA);
  if (!WiFi.begin(configStore.wifiSSID, configStore.wifiPass))
    return;
  
  unsigned long timeoutMs = millis() + WIFI_NET_CONNECT_TIMEOUT;
  while ((timeoutMs > millis()) && (WiFi.status() != WL_CONNECTED))
  {
    delay(100);
    if (!BlynkState::is(MODE_CONNECTING_NET)) {
      WiFi.disconnect();
      return;
    }
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    if (powerup && !provisioning) {
      BlynkState::set(MODE_OTA_UPGRADE);
    } else {
      BlynkState::set(MODE_CONNECTING_CLOUD);
    }
  } else {
    BlynkState::set(MODE_ERROR);
  }
}

void enterConnectCloud() {
  BlynkState::set(MODE_CONNECTING_CLOUD);

  Blynk.disconnect();
  Blynk.config(configStore.cloudToken, configStore.cloudHost, configStore.cloudPort);
  Blynk.connect(0);

  unsigned long timeoutMs = millis() + WIFI_CLOUD_CONNECT_TIMEOUT;
  while ((timeoutMs > millis()) &&
        (Blynk.connected() == false))
  {
    Blynk.run();
    if (!BlynkState::is(MODE_CONNECTING_CLOUD)) {
      Blynk.disconnect();
      return;
    }
  }
  
  if (Blynk.connected()) {
    BlynkState::set(MODE_RUNNING);
    //CONNECTED TO THE CLOUD

    if (!configStore.flagConfig) {
      configStore.flagConfig = true;
      config_save();
      DEBUG_PRINT("Configuration stored to flash");
    }
  } else {
    server_connect_fails++;
    if (provisioning && server_connect_fails == 1) {
      // Set port to 8080
      configStore.cloudPort = 8080;
      // Make this function run again
      BlynkState::set(MODE_CONNECTING_CLOUD);
    } else {
      BlynkState::set(MODE_ERROR);
    }
  }
}

void enterSwitchToSTA() {
  BlynkState::set(MODE_SWITCH_TO_STA);

  DEBUG_PRINT("Switching to STA...");

  WiFi.mode(WIFI_OFF);
  delay(1000);
  WiFi.mode(WIFI_STA);

  BlynkState::set(MODE_CONNECTING_NET);
}

void running() {
  Blynk.run();  
}

void enterError() {
  BlynkState::set(MODE_ERROR);
  DEBUG_PRINT("ERROR thrown to main loop");
}
