
#include <Arduino.h>
#include <WiFi.h>

#define CHIPCHOP_DEBUG true

#include <ChipChop_Config.h> 
#include <ChipChopEngine.h> 
extern ChipChopEngine ChipChop;
#include <ChipChopPlugins.h>
extern ChipChopPluginsManager ChipChopPlugins;
#include <SystemController.h> 
extern SystemManager SystemController;

#include <ChipChop_Includes.h>
extern json_mini JSON;


String server_uri = "wss://api3.chipchop.io/wsdev/";
String uuid = "ChpChpUsRapi3x3f1d594827f9403380022736f8461dff";
String auth_code = "50a1f9bd3bfa41d39b84554935c55837";
String device_id = "comrade_base";


const String _quote = "\"";

void ChipChop_onCommandReceived(String target_component,String value, String source, int command_age){
        Serial.println(target_component);
        Serial.println(value);

        //broadcast this event first to other buoys
        int obj_length = 3;
        json_mini::JSON_OBJ obj[obj_length] = {
            {"command",_quote + target_component + _quote},
            {"target","motor"}, 
            {"value",_quote + value + _quote}
        };
        String request = JSON.stringify(obj, obj_length);
        request = request + " ";


        #if MY_LORA_MODE == LORA_NODE || MY_LORA_MODE == LORA_BEACON
        if (value == "raise") {
            MotorController.setDirection(1);
            MotorController.setPower(1);
        } else if (value == "lower") {
            MotorController.setDirection(0);
            MotorController.setPower(1);
        } else {
            MotorController.setPower(0);
        }
        #endif

        #if MY_LORA_MODE == LORA_BASE
        LoraController.send(target_component, request, "set");
        #endif 

        ChipChop.updateStatus(target_component,value);
   

}

unsigned long heap_timer = 0;
unsigned long wifi_timer = 0;
unsigned long wifi_wait = 0;

void setup(){

        Serial.begin(115200);
        delay(3000);

//    #if MY_LORA_MODE == LORA_NODE || MY_LORA_MODE == LORA_BASE

//        WiFi.begin("Buldog 2.4", "&ojaZA=h37h0k?pha");
//        WiFi.begin("Pokedex", "asdfghjkl");

//        wifi_wait = millis();
//        Serial.print("WiFi Connecting");
//        while (WiFi.status() != WL_CONNECTED && millis() - wifi_wait < 30000)
//        {
//            delay(500);
//            Serial.print(".");
//        }
//        WifiPortal.ssid("Pokedex", "asdfghjkl");
//        WifiPortal.ssid("comrade_base", "1234567890");
        WifiPortal.ssid("Buldog 2.4", "&ojaZA=h37h0k?pha");
        delay(5000);


        ChipChop.debug(true); //set to false for production
        ChipChop.commandCallback(ChipChop_onCommandReceived);
        ChipChop.start(server_uri, uuid, device_id, auth_code);
//    #endif

    ChipChopPlugins.start();
    SystemController.start();

   
    
}

void loop(){
//    #if MY_LORA_MODE == LORA_NODE || MY_LORA_MODE == LORA_BASE
    ChipChop.run();

//    if ((WiFi.status() != WL_CONNECTED) && (millis() - wifi_timer > 1000)){
//        Serial.print("Reconnecting to Wifi...");
//        WiFi.reconnect();
//        while ((WiFi.status() != WL_CONNECTED) && (millis() - wifi_wait < 60000)){
//            delay(500);
//            Serial.print(".");
//        }
//        wifi_timer = millis();
//        wifi_wait = millis();
//    }
    
//    #endif
    
    ChipChopPlugins.run();
    SystemController.run();

    if(millis() - heap_timer > 10000){
        
        Serial.print("***  ");
        Serial.print(ESP.getFreeHeap());
        Serial.println(" bytes left ***");

        heap_timer = millis();
    }
}


