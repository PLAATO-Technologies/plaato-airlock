#define BOARD_FIRMWARE_VERSION        "1.3.4"          // Must be same as bin file name
#define BOARD_HARDWARE_VERSION        "2.0.0"          // In use during provisioning
#define PRODUCT_NUMBER                ""

#define BOARD_NAME                    "PLAATO"        // Name of your product. Should match App Export request info.
#define BOARD_VENDOR                  "PLAATO"        // Name of your company. Should match App Export request info.
#define BOARD_TEMPLATE_ID             ""              // Blynk App Template ID

#define PRODUCT_WIFI_SSID             "PLAATO"         // Name of the device, to be displayed during configuration. Should match export request info.
#define BOARD_CONFIG_AP_URL           "plaato.cc"      // Config page will be available in a browser at 'http://our-product.cc/

#define WIFI_NET_CONNECT_TIMEOUT      30000
#define WIFI_CLOUD_CONNECT_TIMEOUT    15000
#define WIFI_CLOUD_SYNC_TIMEOUT       5000
#define WIFI_AP_CONFIG_PORT           80
#define WIFI_AP_IP                    IPAddress(192, 168, 4, 1)
#define WIFI_AP_Subnet                IPAddress(255, 255, 255, 0)
//#define WIFI_CAPTIVE_PORTAL_ENABLE

#define USE_TICKER
//#define USE_TIMER_ONE
//#define USE_TIMER_THREE

#if defined(APP_DEBUG)
  #define DEBUG_PRINT(...) BLYNK_LOG1(__VA_ARGS__)
#else
  #define DEBUG_PRINT(...)
#endif

