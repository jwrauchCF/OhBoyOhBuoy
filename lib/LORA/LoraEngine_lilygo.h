#ifndef LORA_ENGINE_H
#define LORA_ENGINE_H

#include <Arduino.h>


class LoraEngine{
    
    using receiveCallbackType = void(*)(String, int);

    private:
        
        unsigned long sendTimeout = 0;

    public:

        receiveCallbackType receiveCallback;
        void setReceiveCallback(receiveCallbackType cb){
            receiveCallback = cb;
        }
        bool is_sending = 0;

        LoraEngine();
        void init();
        void run();
        void send(String request);

};

#endif