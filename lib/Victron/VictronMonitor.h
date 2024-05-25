#ifndef VICTRON_CONTROLLER_H
#define VICTRON_CONTROLLER_H

#include <Arduino.h>
#include <ChipChop_Config.h>

class VictronManager{

    private:
       using listenerCallbackType = void(*)();
        typedef struct{
            byte event = 0;
            listenerCallbackType callbacks[VICTRON_TOTAL_CALLBACKS];
        } _LISTENER;

        _LISTENER _listener_events[VICTRON_TOTAL_EVENTS];
        void emit_event(byte event);


    public:

        VictronManager();
        void init();
        void addListener(byte event,listenerCallbackType cb);
        void run();

};

#endif