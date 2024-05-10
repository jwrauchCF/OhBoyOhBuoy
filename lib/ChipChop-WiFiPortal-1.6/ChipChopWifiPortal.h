#ifndef CHIPCHOP_WIFI_PORTAL_H
#define CHIPCHOP_WIFI_PORTAL_H
    #include <Arduino.h>
    #include <ChipChop_Config.h>

    #ifdef ESP32
        #include <WiFi.h>
        #define Wifi_Wrong_Password_Status 4 //this is incorrect, doesn't exist on esp32
    #else
        #include <ESP8266WiFi.h>
        #define Wifi_Wrong_Password_Status 6
    #endif

   #include <DNSServer.h>


class ChipChopWifiPortal{

    private:

        // typedef std::function<void(int)> listenerCallbackType;
        using listenerCallbackType = void(*)(int);
        typedef struct{
            byte event = 0;
            listenerCallbackType callbacks[WIFI_PORTAL_TOTAL_CALLBACKS];
        } _LISTENER;

        _LISTENER _listener_events[WIFI_PORTAL_TOTAL_EVENTS];
        void emit_event(byte event, int data);

        using statusCallbackType = void (*)(uint8_t);
        statusCallbackType statusCallbackResponse;
        boolean hasCallback = 0;

        String softAP_ssid = WIFI_PORTAL_AP_SSID;
        String softAP_password = WIFI_PORTAL_AP_PASS;

        bool _start_chipchop_on_connect = WIFI_PORTAL_START_CHIPCHOP_ON_CONNECT;

        String credentials_saved;
        int credentials_origin = 0; // 0 not defined, 1 from file system, 2 from portal
        String ssids_found = "";

        String _current_ssid = "";
        String _current_password = "";

        int _connected_ssid_index = -1; // index from the saved credentials list

        // DNS server
        const byte DNS_PORT = 53;
        DNSServer dnsServer;
       
        unsigned int status = WL_IDLE_STATUS; // 0
        unsigned long lastReconnectMillis = 0;
        unsigned long _RECONNECT_INTERVAL = WIFI_PORTAL_RECONNECT_INTERVAL;

        void scanNetworks();
        void loadCredentials();
   
        void saveCredentials(int explicitSave = 0);
        void forgetCredentials(String ssid);

        //handle HTTP
        void handleWifi();
        void handleWifiSave();

        //*** SETTINGS ***///
        boolean _ready = 0;
        bool _started = 0; //needs to be initialised when running as a plugin
        //auto reconnect to last connected ssid
        boolean _auto_reconnect = WIFI_PORTAL_AUTO_RECONNECT;

        //only connect to the last succesfully connected ssid
        String _connect_to = WIFI_PORTAL_CONNECT_TO; // "ANY", ANY_KEEP_PREFERRED or LAST_CONNECTED

        // close the portal after certain time
        // note: portal is always started on loss of connection

        boolean _auto_restart = WIFI_PORTAL_AUTO_RESTART; // restart  the portal on connection drop
        unsigned long _portal_timeout = WIFI_PORTAL_TIMEOUT; 
        unsigned long _saved_portal_timeout = 180000; 
        unsigned long portalTimeoutMillis = 0;
        boolean _portal_running = 1;
        
        void getNextValidSSID();


    public:
       
        ChipChopWifiPortal();
        void init();
        void start();

        void addListener(byte event,listenerCallbackType cb);

        void run();

        

        void connectWifi();

        void closePortal();
        void restartPortal();
        String ssid();
        String password();
        String portalName();

        void statusCallback(statusCallbackType cb){
            statusCallbackResponse = cb;
            hasCallback = true;
        }

        void setTimeout(unsigned long millis){
            _portal_timeout = millis;
            _saved_portal_timeout = millis;
        }
        void autoReconnect(boolean reconnect = 1){
            _auto_reconnect = reconnect;
        }

         void autoRestart(boolean restart = 1){
            _auto_restart = restart;
        }
        
        void connectTo(String type){ // ANY, ANY_KEEP_PREFERRED or LAST_CONNECTED
            _connect_to = type;
        }
        void autoReconnectInterval(unsigned long timer){
            _RECONNECT_INTERVAL = timer;
        }

};


#endif



