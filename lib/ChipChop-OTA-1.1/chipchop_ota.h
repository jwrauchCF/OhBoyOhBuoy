#ifndef CHIPCHOP_OTA
#define CHIPCHOP_OTA
    #include <Arduino.h>
    #include <ChipChop_Config.h>

class ChipChop_ota{

    private:
    public:

        String OTA_VERSION = OTA_FIRMWARE_VS;
        ChipChop_ota();
        void init();

        void downloadUpdate(String uri);
        

};


#endif