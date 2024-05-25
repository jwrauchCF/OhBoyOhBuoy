#include <Arduino.h>
#include <json_mini.h>
json_mini JSON;

json_mini::json_mini(){

}

String json_mini::stringify(JSON_OBJ obj[], int obj_length){

    String result = "{";

    for(int i = 0; i < obj_length; i++){

        JSON_OBJ entry = obj[i];
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

// Simple public function that finds a value based on a key in a JSON formated string.
// This is a very very very simple function, for anything more complex use a library like ArduinoJson
String json_mini::find(const String obj, const String& property){
    char next;
    int start = 0, end = 0;
    int npos = -1;

    String name = _quote + property + _quote;
    Serial.println("json");
    Serial.println(obj);
    Serial.println(name);
    Serial.println(obj.indexOf(name));
    Serial.println("end json");
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
        String temp = obj.substring(start, obj.length() - 1);
        end = temp.indexOf("}") + 1;
        return temp.substring(0, end);
        // start++;

    }

    unsigned int i = start;
    
    while(i++ < obj.length() -1){
        
        if(obj.charAt(i) == '}'){
            end = i - 1;
             if(obj.charAt(i - 1) != '"'){ //if start of string else start of number
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