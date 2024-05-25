#include <Arduino.h>
#include <LoraController.h>
LoraManager LoraController;

#include <ChipChopEngine.h> 
extern ChipChopEngine ChipChop;

#include <ChipChopPlugins.h>
extern ChipChopPluginsManager ChipChopPlugins;

#include <json_mini.h>
extern json_mini JSON;


#if LORA_BOARD == LILYGO
    #include "LoraEngine_lilygo.h"
#elif LORA_BOARD == HELTEC_STICK
    #include <LoraEngine_heltec_stick.h>
#endif

extern LoraEngine loraEngine;

LoraManager::LoraManager(){
    
}

void LoraReceiveCallback(String packet, int signal_strength){
    LoraController.receive(packet, signal_strength);
}

void LoraManager::init(){
    ChipChopPlugins.addListener(PLUGINS_RUN,[](int val){
        LoraController.run();
    });

    loraEngine.setReceiveCallback(LoraReceiveCallback);
    loraEngine.init();

    send_swarm_hello();
}

void LoraManager::run(){
    loraEngine.run();

    if(millis() - last_ping_time > ping_frequency){
        for(byte i = 0; i < LORA_SWARM_SIZE;i++){
            if(my_swarm[i].lora_id != MY_LORA_ID){
                if(my_swarm[i].last_ping < (millis() - 30000)){
                    my_swarm[i].isAlive = 0;
                    my_swarm[i].signal_strength = -1000;
                }
            }
        }
        send("all","ok","ping");
        last_ping_time = millis(); 
    }

}

void LoraManager::send_swarm_hello(){

        int obj_length = 3;
        json_mini::JSON_OBJ obj[obj_length] = {
            {"comm","\"hlo\""},
            {"grp",_quote + MY_LORA_GROUP + _quote}, 
            {"id",_quote + MY_LORA_ID + _quote}
        };
      String request = JSON.stringify(obj, obj_length);
      request = request + " ";

      Serial.print("lora send => ");
      Serial.println(request);

      loraEngine.send(request);
      
}

int LoraManager::found(String id, int signal_strength){

    for(byte i = 0; i < LORA_SWARM_SIZE;i++){
        if(my_swarm[i].lora_id == id){
            my_swarm[i].isAlive = 1;
            my_swarm[i].signal_strength = signal_strength;
            return my_swarm[i].lora_mode;
        }
    }

    return false;
}

void LoraManager::receive(String request,int signal_strength){
      
      Serial.print("lora receive => ");
      Serial.print(request);
      Serial.print( " - ");
      Serial.println(signal_strength);

      String command = JSON.find(request, "comm"); 
      Serial.println(command);

      
      if(command != "undefined"){
            String id = JSON.find(request, "id"); 

            if(command == "ping"){
                found(id,signal_strength); //not relevant, just to mark the isAlive
                return;
            }

            if(command == "hlo"){
                String group = JSON.find(request, "grp");
                if(group != MY_LORA_GROUP){
                    return;
                }
                found(id,signal_strength); //not relevant, just to mark the isAlive
                return;
            }
                
            int sender_swarm_mode = found(id,signal_strength);
            if(sender_swarm_mode == 0){
                return;
            }
            String target = JSON.find(request, "trgt"); 
            String message = JSON.find(request, "msg");


            // I'm either a beacon receiving a message from the Node or I am the Node receiving my own message
            // any message to "all" should be already handled by the Node internally
            if(target == "all" && sender_swarm_mode == LORA_NODE && MY_LORA_MODE == LORA_BEACON){
               Serial.print("msg = ");
               Serial.println(message);
                LoraController.emit_event(LORA_MESSAGE_RECEIVED,message);
                return;
                
            }

            // I'm a beacon and this is a forwarded message from the base
            if(target == MY_LORA_ID && MY_LORA_MODE == LORA_BEACON && sender_swarm_mode == LORA_NODE){ 
                //pass the message to the SystemController for processing
                LoraController.emit_event(LORA_MESSAGE_RECEIVED,message);
                return;
            }

            // I'm a Lora Node and this is a message from a beacon
            if(target == MY_LORA_ID && MY_LORA_MODE == LORA_NODE && sender_swarm_mode == LORA_BEACON){

                LoraController.emit_event(LORA_MESSAGE_RECEIVED,message);
                //forward the message to Base
                send("base",message, command);
                return;
            }

            if(MY_LORA_MODE == LORA_NODE && sender_swarm_mode == LORA_BASE){
                //forward the request from the base to a target beacon or to the entire swarm
                if(target != MY_LORA_ID){

                    send(target,message, command);

                    //I'm a node and it's a message to the entire swarm from the base so I should also act on it
                    if(target == "all"){
                        LoraController.emit_event(LORA_MESSAGE_RECEIVED,message);
                    }
                    return;
                    
                ///I'm a node and it's a message sent to me personally from the base
                }else if(target == MY_LORA_ID){
                    LoraController.emit_event(LORA_MESSAGE_RECEIVED,message);
                    return;
                }

                
            }

      }


      
}

void LoraManager::send(String target,String msg, String command){

    int obj_length = 4;
    json_mini::JSON_OBJ obj[obj_length] = {
        {"trgt",_quote + target + _quote},
        {"comm",_quote + command + _quote},
        {"id",_quote + MY_LORA_ID + _quote},
        {"msg",_quote + msg + _quote},
        
    };
    String request = JSON.stringify(obj, obj_length);
    request = request + " ";
    Serial.print("lora send => ");
    Serial.println(request);

    loraEngine.send(request);

}



///events handling //////
void LoraManager::addListener(byte event,listenerCallbackType cb){
        for(byte i = 0; i < LORA_TOTAL_EVENTS; i++){
            if(_listener_events[i].event == event){
                for(byte x = 0; x < LORA_TOTAL_CALLBACKS; x++){
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

        SERIAL_LOG.println(F("Lora: Exceeded number of event callbacks"));
}

void LoraManager::emit_event(byte event, String data){
    // SERIAL_LOG.println(event);
    for(byte i = 0; i < LORA_TOTAL_EVENTS; i++){
        if(_listener_events[i].event == event){
            for(byte x = 0; x < LORA_TOTAL_CALLBACKS; x++){
                if(_listener_events[i].callbacks[x]){
                    _listener_events[i].callbacks[x](data);
                }
            }

            return;
        }
    }
}