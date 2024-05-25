#ifndef CHIPCHOP_DS1820
#define CHIPCHOP_DS1820
    #include <Arduino.h>
    #include <ChipChop_Config.h>

class CC_DS1820{

    private:

       typedef struct{
            String name;
            float value;
        } _SENSOR;

        //NOTE: the sensors by name are specified in the ChipChop_Config.h
        _SENSOR _SENSORS[DS1820_TOTAL_SENSORS] = DS1820_SENSOR_LIST;

        bool _ready = 0;

        unsigned long _check_timer = 0;
        unsigned long _check_interval = DS1820_READ_INTERVAL;

        int sensors_found = 0;

        

    public:
        

        CC_DS1820();
        void init();


        void start();
        void run();

        float getTemperature(String component_name);
        float getTemperature();

        

};


#endif