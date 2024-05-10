
#include <cc_keepAlive.h>
extern CC_KeepAlive KeepAlive;
#include <cc_Prefs.h>
extern ChipChopPrefsManager PrefsManager;

class pluginsInit{

    public:
        
    void start(){ 
        
        KeepAlive.init();
        PrefsManager.init();

	}
};