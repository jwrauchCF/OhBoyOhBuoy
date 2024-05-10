#ifndef CHIPCHOP_KEEPALIVE_H
#define CHIPCHOP_KEEPALIVE_H

#include <Arduino.h>
#include <ChipChop_Config.h>

class CC_KeepAlive{

    private:
        using listenerCallbackType = void(*)();
        typedef struct{
            byte event = 0;
            listenerCallbackType callbacks[KEEP_ALIVE_TOTAL_CALLBACKS];
        } _LISTENER;

        _LISTENER _listener_events[KEEP_ALIVE_TOTAL_EVENTS];
        void emit_event(byte event);

        ////////////////////////

        unsigned long _check_timer = 0;
        unsigned long _check_interval = 15000;

        int _restart_in_progress = 0;
        unsigned long _restart_countdown = 0;

        byte _keep_alive_mode = KEEP_ALIVE_MODE; // 0. off, 1. restart wifi 2. full restart 3.auto
        byte _restart_try = 1;
        String _restart_reason = "Restart happened after initial wakeup";
        unsigned long _last_chipchop_connection_ok = 0;
        unsigned long _keep_alive_timeout = KEEP_ALIVE_TIMEOUT; //2 mins default, minimum 30sec max 10 minutes
        int _curr_connection_status = 0;
        
        
        
        void restartSockets();
        void handle_restart();

    public:

        CC_KeepAlive();
        void init();
        void addListener(byte event,listenerCallbackType cb);
   
        void keepAliveCheck();
        
        bool chipchop_connected = 0;
        bool chipchop_exists = 1;

        void restart();
        void cancelRestart();
        void setMode(byte mode, int timeout = 120000){
            _keep_alive_mode = mode;
            if(timeout < 30000){
                _keep_alive_timeout = 30000;
            }else if (timeout > 600000){
                _keep_alive_timeout = 600000;
            }else{
                _keep_alive_timeout = timeout;
            }
        }

        void run();

};

#endif