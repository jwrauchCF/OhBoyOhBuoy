#ifndef WINCH_CONTROLLER_H
#define WINCH_CONTROLLER_H

#include <Arduino.h>
#include <ChipChop_Config.h>

class WinchManager{

    private:
       using listenerCallbackType = void(*)();
        typedef struct{
            byte event = 0;
            listenerCallbackType callbacks[WINCH_TOTAL_CALLBACKS];
        } _LISTENER;

        _LISTENER _listener_events[WINCH_TOTAL_EVENTS];
        void emit_event(byte event);

        float target_depth = 100.0;
        float current_depth = 0.0;

        bool can_run = 0;
        void stop(){
            can_run = 0;
        }


    public:

        WinchManager();
        void start(){
            can_run = 1;
        }
        
        void addListener(byte event,listenerCallbackType cb);
        void run();
        void setTargetDepth(float depth){
            target_depth = depth;
        }

};

#endif