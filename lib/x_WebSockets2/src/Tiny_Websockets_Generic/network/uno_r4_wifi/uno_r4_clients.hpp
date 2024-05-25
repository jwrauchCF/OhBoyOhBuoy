
 
#pragma once

#include <Tiny_Websockets_Generic/internals/ws_common.hpp>
#include <Tiny_Websockets_Generic/network/tcp_client.hpp>

namespace websockets2_generic
{
  namespace network2_generic
  {
    template <class WifiClientImpl>
    class GenericUnoR4WiFiTcpClient : public TcpClient 
    {
      public:
        GenericUnoR4WiFiTcpClient(WifiClientImpl c) : client(c) 
        {

        }
    
        GenericUnoR4WiFiTcpClient() {}
    
        bool connect(const WSString& host, const int port) 
        {

          yield();
            auto didConnect = client.connect(host.c_str(), port);

          return didConnect;
        }
    
        bool poll() 
        {
          yield();
          return client.available();
        }
    
        bool available() override 
        {
          return client.connected();
        }

        void send(const WSString& data) override 
        {
          yield();
          client.write(reinterpret_cast<uint8_t*>(const_cast<char*>(data.c_str())), data.size());
          yield();
        }
    
        void send(const WSString&& data) override 
        {
          yield();
          client.write(reinterpret_cast<uint8_t*>(const_cast<char*>(data.c_str())), data.size());
          yield();
        }
    
        void send(const uint8_t* data, const uint32_t len) override 
        {
          yield();
          client.write(data, len);
          yield();
        }
    
        WSString readLine() override 
        {
          WSString line = "";
    
          int ch = -1;
          
          const uint64_t millisBeforeReadingHeaders = millis();
          
          while( ch != '\n' && available()) 
          {
            if (millis() - millisBeforeReadingHeaders > _CONNECTION_TIMEOUT) 
              return "";
              
            ch = client.read();
            
            if (ch < 0) 
              continue;
              
            line += (char) ch;
          }
    
          return line;
        }


        uint32_t read(uint8_t* buffer, const uint32_t len) override 
        {
          yield();
          
          return 
            client.read(buffer, len);
        }
    
        void close() override 
        {
          yield();
          //IMPORTANT: must clear the rx_buffer from the previous server connection/response if any
          while(client.available()){
            client.read();
          }
         
          client.stop();
         
        }
    
        virtual ~GenericUnoR4WiFiTcpClient() 
        {
         
          client.stop();
          
        }
    
      protected:
        WifiClientImpl client;
    
        int getSocket() const override 
        {
          return -1;
        }
    };
  }   // namespace network2_generic
}     // namespace websockets2_generic
