#include <Arduino.h>
#include <ChipChop_Config.h>
#include <chipchop_ota.h>
ChipChop_ota ChipChop_OTA;

    #ifdef ESP32
        #include <WiFi.h>
        #include <HTTPClient.h>
        #include <HTTPUpdate.h>
    #else
        #include <ESP8266WiFi.h>
        #include <ESP8266HTTPClient.h>
        #include <ESP8266httpUpdate.h>
    #endif

#include <ChipChopEngine.h> 
extern ChipChopEngine ChipChop;

#include <ChipChopPlugins.h>
extern ChipChopPluginsManager ChipChopPlugins;

void ChipChop_OTA_update_started(){
    ChipChop.triggerEvent("ota","update started");
}

void ChipChop_OTA_update_finished() {
    
}

void ChipChop_OTA_update_progress(int cur, int total) {
    float percent = cur * 100 / total;
     SERIAL_LOG.print(percent);
     SERIAL_LOG.println("%");

}

void ChipChop_OTA_update_error(int err) {
    // Serial.println("error ota");
    ChipChop.triggerEvent("ota","error");
    ChipChop.updateStatus("ota",ChipChop_OTA.OTA_VERSION);

}

ChipChop_ota::ChipChop_ota(){

    // ChipChopPlugins.addListener(PLUGINS_STARTED,[](int val){
    //         ChipChop_OTA.start();
            
    // });

    //  ChipChop.updateStatus("ota",ChipChop_ota.OTA_VERSION);

    #ifdef ESP32
            httpUpdate.onStart(ChipChop_OTA_update_started);
            httpUpdate.onEnd(ChipChop_OTA_update_finished);
            httpUpdate.onProgress(ChipChop_OTA_update_progress);
            httpUpdate.onError(ChipChop_OTA_update_error);
      #else
            ESPhttpUpdate.setClientTimeout(2000);
            ESPhttpUpdate.onStart(ChipChop_OTA_update_started);
            ESPhttpUpdate.onEnd(ChipChop_OTA_update_finished);
            ESPhttpUpdate.onProgress(ChipChop_OTA_update_progress);
            ESPhttpUpdate.onError(ChipChop_OTA_update_error);
      #endif

}
void ChipChop_ota::init(){
   
    ChipChop.updateStatus("ota",ChipChop_OTA.OTA_VERSION);

    ChipChop.addListener(CC_COMMAND_RECEIVED,[](String target_component,String command_value, String command_source, int command_age){
        if(target_component == "ota"){
            ChipChop_OTA.downloadUpdate(command_value);
        }
 
    });

}

void ChipChop_ota::downloadUpdate(String uri){

    WiFiClient wifi_client;
     #ifdef ESP32
            t_httpUpdate_return ret = httpUpdate.update(wifi_client, uri);
      #else
            t_httpUpdate_return ret = ESPhttpUpdate.update(wifi_client, uri);
      #endif


    switch (ret) {
         #ifdef ESP32
            case HTTP_UPDATE_FAILED: 
            Serial.printf("HTTP_UPDATE_FAILED Error (%d): %s\n", httpUpdate.getLastError(), httpUpdate.getLastErrorString().c_str());
            ChipChop_OTA_update_error(1);

         #else
            case HTTP_UPDATE_FAILED: Serial.printf("HTTP_UPDATE_FAILED Error (%d): %s\n", ESPhttpUpdate.getLastError(), ESPhttpUpdate.getLastErrorString().c_str()); 
            ChipChop_OTA_update_error(1);
         #endif
        
        break;

        case HTTP_UPDATE_NO_UPDATES: Serial.println("HTTP_UPDATE_NO_UPDATES"); 
        break;

        case HTTP_UPDATE_OK: Serial.println("HTTP_UPDATE_OK"); 
        break;
    }

}

