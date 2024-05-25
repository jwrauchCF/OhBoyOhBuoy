#include <Arduino.h>
#include <TemperatureController.h>
TemperatureManager TemperatureController;

#include <ChipChopEngine.h> 
extern ChipChopEngine ChipChop;

#include <ChipChopPlugins.h>
extern ChipChopPluginsManager ChipChopPlugins;


TemperatureManager::TemperatureManager(){
 
}

void TemperatureManager::init(){
    ChipChopPlugins.addListener(PLUGINS_RUN,[](int val){
        TemperatureController.run();
    });


}

void TemperatureManager::registerSensor(String name){
    for(byte i = 0; i < TEMPERATURE_CONTROLLER_MAX_SENSORS; i++){
        if(_SENSORS[i].name == name){
            Serial.println("TemperatureController => error: Trying to register a sensor that already exists: " + name);
            return;
        }else if(_SENSORS[i].name == ""){
            _SENSORS[i].name = name;
            _SENSORS[i].value = -1000.0;
            return;
        }

    }
    Serial.println("TemperatureController => error: Trying to register too many sensors. Increase the TEMPERATURE_CONTROLLER_MAX_SENSORS in ChipChop_Config");
}

void TemperatureManager::updateStatus(String name, float value){
    for(byte i = 0; i < TEMPERATURE_CONTROLLER_MAX_SENSORS; i++){
        if(_SENSORS[i].name == name){
            _SENSORS[i].value = value;
            emit_event(TEMPERATURE_NEW_READING, name, value);
        }

    }
}

void TemperatureManager::run(){

}

///events handling //////
void TemperatureManager::addListener(byte event,listenerCallbackType cb){
        for(byte i = 0; i < TEMPERATURE_CONTROLLER_TOTAL_EVENTS; i++){
            if(_listener_events[i].event == event){
                for(byte x = 0; x < TEMPERATURE_CONTROLLER_TOTAL_CALLBACKS; x++){
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

        SERIAL_LOG.println(F("Temperature: Exceeded number of event callbacks"));
}

void TemperatureManager::emit_event(byte event, String sensor_name, float value){
    // SERIAL_LOG.println(event);
    for(byte i = 0; i < TEMPERATURE_CONTROLLER_TOTAL_EVENTS; i++){
        if(_listener_events[i].event == event){
            for(byte x = 0; x < TEMPERATURE_CONTROLLER_TOTAL_CALLBACKS; x++){
                if(_listener_events[i].callbacks[x]){
                    _listener_events[i].callbacks[x](sensor_name,value);
                }
            }

            return;
        }
    }
}