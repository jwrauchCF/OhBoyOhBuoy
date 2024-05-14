
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
String device_id = "motor_test_2";

#include <OneWire.h>
#include <DallasTemperature.h>
#define ONE_WIRE_BUS 6

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
int temperature = 0;

#include <VescUart.h>
#define RXD1 18
#define TXD1 17

VescUart UART;

String power = "OFF";
String direction = "";
int speed = 0;
int active_speed = 0;
int current = 0;
float voltage = 0.0;
int health = 0;

int maxRPM = 8000;
int minRPM = 50;
int setRPM = 0;
int stepSize = 50;
int delayTime = 20;

void setDirection(String val);
void setSpeed(int val);
void setPower(String val);
void accelerate(int val);
void deccelerate();

//////////////////

//NOTES:
// every successful command send/read should flag the general health status as "ok"
// on fail/error raise general alarm throughout the flock
// maybe before winching operation there should be first an "arming" stage when a read command is sent to modbus to assess the connection
// then if ok, return "ok" to the main Lora node and then the main node should issue a global "start" to all minions

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

        accelerate(setRPM);
    }else{
        deccelerate();
    }
}

void accelerate(int rpm) {
    for (int r = minRPM; r < rpm; r += stepSize) {
        UART.setRPM(r);
    }
    UART.setRPM(rpm);
}

void deccelerate() {
    for (int r = setRPM; r >= minRPM; r -= stepSize) {
        UART.setRPM(r);
    }
    UART.setRPM(0);
}


void setDirection(String val){

    //if motor is running, stop it and then change direction
    if(power == "ON"){
        setPower("OFF");
        ChipChop.triggerEvent("power","OFF");
    }
    if(val == "forward"){
        direction = "forward";
        setRPM = abs(setRPM);
    }else{
        direction = "reverse";
        setRPM = -1 * abs(setRPM);
    }
}

void setSpeed(int val){

    // no idea if we can set the speed whilst the motor is running?
    speed = val;
    setRPM = val * maxRPM / 100;
}


unsigned long power_timer = 0;
unsigned long status_timer = 0;
unsigned long wifi_timer = 0;
unsigned long wifi_wait = 0;

void ChipChop_onCommandReceived(String target,String value, String command_source, int command_age){
    Serial.println(target);
    Serial.println(value);

    if(target == "power"){
        power = value;
        setPower(value);
        ChipChop.updateStatus(target,value);
    }else if(target == "direction"){
        if (power == "OFF") {
            direction = value;
            setDirection(direction);
            ChipChop.updateStatus(target,value);
        }
    }else if(target == "speed"){
        if (power == "OFF") {
            speed = value.toInt();
            setSpeed(speed);
            ChipChop.updateStatus(target,value);
        }
    }

//    ChipChop.updateStatus(target,value);
   

}


void sendFullStatus(){
    ChipChop.updateStatus("power",power);
    ChipChop.updateStatus("direction",direction);
    ChipChop.updateStatus("speed",speed);
    ChipChop.updateStatus("active_speed",active_speed);
    ChipChop.updateStatus("current",current);
    ChipChop.updateStatus("voltage",voltage);
    ChipChop.updateStatus("health",temperature);
}


//const char *SSID = "Buldog 2.4";
//const char *password = "&ojaZA=h37h0k?pha";
const char *SSID = "Pokedex";
const char *password = "asdfghjkl";

void setup(){
    Serial.begin(9600);
    delay(1000);

    WiFi.begin(SSID, password);

    Serial.print("WiFi Connecting");
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }
    
    sensors.begin();

    Serial1.begin(115200, SERIAL_8N1, RXD1, TXD1);
    delay(1000);
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

    //To keep signaling the controller.  IDK how to handle this more cleanly.
    if ((power == "ON") && (millis() - power_timer > 50)) {
        UART.setRPM(setRPM);
        power_timer = millis();
    }

    if ((WiFi.status() != WL_CONNECTED) && (millis() - wifi_timer > 1000)){
        Serial.print("Reconnecting to Wifi...");
//        WiFi.disconnect();
        WiFi.reconnect();
        while ((WiFi.status() != WL_CONNECTED) && (millis() - wifi_wait < 60000)){
            delay(500);
            Serial.print(".");
        }
        wifi_timer = millis();
        wifi_wait = millis();
    }

    if(millis() - status_timer > 1000){

        Serial.println("Preforming status check.");

        UART.getVescValues();
        sensors.requestTemperatures();

        active_speed = UART.data.rpm / 4;
        Serial.print("Active Speed = ");
        Serial.println(active_speed);
        current = UART.data.avgInputCurrent;
        Serial.print("Current = ");
        Serial.println(current);
        voltage = UART.data.inpVoltage;
        Serial.print("Voltage = ");
        Serial.println(voltage);
        temperature = sensors.getTempCByIndex(0);
        Serial.print("Temperature = ");
        Serial.println(temperature);
        // delay(1000);

        status_timer = millis();

        sendFullStatus();
    }
    
}


