#ifndef COOLING_CONTROLLER_H
#define COOLING_CONTROLLER_H

#include <Arduino.h>
#include <ChipChop_Config.h>

class CoolingManager{

    private:
       using listenerCallbackType = void(*)();
        typedef struct{
            byte event = 0;
            listenerCallbackType callbacks[COOLING_CONTROLLER_TOTAL_CALLBACKS];
        } _LISTENER;

        _LISTENER _listener_events[COOLING_CONTROLLER_TOTAL_EVENTS];
        void emit_event(byte event);


    public:

        CoolingManager();
        void init();
        void addListener(byte event,listenerCallbackType cb);
        void run();

};

#endif