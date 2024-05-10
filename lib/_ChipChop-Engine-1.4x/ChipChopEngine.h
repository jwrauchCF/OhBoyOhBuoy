/****************************************************************************************************************************
  ChipChopEngine.h
  For ChipChop Arduino Library

  This library handles all the communication with ChipChop.io IoT engine automatically and exposes some useful little utilities that you can call from your main sketch or any other class.

  Built by ChipChop Gizmo - https://ChipChop.io/api_docs/chipchop-arduino-library 
  Licensed under GNU license

  Version 1.27

  Version Modified By   Date      Comments
  ------- -----------  ---------- -----------
  1.00    Gizmo        24/05/2022 Initial build to incorporate a number of smaller classes & utilities into a single portable class 
  1.24    Gizmo        25/06/2023 Changed to using Websockets2 Generic library (from ArduinoWebsockets 0.5.3) and provides a cut-down version for ESP8266/ESP32
  1.34    Gizmo        25/08/2023 Includes the WebSocket support for UNO R4 WiFi in the provided "Websockets2 Generic - Modified" library
                                  Improved initial connection handshake procedure to avoid long waits and system penalties when attempting reconnections during testing

  1.36    Gizmo        04/11/2023 Added the option to explicitly close the connection ChipChop
                                  Added the option to remove a component from the heartbeat so it's status won't be sent
                                  Added ChipChop.connected property

  1.40    Gizmo        17/02/2024 Library event management reworked 
                                  keepAlive() added - option 2 is ESP Only
                                  requestRate() added
                                  restart() added


 *****************************************************************************************************************************/


#ifndef CHIPCHOP_H
#define CHIPCHOP_H

#include <Arduino.h>
#include <ChipChop_Config.h>

class ChipChopEngine{

    using commandCallbackType = void (*)(String,String,String,int);
    using errorCallbackType = void (*)(String);
    using listenerCallbackType = void(*)(String,String,String,int);
        // typedef std::function<void(String,String,String,int)> listenerCallbackType;
    private:

        String  _SERVER = "";

        const String _quote = "\"";
        
        typedef struct{
            byte event = 0;
            listenerCallbackType callbacks[CC_TOTAL_CALLBACKS];
        } _LISTENER;

        _LISTENER _listener_events[CC_TOTAL_EVENTS];
        void emit_event(byte event, String a, String b, String c, int d);

        

        typedef struct{
            String event;
            String status;
        } _EVENT;

        bool _CAN_RUN = CC_CAN_RUN;

        _EVENT _send_buffer[11];
        int buffer_size = 0;
        unsigned long _send_rate = CC_SEND_RATE;
        unsigned long _send_pulse_timer = 0;
        void sendNextEvent();
        void addEventToBuffer(String type, String status);
        void popBufferQueue(int poz);
        void limitPopBufferQueue(String event, String status);
        void clearBufferQueue();

        bool _LOG_ENABLED = false;
        bool _LOG_RESPONSE_ONLY = false;
        
        String _UUID;
        String _DEVICE_ID;
        String _AUTH_CODE;

        unsigned long lastPollTime = 0;
       
        unsigned long _HEARTBEAT_INTERVAL = CC_HEARTBEAT_INTERVAL; // setting to 0 changes the device to a "status request" only mode
        unsigned long lastHeartbeatSendTime = 0;
        unsigned long lastDateTimeRefresh = 0;
        bool _can_send_heartbeat = 1;

        unsigned long eventSpacing = 0;
        unsigned long lastEventTime = 0;
        byte eventsSent = 0;

        unsigned long _SERVER_TIME = 0; // received server time in millis used to run a real time clock
        unsigned long _DEVICE_TIME_MILLIS = 0;
        
       void _respond_to_status_request(const String& message_id);
       void _302_api_redirect(const String& api_uri);

        bool socket_restart_mode = 0;
        unsigned long socket_restart_timer = 0;
        int socket_restart_timeout = 0;

        void _send_heartbeat();
        void _validate_trigger_event(String component_name);
        void _send_trigger_event(String status);

        void _connect_socket();

        String zeroPrefix(int num);

        void setComponentStatus(const String component, String value);
        String componentsToJson(String triggerEventComponent);

        unsigned long stringToLong(String input);
        float stringToFloat(String input);

        commandCallbackType commandReceivedCallback;
        bool hasCommandCallback = false;
        

        void sendCommandCallback(String &target, String &value, String &source, int &command_age){
            if(hasCommandCallback == true){
                commandReceivedCallback(target, value, source, command_age);
            }
        }

        errorCallbackType errorReceivedCallback;
        bool hasErrorCallback = false;
                
        void sendErrorCallback(String error){
            if(hasErrorCallback == true){
                errorReceivedCallback(error);
            }
        }


        void setTime();
        bool _use_clock = CC_USE_CLOCK;

        ///The basic MD5 functions
        typedef unsigned long MD5_u32plus;

        typedef struct {
            MD5_u32plus lo, hi;
            MD5_u32plus a, b, c, d;
            unsigned char buffer[64];
            MD5_u32plus block[16];
        } MD5_CTX;

        static unsigned char* MD5_make_hash(char *arg);
        static unsigned char* MD5_make_hash(char *arg,size_t size);
        static char* MD5_make_digest(const unsigned char *digest, int len);
        static const void *MD5_body(void *ctxBuf, const void *data, size_t size);
        static void MD5Init(void *ctxBuf);
        static void MD5Final(unsigned char *result, void *ctxBuf);
        static void MD5Update(void *ctxBuf, const void *data, size_t size);

    public:
        void addListener(byte event,listenerCallbackType cb);
        //These have to be set as "public" but they are used by internal plugins and websockets so should not be called from anywhere and should be considered private
        typedef struct{
            String key;
            String value;
        } _JSON;
        int _MAX_COMPONENTS = CC_MAX_COMPONENTS;
        _JSON _COMPONENTS[CC_MAX_COMPONENTS];
        void _process_server_message(const String& data); 
        void connectionClosed();
        void restartSockets();
        void pause();
        void unPause();
        ///////

        unsigned long lastReconnectTime = 0;
        bool connected = false;
        bool handshakeComplete = 0;

        ChipChopEngine();
        void run();

        void start(String server_uri, String uuid, String device_id, String auth_code );
        void closeConnection();
        
        void debug(bool enabled){
            _LOG_ENABLED = enabled;
        }
        void debug_response_only(bool enabled){
            _LOG_ENABLED = enabled;
            _LOG_RESPONSE_ONLY = enabled;
        }

        void requestRate(int rate){
            if(rate < 500){
                rate = 500;
            }
            _send_rate = rate;
        }

        void log(const String &data);
        void log(const __FlashStringHelper *ifsh){ 
            log(reinterpret_cast<const char *>(ifsh)); 
        }


        void commandCallback(commandCallbackType cb){
            commandReceivedCallback = cb;
            hasCommandCallback = true;
        }
        void errorCallback(errorCallbackType cb){
            errorReceivedCallback = cb;
            hasErrorCallback = true;
        }

  
        String JSON_find(const String& obj, const String& property);
        String JSON_stringify(_JSON obj[], int obj_length);
        void toRGB(const String& obj, int (&rgb)[3]);

        // set the heartbeat interval in seconds, the minimum interval is 10 seconds and anything faster
        // will incur system violations marked on the account and potential dropping of the communication channel
        void heartBeatInterval(int interval){
            if(interval < 10){
                interval = 10;
            }
            _HEARTBEAT_INTERVAL = interval * 1000; //convert to milliseconds
        }

        void triggerEvent(String component_name,String status);
        void triggerEvent(String component_name,int status);
        void triggerEvent(String component_name,float status);

        void updateStatus(String component_name,String status);
        void updateStatus(String component_name,int status);
        void updateStatus(String component_name,float status);

        void removeStatus(String component_name);

        struct time_struct{
            int year;
            int yearShort;
            int month;
            int day;
            int hour;
            int minute;
            int second;
            unsigned long timestamp; //UNIX timestamp in seconds
            String monthLong;
            String monthShort;
            String formatted;
        };

        time_struct Date;
        String dateFormat = CC_DATE_FORMAT;
        void setDateFormat(String format){
            dateFormat = format;
        };

        void useClock(bool use){
            _use_clock = use;
        }

};

// extern ChipChopEngine ChipChop;

#endif



