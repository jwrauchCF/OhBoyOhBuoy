#include <Arduino.h>
#include <RTCManager.h>
RTCManager RTC;

#include <ChipChopEngine.h> 
extern ChipChopEngine ChipChop;

#include <ChipChopPlugins.h>
extern ChipChopPluginsManager ChipChopPlugins;

RTCManager::RTCManager(){

}

void RTCManager::init(){

}

void RTCManager::setTime(unsigned long timestamp){

}

unsigned long RTCManager::getTime(){
    return 0;
}

void RTCManager::setAlarm(int alarm_no, byte hour, byte minute){

}

///events handling //////
void RTCManager::addListener(byte event,listenerCallbackType cb){
        for(byte i = 0; i < RTC_TOTAL_EVENTS; i++){
            if(_listener_events[i].event == event){
                for(byte x = 0; x < RTC_TOTAL_CALLBACKS; x++){
                    if(!_listener_events[i].callbacks[x]){
                        _listener_events[i].callbacks[x] = cb;
                        return;
                    }
                }
            }else if(_listener_events[i].event == 0){
                _listener_events[i].event = event;
                _listener_events[i].callbacks[0] = cb;
                return;
            }
        }

        SERIAL_LOG.println(F("Motor: Exceeded number of event callbacks"));
}

void RTCManager::emit_event(byte event){
    // SERIAL_LOG.println(event);
    for(byte i = 0; i < RTC_TOTAL_EVENTS; i++){
        if(_listener_events[i].event == event){
            for(byte x = 0; x < RTC_TOTAL_CALLBACKS; x++){
                if(_listener_events[i].callbacks[x]){
                    _listener_events[i].callbacks[x]();
                }
            }

            return;
        }
    }
}