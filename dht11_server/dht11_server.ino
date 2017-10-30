/*
  Upload Data to IoT Server ThingSpeak (https://thingspeak.com/):
  Support Devices: LG01 
  
  Example sketch showing how to get data from remote LoRa node, 
  Then send the value to IoT Server

  It is designed to work with the other sketch dht11_client. 

  modified 24 11 2016
  by Edwin Chen <support@dragino.com>
  Dragino Technology Co., Limited
*/

#include <SPI.h>
#include <RH_RF95.h>
//#include <Console.h>
#include "ThingSpeak.h"
#include "YunClient.h"
YunClient client;
RH_RF95 rf95;

//If you use Dragino IoT Mesh Firmware, uncomment below lines.
//For product: LG01. 
#define BAUDRATE 115200

unsigned long myChannelNumber = 234900;
const char * myWriteAPIKey = "UHNXVPOLFMCE6077";
uint16_t crcdata = 0;
uint16_t recCRCData = 0;
float frequency = 923.6;
//float frequency = 868.0;

void setup()
{
    Bridge.begin(BAUDRATE);
//    Console.begin();// Don't use Console here, since it is conflict with the ThinkSpeak library. 
//    while(!Console);
    ThingSpeak.begin(client);
    
    if (!rf95.init())
//        Console.println("init failed")
    ;
    // Setup ISM frequency
    rf95.setFrequency(frequency);
    // Setup Power,dBm
    rf95.setTxPower(13);
    // キャリアセンス
    rf95.setCADTimeout((unsigned long)5);
//    Console.println("Start Listening ");
}

uint16_t calcByte(uint16_t crc, uint8_t b)
{
    uint32_t i;
    crc = crc ^ (uint32_t)b << 8;
  
    for ( i = 0; i < 8; i++)
    {
      if ((crc & 0x8000) == 0x8000)
        crc = crc << 1 ^ 0x1021;
      else
        crc = crc << 1;
    }
    return crc & 0xffff;
}

uint16_t CRC16(uint8_t *pBuffer, uint32_t length)
{
    uint16_t wCRC16 = 0;
    uint32_t i;
    if (( pBuffer == 0 ) || ( length == 0 ))
    {
        return 0;
    }
    for ( i = 0; i < length; i++)
    {
        wCRC16 = calcByte(wCRC16, pBuffer[i]);
    }
    return wCRC16;
}

uint16_t recdata( unsigned char* recbuf, int Length)
{
    crcdata = CRC16(recbuf, Length - 2); //Get CRC code
    recCRCData = recbuf[Length - 1]; //Calculate CRC Data
    recCRCData = recCRCData << 8; //
    recCRCData |= recbuf[Length - 2];
}
void loop()
{

    if (rf95.waitAvailableTimeout(2000))// Listen Data from LoRa Node
    {
        uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];//receive data buffer
        uint8_t len = sizeof(buf);//data buffer length
        if (rf95.recv(buf, &len))//Check if there is incoming data
        {
            recdata( buf, len);
            if(crcdata == recCRCData) //Check if CRC is correct
            { 
//                Console.println("Get Data from LoRa Node");
//                Console.print(buf[0]);Console.print(buf[1]);Console.println(buf[2]);
                if(buf[0] == 1||buf[1] == 1||buf[2] ==1) //Check if the ID match the LoRa Node ID
                {
                    uint8_t data[] = "   Server ACK";//Reply 
                    data[0] = buf[0];
                    data[1] = buf[1];
                    data[2] = buf[2];
//                    Console.print(data[0]);Console.print(data[1]);Console.println(data[2]);
                    rf95.send(data, sizeof(data));// Send Reply to LoRa Node
                    rf95.waitPacketSent();
                    int newData[4] = {0, 0, 0, 0}; //Store Sensor Data here
                    for (int i = 0; i < 2; i++)
                    {
                        newData[i] = buf[i + 3];
                    }
                    float h = newData[0];
                    float t = newData[1];
//                    Console.print("h:");Console.println(h);
//                    Console.print("t:");Console.println(t);
                    ThingSpeak.setField(2,h); // 
                    ThingSpeak.setField(1,t);
                    ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey);   // Send Data to IoT Server.
                }
            }       
         }
         else
         {
//              Console.println("recv failed");
              ;
          }
     }
}


