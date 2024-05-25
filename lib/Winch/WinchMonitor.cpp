#include <Arduino.h>
#include <WinchMonitor.h>
WinchManager WinchController;

#include <ChipChopEngine.h> 
extern ChipChopEngine ChipChop;

#include <ChipChopPlugins.h>
extern ChipChopPluginsManager ChipChopPlugins;

WinchManager::WinchManager(){

}

void WinchManager::start(){
    ChipChopPlugins.addListener(PLUGINS_RUN,[](int val){
        WinchController.run();
    });
}

void WinchManager::run(){

}

///events handling //////
void WinchManager::addListener(byte event,listenerCallbackType cb){
        for(byte i = 0; i < WINCH_TOTAL_EVENTS; i++){
            if(_listener_events[i].event == event){
                for(byte x = 0; x < WINCH_TOTAL_CALLBACKS; x++){
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

void WinchManager::emit_event(byte event){
    // SERIAL_LOG.println(event);
    for(byte i = 0; i < WINCH_TOTAL_EVENTS; i++){
        if(_listener_events[i].event == event){
            for(byte x = 0; x < WINCH_TOTAL_CALLBACKS; x++){
                if(_listener_events[i].callbacks[x]){
                    _listener_events[i].callbacks[x]();
                }
            }

            return;
        }
    }
}