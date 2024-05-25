#ifndef RTC_MANAGER_H
#define RTC_MANAGER_H

#include <Arduino.h>
#include <ChipChop_Config.h>

class RTCManager{

    private:
       using listenerCallbackType = void(*)();
        typedef struct{
            byte event = 0;
            listenerCallbackType callbacks[RTC_TOTAL_CALLBACKS];
        } _LISTENER;

        _LISTENER _listener_events[RTC_TOTAL_EVENTS];
        void emit_event(byte event);


    public:

        RTCManager();
        void init();
        void setTime(unsigned long timestamp);
        unsigned long getTime();
        void setAlarm(int alarm_no, byte hour, byte minute);

        void addListener(byte event,listenerCallbackType cb);


};

#endif