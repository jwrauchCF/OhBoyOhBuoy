#ifndef GYRO_MONITOR_H
#define GYRO_MONITOR_H

#include <Arduino.h>
#include <ChipChop_Config.h>

class GyroManager{

    private:
        using listenerCallbackType = void(*)(int AcX, int AcY, int AcZ);
        typedef struct{
            byte event = 0;
            listenerCallbackType callbacks[GYRO_TOTAL_CALLBACKS];
        } _LISTENER;

        _LISTENER _listener_events[GYRO_TOTAL_EVENTS];
        void emit_event(byte event, int AcX, int AcY, int AcZ);

        int MPU_addr = 0x68;  // I2C address of the MPU-6050
        int16_t AcX,AcY,AcZ,temperature,GyX,GyY,GyZ;

        unsigned long read_interval = 2000;
        unsigned long last_sensor_read = 0;
        void read();


    public: 

        GyroManager();
        void addListener(byte event,listenerCallbackType cb);
        void init();
        void run();

};

#endif