#include <SPI.h>              // SPI
#include "ArduinoLowPower.h"  // Low Power Library
#include "ads1241.h"

// TODO : Add Battery measure with Op Amp ADA4807.
// TODO : Change to C++ Librairie
// Consumption running : 13 mA 
// Send LoRa : 18 mA
// Low Power
// Conso LoRa OTAA only : 13mA TX, 105µA Deep Sleep.
// Conversion occurs every 40 ms (Speed and Data Rate at default value), so we wait 50ms beetween conversions
// La LED BuildIn ne devrait pas etre mise sur la même broche que le signal DRDY de l'ADS1241

#define FRAME_DELAY       5000
#define LOW_POWER         false
#define DEBUG
//#undef DEBUG
#define NB_MEASUREMENT    10
#define NB_EXTREM         2 

uint32_t frameDelay = FRAME_DELAY;
uint32_t tabWeight[NB_MEASUREMENT];
uint32_t tareWeight[2]={0,2220};        // tareWeight[0] is associated with tareValNum[0]
uint32_t tareValNum[2]={0x64FD1,0x7C5A0};     // tareWeight[1] is associated with tareValNum[1]
float coefConversion=0.023198704216; //(tareWeight[1]-tareWeight[0])/(tareValNum[1]-tareValNum[0]);
float originConversion = -9596.78;//tareValNum[0]-tareWeight[0]*coefConversion;
float weight;

/**** ADS1241 PIN ****/
const int ADS1241_DRDY_PIN = 6;    //Pin Data Ready
const int ADS1241_CS_PIN = 7;      //Pin Slave Select
const int ADS1241_PDWN_PIN = 4;    //Pin Power Down
const int ADS1241_RESET_PIN = 5;   //Pin Reset 

 
void setup() {
  
  /***** Serial Link Configuration ******/
  Serial.begin(115200);
  //while (!Serial);
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
 
  pinMode(ADS1241_DRDY_PIN, INPUT);
  pinMode(ADS1241_CS_PIN, OUTPUT);
  pinMode(ADS1241_PDWN_PIN, OUTPUT);
  pinMode(ADS1241_RESET_PIN, OUTPUT);
  SPI.begin();
  SPI.beginTransaction(SPISettings(1000000,MSBFIRST,SPI_MODE1));
  digitalWrite(ADS1241_RESET_PIN,LOW); 
  delay(1);
  digitalWrite(ADS1241_PDWN_PIN,HIGH);
  digitalWrite(ADS1241_RESET_PIN,HIGH);
  delay(1);        
  writeRegister(ACR_REG,0x40); // Unipolar format
  writeRegister(MUX_REG,AIN0_AIN1); // MUX
  writeRegister(SETUP_REG,GAIN_128);  // GAIN
}

void loop() {
  getWeight();
  if(LOW_POWER == true)   enterLowPower();
  else                    wait();
}


void getWeight(void){
  uint32_t valNum = 0;
  uint32_t valNumAverage = 0;
  bool isTriFinished;
  
  /**** Get Numeric Values ****/
  for(uint8_t i=0;i<NB_MEASUREMENT;i++){
    while(digitalRead(ADS1241_DRDY_PIN)==HIGH);
    valNum = (readRegister(DOR2_REG)<<16) | (readRegister(DOR1_REG)<<8) | readRegister(DOR0_REG);
    Serial.print(" Digital value  : 0x");
    Serial.println(valNum, HEX);
    tabWeight[i]=valNum;
    delay(50);
  }

  /**** Sort the Table ****/
  do{
    isTriFinished=1;
    for(uint8_t i=0;i<NB_MEASUREMENT-1;i++){
      uint32_t temp;
      if(tabWeight[i]>tabWeight[i+1]){
        temp=tabWeight[i+1];
        tabWeight[i+1]=tabWeight[i];
        tabWeight[i]=temp;
        isTriFinished=0;
      }
    }
  }while(isTriFinished==0);
  
/*  #ifdef DEBUG
    for(uint8_t i=0;i<NB_MEASUREMENT;i++){
      Serial.println(tabWeight[i],HEX);
    }
  #endif*/

  /**** Average Calculation with Extrem value removal****/
  for(uint8_t i=NB_EXTREM;i<(NB_MEASUREMENT-NB_EXTREM);i++){
    valNumAverage+=tabWeight[i];
  }
  valNumAverage/=NB_MEASUREMENT-2*NB_EXTREM;
  
  #ifdef DEBUG
  Serial.print(" Digital value average : 0x");
  Serial.println(valNumAverage, HEX);
  #endif  */
  /***** Computation of the analog value ****/
  weight=valNumAverage*coefConversion + originConversion;
  #ifdef DEBUG
  Serial.print(" Weight in grams : ");
  Serial.println(weight);
  Serial.print("coeff directeur : ");
  Serial.println(coefConversion);
  #endif
  Serial.print(" Voltage : ");
  Serial.println(valNumAverage*3.3/16777216,DEC); 
  Serial.println("\n"); 
}


void writeRegister(uint8_t reg,uint8_t valeur)
{
  digitalWrite(ADS1241_CS_PIN,LOW);   // Chip Select Low
  SPI.transfer(WREG_CMD | reg);       // Write Register 
  delay(1);
  SPI.transfer(0x00);                 // Number of register to write
  delay(1);
  SPI.transfer(valeur);               // Value
  digitalWrite(ADS1241_CS_PIN,HIGH);  // Chip Select High
}

uint8_t readRegister(uint8_t reg)
{
  uint8_t data;
  digitalWrite(ADS1241_CS_PIN,LOW);   // Chip Select Low
  SPI.transfer(RREG_CMD | reg);       // Read Register 
  delay(1);
  SPI.transfer(0x00);                 // Number of register to read
  delay(1);
  data = SPI.transfer(0x00);          // Value read
  digitalWrite(ADS1241_CS_PIN,HIGH);  // Chip Select High
  return data;
}


void enterLowPower(void){
  Serial.print(" Processor goes in Low Power mode during : ");
  Serial.print(frameDelay);Serial.println("ms\r\n");
  Serial.print(" Using the Low Power features usually corrupts the Serial Link ");
  LowPower.deepSleep(frameDelay); 
  Serial.println(" Wake Up !!!");  
}

void wait(void){
  //Serial.print(" Processor is going to wait during : ");
  //Serial.print(frameDelay);Serial.println("ms\r\n");
  digitalWrite(ADS1241_PDWN_PIN,LOW);   // Test du Low Power
  delay(frameDelay); 
  digitalWrite(ADS1241_PDWN_PIN,HIGH);
  delay(50); // Wait for the next conversion
}
