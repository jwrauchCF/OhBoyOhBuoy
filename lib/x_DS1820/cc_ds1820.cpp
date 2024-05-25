#include <Arduino.h>
#include <ChipChop_Config.h>
#include <cc_ds1820.h>
CC_DS1820 DS1820;

#include <OneWire.h>
#include <DallasTemperature.h>

#include <ChipChopPlugins.h>
extern ChipChopPluginsManager ChipChopPlugins;

#include <TemperatureController.h>
extern TemperatureManager TemperatureController;

OneWire DS1820_oneWire(DS1820_PIN);
DallasTemperature DS1820_sensors(&DS1820_oneWire);

CC_DS1820::CC_DS1820(){

}

void CC_DS1820::init(){
    for(byte i = 0; i < DS1820_TOTAL_SENSORS; i++){
        TemperatureController.registerSensor(_SENSORS[i].name); //register this sensor withe the TemperatureController
    }

    ChipChopPlugins.addListener(PLUGINS_RUN,[](int val){
        DS1820.run();
    });
    
}

void CC_DS1820::start(){

    DS1820_sensors.begin();
    sensors_found = DS1820_sensors.getDeviceCount();

    if(sensors_found > 0){
        _ready = 1;
        SERIAL_LOG.print("DS1820 => Sensors found: ");
        SERIAL_LOG.println(sensors_found);
    }else{
        SERIAL_LOG.print("DS1820 problem => No sensors found on the OneWire");
    }

    
}


float CC_DS1820::getTemperature(String component_name){
    for(byte i = 0; i < sensors_found; i++){
        if(_SENSORS[i].name == component_name){
            return _SENSORS[i].value;
        }
    }

    return -1000.0;
}


void CC_DS1820::run(){

    if(_ready){
        if(millis() - _check_timer >= _check_interval){
            DS1820_sensors.requestTemperatures(); 
            for(byte i = 0; i < sensors_found; i++){
                float temp = DS1820_sensors.getTempCByIndex(i);
                _SENSORS[i].value = temp;
                TemperatureController.updateStatus(_SENSORS[i].name,temp);
            }
            _check_timer = millis();
        }
    }
}

//////////


