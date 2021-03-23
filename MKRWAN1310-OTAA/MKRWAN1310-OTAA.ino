#include <MKRWAN.h>           // LoRaWAN Library
#include <CayenneLPP.h>       // CAYENNE LPP
#include "ArduinoLowPower.h"  // Low Power Library


#define FRAME_DELAY       10000
#define DATA_RATE         5
#define ADAPTIVE_DR       false
#define CONFIRMED         false
#define PORT              1
#define CAYENNE_LPP       true
#define LOW_POWER         false
#define DEBUG
//#undef DEBUG


/**** LoRaWAN Setup ******/
LoRaModem modem;
String appEui = ""; 
String appKey = "";  
uint8_t dataToSend[20] = {0}; // Data to send
CayenneLPP dataToSendCayenne(160);
int dRate = 5;
String reponse;

 
void setup() {
  int connected;
  
  /***** Serial Link Configuration ******/
  Serial.begin(115200);
  while (!Serial);
  pinMode(LED_BUILTIN, OUTPUT);
  for(uint32_t i=0;i<10;i++){
      digitalWrite(LED_BUILTIN, HIGH);
      delay(200);
      digitalWrite(LED_BUILTIN, LOW);
      delay(200);
  }
  Serial.println("\r\n\r\n\r\n");
  Serial.println("########################################");
  Serial.println("########   LoRaWAN MKRWAN Scale   ######");
  Serial.println("#########     OTAA activation   ########\r\n");

  /****** Configure Here the First Module *****/
  //Enter here module configuration

   /****** Configure Here the Second Module *****/
  //Enter here module configuration
  
  /**** LoRaWAN Configuration *****/
  if (!modem.begin(EU868)) {
    Serial.println("Failed to start module");
    Serial.print("Your device EUI is: ");
    Serial.println(modem.deviceEUI());
    while (1);
  };
  
  do{
  Serial.println(" JOIN procedure in progress ...");  
  connected = modem.joinOTAA(appEui, appKey);
  if (!connected)   Serial.println(" JOIN OTAA failed!!! Retry...");
  else              Serial.println(" JOIN procedure : SUCCESS !\r\n");
  }while(!connected);
  modem.dataRate(dRate);
  modem.setADR(1);
  modem.setPort(PORT);
  Serial.print(" Frame will be sent every ");Serial.print((FRAME_DELAY<7000)?7000:FRAME_DELAY);Serial.println("ms\r\n");  
 
}

void loop() {
  // Call here the first Module ie : getTemperature();
  getTemperature();
  // Call here the seconde Module ie : getWeight();
  getWeight();
  // Etc ...
  sendLoRa();
  if(LOW_POWER == true)   enterLowPower();
  else                    wait();

}


void sendLoRa(){
  int status;
  if(CONFIRMED)   Serial.print(" Uplink CONFIRMED on PORT ");
  else            Serial.print(" Uplink UNCONFIRMED on PORT ");
  Serial.println(PORT);
  Serial.println(" Sending CAYENNE LPP (Low Power Payload) ");
  modem.beginPacket();
  //modem.write(dataToSend,1);
  modem.write(dataToSendCayenne.getBuffer(),dataToSendCayenne.getSize());
  status = modem.endPacket(CONFIRMED);
  if (status < 0) {
    Serial.println(" Send Frame failed!!!");
  } else {
    Serial.println(" Frame sent. Waiting for Downlink...");
  } 
  delay(3000); // Wait for downlink

  char rcv[64];
  int i = 0;
  if (!modem.available()) {
    Serial.println(" No data received\r\n");
  }
  else {
    while (modem.available()) {
      rcv[i++] = (char)modem.read();
    }
    Serial.print("Received: ");
    for (unsigned int j = 0; j < i; j++) {
      Serial.print(rcv[j] >> 4, HEX);
      Serial.print(rcv[j] & 0xF, HEX);
      Serial.print(" ");
    }  
    Serial.println();
  } 
  
  dataToSendCayenne.reset();
}


void getTemperature(void){
  // Get Temperature
  /* Write here your code to get the temperature */
  // Update data to send
  dataToSendCayenne.addTemperature(7, 26.5f);
}

void getWeight(void){
  // Get Weight
  /* Write here your code to get the weight */
  // Update data to send 
  dataToSendCayenne.addAnalogOutput(1, 75.45f); // 
}

void enterLowPower(void){
  Serial.print(" Processor goes in Low Power mode during : ");
  Serial.print(FRAME_DELAY);Serial.println("ms");
  Serial.print(" Using the Low Power features usually corrupts the Serial Link ");
  LowPower.deepSleep(FRAME_DELAY); 
  Serial.println(" Wake Up !!!");  
}

void wait(void){
  Serial.print(" Processor is going to wait during : ");
  Serial.print(FRAME_DELAY);Serial.println("ms");
  delay(FRAME_DELAY); 
}
