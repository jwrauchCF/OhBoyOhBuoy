#include <Arduino.h>
#include <LoRa_lilygo_lib.h>
#include <SPI.h>

#include <LoraEngine_lilygo.h>
LoraEngine loraEngine;


#define SCK     5    // GPIO5  -- SX1278's SCK
#define MISO    19   // GPIO19 -- SX1278's MISO
#define MOSI    27   // GPIO27 -- SX1278's MOSI
#define SS      18   // GPIO18 -- SX1278's CS
#define RST     14   // GPIO14 -- SX1278's RESET
#define DI0     26   // GPIO26 -- SX1278's IRQ(Interrupt Request)
#define LORA_BAND   915

LoraEngine::LoraEngine(){
    
}

void OnTxDone( void ){
    loraEngine.is_sending = 0;
    LoRa.receive();

    Serial.println("TX done");
}


void onReceive(int packetSize){
    if (packetSize == 0){
        return;
    }

    int signal_strength = LoRa.packetRssi();
    String incoming = ""; 
    while (LoRa.available()) {            // can't use readString() in callback, so
        incoming += (char)LoRa.read();      // add bytes one by one
    }

    loraEngine.receiveCallback(incoming, signal_strength);

}

void LoraEngine::init(){

    SPI.begin(SCK, MISO, MOSI, SS);
    LoRa.setPins(SS, RST, DI0);
    if (!LoRa.begin(LORA_BAND * 1E6)) {
        Serial.println("Starting LoRa failed!");
        while (1);
    }

    LoRa.onTxDone(OnTxDone);
    LoRa.onReceive(onReceive);
    LoRa.receive();

}

void LoraEngine::run(){
    // LoRa.receive();
    if(is_sending == 1){
        if(millis() - sendTimeout > 2000){
            Serial.println("LoRa error sending...2s timeout reached and sending didn't complete, switching to receive mode");
            is_sending = 0;
            LoRa.receive();
        }
    }
}


void LoraEngine::send(String request){
    is_sending = 1;
    sendTimeout = millis();
    LoRa.beginPacket();
    LoRa.print(request);
    LoRa.endPacket(true); //<<< NOTE: true = async non-blocking, needs testing

}