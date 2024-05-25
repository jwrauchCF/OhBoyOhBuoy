#ifndef SYSTEM_CONTROLLER_H
#define SYSTEM_CONTROLLER_H

#include <Arduino.h>
#include <ChipChop_Config.h>

class SystemManager{

    private:
       using listenerCallbackType = void(*)(String, String, String);
        typedef struct{
            byte event = 0;
            listenerCallbackType callbacks[SYSTEM_TOTAL_CALLBACKS];
        } _LISTENER;

        _LISTENER _listener_events[SYSTEM_TOTAL_EVENTS];
        void emit_event(byte event,String target,String command, String value);
        void handleLoraMessage(const String& msg);

        void handleGyroscope(int AcX, int AcY, int AcZ);
        void handleTemperature(String sensor, int tmp);
        void handleMotor(String command, String value);
        

    public:

        SystemManager();
        void start();
        void addListener(byte event,listenerCallbackType cb);
        void run();

};

#endif