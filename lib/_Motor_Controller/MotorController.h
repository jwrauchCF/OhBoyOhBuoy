#ifndef MOTOR_CONTROLLER_H
#define MOTOR_CONTROLLER_H

#include <Arduino.h>
#include <ChipChop_Config.h>

class MotorManager{

    private:
       using listenerCallbackType = void(*)();
        typedef struct{
            byte event = 0;
            listenerCallbackType callbacks[MOTOR_CONTROLLER_TOTAL_CALLBACKS];
        } _LISTENER;

        _LISTENER _listener_events[MOTOR_CONTROLLER_TOTAL_EVENTS];
        void emit_event(byte event);

        unsigned long vesc_push_timer = 0;

        unsigned long TRAVEL_CHECK_INTERVAL = 10000;
        unsigned long travel_check_timer = 0;
        bool check_travel = 0;

        bool power = 0;
        bool direction = 1; // 1. forward 0.back
        int speed = 20;

        int maxRPM = 8000;
        int minRPM = 900;
        int current_RPM = 0;
        int stepSize = 50;
        int delayTime = 20; //???

        float target_distance = 100.0; // main drop target 100 meters
        float travel_distance = 0.0; // 0 meters

        float current_distance = 0;

        int active_speed = 0;
        int current = 0;
        float voltage = 0.0;
        int health = 0;

        void travelCheck();

    public:

        MotorManager();
        void setDirection(bool val);
        void setSpeed(int val);
        void setPower(bool val);
        void start_motor(int val);
        void stop_motor();

        void sendStatus();
        void getStatus();


        void setDropDistance(float val){
            target_distance = val;
        }

        void preStartCheck(){
            getStatus();
            sendStatus();
        }

        void init();
        void addListener(byte event,listenerCallbackType cb);
        void run();

};

#endif
