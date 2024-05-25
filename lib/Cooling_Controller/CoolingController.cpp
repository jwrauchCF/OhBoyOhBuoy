#include <Arduino.h>
#include <CoolingController.h>
CoolingManager CoolingController;

#include <ChipChopEngine.h> 
extern ChipChopEngine ChipChop;

#include <ChipChopPlugins.h>
extern ChipChopPluginsManager ChipChopPlugins;

CoolingManager::CoolingManager(){

}

void CoolingManager::init(){
    ChipChopPlugins.addListener(PLUGINS_RUN,[](int val){
        CoolingController.run();
    });
}

void CoolingManager::run(){

}

///events handling //////
void CoolingManager::addListener(byte event,listenerCallbackType cb){
        for(byte i = 0; i < COOLING_CONTROLLER_TOTAL_EVENTS; i++){
            if(_listener_events[i].event == event){
                for(byte x = 0; x < COOLING_CONTROLLER_TOTAL_CALLBACKS; x++){
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

void CoolingManager::emit_event(byte event){
    // SERIAL_LOG.println(event);
    for(byte i = 0; i < COOLING_CONTROLLER_TOTAL_EVENTS; i++){
        if(_listener_events[i].event == event){
            for(byte x = 0; x < COOLING_CONTROLLER_TOTAL_CALLBACKS; x++){
                if(_listener_events[i].callbacks[x]){
                    _listener_events[i].callbacks[x]();
                }
            }

            return;
        }
    }
}