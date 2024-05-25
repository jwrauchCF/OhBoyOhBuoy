#ifndef GPS_MONITOR_H
#define GPS_MONITOR_H

#include <Arduino.h>
#include <ChipChop_Config.h>

class GPSManager{



    private:
       




    public:

        GPSManager();
        void init();
        void getPosition(float (&latlong)[2]);

};

#endif