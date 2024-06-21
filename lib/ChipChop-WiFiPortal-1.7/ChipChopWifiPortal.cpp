#include <Arduino.h>
#include <ChipChop_Config.h> 

#include <ChipChopWifiPortal.h>
ChipChopWifiPortal WifiPortal;

#ifdef ESP32
    #include <WiFi.h>
    #include <WebServer.h>
    #include <ESPmDNS.h>
    WebServer server(80);
#else
    #include <ESP8266WiFi.h>
    #include <ESP8266WebServer.h>
    #include <ESP8266mDNS.h>
    ESP8266WebServer server(80);
#endif

// #include <LittleFS.h>
#include <portal_html.h>

#include <WiFiClient.h>
#include <DNSServer.h>

#include <ChipChopEngine.h> 
extern ChipChopEngine ChipChop;

#include <ChipChopPlugins.h>
extern ChipChopPluginsManager ChipChopPlugins;

#include <cc_Prefs.h>
extern ChipChopPrefsManager PrefsManager;

/* Soft AP network parameters */
IPAddress apIP(192, 168, 30, 1);
IPAddress netMsk(255, 255, 255, 0);

ChipChopWifiPortal::ChipChopWifiPortal(){

}

String ChipChopWifiPortal::portalName(){
    return softAP_ssid;
}

void ChipChopWifiPortal::init(){
    ChipChopPlugins.addListener(PLUGINS_RUN,[](int val){
            WifiPortal.run();
    });

    ChipChopPlugins.addListener(PLUGINS_STARTED,[](int val){
            // ChipChopPlugins.initLittleFS();
            WifiPortal.start();  
    });

}

void ChipChopWifiPortal::start(){

    if(_start_chipchop_on_connect){
        ChipChop.pause();
    }
    _started = 1;

    emit_event(WIFI_PORTAL_STATUS, WIFI_PORTAL_STARTING);

    if(String(WIFI_PORTAL_CUSTOM_HOSTNAME) != ""){
        #ifdef ESP32
            WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE, INADDR_NONE);
            WiFi.setHostname(WIFI_PORTAL_CUSTOM_HOSTNAME);
        #else
            WiFi.hostname(WIFI_PORTAL_CUSTOM_HOSTNAME);
        #endif
    }

    if(WIFI_PORTAL_USE_MDNS == 1 && String(WIFI_PORTAL_MDNS_DOMAIN_NAME) !=""){
        if (!MDNS.begin(WIFI_PORTAL_MDNS_DOMAIN_NAME))
            {
                SERIAL_LOG.println("Error setting up MDNS responder!");
                while (1)
                {
                    delay(1000);
                }
            }

        MDNS.addService("http", "tcp", 80);
    }

    WiFi.mode(WIFI_AP_STA); //set the mode to station and softAP 

    SERIAL_LOG.println(F("Configuring access point..."));
    WiFi.softAPConfig(apIP, apIP, netMsk);
    WiFi.softAP(WIFI_PORTAL_HOTSPOT_SSID, WIFI_PORTAL_HOTSPOT_PASSWORD);
    // delay(500);  // Without delay I've seen the IP address blank
   
    SERIAL_LOG.print(F("AP IP address: "));
    SERIAL_LOG.println(WiFi.softAPIP());

    /* Setup the DNS server redirecting all the domains to the apIP */
    dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
    dnsServer.start(DNS_PORT, "*", apIP);

    /* Setup web pages: root, wifi config pages, SO captive portal detectors and not found. */
    server.on("/", [this](){ handleWifi(); });
    server.on("/wifi", [this](){ handleWifi(); });
    server.on("/hotspot-detect.html", [this](){ handleWifi(); }); //<<< Apple motherfucker keeps requesting this page non stop
    server.on("/wifisave", [this](){handleWifiSave(); });
    server.on("/generate_204", [this](){ handleWifi(); });  // Android captive portal. Maybe not needed. Might be handled by notFound handler.
    server.on("/fwlink", [this](){ handleWifi(); });        // Microsoft captive portal. Maybe not needed. Might be handled by notFound handler.
    server.onNotFound([this](){handleWifi();});
    server.begin();  // Web server start
    SERIAL_LOG.println(F("HTTP server started"));

     if(_portal_timeout != 0){
        portalTimeoutMillis = millis();
     }


    loadCredentials();
}


void ChipChopWifiPortal::run(){


    if(_auto_reconnect == 1 && _RECONNECT_INTERVAL > 0){
    //   if(_ready == 1){
        if(millis() - lastReconnectMillis > _RECONNECT_INTERVAL){
          if(WiFi.status() != WL_CONNECTED) {
            SERIAL_LOG.println(F("Trying to reconnect"));

            emit_event(WIFI_PORTAL_STATUS, WIFI_PORTAL_DISCONNECTED);

            if(_current_ssid != ""){ 
                SERIAL_LOG.print(F("connecting to: "));
                SERIAL_LOG.println(_current_ssid);

                if(_start_chipchop_on_connect){
                    ChipChop.pause();
                  }

                emit_event(WIFI_PORTAL_STATUS, WIFI_PORTAL_RECONNECTING);
                // WiFi.disconnect();
                WiFi.begin((const char*)_current_ssid.c_str(), (const char*)_current_password.c_str());

                int connRes = WiFi.waitForConnectResult(10000);
                if (connRes == WL_CONNECTED){
                  emit_event(WIFI_PORTAL_STATUS, WIFI_PORTAL_CONNECTED);

                  SERIAL_LOG.println(String("Connected to ssid: " + _current_ssid));
                  if(_start_chipchop_on_connect){
                    ChipChop.unPause();
                  }

                }else{
                  if(_connect_to != "LAST_CONNECTED"){
                    _connected_ssid_index = -1;
                    if(_start_chipchop_on_connect){
                        ChipChop.pause();
                    }
                    getNextValidSSID();
                  }
                }

            }else{
                _connected_ssid_index = -1;
                if(_start_chipchop_on_connect){
                    ChipChop.pause();
                }
                getNextValidSSID();
            }

            if(_auto_restart == 1 && _portal_running == 0){
              restartPortal();
            }
          }else{

            // SERIAL_LOG.println(String("Connected to ssid: " + _current_ssid));
          }

          lastReconnectMillis = millis();
        }
    //   }
    }


    if(_portal_timeout > 0){
      if(millis() - portalTimeoutMillis > _portal_timeout){
        closePortal();
      }
    }

    if(_portal_running == 1){
      dnsServer.processNextRequest();
      server.handleClient();
    }

   
}

void ChipChopWifiPortal::closePortal(){
    emit_event(WIFI_PORTAL_STATUS, WIFI_PORTAL_CLOSING);

    SERIAL_LOG.println(F("Closing Portal"));
   _portal_running = 0;
    _portal_timeout = 0;
    WiFi.mode(WIFI_STA); // close the portal by setting the mode to station only

}

void ChipChopWifiPortal::restartPortal(){
    emit_event(WIFI_PORTAL_STATUS, WIFI_PORTAL_RESTARTING);

    SERIAL_LOG.println(F("restarting portal"));
     WiFi.mode(WIFI_AP_STA); // open the portal by setting the mode to AP_STA

    _portal_running = 1;
    _portal_timeout = _saved_portal_timeout;
    portalTimeoutMillis = millis();
   

}

void ChipChopWifiPortal::connectWifi(){
   
  if(_current_ssid == ""){ // only check for ssid as it can be an open network wthout a password
    return;
  }

  // digitalWrite(BUILT_IN_LED,LOW); //light the blue led when not connected
  emit_event(WIFI_PORTAL_STATUS, WIFI_PORTAL_CONNECTING);


  SERIAL_LOG.print(F("Connecting to wifi..."));
//   WiFi.disconnect();
  SERIAL_LOG.println((const char*)_current_ssid.c_str()); 

  WiFi.begin((const char*)_current_ssid.c_str(), (const char*)_current_password.c_str());
  int connRes = WiFi.waitForConnectResult(5000);
  SERIAL_LOG.print(F("Connection response: " ));
  SERIAL_LOG.println(connRes);

  if(connRes != WL_CONNECTED){ // status 6 is WL_CONNECT_WRONG_PASSWORD but only implemented for esp8266 not esp32

    if(_start_chipchop_on_connect){
        ChipChop.pause();
    }
    // clear wrong credentials so we are not constantly trying to re-connect
    _current_ssid = "";
    _current_password = "";

    if(_connect_to == "LAST_CONNECTED"){ //allways connect only to the first ssid in the saved list
        //don't do anything, the autoconnect will pickup the next reconnect just set the next ssid to be the first in the list
        _ready = 1; //allow the autoconnect to start

    }else{//it's a _connect_to = "ANY" or "ANY_KEEP_PREFERRED"
          getNextValidSSID();
    }

  }else if (connRes == WL_CONNECTED){
    server.send(200, "text/html", "<strong style='font-size:30px;'>Your device is now WiFi connected!<br><br>You can now disconnect from this hotspot and re-connect your phone to your normal WiFi.</strong>");
    server.client().stop();  // Stop is needed because we sent no content length
    
    emit_event(WIFI_PORTAL_STATUS, WIFI_PORTAL_CONNECTED);

    saveCredentials();
    _ready = 1;
    _connected_ssid_index = -1;
      
    SERIAL_LOG.println(String("Connected to SSID:" + _current_ssid));

    if(_start_chipchop_on_connect){
        ChipChop.unPause();
    }
  }
}

//NOT USED: can be used maybe from the portal to explicitly clean the saved credentials list
// important, make sure to adjust the connected_ssid_index 
// void ChipChopWifiPortal::forgetCredentials(String ssid) {
//   SERIAL_LOG.println("Removing incorrect credentials");

//   unsigned int start = credentials_saved.indexOf("#" + ssid + ",");
//   int end = credentials_saved.indexOf("#",start + 1);
//   unsigned int length = end - start;
//   credentials_saved.remove(start + 1,length);

//   File file = LittleFS.open("/wifi.txt", "w");
//   file.print(credentials_saved);
//   file.close();

//   SERIAL_LOG.println(credentials_saved);
// }

String ChipChopWifiPortal::ssid(){
    return _current_ssid;
}

String ChipChopWifiPortal::password(){
    return _current_password;
}
/** Store WLAN credentials to file wifi.txt */
void ChipChopWifiPortal::saveCredentials(int explicitSave) {


  if(explicitSave == 0 && _connect_to == "ANY_KEEP_PREFERRED"){
    return;
  }

  SERIAL_LOG.println(F("Updating credentials"));
  emit_event(WIFI_PORTAL_STATUS, WIFI_PORTAL_SAVING_CREDENTIALS);

  // check if same credentials already exist. 
  String test_ssid = "#";
  test_ssid += _current_ssid;
  test_ssid += ",";

  String test_full = test_ssid;
  test_full += _current_password;
  test_full += "#";

  int exists_at = credentials_saved.indexOf(test_full);

  if(exists_at != -1 && exists_at == 0){//Credentials already exists, don't save anything
    //   SERIAL_LOG.println(F("Credentials already exists, don't save anything"));
      return;
  }

  if(credentials_saved.indexOf(test_ssid) != -1){ // ssid exists, remove and shift later to first position
    //   SERIAL_LOG.println(F("Updating credentials to new ones. Removing old one first."));
      
      unsigned int start = credentials_saved.indexOf(test_ssid);
      int end = credentials_saved.indexOf("#",start + 1);
      unsigned int length = end - start;
      credentials_saved.remove(start + 1,length);

  }

  String temp_credentials = "#";
  temp_credentials += _current_ssid;
  temp_credentials += ",";
  temp_credentials += _current_password;
  if(credentials_saved.length() == 0){
     temp_credentials += "#";
  }
  

  credentials_saved = temp_credentials + credentials_saved;

  PrefsManager.save("wifiportal","wifi",credentials_saved);

  _connected_ssid_index = 0;
}

/** Load WLAN credentials from file wifi.txt */
void ChipChopWifiPortal::loadCredentials(){


    emit_event(WIFI_PORTAL_STATUS, WIFI_PORTAL_LOADING_CREDENTIALS);

    credentials_saved = PrefsManager.get("wifiportal","wifi", true); //create the file if it doesn't exists
    
    SERIAL_LOG.println(F("Credentials_loaded"));

    if(_current_ssid != ""){
        saveCredentials(1);
    }
    //   SERIAL_LOG.print(">>");
    //   SERIAL_LOG.println(credentials_saved);

    getNextValidSSID();


}

void ChipChopWifiPortal::getNextValidSSID(){

    // SERIAL_LOG.println("getNextValidSSID");

    if(credentials_saved.length() == 0){
        return;
    }

    scanNetworks();

    // loop through saved ssids and check if it exists in the scanned networks
    // if it does, try to connect 
    // NOTE: for other systems if the credentials don't match start the portal or once connected dispose of the portal 
    int start = 0, end = 0; // start from byte 1 to skip the starting #
    int pp = credentials_saved.length() -1;

    int ssid_index_counter = -1;
    // int total_saved = 0;
    // for(int i = 0; i <= pp; i++){
    //   if(credentials_saved.charAt(i) == ','){
    //     total_saved++;
    //   }
    // }

    
    
    for(int i = -1; i <= pp; i++){
        if(credentials_saved.charAt(i) == ','){
            end = i ;
            String saved_ssid = credentials_saved.substring(start, end);
            SERIAL_LOG.println(saved_ssid);
            ssid_index_counter++;

            int found = ssids_found.indexOf("#" + saved_ssid + "#"); 
            if(found != -1 && ssid_index_counter > _connected_ssid_index){

                _current_ssid = saved_ssid;
                start = end + 1;
                end = credentials_saved.indexOf("#",end);

                _current_password = credentials_saved.substring(start, end);

                credentials_origin = 1;

                _connected_ssid_index = ssid_index_counter;

                // SERIAL_LOG.println(saved_ssid);
                SERIAL_LOG.print(F("Found saved credentials for: "));
                SERIAL_LOG.println(_current_ssid);
                
                break;
            }

            
        }else if(credentials_saved.charAt(i) == '#' && i != pp){ // end of entry but more entries to follow
            start = i + 1;
        }
    }

    if(_current_ssid != ""){ // only check for ssid as it can be an open network wthout a password 
            connectWifi();

    }else{ // no matching credentials found in the current scan, activate the run loop
        _ready = 1;
    }

}

void ChipChopWifiPortal::scanNetworks(){
 
  ssids_found = "#";

  int n = WiFi.scanNetworks();
  // SERIAL_LOG.println("scan done");
  if (n > 0) {
    for (int i = 0; i < n; i++) {
       ssids_found += WiFi.SSID(i); 
       ssids_found += "#"; // note, the string starts and ends with "#" so the ssids can quickly be searched for indexOf("#" + name + "#")
    }
  }

  SERIAL_LOG.print(F("Networks found: "));
  SERIAL_LOG.println(ssids_found);
}


/** Wifi config page handler */
void ChipChopWifiPortal::handleWifi() {
  server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  server.sendHeader("Pragma", "no-cache");
  server.sendHeader("Expires", "-1");

  String Page = String(PORTAL_HTML);

  if(_current_ssid != ""){

      String n = "connected_ssid=\"";
      n += _current_ssid;
      n += "\"";

      Page.replace("connected_ssid=\"???\"",n);
  }

  String ssidList = "[";
  int n = WiFi.scanNetworks();
  if (n > 0) {
    for (int i = 0; i < n; i++) {
        ssidList += '"';
        ssidList += WiFi.SSID(i); 
        ssidList += '"';

        if( i < n -1){
          ssidList += ',';
        }
    }
  }
  ssidList += "]";
  SERIAL_LOG.println(ssidList);

  Page.replace("[]",ssidList);
  

  server.send(200, "text/html", Page);
  server.client().stop();  // Stop is needed because we sent no content length
  Page = "";
}

/** Handle the WLAN save form and redirect to WLAN config page again */
void ChipChopWifiPortal::handleWifiSave() {
  _current_ssid = server.arg("ssid");
  _current_password = server.arg("pass");

  SERIAL_LOG.println(_current_ssid);
  SERIAL_LOG.println(_current_password);

  if(_current_ssid != ""){ // only check for ssid as it can be an open network without password
    credentials_origin = 2;
    _connected_ssid_index = -1;
    saveCredentials(1);
    connectWifi();
  }

}


///events handling //////
void ChipChopWifiPortal::addListener(byte event,listenerCallbackType cb){
        for(byte i = 0; i < WIFI_PORTAL_TOTAL_EVENTS; i++){
            if(_listener_events[i].event == event){
                for(byte x = 0; x < WIFI_PORTAL_TOTAL_CALLBACKS; x++){
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

}

void ChipChopWifiPortal::emit_event(byte event, int data){
    // SERIAL_LOG.println(event);
    for(byte i = 0; i < WIFI_PORTAL_TOTAL_EVENTS; i++){
        if(_listener_events[i].event == event){
            for(byte x = 0; x < WIFI_PORTAL_TOTAL_CALLBACKS; x++){
                if(_listener_events[i].callbacks[x]){
                    _listener_events[i].callbacks[x](data);
                }
            }

            return;
        }
    }
}

//////////


  