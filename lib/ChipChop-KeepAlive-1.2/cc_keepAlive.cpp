#include <Arduino.h>
#include <cc_keepAlive.h>
CC_KeepAlive KeepAlive;

#ifdef ESP32
  #include <WiFi.h>
#elif defined(ARDUINO_UNOR4_WIFI)
    #include <WiFiS3.h>
#elif ESP8266
  #include <ESP8266WiFi.h>
#endif

#include <ChipChopEngine.h> 
extern ChipChopEngine ChipChop;

#include <ChipChopPlugins.h>
extern ChipChopPluginsManager ChipChopPlugins;


CC_KeepAlive::CC_KeepAlive(){

}
void CC_KeepAlive::init(){

    ChipChopPlugins.addListener(PLUGINS_RUN,[](int val){
            KeepAlive.run();
    });

    ChipChop.addListener(CC_CONNECTED,[](String a, String b, String c, int d){
            KeepAlive.chipchop_connected = 1;
            KeepAlive.keepAliveCheck();
    });

    ChipChop.addListener(CC_SOCKET_CLOSED,[](String a, String b, String c, int d){
            KeepAlive.chipchop_connected = 0;
            KeepAlive.keepAliveCheck();
    });
}

void CC_KeepAlive::keepAliveCheck(){

        if(WiFi.status() == WL_CONNECTED){
            
            if(_curr_connection_status == 0){
                _curr_connection_status = 1;
                _last_chipchop_connection_ok = millis();
            }

            if(millis() - _last_chipchop_connection_ok > _keep_alive_timeout){ 
                SERIAL_LOG.println(_restart_reason);
                _last_chipchop_connection_ok = millis();
                _curr_connection_status = 0;
                handle_restart();

            }else if(_curr_connection_status == 1 && chipchop_connected == 0){ //connecting to ChipChop
                SERIAL_LOG.println(F("Keep Alive => ChipChop passed WiFi connection stage"));
                //status can only be 1 after the first WiFi connect or if it was disconnected 
                _last_chipchop_connection_ok = millis();
                _curr_connection_status = 2;
                _restart_reason = "Restart after WiFi connected";

            }else if(_curr_connection_status == 2 && chipchop_connected == 0){ //we are in waiting for the timeout to either connect to ChipChop or restart
                //do nothing 
                
            }else if(_curr_connection_status == 2 && chipchop_connected == 1){ //just connected to ChipChop (hello & handshake)
                 
                _restart_try = 1;
                _restart_reason = "Restart after ChipChop just connected";
                SERIAL_LOG.println(F("Keep Alive => ChipChop just got connected"));
    
                _last_chipchop_connection_ok = millis(); //reset the timer
                _curr_connection_status = 3; //upgrade status to full

            }else if(_curr_connection_status == 3 && chipchop_connected == 1){ //ChipChop is running
                _last_chipchop_connection_ok = millis(); //keep resetting the timer

            }else if(_curr_connection_status == 3 && chipchop_connected == 0){ //we've lost ChipChop, restart countdown will start
                _curr_connection_status = 2;

                SERIAL_LOG.println(F("Keep Alive => ChipChop connection error detected"));
                _restart_reason = "Restart after ChipChop socket connection loss";
            }

        }else{ //no WiFi
            if(_curr_connection_status == 0 || _curr_connection_status != 1){
                //nothing
            }else{
                _curr_connection_status = 1;
            }
        }


}

//not used
void CC_KeepAlive::restartSockets(){

    ChipChop.restartSockets();
    _last_chipchop_connection_ok = millis();

}

void CC_KeepAlive::handle_restart(){
    if(_keep_alive_mode == KEEP_ALIVE_AUTO){
        
        if(_restart_try == 1){
            WiFi.disconnect();
            WiFi.reconnect();
            _restart_try++;
            _curr_connection_status = 0;
            _last_chipchop_connection_ok = millis();

        }else if(_restart_try == 2){
            #if defined(ESP32) || defined(ESP8266) 
                _restart_in_progress = 1;
                _restart_countdown = millis();
                emit_event(KEEP_ALIVE_RESTARTING);
            #endif 
            _restart_try = 1;
        }

    }else{
        // if(_keep_alive_mode == KEEP_ALIVE_SOCKETS){
        //         restartSockets();
        // }

        if(_keep_alive_mode == KEEP_ALIVE_WIFI){
                WiFi.disconnect();
                WiFi.reconnect();
                _curr_connection_status = 0;
                _last_chipchop_connection_ok = millis();
        }else if(_keep_alive_mode == KEEP_ALIVE_RESTART){
                //TODO: check for restarting other type of boards if poss
            #if defined(ESP32) || defined(ESP8266) 
                _restart_in_progress = 1;
                _restart_countdown = millis();
                emit_event(KEEP_ALIVE_RESTARTING);
            #else
                WiFi.disconnect();
                WiFi.reconnect();
            #endif 
            
        }
    }
      
}

void CC_KeepAlive::restart(){
    ChipChop.closeConnection();
    
    ESP.restart();
}

void CC_KeepAlive::cancelRestart(){
    _restart_in_progress = 0; //just on case it fails the restart!?
    _curr_connection_status = 0;
    _last_chipchop_connection_ok = millis();

}

void CC_KeepAlive::run(){


    // if(chipchop_exists == 0){
    //     // SERIAL_LOG.println("no chipchop");
    //     return;
    // }

    if(_restart_in_progress == 0){
        if(millis() - _check_timer > _check_interval){
            keepAliveCheck();
            _check_timer = millis();
        }

    }else{
        if(millis() - _restart_countdown > 1000){
            _restart_in_progress = 0; //just on case it fails the restart!?
            _curr_connection_status = 0;
            _last_chipchop_connection_ok = millis();

            ChipChop.closeConnection();

            ESP.restart();
        }

        
    }
}

///events handling //////
void CC_KeepAlive::addListener(byte event,listenerCallbackType cb){
        for(byte i = 0; i < KEEP_ALIVE_TOTAL_EVENTS; i++){
            if(_listener_events[i].event == event){
                for(byte x = 0; x < KEEP_ALIVE_TOTAL_CALLBACKS; x++){
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

        SERIAL_LOG.println(F("ChipChop => Keep Alive: Exceeded number of event callbacks"));
}

void CC_KeepAlive::emit_event(byte event){
    // SERIAL_LOG.println(event);
    for(byte i = 0; i < KEEP_ALIVE_TOTAL_EVENTS; i++){
        if(_listener_events[i].event == event){
            for(byte x = 0; x < KEEP_ALIVE_TOTAL_CALLBACKS; x++){
                if(_listener_events[i].callbacks[x]){
                    _listener_events[i].callbacks[x]();
                }
            }

            return;
        }
    }
}

//////////
