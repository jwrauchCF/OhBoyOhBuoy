#include <Arduino.h>
#include <heltec_unofficial.h>

#include <LoraEngine_heltec_stick.h>
LoraEngine loraEngine;




LoraEngine::LoraEngine(){
    
}

#define PAUSE               30
#define FREQUENCY           915.0       // for US
#define BANDWIDTH           250.0
#define SPREADING_FACTOR    10
#define TRANSMIT_POWER      22

#define BUFFER_SIZE        255 // Define the payload size here


String rxdata;
volatile bool rxFlag = false;
uint64_t minimum_pause;

char sendingPacket[BUFFER_SIZE];
char receivedPacket[BUFFER_SIZE];
int16_t signal_strength,rxSize;


void receive_flag() {
  rxFlag = true;
}

void LoraEngine::init(){

  heltec_setup();
  both.println("Radio init");
  radio.begin();
  radio.setDio1Action(receive_flag);
  both.printf("Frequency: %.2f MHz\n", FREQUENCY);
  radio.setFrequency(FREQUENCY);
  both.printf("Bandwidth: %.1f kHz\n", BANDWIDTH);
  radio.setBandwidth(BANDWIDTH);
  both.printf("Spreading Factor: %i\n", SPREADING_FACTOR);
  radio.setSpreadingFactor(SPREADING_FACTOR);
  both.printf("TX power: %i dBm\n", TRANSMIT_POWER);
  radio.setOutputPower(TRANSMIT_POWER);
  radio.startReceive(RADIOLIB_SX126X_RX_TIMEOUT_INF);
}

void LoraEngine::run(){
    if (rxFlag) {
        rxFlag = false;
        radio.readData(rxdata);
        if (_radiolib_status == RADIOLIB_ERR_NONE){

            loraEngine.receiveCallback(rxdata.c_str(), radio.getRSSI());
            
        }
        radio.startReceive(RADIOLIB_SX126X_RX_TIMEOUT_INF);
    }

}


void LoraEngine::send(String request){
    // request.toCharArray(sendingPacket, request.length());
    // Radio.Send((uint8_t *)sendingPacket, strlen(sendingPacket) );

    radio.transmit(request);
    radio.setDio1Action(receive_flag);
    radio.startReceive(RADIOLIB_SX126X_RX_TIMEOUT_INF);

}
