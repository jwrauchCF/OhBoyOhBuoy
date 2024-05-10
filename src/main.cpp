
#include <Arduino.h>
#include <WiFi.h>

#define CHIPCHOP_DEBUG true //set to false for production
#include <ChipChop_Config.h> 
#include <ChipChopEngine.h> 
extern ChipChopEngine ChipChop;
//Plugins included: ChipChop Engine,Keep Alive,Preferences Manager,OTA (Over the air updates),WiFi Portal,
#include <ChipChopPlugins.h>
extern ChipChopPluginsManager ChipChopPlugins;
#include <ChipChop_Includes.h>

//The connection credentials can be found in the device settings Dev Console > Devices
String server_uri = "wss://api3.chipchop.io/wsdev/";
String uuid = "ChpChpUsRapi3x3f1d594827f9403380022736f8461dff";
String auth_code = "50a1f9bd3bfa41d39b84554935c55837";
String device_id = "motor_test";

HardwareSerial RS485_Serial(1);

#include <ModbusMaster.h>
ModbusMaster modbus;
uint8_t modbus_result;
uint16_t comm = 0x03;
uint16_t param = 0x0D;

String power = "OFF";
String direction = "";
int speed = 0;
int active_speed = 0;
int current = 0;
float voltage = 0.0;
int health = 0;


typedef struct{
    int preset_speed = 0x0B;
    int active_speed = 0x0C;
    int current = 0x0D;
    int voltage = 0x0E;
    int health = 0x0F;
} _MODBUS_COMMANDS;

_MODBUS_COMMANDS modbus_command;

//not used but needs to be here
void preTransmission(){}
void postTransmission(){}
void setDirection(String val);
void setSpeed(int val);
void setPower(String val);

//////////////////

//NOTES:
// every successful command send/read should flag the general health status as "ok"
// on fail/error raise general alarm throughout the flock
// maybe before winching operation there should be first an "arming" stage when a read command is sent to modbus to assess the connection
// then if ok, return "ok" to the main Lora node and then the main node should issue a global "start" to all minions

void modbusSend(uint16_t command, uint16_t param){
    Serial.print("sending => ");
    Serial.print(command);
    Serial.print(param);

    modbus.begin(1,RS485_Serial);
    modbus.preTransmission(preTransmission);
    modbus.postTransmission(postTransmission);
    modbus_result = modbus.writeSingleRegister(command,param);

    //TODO: we should return the modbus response to the caller function
    // so we can verify if the command was successful and update the status 
    // or if it's a fail start some error mode chain of events...maybe try again first and then raise general alert through the entire flock

    if(modbus_result == modbus.ku8MBSuccess){
        int val = modbus.getResponseBuffer(0);

        Serial.print("modbus responded => ");
        Serial.println(val);
        modbus.clearResponseBuffer();
       
    }else{
        Serial.print("modbus response error =>");
        Serial.println(modbus_result);
        
    }
}

int_least64_t modbusRead(uint16_t command){

    Serial.print("request read => ");
    Serial.print(command);

    modbus.begin(1,RS485_Serial);
    modbus.preTransmission(preTransmission);
    modbus.postTransmission(postTransmission);

    modbus_result = modbus.readHoldingRegisters(command,1);
    if(modbus_result == modbus.ku8MBSuccess){
            int val = modbus.getResponseBuffer(0);

            Serial.print("modbus read response => ");
            Serial.println(val);

            modbus.clearResponseBuffer();
            return val;

       }else{
             Serial.print("modbus error reading =>");
             Serial.println(modbus_result);
            return -1;
       }
}

void setPower(String val){
    if(val == "ON"){
        //if no direction was set, go forward by default
        if(direction == ""){
            direction = "forward";
            ChipChop.triggerEvent("direction",direction);
            setDirection(direction);
            return;
        }
        //if no speed was set, go at 10%
        if(speed == 0){
            speed = 10;
            ChipChop.triggerEvent("speed",speed);
            setSpeed(speed);
            return;
        }

        comm = 0x03;
        param = 0x0D;
    }else{
        comm = 0x04;
        param = 0x00;
    }

    modbusSend(comm,param);
}

void setDirection(String val){

    //if motor is running, stop it and then change direction
    if(power == "ON"){
        setPower("OFF");
        ChipChop.triggerEvent("power","OFF");
    }
    if(val == "forward"){
        comm = 0x07;
        param = 0x01;
    }else{
        comm = 0x07;
        param = 0x14;
    }

    modbusSend(comm,param);
}

void setSpeed(int val){

    // no idea if we can set the speed whilst the motor is running?
    comm = 0x06;
    param = val;

    modbusSend(comm,param);

    // int test_speed = modbusRead(modbus_command.preset_speed);
}


unsigned long read_timer = 0;

void ChipChop_onCommandReceived(String target,String value, String command_source, int command_age){
    Serial.println(target);
    Serial.println(value);

    if(target == "power"){
        power = value;
        setPower(value);
    }else if(target == "direction"){
        direction = value;
        setDirection(direction);
    }else if(target == "speed"){
        speed = value.toInt();
        setSpeed(speed);
    }

    ChipChop.updateStatus(target,value);
   

}


void sendFullStatus(){
    ChipChop.updateStatus("power",power);
    ChipChop.updateStatus("direction",direction);
    ChipChop.updateStatus("speed",speed);
    ChipChop.updateStatus("active_speed",active_speed);
    ChipChop.updateStatus("current",current);
    ChipChop.updateStatus("voltage",voltage);
    ChipChop.updateStatus("health",health);
}


const char *SSID = "GlobeAtHome_d7538_2.4";
const char *password = "ykB3fSUy";

void setup(){
    Serial.begin(115200);
    RS485_Serial.begin(115200, SERIAL_8N1, 16, 17);
    delay(1000);

    WiFi.begin(SSID, password);

    Serial.print("WiFi Connecting");
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }


    ChipChop.debug(true); //set to false for production
    ChipChop.commandCallback(ChipChop_onCommandReceived);
    ChipChop.start(server_uri, uuid, device_id, auth_code);

    //Start all plugins
    ChipChopPlugins.start();

    sendFullStatus();

}

void loop(){

    ChipChop.run();
    ChipChopPlugins.run();

    if(millis() - read_timer > 10000){

        // active_speed = modbusRead(modbus_command.active_speed);
        // delay(1000);
        // current = modbusRead(modbus_command.current);
        // delay(1000);
        int v = modbusRead(modbus_command.voltage);
        // delay(1000);
        voltage = v * 0.1;
        // health = modbusRead(modbus_command.health);
        // delay(1000);

        read_timer = millis();

        sendFullStatus();
    }
    
}


