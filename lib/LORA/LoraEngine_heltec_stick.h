#ifndef LORA_ENGINE_H
#define LORA_ENGINE_H

#include <Arduino.h>


class LoraEngine{
    
    using receiveCallbackType = void(*)(String, int);

    private:
        
        

    public:

        receiveCallbackType receiveCallback;
        void setReceiveCallback(receiveCallbackType cb){
            receiveCallback = cb;
        }

        LoraEngine();
        void init();
        void run();
        void send(String request);

};

#endif