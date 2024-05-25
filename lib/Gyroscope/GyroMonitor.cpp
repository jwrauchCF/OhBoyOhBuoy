#include <Arduino.h>
#include <GyroMonitor.h>
GyroManager GyroMonitor;

#include <ChipChopEngine.h> 
extern ChipChopEngine ChipChop;

#include <ChipChopPlugins.h>
extern ChipChopPluginsManager ChipChopPlugins;

#include <TemperatureController.h>
extern TemperatureManager TemperatureController;

#include <Wire.h>


GyroManager::GyroManager(){

}

void GyroManager::init(){
    ChipChopPlugins.addListener(PLUGINS_RUN,[](int val){
        GyroMonitor.run();
    });

    TemperatureController.registerSensor(GYRO);

    Wire.begin();
    Wire.beginTransmission(MPU_addr);
    Wire.write(0x6B);  // PWR_MGMT_1 register
    Wire.write(0);     // set to zero (wakes up the MPU-6050)
    Wire.endTransmission(true);

}

void GyroManager::run(){

    if(millis() - last_sensor_read > read_interval){
        read();
        last_sensor_read = millis();
        emit_event(GYRO_VALUES,AcX,AcY,AcZ);

        TemperatureController.updateStatus(GYRO,temperature);
    }

}

void GyroManager::read(){
    Wire.beginTransmission(MPU_addr);
    Wire.write(0x3B);  // starting with register 0x3B (ACCEL_XOUT_H)
    Wire.endTransmission(false);
    Wire.requestFrom(MPU_addr,14,1);  // request a total of 14 registers
    AcX = Wire.read()<<8|Wire.read();  // 0x3B (ACCEL_XOUT_H) & 0x3C (ACCEL_XOUT_L)
    AcY = Wire.read()<<8|Wire.read();  // 0x3D (ACCEL_YOUT_H) & 0x3E (ACCEL_YOUT_L)
    AcZ = Wire.read()<<8|Wire.read();  // 0x3F (ACCEL_ZOUT_H) & 0x40 (ACCEL_ZOUT_L)
    temperature = Wire.read()<<8|Wire.read();  // 0x41 (TEMP_OUT_H) & 0x42 (TEMP_OUT_L)
    temperature = ((temperature)/340 + 36.53);

    // GyX = Wire.read()<<8|Wire.read();  // 0x43 (GYRO_XOUT_H) & 0x44 (GYRO_XOUT_L)
    // GyY = Wire.read()<<8|Wire.read();  // 0x45 (GYRO_YOUT_H) & 0x46 (GYRO_YOUT_L)
    // GyZ = Wire.read()<<8|Wire.read();  // 0x47 (GYRO_ZOUT_H) & 0x48 (GYRO_ZOUT_L)
    // Serial.print("Accelerometer Values: \n");
    // Serial.print("AcX: "); Serial.print(AcX); Serial.print("\nAcY: "); Serial.print(AcY); Serial.print("\nAcZ: "); Serial.print(AcZ);   
    // Serial.print("\nTemperature: " );  Serial.println(Tmp);
    // Serial.print("\nGyroscope Values: \n");
    // Serial.print("GyX: "); Serial.print(GyX); Serial.print("\nGyY: "); Serial.print(GyY); Serial.print("\nGyZ: "); Serial.print(GyZ);
    // Serial.print("\n");


}


///events handling //////
void GyroManager::addListener(byte event,listenerCallbackType cb){
        for(byte i = 0; i < GYRO_TOTAL_EVENTS; i++){
            if(_listener_events[i].event == event){
                for(byte x = 0; x < GYRO_TOTAL_CALLBACKS; x++){
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

void GyroManager::emit_event(byte event, int AcX, int AcY, int AcZ){
    // SERIAL_LOG.println(event);
    for(byte i = 0; i < GYRO_TOTAL_EVENTS; i++){
        if(_listener_events[i].event == event){
            for(byte x = 0; x < GYRO_TOTAL_CALLBACKS; x++){
                if(_listener_events[i].callbacks[x]){
                    _listener_events[i].callbacks[x](AcX,AcY,AcZ);
                }
            }

            return;
        }
    }
}
