
#include <Wire.h>
#include <OneWire.h>
#include <DallasTemperature.h>

#include <SPIFlash.h>
#include <Scheduler.h>

#include "nRF24L01.h"
#include "RF24.h"
#include "printf.h"
#include "RF24Network.h"
#include "RF24Mesh.h"

#include <SHA204.h>
#include <SHA204Definitions.h>
#include "SHA204ReturnCodes.h"
#include <SHA204I2C.h>

// ADS1100 I2C address is 0x48(72)
#define Addr 0x48

#define FLASH_SS 8
#define SPDT_SWITCH 3

RF24 radio(4,7);
RF24Network network(radio);
RF24Mesh mesh(radio, network);

SPIFlash flash(FLASH_SS, 0x1F65);

SHA204I2C sha204dev(0x64);

#define ONE_WIRE_BUS 18
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);


void setup() {
  
  Serial.begin(115200);

  pinMode(SPDT_SWITCH, OUTPUT);
  pinMode(4,OUTPUT);
  
  pinMode(8,OUTPUT);
  pinMode(9,OUTPUT);
  pinMode(ONE_WIRE_BUS, OUTPUT);

  digitalWrite(4,LOW);

  digitalWrite(SPDT_SWITCH, LOW);
  digitalWrite(8,HIGH);
  digitalWrite(9,HIGH);
  digitalWrite(ONE_WIRE_BUS, LOW);

  Scheduler.begin(256);

  // to give oportunity to i2c comunication
  oneWire.skip();

  // NRF24 initialization
  printf_begin();
  mesh.setNodeID(0x01);
  // Connect to the mesh
  Serial.println(F("Connecting to the mesh..."));
  mesh.begin();

  /*radio.begin();
  radio.setChannel(1);
  radio.setDataRate(RF24_1MBPS);
  radio.setAutoAck(false);
  //radio.disableCRC();
  radio.openWritingPipe( 0x5544332211LL ); // set address for outcoming messages
  radio.openReadingPipe( 1, 0x5544332211LL ); // set address for incoming messages*/
  
  radio.printDetails();

  // External Flash initialization
  if (flash.initialize())
  {
    Serial.println("Flash initialized.");
  }
  else
  {
    Serial.println("Flash FAIL!");
  }

  Serial.print("DeviceID: ");
  Serial.println(flash.readDeviceId(), HEX);

/*
  Serial.print("Erasing Flash chip ... ");
  flash.chipErase();
  while(flash.busy());
  Serial.println("DONE");

  for(int i=0; i < 100; i++)
  {
    flash.writeByte(i, 0x35+i);
  }
*/

  // read test
  for(int i=0; i < 100; i++)
  {
    Serial.print(flash.readByte(i), HEX);
    Serial.print(" ");
  }

  delay(1000);

  digitalWrite(18,LOW);
  delay(1000);
  
  // crypto chip
  Wire.begin();
  //Wire.setClock(400000L);
  //Wire.setClock(100000L);
  //sha204dev.init();
  
  auto wakeupExample = []()
  {
    uint8_t response[SHA204_RSP_SIZE_MIN];
    byte returnValue;
    
    returnValue = sha204dev.resync(4, &response[0]);
    for (int i=0; i<SHA204_RSP_SIZE_MIN; i++) {
      Serial.print(response[i], HEX);
      Serial.print(" ");
    }
    Serial.println();
  };

  auto serialNumberExample = []()
  {
    uint8_t serialNumber[9];
    byte returnValue;
    
    returnValue = sha204dev.serialNumber(&serialNumber[0]);
    for (int i=0; i<9; i++) {
      Serial.print(serialNumber[i], HEX);
      Serial.print(" ");
    }
    Serial.println();
   
    Serial.println("-------"); 
    
    return returnValue;
  };
  
  //wakeupExample(); // dummy
 // for(int i=0; i<100; i++)
  {
    //delay(1000);
    //for(int i=0; i<5; i++)
  //serialNumberExample(); // dummy
  }

  auto macChallengeExample = []()
{
  uint8_t command[MAC_COUNT_LONG];
  uint8_t response[MAC_RSP_SIZE];

  const uint8_t challenge[MAC_CHALLENGE_SIZE] = {
    0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
    0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF,
    0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
    0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF
  };

  uint8_t ret_code = sha204dev.execute(SHA204_MAC, 0, 0, MAC_CHALLENGE_SIZE, 
    (uint8_t *) challenge, 0, NULL, 0, NULL, sizeof(command), &command[0], 
    sizeof(response), &response[0]);

  for (int i=0; i<SHA204_RSP_SIZE_MAX; i++)
  {
    Serial.print(response[i], HEX);
    Serial.print(' ');
  }
  Serial.println();
  
  return ret_code;
};

  //macChallengeExample();


  auto dump_configuration = []()
  {
    uint8_t readCommand[READ_COUNT];
    uint8_t readResponse[READ_4_RSP_SIZE];
  
    // list all configurations
    for(int i=0; i<= 0x15; i++)
    {
      uint8_t returnCode = sha204dev.read(readCommand, readResponse, SHA204_ZONE_CONFIG, i*4);
      printf("%02X ", readResponse[SHA204_BUFFER_POS_DATA], HEX);
      printf("%02X ", readResponse[SHA204_BUFFER_POS_DATA+1], HEX);
      printf("%02X ", readResponse[SHA204_BUFFER_POS_DATA+2], HEX);
      printf("%02X\n", readResponse[SHA204_BUFFER_POS_DATA+3], HEX);
    }
  };

  auto isLocked = []()
  {
    uint8_t tx_buffer[READ_COUNT];
    uint8_t rx_buffer[READ_4_RSP_SIZE];
    uint8_t ret_code;
    uint8_t lockConfig = 0;
    uint8_t lockValue = 0;
    
    // Read out lock config bits to determine if locking is possible
    ret_code = sha204dev.read(tx_buffer, rx_buffer, SHA204_ZONE_CONFIG, 0x15<<2);
    if (ret_code != SHA204_SUCCESS)
    {
      Serial.print(F("Failed to determine device lock status. Response: ")); Serial.println(ret_code, HEX);
      return ret_code;
    }
    else
    {
      lockConfig = rx_buffer[SHA204_BUFFER_POS_DATA+3];
      lockValue = rx_buffer[SHA204_BUFFER_POS_DATA+2];
    }

    return ret_code;
    
  };

  dump_configuration();
  isLocked();

  Scheduler.start(setup_status, loop_status);
  Scheduler.start(setup_i2c, loop_i2c);

}

void setup_status()
{
  
}

void loop_status()
{
  static bool togle = false;

  if(togle)
    digitalWrite(9,LOW);
  else
    digitalWrite(9,HIGH);

  togle = !togle;
  
  delay(100); 
}

void setup_i2c()
{
  // Initialise I2C communication as MASTER
  //Wire.begin();
  // Start I2C Transmission
  Wire.beginTransmission(Addr);
  // Continuous conversion mode, 8 SPS, 1PGA
  Wire.write(0x0C);
  // Stop I2C Transmission
  Wire.endTransmission();

}

void loop_i2c()
{
  
 unsigned int data[2];

  // Request 2 bytes of data
  Wire.requestFrom(Addr, 2);

  // Read 2 bytes of data
  // raw_adc msb, raw_adc lsb
  if (Wire.available() == 2)
  {
    data[0] = Wire.read();
    data[1] = Wire.read();
  }

  // Convert the data
  float raw_adc = (data[0] * 256.0) + data[1];
  if (raw_adc > 32767)
  {
    raw_adc -= 65536;
  }

  // Output data to serial monitor
  //Serial.print("Digital Value of Analog Input : ");
  //Serial.println(raw_adc);

  // 50v -- 65536 | x -- raw_adc -> normalization 1.056
  Serial.print("Voltage: ");
  Serial.println((((float)(raw_adc*50))/65536)*1.056*2);

  Wire.end();
  
  //delay(100);
  
  sensors.begin();

  Serial.print("Requesting temperatures...");
  sensors.requestTemperatures(); // Send the command to get temperatures
  Serial.println("DONE");
  Serial.print("Temperature for the device 1 (index 0) is: ");
  Serial.println(sensors.getTempCByIndex(0));

  oneWire.skip();
  Wire.begin();

  delay(1000);

}

uint32_t displayTimer = 0;
void loop() {

  mesh.update();

  displayTimer = millis();

  // Send an 'M' type message containing the current millis()
  if (!mesh.write(&displayTimer, 'M', sizeof(displayTimer))) {

    // If a write fails, check connectivity to the mesh network
    if ( ! mesh.checkConnection() ) {
      //refresh the network address
      Serial.println("Renewing Address");
      mesh.renewAddress();
    } else {
      Serial.println("Send fail, Test OK");
    }
  } else {
    Serial.print("Send OK: "); Serial.println(displayTimer);
  }

  delay(1000);

}
