#include <json_mini.h>
extern json_mini JSON;
#include <cc_Prefs.h>
extern ChipChopPrefsManager PrefsManager;
#include <LoraController.h>
extern LoraManager LoraController;
#include <TemperatureController.h>
extern TemperatureManager TemperatureController;
#include <VictronMonitor.h>
extern VictronManager VictronMonitor;
#include <cc_ds1820.h>
extern CC_DS1820 DS1820;
#include <MotorController.h>
extern MotorManager MotorController;
#include <CoolingController.h>
extern CoolingManager CoolingController;
#include <GPSMonitor.h>
extern GPSManager GPSMonitor;
// #include <GyroMonitor.h>
// extern GyroManager GyroMonitor;
#include <RTCManager.h>
extern RTCManager RTC;

class pluginsInit{

    public:
        
    void start(){ 

        PrefsManager.init();
        LoraController.init();
        TemperatureController.init();
        DS1820.init();
        VictronMonitor.init();
        MotorController.init();
        CoolingController.init();
        GPSMonitor.init();
        // GyroMonitor.init();
        RTC.init();

	}
};
