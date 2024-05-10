#ifndef CHIPCHOP_PLUGINS_H
#define CHIPCHOP_PLUGINS_H
#include <Arduino.h>
#include <ChipChop_Config.h>
//###include following plugins definitions
// class CC_KeepAlive;
// class ChipChopSavePrefs;
// class ChipChop_OTA;
// class WifiPortal;


class ChipChopPluginsManager{

    private:
        using listenerCallbackType = void(*)(int);
        // typedef std::function<void(int)> listenerCallbackType;
        
        typedef struct{
            byte event = 0;
            listenerCallbackType callbacks[PLUGINS_TOTAL_CALLBACKS];
        } _LISTENER;

        _LISTENER _listener_events[PLUGINS_TOTAL_EVENTS];

        void emit_event(byte event, int data);

        bool _chipchop_exists = 0;

    public:
        ChipChopPluginsManager();

        bool fileSystemOK = 0;
        bool test = 1;

        void start();
        void run();
        void addListener(byte event,listenerCallbackType cb);
        void initLittleFS();
       
        // WifiPortal* wifiPortal;
        // ChipChop_OTA* ota;


};

#endif