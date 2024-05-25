
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
String device_id = "motor_test_1";


const String _quote = "\"";

#if MY_LORA_MODE == LORA_NODE
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

        LoraController.send("all",request,"set");


        if(target_component == "power"){
            if(value == "ON"){
                MotorController.setPower(1);
            }else{
                MotorController.setPower(0);
            }
            
        }else if(target_component == "direction"){
            if(value == "forward"){
                MotorController.goToTarget();
            }else{
                MotorController.goHome();
            }
            
        }else if(target_component == "speed"){
            MotorController.setSpeed(value.toInt());
        }

        ChipChop.updateStatus(target_component,value);
   

}
#endif

unsigned long heap_timer = 0;

void setup(){

        Serial.begin(115200);
        delay(1000);

    #if MY_LORA_MODE == LORA_NODE

        WiFi.begin("Buldog 2.4", "&ojaZA=h37h0k?pha");

        Serial.print("WiFi Connecting");
        while (WiFi.status() != WL_CONNECTED)
        {
            delay(500);
            Serial.print(".");
        }



        ChipChop.debug(true); //set to false for production
        ChipChop.commandCallback(ChipChop_onCommandReceived);
        ChipChop.start(server_uri, uuid, device_id, auth_code);
    #endif

    ChipChopPlugins.start();
    SystemController.start();

   
    
}

void loop(){
    #if MY_LORA_MODE == LORA_NODE
    ChipChop.run();
    #endif
    
    ChipChopPlugins.run();
    SystemController.run();

    if(millis() - heap_timer > 10000){
        
        Serial.print("***  ");
        Serial.print(ESP.getFreeHeap());
        Serial.println(" bytes left ***");

        heap_timer = millis();
    }
    
}


