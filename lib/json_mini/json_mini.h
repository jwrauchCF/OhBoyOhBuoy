#ifndef JSON_H
#define JSON_H
    #include <Arduino.h>


class json_mini{

    private:

      const String _quote = "\"";

      

    public:

        typedef struct{
            String key;
            String value;
        } JSON_OBJ;

        json_mini();

        String stringify(JSON_OBJ obj[], int obj_length);
        String find(const String obj, const String& property);


};

extern json_mini JSON;

#endif