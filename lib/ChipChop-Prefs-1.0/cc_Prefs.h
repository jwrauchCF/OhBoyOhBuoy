#ifndef CHIPCHOP_PREFS_MANAGER_H
#define CHIPCHOP_PREFS_MANAGER_H

#include <Arduino.h>
#include <ChipChop_Config.h>

class ChipChopPrefsManager{

    

    private:
        
        ////////////////////////
       bool autosave = PREFS_MANAGER_AUTO_SAVE;
       void saveFormatted(String group,String key,String value);
       void parseValue(String val);
       

    public:

        ChipChopPrefsManager();
        void init();

        void saveChipChopStatus(String key,String value);
        void saveAllChipChopStatuses();
        void loadChipChopStatus();

        int isNumber(String val);

        String get(String group,String key,bool create); 
        String get(String group,String key); 
        void save(String group,String key,String value);
        void save(String group,String key,int value);
        void save(String group,String key,float value);
        
        void remove(String group,String key);
        void remove(String group);
        void listPrefs(String group);
       

};

#endif