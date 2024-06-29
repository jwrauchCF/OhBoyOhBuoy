#include <Arduino.h>
#include <SystemController.h>
SystemManager SystemController;

#include <json_mini.h>
extern json_mini JSON;

#include <ChipChopEngine.h> 
extern ChipChopEngine ChipChop;

#include <ChipChopPlugins.h>
extern ChipChopPluginsManager ChipChopPlugins;

// #include <ChipChop_Includes.h> //<<< don't use here, creates an include loop 
#include <cc_Prefs.h>
extern ChipChopPrefsManager PrefsManager;
#include <LoraController.h>
extern LoraManager LoraController;
#include <TemperatureController.h>
extern TemperatureManager TemperatureController;
#include <VictronMonitor.h>
extern VictronManager VictronMonitor;
#include <MotorController.h>
extern MotorManager MotorController;
#include <CoolingController.h>
extern CoolingManager CoolingController;
#include <GPSMonitor.h>
extern GPSManager GPSMonitor;
#include <GyroMonitor.h>
extern GyroManager GyroMonitor;
#include <RTCManager.h>
extern RTCManager RTC;

SystemManager::SystemManager(){

}

void SystemManager::start(){

    LoraController.addListener(LORA_MESSAGE_RECEIVED,[](String message){
        SystemController.handleLoraMessage(message);
    });

    GyroMonitor.addListener(GYRO_VALUES,[](int AcX, int AcY, int AcZ){
        SystemController.handleGyroscope(AcX, AcY,AcZ);
    });

    TemperatureController.addListener(TEMPERATURE_NEW_READING,[](String sensor_name, float value){
        SystemController.handleTemperature(sensor_name,value);
    });
}

///**** LORA MESSAGE HANDLER ****////
void SystemManager::handleLoraMessage(const String& msg){

    Serial.print("msg = "); Serial.println(msg);
    String command = JSON.find(msg, "command"); 
    String target = JSON.find(msg, "target"); 
    String value = JSON.find(msg, "value"); 

    //TODO: this is where we can intercept the message and make decisions
    // we depending on the target component we can either chose to do some logic here
    if(target == "motor"){
        handleMotor(command, value);
    }
    // or we can just pass the message    
    // emit_event(SYSTEM_COMMAND_RECEIVED, target,command,value); 
    
}
//// END LORA MESSAGE HANDLER ////

/// MOTOR HANDLER ///
void SystemManager::handleMotor(String command, String value){
    if (value == "raise") {
        MotorController.setDirection(1);
        MotorController.setPower(1);
    } else if (value == "lower") {
        MotorController.setDirection(0);
        MotorController.setPower(1);
    } else {
        MotorController.setPower(0);
    }
}

/// GYROSCOPE HANDLER ///
void SystemManager::handleGyroscope(int AcX, int AcY, int AcZ){
    Serial.print("x: "); Serial.print(AcX); Serial.print(" y: "); Serial.print(AcY); Serial.print(" z: "); Serial.println(AcZ);
    
}

/// TEMPERATURE HANDLER ///
void SystemManager::handleTemperature(String sensor, int temperature){

    Serial.print(sensor + " °C: " );  
    Serial.println(temperature);

    if(sensor == GYRO){
        
    }
}




void SystemManager::run(){

}

///events handling //////
void SystemManager::addListener(byte event,listenerCallbackType cb){
        for(byte i = 0; i < SYSTEM_TOTAL_EVENTS; i++){
            if(_listener_events[i].event == event){
                for(byte x = 0; x < SYSTEM_TOTAL_CALLBACKS; x++){
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

void SystemManager::emit_event(byte event,String target,String command, String value){
    // SERIAL_LOG.println(event);
    for(byte i = 0; i < SYSTEM_TOTAL_EVENTS; i++){
        if(_listener_events[i].event == event){
            for(byte x = 0; x < SYSTEM_TOTAL_CALLBACKS; x++){
                if(_listener_events[i].callbacks[x]){
                    _listener_events[i].callbacks[x](target,command,value);
                }
            }

            return;
        }
    }
}
