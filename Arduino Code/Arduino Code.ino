#include <SoftwareSerial.h> // library for wifi module
#include <MQUnifiedsensor.h> // library for MQ135 sensor
#include <dht.h> // library for DHT11 sensor

SoftwareSerial esp(10, 11); // TX=10, RX=11
unsigned long starttime;

String myAPIkey = "2RBIX9P3YKF4M5UI"; // API Key to write the data on the channel

//these will be used to measure different values from the sensors
float aqi = 0;
float CO = 0;
float Alcohol = 0;
float CO2 = 0;
float NH4 = 0;
float Acetone = 0;
float temp = 0;
float humidity = 0;

//definitions for MQ135
MQUnifiedsensor MQ135("Arduino UNO", 5, 10, A0, "MQ-135"); // MQ135(arduino, Voltage_Resolution, ADC_Bit_Resolution, pin, type);

//definition for DHT sensor
dht DHT;

// setup would run once
void setup() 
{                
  //start serial communications
  Serial.begin(9600); //default port for serial monitor
  esp.begin(115200); //default port for ESP8266

  //message for initializing
  Serial.println("Initializing...");

  //enable software serial
  esp.println("AT");
  delay(2000);

  //if the wifi odule is found
  if(esp.find("OK")){
      Serial.println("Communication with ESP8266 module: OK");
  }
  //in case we don't find it
  else {
    Serial.println("ESP8266 module ERROR");
  }

  //trying to connect to wifi
  Serial.println("Connecting wi-fi...");
  String cmd ="AT+CWMODE=1";
  esp.println(cmd);
  delay(2000);
  esp.flush();
  esp.println("AT+CWJAP=\"nopee\",\"welp22welp\"\r\n"); //contains SSID=nopee, password=welp22welp
  delay(5000);

  //if connection succeeds
  if(esp.find("OK")){
    Serial.println("Connection succeeded!");
  }
  //in case it fails
  else{
    Serial.println("Connection failed!");
  }
  Serial.println();

  //Setting up MQ135

  //Set math model to calculate the PPM concentration and the value of constants
  MQ135.setRegressionMethod(1); //PPM =  a*ratio^b
  
  MQ135.init(); //configure the pin of arduino

  // In this routine the sensor will measure the resistance of the sensor supposedly before being pre-heated
  // and on clean air (Calibration conditions), setting up R0 value.
  Serial.print("Calibrating please wait.");
  float calcR0 = 0;
  for(int i = 1; i<=10; i ++)
  {
    MQ135.update(); // Update data, the arduino will be read the voltage on the analog pin
    calcR0 += MQ135.calibrate(3.6);
    Serial.print(".");
  }
  MQ135.setR0(calcR0/10);
  Serial.println("  done!.");

  //keeping errors in check  
  if(isinf(calcR0)) {Serial.println("Warning: Conection issue founded, R0 is infite (Open circuit detected) please check your wiring and supply"); while(1);}
  if(calcR0 == 0){Serial.println("Warning: Conection issue founded, R0 is zero (Analog pin with short circuit to ground) please check your wiring and supply"); while(1);}
  //MQ Setup and calibration finished

  //initializing sampling
  Serial.print("Sampling...");
}



// the loop 

void loop()

{
  //reading the values of sensors
  aqi = analogRead(A0);
  DHT.read11(2);

  //printing the AQI on monitor
  Serial.print("Airquality = ");
  Serial.println(aqi);

  /*
    Exponential regression:
  GAS      | a      | b
  CO       | 605.18 | -3.937  
  Alcohol  | 77.255 | -3.18 
  CO2      | 110.47 | -2.862
  Toluen  | 44.947 | -3.445
  NH4      | 102.2  | -2.473
  Aceton  | 34.668 | -3.369
  */

  // MQ Reading and updating section
  MQ135.update(); // Update data, the arduino will be read the voltage on the analog pin
  //setting to measure CO conc. in PPM
  MQ135.setA(605.18); MQ135.setB(-3.937);
  CO = MQ135.readSensor();

  //setting to measure Alcohol conc. in PPM
  MQ135.setA(77.255); MQ135.setB(-3.18);
  Alcohol = MQ135.readSensor();

  //setting to measure CO2 conc. in PPM
  MQ135.setA(110.47); MQ135.setB(-2.862);
  CO2 = MQ135.readSensor();

  //setting to measure NH4 conc. in PPM
  MQ135.setA(102.2 ); MQ135.setB(-2.473);
  NH4 = MQ135.readSensor();

  //setting to measure Acetone conc. in PPM
  MQ135.setA(34.668); MQ135.setB(-3.369);
  Acetone = MQ135.readSensor();

  //reading temperature
  temp = DHT.temperature;

  //reading humidity
  humidity = DHT.humidity;

  //calling funtion to write to Thingspeak using ESP8266 module
  writeThingSpeak();

  //creating 60 seconds delay
  Serial.print("Wait 60 seconds");
  delay(60000);


  //initializing new cycle
  Serial.println();
  Serial.print("Sampling...");
  starttime = millis();
}

//function to write to thingspeak
void writeThingSpeak(void)
{
  //calling function to connect to thingspeak
  startThingSpeakCmd();

  // preparing the string GET
  String getStr = "GET /update?api_key=";
  
  getStr += myAPIkey;
  getStr +="&field1=";
  getStr += String(aqi);
  getStr +="&field2=";
  getStr += String(temp);
  getStr +="&field3=";
  getStr += String(CO);
  getStr +="&field4=";
  // We have added 400 PPM because when the library is calibrated it assumes the current state of the
  // air as 0 PPM, and it is considered today that the CO2 present in the atmosphere is around 400 PPM.
  getStr += String(CO2+400);
  getStr +="&field5=";
  getStr += String(NH4);
  getStr +="&field6=";
  getStr += String(Alcohol);
  getStr +="&field7=";
  getStr += String(Acetone);
  getStr +="&field8=";
  getStr += String(humidity);
  getStr += "\r\n\r\n";

  //calling function to update the value
  GetThingspeakcmd(getStr); 

}

//function to connect to thingspeak
void startThingSpeakCmd(void)
{
  esp.flush();
  String cmd = "AT+CIPSTART=\"TCP\",\"";
  cmd += "184.106.153.149"; // api.thingspeak.com IP address
  cmd += "\",80";
  esp.println(cmd);

  if(esp.find("Error"))
  {
    return;
  }
}

//function to update the value
String GetThingspeakcmd(String getStr)
{
  String cmd = "AT+CIPSEND=";
  cmd += String(getStr.length());
  esp.println(cmd);

  if(esp.find(">"))
  {
    esp.print(getStr);
    Serial.println(getStr);
    delay(500);
    String messageBody = "";
    while (esp.available()) 
    {
      String line = esp.readStringUntil('\n');
      if (line.length() == 1) 
      { 
        messageBody = esp.readStringUntil('\n');
      }
    }
    return messageBody;
  }
  else
  {
    esp.println("AT+CIPCLOSE");     
    Serial.println("AT+CIPCLOSE");
  }
}
