#include <Arduino.h>
#include <VictronMonitor.h>
VictronManager VictronMonitor;

#include <ChipChopEngine.h> 
extern ChipChopEngine ChipChop;

#include <ChipChopPlugins.h>
extern ChipChopPluginsManager ChipChopPlugins;

VictronManager::VictronManager(){

}

void VictronManager::init(){
    ChipChopPlugins.addListener(PLUGINS_RUN,[](int val){
        VictronMonitor.run();
    });
}

void VictronManager::run(){

}

///events handling //////
void VictronManager::addListener(byte event,listenerCallbackType cb){
        for(byte i = 0; i < VICTRON_TOTAL_EVENTS; i++){
            if(_listener_events[i].event == event){
                for(byte x = 0; x < VICTRON_TOTAL_CALLBACKS; x++){
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

void VictronManager::emit_event(byte event){
    // SERIAL_LOG.println(event);
    for(byte i = 0; i < VICTRON_TOTAL_EVENTS; i++){
        if(_listener_events[i].event == event){
            for(byte x = 0; x < VICTRON_TOTAL_CALLBACKS; x++){
                if(_listener_events[i].callbacks[x]){
                    _listener_events[i].callbacks[x]();
                }
            }

            return;
        }
    }
}