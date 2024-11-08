bool powerup = false;

enum State {
  MODE_WAIT_CONFIG,
  MODE_CONFIGURING,
  MODE_CONNECTING_NET,
  MODE_CONNECTING_CLOUD,
  MODE_RUNNING,
  MODE_OTA_UPGRADE,
  MODE_SWITCH_TO_STA,
  MODE_ERROR,

  MODE_MAX_VALUE
};

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

#if defined(APP_DEBUG)
const char* StateStr[MODE_MAX_VALUE] = {
  "WAIT_CONFIG",
  "CONFIGURING",
  "CONNECTING_NET",
  "CONNECTING_CLOUD",
  "RUNNING",
  "OTA_UPGRADE",
  "SWITCH_TO_STA",
  "ERROR"
};
#endif

namespace BlynkState
{
  volatile State state;

  State get()        { return state; }
  bool  is (State m) { return (state == m); }
  void  set(State m);
};

