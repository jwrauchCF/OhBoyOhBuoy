/****************************************************************************************************************************
  ChipChopEngine.cpp 
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

#include <Arduino.h>
#include <ChipChopEngine.h>
ChipChopEngine ChipChop;
#include <ChipChop_Config.h>

extern char *stack_start;

#if defined(ESP8266)
#undef PROGMEM
#define PROGMEM
#endif

 #include <WebSockets2_Generic.h>
using namespace websockets2_generic;
WebsocketsClient *webSocket = new WebsocketsClient();


void onEventsCallback(WebsocketsEvent event, String data) {
    if(event == WebsocketsEvent::ConnectionOpened){
        ChipChop.log(F("ChipChop => Opening connection"));

    } else if(event == WebsocketsEvent::ConnectionClosed) {
        unsigned long diff = ((millis() - ChipChop.lastReconnectTime) / 1000);
        if(diff > 15){
            diff = 1;
        }
        unsigned long when = 15 - diff;

        ChipChop.connected = 0;
        ChipChop.handshakeComplete = 0;
        ChipChop.connectionClosed();

        String error = "ChipChop => Connection closed. Don't panic, we'll try again in ";
        String time = String(when);
        error.concat(time);
        error.concat(" seconds");
        ChipChop.log(error);
    }
  
}


void onMessageCallback(WebsocketsMessage message){
    ChipChop._process_server_message(message.data());
}

void ChipChopEngine::connectionClosed(){

     emit_event(CC_SOCKET_CLOSED,"","","",0);
}

void ChipChopEngine::log(const String &data){
    if(_LOG_ENABLED == true){
        Serial.println(data);
    }
}

ChipChopEngine::ChipChopEngine(){
    
    for(int i = 0; i < 11; i++){
        _send_buffer[i] = {"",""};
    }
}
void ChipChopEngine::pause(){
    _CAN_RUN = 0;
    closeConnection();
}
void ChipChopEngine::unPause(){
    _CAN_RUN = 1;
    _connect_socket();
}

void ChipChopEngine::start(String server_uri,String uuid, String device_id, String auth_code ){

    webSocket->onMessage(onMessageCallback);
    webSocket->onEvent(onEventsCallback);

    _SERVER = server_uri;
    _UUID = uuid;
    _DEVICE_ID = device_id;
    _AUTH_CODE = auth_code;
   _connect_socket();

}

void ChipChopEngine::closeConnection(){
    connected = 0;
    handshakeComplete = 0;

    if(webSocket->available()){
        webSocket->close();
    }

    emit_event(CC_SOCKET_CLOSED,"","","",0);

}


void ChipChopEngine::_connect_socket(){

    if(_CAN_RUN == 0){
        return;
    }
    
    if(_SERVER == "" || _UUID == "" || _DEVICE_ID == ""){
        log(F("ChipChop =>  Can't start the connection as some credentials are not provided"));
        return;
    }

    lastReconnectTime = millis();
    lastEventTime = millis();
    lastHeartbeatSendTime = millis();
  
    String uri = _SERVER + "?uuid=" + _UUID + "&device_id=" + _DEVICE_ID;

    closeConnection();

    bool socketConnected = webSocket->connect(uri);
    if(!socketConnected) {
        log(F("ChipChop => Waiting to establish socket connection"));
    }else {
        log(F("ChipChop => Socket connected"));
            lastEventTime = millis();
            lastHeartbeatSendTime = millis();
    }
}

void ChipChopEngine::clearBufferQueue(){
    for(int i = 0; i < 11; i++){
        _send_buffer[i] = {"",""};
    }
    buffer_size = 0;
    _can_send_heartbeat = 1;
}

void ChipChopEngine::popBufferQueue(int poz){
    for(int i = poz + 1; i <= 10; i++){
        _send_buffer[i-1] = _send_buffer[i];
    }
    _send_buffer[10] = {"",""};
    buffer_size--;
}
void ChipChopEngine::limitPopBufferQueue(String event, String status){

    for(int i = 0; i <= 10; i++){
        if(_send_buffer[i].event == "heartbeat"){
            if(i == 10){ //found heartbeat in the 11th place
                for(int x = 1; x <= 9; x++){
                    _send_buffer[x-1] = _send_buffer[x];
                }
                _send_buffer[9] = {event,status}; //pops the first and goes in the 10th place before the heartbeat
                buffer_size = 11;
            }else{
                _send_buffer[i+1] = _send_buffer[i];//move heartbeat one slot up
                _send_buffer[i] = {event,status};
                buffer_size = i + 1;
            }

            
            return;
        }
    }

    if(buffer_size > 11){
        popBufferQueue(0);
    }
    _send_buffer[buffer_size - 1] = {event,status};
    
}



void ChipChopEngine::addEventToBuffer(String event, String status){

        buffer_size++;
        if(buffer_size >= 11){ //goes over 11 entries
            if(event == "heartbeat"){
                popBufferQueue(0); //
                _send_buffer[10] = {event,status};
            }else{
                limitPopBufferQueue(event,status);
            }
        }else{
            if(event == "trigger"){
                limitPopBufferQueue(event,status);
            }else{
                _send_buffer[buffer_size - 1] = {event,status};
            }
            
        }

      
        
}

void ChipChopEngine::sendNextEvent(){
    
    if(buffer_size == 0){
        _can_send_heartbeat = 1;
        return;
    }

    String event_type = _send_buffer[0].event;
    String status = _send_buffer[0].status;

    popBufferQueue(0);
    
    if(event_type == "trigger"){
        _send_trigger_event(status);

    }else{
        _send_heartbeat();
        lastHeartbeatSendTime = millis();
        _can_send_heartbeat = 1;
    }

    

}


void ChipChopEngine::run(){

    if(_CAN_RUN == 0){
        return;
    }

    if(webSocket->available()) {
        webSocket->poll();
    }

    if(_use_clock == true){
        if((millis() - lastDateTimeRefresh) >= 1000){ // update the clock every 1 second
            lastDateTimeRefresh = millis();
            if(_SERVER_TIME > 0){
                setTime();
            }
        }
    }

    if ((millis() - lastReconnectTime) > 15000){
        if(!webSocket->available() || (webSocket->available() && connected == 0)) {
            _connect_socket();
        }
        lastReconnectTime = millis();
        
    }

    if((millis() - eventSpacing) > 60000){
        eventSpacing = millis();
        eventsSent = 0;
       
    }

    // IMPORTANT: 
    // reducing this interval to less that 10 sec will result in initial warnings then connection being closed for a time period and eventually a full account suspension
    // If your device's status doesn't change often you can safely increase this value to a lot longer period

    if(_HEARTBEAT_INTERVAL >= 10000){
        if(_can_send_heartbeat == 1){
            if ((millis() - lastHeartbeatSendTime) > _HEARTBEAT_INTERVAL){
                addEventToBuffer("heartbeat","");
                _can_send_heartbeat = 0;

            }
        }
    }

    if(millis() - _send_pulse_timer > _send_rate){
        sendNextEvent();
        _send_pulse_timer = millis();
    }

}

void ChipChopEngine:: _respond_to_status_request(const String& message_id){
    if(webSocket->available()) {
        String status = componentsToJson("undefined");
        if(status.length() > 2000){

            String error = "ChipChop => Can not send response to status request. The status you are trying to send is too large (";
            String time = String(status.length());
            error.concat(time);
            error.concat(" bytes) and will be refused. The max status size can not exceed 2000 bytes including the length of component names");
            sendErrorCallback(error);
            log(error);
            return;
        }

        int obj_length = 5;
        _JSON obj[obj_length] = {
            {"api_call","\"heartbeat\""},
            {"command","\"status_response\""}, 
            {"uuid",_quote + _UUID + _quote},
            {"device_id",_quote + _DEVICE_ID + _quote},
            {"message_id",_quote + message_id + _quote},
            {"status",status}
        };

        String request = JSON_stringify(obj, obj_length);
        
        webSocket->send(request);

        log(request);
    }else{
        log(F("ChipChop => Can not send heartbeat, socket seems to be closed."));
    }
}

void ChipChopEngine::_send_heartbeat(){

    if(webSocket->available() && connected == 1) {

        String status = componentsToJson("undefined");
        if(status.length() > 2000){
            String error = "ChipChop => Can not send heartbeat. The status you are trying to send is too large (";
            String time = String(status.length());
            error.concat(time);
            error.concat(" bytes) and will be refused. The max status size can not exceed 2000 bytes including the length of component names");

            sendErrorCallback(error);
            log(error);
            return;
        }

        if(status != ""){
            int obj_length = 5;
            _JSON obj[obj_length] = {
                {"api_call","\"heartbeat\""},
                {"command","\"heartbeat\""}, 
                {"uuid",_quote + _UUID + _quote},
                {"device_id",_quote + _DEVICE_ID + _quote},
                {"status",status}
            };

            String request = JSON_stringify(obj, obj_length);

            webSocket->send(request);

            if(_LOG_RESPONSE_ONLY == 0){
                log(request);
            }
            
        }

    }else{
        if(!handshakeComplete){
            log(F("ChipChop => Can not send heartbeat, waiting for handshake to complete."));
        }else{
            log(F("ChipChop => Can not send heartbeat, socket seems to be closed."));
        }
        
    }

    
    
}

void ChipChopEngine::_send_trigger_event(String status){

    if(webSocket->available() && connected == 1) {
            int obj_length = 5;
            _JSON obj[obj_length] = {
                {"api_call","\"heartbeat\""},
                {"command","\"triggerevent\""}, 
                {"uuid",_quote + _UUID + _quote},
                {"device_id",_quote + _DEVICE_ID + _quote},
                {"status",status}
            };

            String request = JSON_stringify(obj, obj_length);
            webSocket->send(request);
            if(_LOG_RESPONSE_ONLY == 0){
                log(request);
            }


    }else{
            if(!handshakeComplete){
                log(F("ChipChop => Can not send trigger event, waiting for handshake to complete."));
            }else{
                log(F("ChipChop => Can not send trigger event, socket seems to be closed."));
            }
    }
}

void ChipChopEngine::_validate_trigger_event(String component_name){

        if(eventsSent > 9){ // max 10 events in a minute, we start counting from zero
            if(eventsSent == 10){ //issue the warning only once when the limit is reached
                    unsigned long next_event_time = 60 - ((millis() - eventSpacing) / 1000);
                    log(F("ChipChop => Warning: 10 trigger events per minute exceeded. Any queued events will still be processed."));
                    log("Next event can be sent in:" + String(next_event_time) + " seconds");
            }
            eventsSent++;
            return;
            
        }else if(eventsSent == 1){
            eventSpacing = millis();
        }


        String status = componentsToJson(component_name);
        if(status.length() > 2000){
            String error = "ChipChop => Can not send trigger event. The status you are trying to send is too large (";
            String time = String(status.length());
            error.concat(time);
            error.concat(" bytes) and will be refused. The max status size can not exceed 2000 bytes including the length of component names");
            sendErrorCallback(error);
            log(error);
            return;
        }
        
        addEventToBuffer("trigger",status);
        eventsSent++;

     
    
}

void ChipChopEngine::removeStatus(String component_name){
    if(component_name == ""){
        return;
    }

    int i;
    for( i = 0; i < _MAX_COMPONENTS; i++){
        _JSON entry = _COMPONENTS[i];
        String name = entry.key;
        if(name == component_name){
            _COMPONENTS[i].key = "";
            _COMPONENTS[i].value = "";
            return;
        }
    }
}

void ChipChopEngine::triggerEvent(String component_name,String status){
     updateStatus(component_name,status);
    _validate_trigger_event(component_name);
}

void ChipChopEngine::triggerEvent(String component_name,int status){
    updateStatus(component_name,status);
    _validate_trigger_event(component_name);
}

void ChipChopEngine::triggerEvent(String component_name,float status){
    updateStatus(component_name,status);
    _validate_trigger_event(component_name);
}


void ChipChopEngine::updateStatus(String component_name,String status){
    status = _quote + status + _quote;
    setComponentStatus(component_name,status);
}

void ChipChopEngine::updateStatus(String component_name,int status){
    String intStatus = String(status);
    setComponentStatus(component_name,intStatus);
}

void ChipChopEngine::updateStatus(String component_name,float status){
    String floatStatus = String(status);
    setComponentStatus(component_name,floatStatus);
}

void ChipChopEngine::_process_server_message(const String& request){
    
    //  Serial.println(ESP.getFreeHeap());
    //  char stack;
    //  Serial.println(stack_start - &stack );
     
    log(request);

    // String data = request;
   
    // emit_event(MAIN_LOG,request);

    // char stack2;
    //  Serial.println(stack_start - &stack2);

    // Serial.println(ESP.getFreeHeap());
  

    String reqStatus = JSON_find(request, "status"); 

    if(reqStatus != "undefined"){

        String timeT = JSON_find(request, "timestamp");
        if(timeT != "undefined"){
            _SERVER_TIME = stringToLong(timeT);
            _DEVICE_TIME_MILLIS = millis();
        }

        if(reqStatus == "Howdy Partner!"){
          
            lastEventTime = millis();
            lastHeartbeatSendTime = millis();
            connected = 1;
            handshakeComplete = 1;
            clearBufferQueue();
         
            emit_event(CC_CONNECTED,"","","",0);

        }else if(reqStatus == "remote_command" && connected == 1){
            String source = JSON_find(request, "source"); 
            String target = JSON_find(request, "target"); 
            String value = JSON_find(request, "value"); // always returns String, the user callback should decide if it's a number or string
            
            String commT = JSON_find(request, "command_time");
            int command_age = 0;

            if(commT !="undefined"){
                command_age = stringToLong(commT);
                command_age = _SERVER_TIME - command_age;
            }

            emit_event(CC_COMMAND_RECEIVED,target, value, source, command_age);
            sendCommandCallback(target, value, source, command_age);


        }else if(reqStatus == "error"){ // Level 2 type of errors, usually related to updating device status, Alexa, Actions etc.
            String error = JSON_find(request, "message"); 
            sendErrorCallback(error);
            log("error =>" + error);

        }else if(reqStatus == "handshake"){

            connected = 0;
            handshakeComplete = 0;
            
            emit_event(CC_SOCKET_CLOSED,"","","",0);

            String temp_auth = _AUTH_CODE + JSON_find(request, "code");
            char auth[temp_auth.length() + 1];
            temp_auth.toCharArray(auth, temp_auth.length() + 1);

            unsigned char* hash = MD5_make_hash(auth);
            char *md5str = MD5_make_digest(hash, 16);
            free(hash);

            int obj_length = 4;
            _JSON obj[obj_length] = {
                {"api_call","\"handshake_response\""},
                {"response_code",_quote + String(md5str) + _quote}, 
                {"uuid",_quote + _UUID + _quote},
                {"device_id",_quote + _DEVICE_ID + _quote}
            };

            String response = JSON_stringify(obj, obj_length);
            free(md5str);
            webSocket->send(response);

        }else if(reqStatus == "status_request"){

            lastHeartbeatSendTime = millis();
            _respond_to_status_request(JSON_find(request, "message_id"));


        }else if(reqStatus == "302_api_redirect"){
            String server_uri = JSON_find(request, "server_uri");
            _302_api_redirect(server_uri);

        }else if(reqStatus == "reconnect_request"){
            String timeout = JSON_find(request, "timeout");
            socket_restart_timeout = timeout.toInt();
            socket_restart_timer = millis();
            socket_restart_mode = 1;
            log(F("ChipChop => (420) Connection Watch asked if we can restart the chanel in the next 30 sec to see if we can improve the heartbeat"));

        }else if(reqStatus == "wait"){
            lastReconnectTime = millis();
            log(F("ChipChop => (406) Connection Watch asked us to wait for the channel to clear from the previous connection. We'll knock on the door again in 15 sec."));

        }

    }

    // Level 1 type of error usually related to connectivity, system violations like sending heartbeats too fast, authentication etc
    String reqError = JSON_find(request, "error"); 
    if(reqError != "undefined"){
        sendErrorCallback(reqError);
    }

}

void ChipChopEngine::_302_api_redirect(const String& server_uri){
     log(F("ChipChop => 302 re-direct received, switching connection to a closer api server"));
     _SERVER = server_uri;
    _connect_socket();  
}

void ChipChopEngine::setComponentStatus(const String component, String value){

    if(component == ""){
        return;
    }

    
    for(int i = 0; i < _MAX_COMPONENTS; i++){
        _JSON entry = _COMPONENTS[i];
        String name = entry.key;
        if(name == component){
            _COMPONENTS[i].value = value;
            return;
        
        }
    }

    for(int i = 0; i < _MAX_COMPONENTS; i++){
        _JSON entry = _COMPONENTS[i];
        String name = entry.key;
        if(name == ""){
            _COMPONENTS[i].key = component;
            _COMPONENTS[i].value = value;
            return;
        }
    }
 

}

String ChipChopEngine::componentsToJson(String triggerEventComponent){
    String status;

    for(int i = 0; i < _MAX_COMPONENTS; i++){
        _JSON entry = _COMPONENTS[i];
        String name = entry.key;
        String value = entry.value;

        if(name != ""){
            //used for trigger events where only one component status can be sent
            if(triggerEventComponent != "undefined"){
                if(name == triggerEventComponent){
                    status += _quote + name + _quote + ":{\"value\":" + value + "},";
                    break;
                }
            }else{ //standard heartbeat

                status += _quote + name + _quote + ":{\"value\":" + value + "},";
            }
        }
         
    }

    if(status != ""){
        status.remove(status.length() -1,1);
        status = '{' + status +'}';
        return status;

    }else{
        return "";
    }

    
}

void ChipChopEngine::restartSockets(){

            log(F("ChipChop =>  Destroying WebSockets"));
            webSocket->close();
            connected = 0;
            handshakeComplete = 0;
            delete webSocket;
            webSocket = NULL;

            webSocket = new WebsocketsClient();

            webSocket->onMessage(onMessageCallback);
            webSocket->onEvent(onEventsCallback);
            log(F("ChipChop =>  Restarting WebSockets"));
            _connect_socket();

}

unsigned long ChipChopEngine::stringToLong(String input){
        // ChipChop returns the timestamp in milliseconds which is too large for an integer in Arduino so we need to chop it to 10 digits
        if(input.length() > 10){
            input = input.substring(0,10);
        }
        unsigned long result = strtol(input.c_str(), NULL, input.length());
        return result;
}

float ChipChopEngine::stringToFloat(String input){
        if(input.length() > 10){
            input = input.substring(0,10);
        }

        char temp[input.length() + 1];
        input.toCharArray(temp, input.length() + 1); 
        float result = atof(temp);

        return result;
}

String ChipChopEngine:: zeroPrefix(int num){
  // utility function for digital clock display: adds preceding 0
    String val = String(num);
    if(num < 10){
        val = "0" + val;
    }
    return val;
   
}

String ChipChopEngine::JSON_stringify(_JSON obj[], int obj_length){

    String result = "{";

    for(int i = 0; i < obj_length; i++){

        _JSON entry = obj[i];
        String key = entry.key;
        String value = entry.value;

        result += _quote + key + _quote + ":" + value;

        if(i != obj_length - 1){
            result += ",";
        }

    }
    result += "}";

    return result;
}

void ChipChopEngine::toRGB(const String& obj, int (&rgb)[3]){
        String temp = obj.substring(1,obj.length() - 1);
        temp = temp + ","; //add a "," at the end so it will be used as the last indexOf()
       
        int start_index = 0; 
        int end;
        for(int i = 0; i < 3;i++){
            end = temp.indexOf(",",start_index);
            rgb[i] = temp.substring(start_index,end).toInt();
            start_index = end + 1;

        }
} 

// Simple public function that finds a value based on a key in a JSON formatted string.
// This is a very very very simple function, for anything more complex use a library like ArduinoJson
String ChipChopEngine::JSON_find(const String& obj, const String& property){
    char next;
    int start = 0, end = 0;
    int npos = -1;

    String name = _quote + property + _quote;
    if (obj.indexOf(name) == npos){
        return "undefined";
    } 
    start = obj.indexOf(name) + name.length() + 1; // jumps over the closing " and :
    next = obj.charAt(start);
    if(next == '"'){ //if start of string else start of number
        start ++;
    }else if(next == '{'){ //it's a json object sent as a value, most likely a copy of another device status
        // in this case we are simply looking for the next closing double bracket }}
        // as ChipChop json is in a standardised format it's only in this situation that it can contain a double "}}"
        end = obj.indexOf("}}") + 2;
        return obj.substring(start, end);

    }

    unsigned int i = start;
    
    while(i++ < obj.length() -1){
        if(obj.charAt(i + 1) == '}'){
            end = i;
             if(obj.charAt(i) != '"'){ //if start of string else start of number
                end ++;
            }
            break;
        }else if(obj.charAt(i + 1) == ',' && obj.charAt(i + 2) == '"' ){
            end = i;
             if(obj.charAt(i) != '"'){ //if start of string else start of number
                end ++;
            }
            break;
        }
    }

  return obj.substring(start, end);
}


///events handling //////
void ChipChopEngine::addListener(byte event,listenerCallbackType cb){

        for(byte i = 0; i < CC_TOTAL_EVENTS; i++){
            if(_listener_events[i].event == event){
                for(byte x = 0; x < CC_TOTAL_CALLBACKS; x++){
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

        // log(F("ChipChop => Main Lib: Exceeded number of event callbacks"));
}

void ChipChopEngine::emit_event(byte event, String a, String b, String c, int d){
    // Serial.println(event);
    for(byte i = 0; i < CC_TOTAL_EVENTS; i++){
        if(_listener_events[i].event == event){
            for(byte x = 0; x < CC_TOTAL_CALLBACKS; x++){
                if(_listener_events[i].callbacks[x] != NULL){
                    _listener_events[i].callbacks[x](a,b,c,d);
                }
            }

            return;
        }
    }
}

//////////

/// Date Time stuff ////////
#define LEAP_YEAR(Y)     ( ((1970+(Y))>0) && !((1970+(Y))%4) && ( ((1970+(Y))%100) || !((1970+(Y))%400) ) )

static char buffer[10];
const char monthStr0[] PROGMEM = "";
const char monthStr1[] PROGMEM = "January";
const char monthStr2[] PROGMEM = "February";
const char monthStr3[] PROGMEM = "March";
const char monthStr4[] PROGMEM = "April";
const char monthStr5[] PROGMEM = "May";
const char monthStr6[] PROGMEM = "June";
const char monthStr7[] PROGMEM = "July";
const char monthStr8[] PROGMEM = "August";
const char monthStr9[] PROGMEM = "September";
const char monthStr10[] PROGMEM = "October";
const char monthStr11[] PROGMEM = "November";
const char monthStr12[] PROGMEM = "December";

const PROGMEM char * const PROGMEM monthNames_P[] =
{
    monthStr0,monthStr1,monthStr2,monthStr3,monthStr4,monthStr5,monthStr6,
    monthStr7,monthStr8,monthStr9,monthStr10,monthStr11,monthStr12
};

char* monthStr(uint8_t month)
{
    strcpy_P(buffer, (PGM_P)pgm_read_ptr(&(monthNames_P[month])));
    return buffer;
}

const char monthShortNames_P[] PROGMEM = "ErrJanFebMarAprMayJunJulAugSepOctNovDec";

char* monthShortStr(uint8_t month)
{
   for(int i=0; i < 3; i++){     
      buffer[i] = pgm_read_byte(&(monthShortNames_P[i+ (month*3)])); 
   }
    buffer[3] = 0;
    return buffer;
}

void ChipChopEngine::setTime(){

    unsigned long millisDiff = millis() - _DEVICE_TIME_MILLIS;
    unsigned long currentTime = _SERVER_TIME + (millisDiff/1000);

    String ans = "";

    int daysOfMonth[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
 
    int year, daysTillNow, extraTime, extraDays,
        index, day, month, hour, minute, second,
        flag = 0;
 
    daysTillNow = currentTime / (24 * 60 * 60);
    extraTime = currentTime % (24 * 60 * 60);
    year = 1970;
 
    while (true) {
        if (year % 400 == 0
            || (year % 4 == 0 && year % 100 != 0)) {
            if (daysTillNow < 366) {
                break;
            }
            daysTillNow -= 366;
        }
        else {
            if (daysTillNow < 365) {
                break;
            }
            daysTillNow -= 365;
        }
        year += 1;
    }

    extraDays = daysTillNow + 1;
    if (year % 400 == 0
        || (year % 4 == 0 && year % 100 != 0))
        flag = 1;

    month = 0, index = 0;
    if (flag == 1) {
        while (true) {
 
            if (index == 1) {
                if (extraDays - 29 < 0)
                    break;
                month += 1;
                extraDays -= 29;
            }
            else {
                if (extraDays - daysOfMonth[index] < 0) {
                    break;
                }
                month += 1;
                extraDays -= daysOfMonth[index];
            }
            index += 1;
        }
    }
    else {
        while (true) {
 
            if (extraDays - daysOfMonth[index] < 0) {
                break;
            }
            month += 1;
            extraDays -= daysOfMonth[index];
            index += 1;
        }
    }

    if (extraDays > 0) {
        month += 1;
        day = extraDays;
    }
    else {
        if (month == 2 && flag == 1)
            day = 29;
        else {
            day = daysOfMonth[month - 1];
        }
    }
    hour = extraTime / 3600;
    minute = (extraTime % 3600) / 60;
    second = (extraTime % 3600) % 60;
 
    Date.year = year;
    Date.yearShort = year - 2000;
    Date.month = month;
    Date.day = day;
    Date.hour = hour;
    Date.minute = minute;
    Date.second = second;
    Date.timestamp = currentTime;
    Date.monthLong = monthStr(month);
    Date.monthShort = monthShortStr(month);

    if(dateFormat != ""){
        String formatted = dateFormat;
        formatted.replace("YYYY",String(year));
        formatted.replace("YY",String(year - 2000));
        formatted.replace("MMMM",String(Date.monthLong));
        formatted.replace("MMM",String(Date.monthShort));
        formatted.replace("MM",String(zeroPrefix(month)));
        formatted.replace("dd",String(zeroPrefix(day)));
        formatted.replace("hh",String(zeroPrefix(hour)));
        formatted.replace("mm",String(zeroPrefix(minute)));
        formatted.replace("ss",String(zeroPrefix(second)));

        Date.formatted = formatted;
    }

}


/// MD5 stuff /////
char* ChipChopEngine::MD5_make_digest(const unsigned char *digest, int len) /* {{{ */
{
	char * md5str = (char*) malloc(sizeof(char)*(len*2+1));
	static const char hexits[17] = "0123456789abcdef";
	int i;

	for (i = 0; i < len; i++) {
		md5str[i * 2]       = hexits[digest[i] >> 4];
		md5str[(i * 2) + 1] = hexits[digest[i] &  0x0F];
	}
	md5str[len * 2] = '\0';
	return md5str;
}


#define E(x, y, z)			((z) ^ ((x) & ((y) ^ (z))))
#define G(x, y, z)			((y) ^ ((z) & ((x) ^ (y))))
#define H(x, y, z)			((x) ^ (y) ^ (z))
#define I(x, y, z)			((y) ^ ((x) | ~(z)))


#define STEP(f, a, b, c, d, x, t, s) \
	(a) += f((b), (c), (d)) + (x) + (t); \
	(a) = (((a) << (s)) | (((a) & 0xffffffff) >> (32 - (s)))); \
	(a) += (b);


#if defined(__i386__) || defined(__x86_64__) || defined(__vax__)
# define SET(n) \
	(*(MD5_u32plus *)&ptr[(n) * 4])
# define GET(n) \
	SET(n)
#else
# define SET(n) \
	(ctx->block[(n)] = \
	(MD5_u32plus)ptr[(n) * 4] | \
	((MD5_u32plus)ptr[(n) * 4 + 1] << 8) | \
	((MD5_u32plus)ptr[(n) * 4 + 2] << 16) | \
	((MD5_u32plus)ptr[(n) * 4 + 3] << 24))
# define GET(n) \
	(ctx->block[(n)])
#endif

const void *ChipChopEngine::MD5_body(void *ctxBuf, const void *data, size_t size)
{
	MD5_CTX *ctx = (MD5_CTX*)ctxBuf;
	const unsigned char *ptr;
	MD5_u32plus a, b, c, d;
	MD5_u32plus saved_a, saved_b, saved_c, saved_d;

	ptr = (unsigned char*)data;

	a = ctx->a;
	b = ctx->b;
	c = ctx->c;
	d = ctx->d;

	do {
		saved_a = a;
		saved_b = b;
		saved_c = c;
		saved_d = d;

		STEP(E, a, b, c, d, SET(0), 0xd76aa478, 7)
		STEP(E, d, a, b, c, SET(1), 0xe8c7b756, 12)
		STEP(E, c, d, a, b, SET(2), 0x242070db, 17)
		STEP(E, b, c, d, a, SET(3), 0xc1bdceee, 22)
		STEP(E, a, b, c, d, SET(4), 0xf57c0faf, 7)
		STEP(E, d, a, b, c, SET(5), 0x4787c62a, 12)
		STEP(E, c, d, a, b, SET(6), 0xa8304613, 17)
		STEP(E, b, c, d, a, SET(7), 0xfd469501, 22)
		STEP(E, a, b, c, d, SET(8), 0x698098d8, 7)
		STEP(E, d, a, b, c, SET(9), 0x8b44f7af, 12)
		STEP(E, c, d, a, b, SET(10), 0xffff5bb1, 17)
		STEP(E, b, c, d, a, SET(11), 0x895cd7be, 22)
		STEP(E, a, b, c, d, SET(12), 0x6b901122, 7)
		STEP(E, d, a, b, c, SET(13), 0xfd987193, 12)
		STEP(E, c, d, a, b, SET(14), 0xa679438e, 17)
		STEP(E, b, c, d, a, SET(15), 0x49b40821, 22)

		STEP(G, a, b, c, d, GET(1), 0xf61e2562, 5)
		STEP(G, d, a, b, c, GET(6), 0xc040b340, 9)
		STEP(G, c, d, a, b, GET(11), 0x265e5a51, 14)
		STEP(G, b, c, d, a, GET(0), 0xe9b6c7aa, 20)
		STEP(G, a, b, c, d, GET(5), 0xd62f105d, 5)
		STEP(G, d, a, b, c, GET(10), 0x02441453, 9)
		STEP(G, c, d, a, b, GET(15), 0xd8a1e681, 14)
		STEP(G, b, c, d, a, GET(4), 0xe7d3fbc8, 20)
		STEP(G, a, b, c, d, GET(9), 0x21e1cde6, 5)
		STEP(G, d, a, b, c, GET(14), 0xc33707d6, 9)
		STEP(G, c, d, a, b, GET(3), 0xf4d50d87, 14)
		STEP(G, b, c, d, a, GET(8), 0x455a14ed, 20)
		STEP(G, a, b, c, d, GET(13), 0xa9e3e905, 5)
		STEP(G, d, a, b, c, GET(2), 0xfcefa3f8, 9)
		STEP(G, c, d, a, b, GET(7), 0x676f02d9, 14)
		STEP(G, b, c, d, a, GET(12), 0x8d2a4c8a, 20)

		STEP(H, a, b, c, d, GET(5), 0xfffa3942, 4)
		STEP(H, d, a, b, c, GET(8), 0x8771f681, 11)
		STEP(H, c, d, a, b, GET(11), 0x6d9d6122, 16)
		STEP(H, b, c, d, a, GET(14), 0xfde5380c, 23)
		STEP(H, a, b, c, d, GET(1), 0xa4beea44, 4)
		STEP(H, d, a, b, c, GET(4), 0x4bdecfa9, 11)
		STEP(H, c, d, a, b, GET(7), 0xf6bb4b60, 16)
		STEP(H, b, c, d, a, GET(10), 0xbebfbc70, 23)
		STEP(H, a, b, c, d, GET(13), 0x289b7ec6, 4)
		STEP(H, d, a, b, c, GET(0), 0xeaa127fa, 11)
		STEP(H, c, d, a, b, GET(3), 0xd4ef3085, 16)
		STEP(H, b, c, d, a, GET(6), 0x04881d05, 23)
		STEP(H, a, b, c, d, GET(9), 0xd9d4d039, 4)
		STEP(H, d, a, b, c, GET(12), 0xe6db99e5, 11)
		STEP(H, c, d, a, b, GET(15), 0x1fa27cf8, 16)
		STEP(H, b, c, d, a, GET(2), 0xc4ac5665, 23)


		STEP(I, a, b, c, d, GET(0), 0xf4292244, 6)
		STEP(I, d, a, b, c, GET(7), 0x432aff97, 10)
		STEP(I, c, d, a, b, GET(14), 0xab9423a7, 15)
		STEP(I, b, c, d, a, GET(5), 0xfc93a039, 21)
		STEP(I, a, b, c, d, GET(12), 0x655b59c3, 6)
		STEP(I, d, a, b, c, GET(3), 0x8f0ccc92, 10)
		STEP(I, c, d, a, b, GET(10), 0xffeff47d, 15)
		STEP(I, b, c, d, a, GET(1), 0x85845dd1, 21)
		STEP(I, a, b, c, d, GET(8), 0x6fa87e4f, 6)
		STEP(I, d, a, b, c, GET(15), 0xfe2ce6e0, 10)
		STEP(I, c, d, a, b, GET(6), 0xa3014314, 15)
		STEP(I, b, c, d, a, GET(13), 0x4e0811a1, 21)
		STEP(I, a, b, c, d, GET(4), 0xf7537e82, 6)
		STEP(I, d, a, b, c, GET(11), 0xbd3af235, 10)
		STEP(I, c, d, a, b, GET(2), 0x2ad7d2bb, 15)
		STEP(I, b, c, d, a, GET(9), 0xeb86d391, 21)

		a += saved_a;
		b += saved_b;
		c += saved_c;
		d += saved_d;

		ptr += 64;
	} while (size -= 64);

	ctx->a = a;
	ctx->b = b;
	ctx->c = c;
	ctx->d = d;

	return ptr;
}

void ChipChopEngine::MD5Init(void *ctxBuf)
{
	MD5_CTX *ctx = (MD5_CTX*)ctxBuf;
	ctx->a = 0x67452301;
	ctx->b = 0xefcdab89;
	ctx->c = 0x98badcfe;
	ctx->d = 0x10325476;

	ctx->lo = 0;
	ctx->hi = 0;

    memset(ctx->block, 0, sizeof(ctx->block));
    memset(ctx->buffer, 0, sizeof(ctx->buffer));
}

void ChipChopEngine::MD5Update(void *ctxBuf, const void *data, size_t size)
{
	MD5_CTX *ctx = (MD5_CTX*)ctxBuf;
	MD5_u32plus saved_lo;
	MD5_u32plus used, free;

	saved_lo = ctx->lo;
	if ((ctx->lo = (saved_lo + size) & 0x1fffffff) < saved_lo) {
		ctx->hi++;
	}
	ctx->hi += size >> 29;

	used = saved_lo & 0x3f;

	if (used) {
		free = 64 - used;

		if (size < free) {
			memcpy(&ctx->buffer[used], data, size);
			return;
		}

		memcpy(&ctx->buffer[used], data, free);
		data = (unsigned char *)data + free;
		size -= free;
		MD5_body(ctx, ctx->buffer, 64);
	}

	if (size >= 64) {
		data = MD5_body(ctx, data, size & ~(size_t)0x3f);
		size &= 0x3f;
	}

	memcpy(ctx->buffer, data, size);
}

void ChipChopEngine::MD5Final(unsigned char *result, void *ctxBuf)
{
	MD5_CTX *ctx = (MD5_CTX*)ctxBuf;
	MD5_u32plus used, free;

	used = ctx->lo & 0x3f;

	ctx->buffer[used++] = 0x80;

	free = 64 - used;

	if (free < 8) {
		memset(&ctx->buffer[used], 0, free);
		MD5_body(ctx, ctx->buffer, 64);
		used = 0;
		free = 64;
	}

	memset(&ctx->buffer[used], 0, free - 8);

	ctx->lo <<= 3;
	ctx->buffer[56] = ctx->lo;
	ctx->buffer[57] = ctx->lo >> 8;
	ctx->buffer[58] = ctx->lo >> 16;
	ctx->buffer[59] = ctx->lo >> 24;
	ctx->buffer[60] = ctx->hi;
	ctx->buffer[61] = ctx->hi >> 8;
	ctx->buffer[62] = ctx->hi >> 16;
	ctx->buffer[63] = ctx->hi >> 24;

	MD5_body(ctx, ctx->buffer, 64);

	result[0] = ctx->a;
	result[1] = ctx->a >> 8;
	result[2] = ctx->a >> 16;
	result[3] = ctx->a >> 24;
	result[4] = ctx->b;
	result[5] = ctx->b >> 8;
	result[6] = ctx->b >> 16;
	result[7] = ctx->b >> 24;
	result[8] = ctx->c;
	result[9] = ctx->c >> 8;
	result[10] = ctx->c >> 16;
	result[11] = ctx->c >> 24;
	result[12] = ctx->d;
	result[13] = ctx->d >> 8;
	result[14] = ctx->d >> 16;
	result[15] = ctx->d >> 24;

	memset(ctx, 0, sizeof(*ctx));
}
unsigned char* ChipChopEngine::MD5_make_hash(char *arg)
{
	MD5_CTX context;
	unsigned char * hash = (unsigned char *) malloc(16);
	MD5Init(&context);
	MD5Update(&context, arg, strlen(arg));
	MD5Final(hash, &context);
	return hash;
}
unsigned char* ChipChopEngine::MD5_make_hash(char *arg,size_t size)
{
	MD5_CTX context;
	unsigned char * hash = (unsigned char *) malloc(16);
	MD5Init(&context);
	MD5Update(&context, arg, size);
	MD5Final(hash, &context);
	return hash;
}