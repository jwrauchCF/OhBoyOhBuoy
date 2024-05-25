#ifndef LORA_CONTROLLER_H
#define LORA_CONTROLLER_H

#include <Arduino.h>
#include <ChipChop_Config.h>

class LoraManager{

    private:
       using listenerCallbackType = void(*)(String);
        typedef struct{
            byte event = 0;
            listenerCallbackType callbacks[LORA_TOTAL_CALLBACKS];
        } _LISTENER;

        _LISTENER _listener_events[LORA_TOTAL_EVENTS];
        

        typedef struct{
            byte isAlive;
            int lora_mode;
            String lora_id;
            unsigned long last_ping;
            int signal_strength;
        } _SWARM_DEVICE;

        _SWARM_DEVICE my_swarm[LORA_SWARM_SIZE] = {
            {0,LORA_BASE,"base",0,-1000},
            {0,LORA_NODE,"esp1",0,-1000},
            {0,LORA_BEACON,"esp2",0,-1000},
            {0,LORA_BEACON,"esp3",0,-1000},
            {0,LORA_BEACON,"esp4",0,-1000}
        };

        byte status = LORA_IDLE;

        int found(String id, int signal_strength);

        const String _quote = "\"";

        void send_ping();

        unsigned long ping_frequency = 30000;
        unsigned long last_ping_time = 0; 

    public:

        byte my_state;
        void emit_event(byte event,String data);

        LoraManager();
        void init();
        void addListener(byte event,listenerCallbackType cb);
        void run();
        void send(String target,String request,String command);
        void receive(String request, int signal_strength);
        void send_swarm_hello();
        

        void triggerEvent(String component_name,String status);
        void triggerEvent(String component_name,int status);
        void triggerEvent(String component_name,float status);

        void updateStatus(String component_name,String status);
        void updateStatus(String component_name,int status);
        void updateStatus(String component_name,float status);

};

#endif