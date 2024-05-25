#ifndef TEMPERATURE_CONTROLLER_H
#define TEMPERATURE_CONTROLLER_H

#include <Arduino.h>
#include <ChipChop_Config.h>

class TemperatureManager{

    private:
       using listenerCallbackType = void(*)(String sensor_name, float temperature);
        typedef struct{
            byte event = 0;
            listenerCallbackType callbacks[TEMPERATURE_CONTROLLER_TOTAL_CALLBACKS];
        } _LISTENER;

        _LISTENER _listener_events[TEMPERATURE_CONTROLLER_TOTAL_EVENTS];
        void emit_event(byte event, String sensor_name, float value);

        typedef struct{
            String name = "";
            float value = -1000.0;
        } _SENSOR;
        int _MAX_SENSORS =  TEMPERATURE_CONTROLLER_MAX_SENSORS;
        _SENSOR _SENSORS[TEMPERATURE_CONTROLLER_MAX_SENSORS];


    public:

        TemperatureManager();
        void init();
        void addListener(byte event,listenerCallbackType cb);
        void registerSensor(String name);
        void run();

        void updateStatus(String name, float value);

};

#endif