#include <Arduino.h>

#include <MotorController.h>
MotorManager MotorController;

#include <ChipChopEngine.h> 
extern ChipChopEngine ChipChop;

#include <ChipChopPlugins.h>
extern ChipChopPluginsManager ChipChopPlugins;

#include <SystemController.h>
extern SystemManager SystemController;

#include <VescUart.h>
VescUart UART;


MotorManager::MotorManager(){
  
}

void MotorManager::init(){
    ChipChopPlugins.addListener(PLUGINS_RUN,[](int val){
        MotorController.run();
    });

    // HardwareSerial motorSerial(MOTOR_SERIAL);
    Serial1.begin(115200, SERIAL_8N1, MOTOR_RX, MOTOR_TX);

    UART.setSerialPort(&Serial1);

    if (UART.getVescValues()) {
        Serial.println("VESC Data Setup Success!");
        Serial.print("RPM:");
        Serial.println(UART.data.rpm);
        Serial.print("Input Voltage:");
        Serial.println(UART.data.inpVoltage);
        Serial.print("Amp Hours:");
        Serial.println(UART.data.ampHours);

        Serial.println("VESC connection successful!");
      } else {
        Serial.println("VESC connection failed!");
      }
    
}

void MotorManager::setPower(bool val){
    if(val == 1){
        start_motor(current_RPM);
    }else{
        stop_motor();
    }
}

void MotorManager::start_motor(int rpm) {
    for (int r = minRPM; r < rpm; r += stepSize) {
        UART.setRPM(r);
    }
    UART.setRPM(rpm);
    emit_event(MOTOR_CONTROLLER_RUNNING);
}

void MotorManager::stop_motor() {
    for (int r = current_RPM; r >= minRPM; r -= stepSize) {
        UART.setRPM(r);
    }
    UART.setRPM(0);

    emit_event(MOTOR_CONTROLLER_STOPPED);
}


void MotorManager::setDirection(bool val, bool start_immediately){

    //if motor is running, stop it and then change direction
    if(power == 1){
        setPower(0);
        // ChipChop.triggerEvent("power","OFF");
    }
    if(val == 1){
        direction = 1;
        current_RPM = abs(current_RPM);
    }else{
        direction = 0;
        current_RPM = -1 * abs(current_RPM);
    }

    if(start_immediately){
        start_motor(current_RPM); 
    }

}

void MotorManager::setSpeed(int val){

    // no idea if we can set the speed whilst the motor is running?
    speed = val;
    int r = val * (maxRPM-minRPM) / 100 + minRPM;
    if ((r > minRPM) && (r < maxRPM)) {
        current_RPM = r;
    } else {
        current_RPM = 0;
    }
}

void MotorManager::getStatus(){
        UART.getVescValues();
        active_speed = UART.data.rpm / 4;
        current = UART.data.avgInputCurrent;
        voltage = UART.data.inpVoltage;

        //TODO: do an error check, if the UART.getVescValues() fails flag the health to an error and broadcast it as an event

}

void MotorManager::travelCheck(){
    //check the rotary encoder
    if(direction == 1){

        current_distance++; //<<< emulation, needs to come from the rotary encoder

        if(current_distance >= travel_distance){
            stop_motor();
        }
    }else{
        current_distance--;
        if(current_distance <= travel_distance){
            stop_motor();
        }
    }

    


}

void MotorManager::run(){
    if ((power == 1) && (millis() - vesc_push_timer > 50)) {
        UART.setRPM(current_RPM);

        //TODO: update the travel distance and once the target is reached stop the motor and broadcast the status

        vesc_push_timer = millis();
    }

    if(check_travel){
        if(millis() - travel_check_timer > TRAVEL_CHECK_INTERVAL){

            travelCheck();
            travel_check_timer = millis();
        }
    }
}

///events handling //////
void MotorManager::addListener(byte event,listenerCallbackType cb){
        for(byte i = 0; i < MOTOR_CONTROLLER_TOTAL_EVENTS; i++){
            if(_listener_events[i].event == event){
                for(byte x = 0; x < MOTOR_CONTROLLER_TOTAL_CALLBACKS; x++){
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

void MotorManager::emit_event(byte event){
    // SERIAL_LOG.println(event);
    for(byte i = 0; i < MOTOR_CONTROLLER_TOTAL_EVENTS; i++){
        if(_listener_events[i].event == event){
            for(byte x = 0; x < MOTOR_CONTROLLER_TOTAL_CALLBACKS; x++){
                if(_listener_events[i].callbacks[x]){
                    _listener_events[i].callbacks[x]();
                }
            }

            return;
        }
    }
}