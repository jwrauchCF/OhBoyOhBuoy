#include <Arduino.h>
#include <cc_Prefs.h>
ChipChopPrefsManager PrefsManager;


#include <ChipChopEngine.h> 
extern ChipChopEngine ChipChop;

#include <ChipChopPlugins.h>
extern ChipChopPluginsManager ChipChopPlugins;

#if defined(KEEP_ALIVE_EXISTS)
    #include <cc_keepAlive.h>
    extern CC_KeepAlive KeepAlive;
#endif


#include <LittleFS.h>

void ChipChopPrefsManager::init(){
   
    ChipChopPlugins.initLittleFS();
    
    #if defined(KEEP_ALIVE_EXISTS) && (KEEP_ALIVE_MODE == KEEP_ALIVE_AUTO) 
        if(autosave == 1){
            KeepAlive.addListener(KEEP_ALIVE_RESTARTING,[](){
                KeepAlive.cancelRestart();
                PrefsManager.saveAllChipChopStatuses();
                KeepAlive.restart();
            });
        }
    #endif

}

ChipChopPrefsManager::ChipChopPrefsManager(){
    
}


void ChipChopPrefsManager::saveChipChopStatus(String key,String value){
    save("chipchop",key,value);
}

void ChipChopPrefsManager::saveAllChipChopStatuses(){


    File root = LittleFS.open("/chipchop","r");
    if(root && root.isDirectory()){
        File file = root.openNextFile();
        while(file){
            bool found = 0;
            for(int i = 0; i < ChipChop._MAX_COMPONENTS; i++){
                ChipChopEngine::_JSON entry = ChipChop._COMPONENTS[i];
   
                if(entry.key == ""){
                    break;
                }else if(entry.key == file.name()){
                    found = 1;
                    break;
                }
            }
            if(found == 0){
                String path = "/chipchop/";
                path.concat(file.name());
                file.close();
                LittleFS.remove(path);
            }
           
            file = root.openNextFile();
        }
    }
    

    for(int i = 0; i < ChipChop._MAX_COMPONENTS; i++){
            ChipChopEngine::_JSON entry = ChipChop._COMPONENTS[i];
            if(entry.key != ""){

                int numTest = isNumber(entry.value);
                if(numTest == 0){
                    String val = entry.value.substring(1,entry.value.length() - 1);
                    save("chipchop",entry.key,val);
                }else if(numTest == 1){
                    int i = entry.value.toInt();
                    save("chipchop",entry.key,i);
                }else if(numTest == 2){
                    float f = entry.value.toFloat();
                    save("chipchop",entry.key,f);
                }
            }
    }
}

void ChipChopPrefsManager::loadChipChopStatus(){

    File root = LittleFS.open("/chipchop","r");
    if(!root){
        Serial.println("no chipchop root");
        return;
    }
    if(!root.isDirectory()){
        Serial.println("chipchop is not a directory");
       return;
    }

    File file = root.openNextFile();
    while(file){
        String val = file.readString(); 
        int numTest = isNumber(val);
        if(numTest == 0){
            ChipChop.updateStatus(file.name(),val);
        }else if(numTest == 1){
            int i = val.toInt();
            ChipChop.updateStatus(file.name(),i);
        }else if(numTest == 2){
            float f = val.toFloat();
            ChipChop.updateStatus(file.name(),f);
        }

        file = root.openNextFile();
    }

}

void ChipChopPrefsManager::save(String group,String key,String value){
        String path = "/";
        path.concat(group);
        path.concat("/");
        path.concat(key);

        #ifdef ESP32
            File file = LittleFS.open(path,"w",true); //create the file if it doesn't exist
        #elif ESP8266
            File file = LittleFS.open(path,"w+"); //create the file if it doesn't exist
        #endif

        if(!file){
            Serial.print("can't save the file: ");
            Serial.print(path);
            Serial.print(">> ");
            Serial.println(value);
            file.close();
        }else{
            file.print(value);
            file.close();
        }
        
        
}
void ChipChopPrefsManager::save(String group,String key,int value){

        save(group,key,String(value));

}
void ChipChopPrefsManager::save(String group,String key,float value){

        save(group,key,String(value));

}

String ChipChopPrefsManager::get(String group,String key){
    return get(group,key,false);
}

String ChipChopPrefsManager::get(String group,String key, bool create){

    String path = "/";
    path.concat(group);
    path.concat("/");
    path.concat(key);

    File file = LittleFS.open(path,"r");
    String text = file.readString();

    
    if(file){
        file.close();
        return text; 
    }else{
        if(create){
            #ifdef ESP32
                File temp = LittleFS.open(path,"w",true); //create the file if it doesn't exist
            #elif ESP8266
                File temp = LittleFS.open(path,"w+"); //create the file if it doesn't exist
            #endif
            if(!file){
                Serial.print("can't open the file >> ");
                Serial.println(path);
                temp.close();
            }else{
                temp.print("");
                temp.close();
            }
            
        }
        return "";
    }
}

void ChipChopPrefsManager::remove(String group,String key){

        String path = "/";
        path.concat(group);
        path.concat("/");
        path.concat(key);

    if(LittleFS.remove(path)){
        Serial.println("prefs deleted");
    } else {
        Serial.println("prefs delete failed");
    }
}

void ChipChopPrefsManager::remove(String group){

    String path = "/";
    path.concat(group);
    // path.concat("/");
    
    if(LittleFS.rmdir(path)){
        Serial.println("Prefs group removed");
    } else {
        Serial.println("Removing prefs group failed");
    }
}

void ChipChopPrefsManager::listPrefs(String group){

    String path = "/";
    path.concat(group);

    File root = LittleFS.open(path,"r");
    if(!root){
        Serial.println(F("failed to open prefs group"));
        return;
    }
    if(!root.isDirectory()){
       
        Serial.println(F("prefs group not a directory"));
        return;
    }

    File file = root.openNextFile();
    while(file){
        Serial.print(file.name());
        Serial.print("\t");
        Serial.println(file.size());
        file = root.openNextFile();
    }
}

int ChipChopPrefsManager::isNumber(String val){

    bool isNum = 1;
    bool isFloat = 0;
    int numDots = 0;
    int numMins = 0;
    for(uint16_t i = 0; i < val.length(); i++){
        if((val[i] >= '0' && val[i] <= '9') || val[i] == '-' || val[i] == '.'){
                if(val[i] == '.'){
                    isFloat = 1;
                    numDots++;
                    if(numDots > 1){
                        return 0;
                    }
                }else if(val[i] == '-'){
                    numMins++;
                    if(numMins > 1){
                        return 0;
                    }
                }
        }else{
            return 0;
        }
    }

    if(isNum){
        if(isFloat){
            return 2;
        }
        return 1;
    }
    return 0;
}

//////////
