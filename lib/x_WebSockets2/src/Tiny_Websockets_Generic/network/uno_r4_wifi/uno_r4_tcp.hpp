/****************************************************************************************************************************
  esp32_tcp.hpp
  For WebSockets2_Generic Library
  
  Based on and modified from Gil Maimon's ArduinoWebsockets library https://github.com/gilmaimon/ArduinoWebsockets
  to support STM32F/L/H/G/WB/MP1, nRF52, SAMD21/SAMD51, SAM DUE, Teensy, RP2040 boards besides ESP8266 and ESP32

  The library provides simple and easy interface for websockets (Client and Server).
  
  Built by Khoi Hoang https://github.com/khoih-prog/Websockets2_Generic
  Licensed under MIT license
  
  Version: 1.13.2

  Version Modified By   Date      Comments
  ------- -----------  ---------- -----------
  1.0.0   K Hoang      14/07/2020 Initial coding/porting to support nRF52 and SAMD21/SAMD51 boards. Add SINRIC/Alexa support
  ...
  1.9.0   K Hoang      30/11/2021 Auto detect ESP32 core version. Fix bug in examples
  1.9.1   K Hoang      17/12/2021 Fix QNEthernet TCP interface
  1.10.0  K Hoang      18/12/2021 Supporting case-insensitive headers, according to RFC2616
  1.10.1  K Hoang      26/02/2022 Reduce QNEthernet latency
  1.10.2  K Hoang      14/03/2022 Fix bug when using QNEthernet staticIP. Add staticIP option to NativeEthernet
  1.10.3  K Hoang      11/04/2022 Use Ethernet_Generic library as default. Support SPI1/SPI2 for RP2040
  1.11.0  K Hoang      08/10/2022 Add support to ESP32 using W5x00 Ethernet
  1.12.0  K Hoang      09/10/2022 Add support to ENC28J60 using EthernetENC or UIPEthernet for all supported boards
  1.12.1  K Hoang      09/10/2022 Fix bug in examples
  1.13.0  K Hoang      11/10/2022 Add support to RP2040W using CYW43439 WiFi
  1.13.1  K Hoang      24/11/2022 Using new WiFi101_Generic library
  1.13.2  K Hoang      06/12/2022 Fix compiler error using QNEthernet v0.17.0
 *****************************************************************************************************************************/
 
#pragma once

#ifdef ARDUINO_UNOR4_WIFI 

    #include <Tiny_Websockets_Generic/internals/ws_common.hpp>
    #include <Tiny_Websockets_Generic/network/tcp_client.hpp>

    #include <Tiny_Websockets_Generic/network/uno_r4_wifi/uno_r4_clients.hpp>
    #include "WiFiS3.h"
    #include "WiFiSSLClient.h"

namespace websockets2_generic
{
  namespace network2_generic
  {
    typedef GenericUnoR4WiFiTcpClient<WiFiClient> UnoR4WiFiTcpClient;
    
    class SecuredUnoR4WiFiTcpClient : public GenericUnoR4WiFiTcpClient<WiFiSSLClient>
    {
      public:
      
        void setCACert(const char* ca_cert)
        {
          this->client.setCACert(ca_cert);
        }
    
        void setCertificate(const char* client_ca)
        {
        //   this->client.setCACert(client_ca);
        }
    
        void setPrivateKey(const char* private_key)
        {
        //   this->client.setCACert(private_key);
        }

        // void setInsecure()
        // {
        //   //nothing
        // }
   };  
  
  };
  // namespace network2_generic
};     // namespace websockets2_generic

#endif 
