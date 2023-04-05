//INA226 HTU21D BMP180 BH1750FVI

// Device found at address 0x23 // BH1750FVI
// Device found at address 0x40 // HTU21D
// Device found at address 0x41 // INA226 [65 太阳能板]  {65, 68} 65太阳能板 68电池
// Device found at address 0x44 // INA226 [68 电池]
// Device found at address 0x77 // BMP180

// INA226：默认地址为0x40。
// HTU21D：默认地址为0x40或0x41，可以通过将SCL引脚连接到GND或VCC来选择地址。
// BMP180：默认地址为0x77。
// BH1750FVI：默认地址为0x23或0x5C，可以通过将ADDR引脚连接到GND或VCC来选择地址。

#include <Arduino.h>
#include <ArduinoOTA.h>
#include <base64.h>

#include <Wire.h>
#include <Adafruit_HTU21DF.h>
#include <BMP180.h>
#include <BH1750.h>
#include <BMP180advanced.h>
#include <INA226.h>

#ifdef ESP8266
    #include <ESP8266WiFi.h>
#else
    #include <WiFi.h>
#endif
#include <WiFiClient.h>
#include <WiFiClientSecure.h>

#include "config.h"

int ledPin = 16;
BH1750 lightMeter(0x23); //光照
Adafruit_HTU21DF htu = Adafruit_HTU21DF();
int ina226_address[] = {0x41, 0x44}; //{65, 68} 65太阳能板 68电池
INA226 ina(Wire);
BMP180advanced myBMP(BMP180_ULTRAHIGHRES);

//时间全局变量
struct tm timeinfo;

// 创建TCP服务器对象
const int serverPort = 8888;
WiFiServer server(serverPort);

WiFiClient tcp_client;
WiFiClient http_client;
WiFiClient http_client_2;

void hasClient (){
  // 检查是否有客户端连接
  if (server.hasClient()) {
    if (!tcp_client.connected()) {
      tcp_client = server.available();
      if (tcp_client) {
        Serial.println("New client connected");
        tcp_client.write(String("connection succeeded\n").c_str());
      }
    }
  }
}
void tcp_send (String content){
  char message[100];
  // 发送数据给客户端
  if (tcp_client && tcp_client.connected()) {
    snprintf(message, sizeof(message), "%s", content.c_str());
    tcp_client.write(message);
  }
  Serial.write(message);
}
void tcp_send_ln (String content){
  char message[100];
  snprintf(message, sizeof(message), "%s\n", content.c_str());
  // 发送数据给客户端
  if (tcp_client && tcp_client.connected()) {
    tcp_client.write(message);
  }
  Serial.write(message);
}
void tcp_send_only (String content){
  char message[100];
  snprintf(message, sizeof(message), "%s\n", content.c_str());
  // 发送数据给客户端
  if (tcp_client && tcp_client.connected()) {
    tcp_client.write(message);
  }
}

void INA226_check(int address){
  tcp_send_ln("INA226_check address:" + String(address));
  bool success = ina.begin(address);
  if(!success)
  {
    //Serial.println("INA226 error");
    tcp_send_ln("INA226 error");
    delay(500);
  }else{
    tcp_send("Mode:                  ");
    switch (ina.getMode())
    {
      case INA226_MODE_POWER_DOWN:      tcp_send_ln("Power-Down"); break;
      case INA226_MODE_SHUNT_TRIG:      tcp_send_ln("Shunt Voltage, Triggered"); break;
      case INA226_MODE_BUS_TRIG:        tcp_send_ln("Bus Voltage, Triggered"); break;
      case INA226_MODE_SHUNT_BUS_TRIG:  tcp_send_ln("Shunt and Bus, Triggered"); break;
      case INA226_MODE_ADC_OFF:         tcp_send_ln("ADC Off"); break;
      case INA226_MODE_SHUNT_CONT:      tcp_send_ln("Shunt Voltage, Continuous"); break;
      case INA226_MODE_BUS_CONT:        tcp_send_ln("Bus Voltage, Continuous"); break;
      case INA226_MODE_SHUNT_BUS_CONT:  tcp_send_ln("Shunt and Bus, Continuous"); break;
      default: tcp_send_ln("unknown");
    }
    
    tcp_send("Samples average:       ");
    switch (ina.getAverages())
    {
      case INA226_AVERAGES_1:           tcp_send_ln("1 sample"); break;
      case INA226_AVERAGES_4:           tcp_send_ln("4 samples"); break;
      case INA226_AVERAGES_16:          tcp_send_ln("16 samples"); break;
      case INA226_AVERAGES_64:          tcp_send_ln("64 samples"); break;
      case INA226_AVERAGES_128:         tcp_send_ln("128 samples"); break;
      case INA226_AVERAGES_256:         tcp_send_ln("256 samples"); break;
      case INA226_AVERAGES_512:         tcp_send_ln("512 samples"); break;
      case INA226_AVERAGES_1024:        tcp_send_ln("1024 samples"); break;
      default: tcp_send_ln("unknown");
    }

    tcp_send("Bus conversion time:   ");
    switch (ina.getBusConversionTime())
    {
      case INA226_BUS_CONV_TIME_140US:  tcp_send_ln("140uS"); break;
      case INA226_BUS_CONV_TIME_204US:  tcp_send_ln("204uS"); break;
      case INA226_BUS_CONV_TIME_332US:  tcp_send_ln("332uS"); break;
      case INA226_BUS_CONV_TIME_588US:  tcp_send_ln("558uS"); break;
      case INA226_BUS_CONV_TIME_1100US: tcp_send_ln("1.100ms"); break;
      case INA226_BUS_CONV_TIME_2116US: tcp_send_ln("2.116ms"); break;
      case INA226_BUS_CONV_TIME_4156US: tcp_send_ln("4.156ms"); break;
      case INA226_BUS_CONV_TIME_8244US: tcp_send_ln("8.244ms"); break;
      default: tcp_send_ln("unknown");
    }

    tcp_send("Shunt conversion time: ");
    switch (ina.getShuntConversionTime())
    {
      case INA226_SHUNT_CONV_TIME_140US:  tcp_send_ln("140uS"); break;
      case INA226_SHUNT_CONV_TIME_204US:  tcp_send_ln("204uS"); break;
      case INA226_SHUNT_CONV_TIME_332US:  tcp_send_ln("332uS"); break;
      case INA226_SHUNT_CONV_TIME_588US:  tcp_send_ln("558uS"); break;
      case INA226_SHUNT_CONV_TIME_1100US: tcp_send_ln("1.100ms"); break;
      case INA226_SHUNT_CONV_TIME_2116US: tcp_send_ln("2.116ms"); break;
      case INA226_SHUNT_CONV_TIME_4156US: tcp_send_ln("4.156ms"); break;
      case INA226_SHUNT_CONV_TIME_8244US: tcp_send_ln("8.244ms"); break;
      default: tcp_send_ln("unknown");
    }
    
    tcp_send("Max possible current:  ");
    tcp_send(String(ina.getMaxPossibleCurrent()));
    tcp_send_ln(" A");

    tcp_send("Max current:           ");
    tcp_send(String(ina.getMaxCurrent()));
    tcp_send_ln(" A");

    tcp_send("Max shunt voltage:     ");
    tcp_send(String(ina.getMaxShuntVoltage()));
    tcp_send_ln(" V");

    tcp_send("Max power:             ");
    tcp_send(String(ina.getMaxPower()));
    tcp_send_ln(" W");
  }
}
String INA226_read(int timestamp,int address){
  tcp_send_ln("INA226_read address:" + String(address));
  String result = "";
  bool success = ina.begin(address);
  if(!success)
  {
    //Serial.println("INA226 error");
    tcp_send_ln("INA226 error");
    delay(500);
  }else{
    String Busvoltage = String (ina.readBusVoltage(), 5);
    String BusPower = String (ina.readBusPower(), 5);
    String ShuntVoltage = String (ina.readShuntVoltage(), 5);
    String ShuntCurrent = String (ina.readShuntCurrent(), 5);

    tcp_send("Bus voltage:   ");
    tcp_send(Busvoltage);
    tcp_send_ln(" V");

    tcp_send("Bus power:     ");
    tcp_send(BusPower);
    tcp_send_ln(" W");


    tcp_send("Shunt voltage: ");
    tcp_send(ShuntVoltage);
    tcp_send_ln(" V");

    tcp_send("Shunt current: ");
    tcp_send(ShuntCurrent);
    tcp_send_ln(" A");
    
    //{0x41, 0x44} {65, 68} 65太阳能板 68电池
    String type;
    if (address == 0x41){
      type = "SolarPanels";
    }else if (address == 0x44)
    {
      type = "Battery";
    }else{
      type = "error";
    }
    if(type != "error"){
      result =  type + ",tag=1 Voltage=" + Busvoltage + " " + timestamp + "\n" +
                type + ",tag=1 Current=" + ShuntCurrent + " " + timestamp + "\n" +
                type + ",tag=1 watts=" + BusPower + " " + timestamp + "\n"
                ;
    }
  }
  tcp_send_ln("");
  return result;
}
void INA226_config(){
  for (int i = 0; i < 2; i++) {
    bool success = ina.begin(ina226_address[i]);
    tcp_send_ln("INA226_config address:" + String(ina226_address[i]));
    if(!success)
    {
      //Serial.println("INA226 error");
      tcp_send_ln("INA226 error");
      delay(500);
    }else{
      // Configure INA226
      ina.configure(INA226_AVERAGES_1, INA226_BUS_CONV_TIME_1100US, INA226_SHUNT_CONV_TIME_1100US, INA226_MODE_SHUNT_BUS_CONT);
      // Calibrate INA226. Rshunt = 0.01 ohm, Max excepted current = 4A
      ina.calibrate(0.1, 0.5);
    }
  }
}

String BH1750_read(int timestamp){
  tcp_send_ln("BH1750_read..");
  String result = "";
  if (lightMeter.measurementReady()) {
    float lux = lightMeter.readLightLevel();
    if(lux>=0){
      tcp_send_ln("Light:\t" + String(lux) + " lx");
      result = "Weather,tag=Brightness Brightness=" + String(lux) + " " + timestamp + "\n";
    }else{
      tcp_send_ln("BH1750 error Light: " + String(lux) + " lx");
    }
  }else{
    tcp_send_ln("BH1750 error");
    delay(500);
  }
  tcp_send_ln("");
  return result;
}

String HTU21D_read(int timestamp){
  tcp_send_ln("HTU21D_read..");
  String result = "";
  if (htu.begin()) {
    float temp = htu.readTemperature();
    float rel_hum = htu.readHumidity();
    float dew_point = temp - (100 - rel_hum) / 5; //计算露点
    tcp_send_ln("temp:\t" + String(temp));
    tcp_send_ln("hum:\t" + String(rel_hum));
    tcp_send_ln("dp:\t" + String(dew_point));
    result = "Weather,tag=Temperature Temperature=" + String(temp) + " " + timestamp + "\n" +
                    "Weather,tag=Humidity Humidity=" + String(rel_hum) + " " + timestamp + "\n" +
                    "Weather,tag=dewpoint dewpoint=" + String(dew_point) + " " + timestamp + "\n"
                    ;
  }else{
    tcp_send_ln("HTU21D error");
  }
  tcp_send_ln("");
  return result;
}

String BMP180_read(int timestamp){
  tcp_send_ln("BMP180_read..");
  String result = "";
  if(myBMP.begin()){
    tcp_send_ln("Temp:\t"+ String(myBMP.getTemperature(), 1) + " +-1.0C");
    tcp_send_ln("Pa:\t"  + String(myBMP.getPressure())       + " +-100Pa");

    String Pressure_hPa = String(myBMP.getPressure_hPa());
    tcp_send_ln("hPa:\t"  + Pressure_hPa + " +-1hPa");
    tcp_send_ln("mmHg:\t" + String(myBMP.getPressure_mmHg()) + " +-0.75mmHg");
    tcp_send_ln("inHg:\t" + String(myBMP.getPressure_inHg()) + " +-0.03inHg");

    tcp_send_ln("SeaLevel hPa:\t"  + String(myBMP.getSeaLevelPressure_hPa(25))  + " hPa");
    tcp_send_ln("SeaLevel mmHg:\t" + String(myBMP.getSeaLevelPressure_mmHg(25)) + " mmHg");
    tcp_send_ln("SeaLevel inHg:\t" + String(myBMP.getSeaLevelPressure_inHg(25)) + " inHg");

    int Forecast = myBMP.getForecast(25);
    switch (Forecast)
    {
      case 0: tcp_send_ln(F("thunderstorm")); break;
      case 1: tcp_send_ln(F("rain")); break;
      case 2: tcp_send_ln(F("cloudy")); break;
      case 3: tcp_send_ln(F("partly cloudy")); break;
      case 4: tcp_send_ln(F("clear")); break;
      case 5: tcp_send_ln(F("sunny")); break;
    }

    result = "Weather,tag=Pressure Pressure=" + Pressure_hPa + " " + timestamp + "\n" +
             "Weather,tag=Forecast Forecast=" + Forecast + " " + timestamp + "\n"
             ;
  }else{
    tcp_send_ln("BMP180 error");
  }
  tcp_send_ln("");
  return result;
}

void i2c_Scanning() {
  byte error, address;
  int deviceCount = 0;

  //Serial.println("Scanning...");
  tcp_send_ln("Scanning...");

  for (address = 1; address < 127; address++) {
    char hexValue[3];
    Wire.beginTransmission(address);
    error = Wire.endTransmission();

    if (error == 0) {
      //Serial.print("Device found at address 0x");
      tcp_send("Device found at address 0x");
      if (address < 16) {
        //Serial.print("0");
        tcp_send("0");
      }
      sprintf(hexValue, "%02X", address);
      //Serial.println(hexValue);
      tcp_send_ln(String(hexValue));

      deviceCount++;
    } else if (error == 4) {
      //Serial.print("Unknown error at address 0x");
      tcp_send("Unknown error at address 0x");
      if (address < 16) {
        //Serial.print("0");
        tcp_send("0");
      }
      sprintf(hexValue, "%02X", address);
      //Serial.println(hexValue);
      tcp_send_ln(String(hexValue));
    }
  }
  if (deviceCount == 0) {
    //Serial.println("No I2C devices found\n");
    tcp_send_ln("No I2C devices found\n");
  } else {
    //Serial.println("Scan complete\n");
    tcp_send_ln("Scan complete\n");
  }
}

void setup() {
  // put your setup code here, to run once:
  pinMode(ledPin, OUTPUT);
  // Wire.begin(D1, D2);

  Serial.begin(115200);
  Serial.println("");
  Serial.println("");
  Serial.println("Booting...");

  WiFi.mode(WIFI_STA);

  WiFi.setHostname(hostname);
  WiFi.begin(ssid, password);

  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("Connection Failed! Rebooting...");
    delay(5000);
    ESP.restart();
  }


  ///////////////////////////////////////////// OTA ////
  ArduinoOTA.setPort(8266);
  ArduinoOTA.setHostname(hostname);
  ArduinoOTA.setPassword("px6216e3");

  // Password can be set with it's md5 value as well
  // MD5(admin) = 21232f297a57a5a743894a0e4a801fc3
  // ArduinoOTA.setPasswordHash("21232f297a57a5a743894a0e4a801fc3");

  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else {  // U_FS
      type = "filesystem";
    }

    // NOTE: if updating FS this would be the place to unmount FS using FS.end()
    tcp_send_ln("|--Start updating--|" + type);
    tcp_client.stop();
    Serial.println("|--Start updating--|" + type);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
    for (int i = 0; i < 2; i++) {
      digitalWrite(ledPin, LOW);
      delay(100);
      digitalWrite(ledPin, HIGH);
      delay(100);
    }
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r\n", (progress / (total / 100)));
    digitalWrite(ledPin, LOW);
    delay(1);
    digitalWrite(ledPin, HIGH);
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      Serial.println("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      Serial.println("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      Serial.println("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      Serial.println("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      Serial.println("End Failed");
    }
    for (int i = 0; i < 5; i++) {
      digitalWrite(ledPin, LOW);
      delay(100);
      digitalWrite(ledPin, HIGH);
      delay(100);
    }
  });
  ArduinoOTA.begin();

  ///////////////////////////////////////////// WIFI ////
  Serial.println("Connected to WiFi");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  ///////////////////////////////////////// TCP SERVER ////
  server.begin();
  Serial.print("Server started on port ");
  Serial.println(serverPort);


  for (int i = 0; i < 20; i++) {
    Serial.print("Check Updates|");
    hasClient ();
    tcp_send_only("Check Updates|");
    ArduinoOTA.handle();
    delay(500);
  }
  Serial.println("");
  tcp_send_ln("");


  //////////////////////////////////////////// NTP ////
  // Set time via NTP, as required for x.509 validation
  configTime(8 * 3600, 0, "ntp.ntsc.ac.cn", "ntp.aliyun.com");

  Serial.print("Waiting for NTP time sync: ");
  tcp_send_only("Waiting for NTP time sync: ");
  time_t now = time(nullptr);
  while (now < 8 * 3600 * 2) {
    delay(500);
    Serial.print(".");
    tcp_send_only(".");
    now = time(nullptr);
  }
  //Serial.println("");
  tcp_send_ln("");
  struct tm timeinfo;
  localtime_r(&now, &timeinfo);
  //Serial.print("Current time: ");
  //Serial.println(asctime(&timeinfo));
  tcp_send_ln("Current time: " + String (asctime(&timeinfo)));

  Wire.begin();
////////////////////////////////// INA226 ////
  INA226_config();
  for (int i = 0; i < 2; i++) {
    INA226_check(ina226_address[i]);
  }
////////////////////////////////// HTU21D ////
  if (!htu.begin()) {
    //Serial.println("HTU21D error");
    tcp_send("HTU21D error");
    delay(500);
  }
////////////////////////////////// BH1750 ////
  if (!lightMeter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE)) {
    tcp_send("BH1750 error");
    delay(500);
  }

  // i2c_Scanning();

  digitalWrite(ledPin, HIGH);
  delay(500);
  digitalWrite(ledPin, LOW);
  delay(500);
  digitalWrite(ledPin, HIGH);

  // delay(2000);
}

void loop() {
    // put your main code here, to run repeatedly:

  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("Connection Failed! Rebooting...");
    delay(5000);
    ESP.restart();
  }
  
  for (int i = 0; i < 2; i++) {
    digitalWrite(ledPin, LOW);
    delay(100);
    digitalWrite(ledPin, HIGH);
    delay(100);
  }

  //Serial.println("================ Version: " + String(VER) + " ================");
  tcp_send_ln("================ Version: " + String(VER) + " ================");

  i2c_Scanning();

  Serial.print("Delay & Check Updates");
  tcp_send_only("Delay & Check Updates");
  for (int i = 0; i < 60; i++) {
    Serial.print(".");
    tcp_send_only(".");
    hasClient ();
    ArduinoOTA.handle();
    delay(1000);
  }
  // delay(10000);

  //Serial.println("execute");
  tcp_send_ln("execute");
  
  for (int i = 0; i < 1; i++) {
    digitalWrite(ledPin, LOW);
    delay(100);
    digitalWrite(ledPin, HIGH);
    delay(100);
  }


  time_t now = time(nullptr);

  // gmtime_r(&now, &timeinfo);
  // Serial.print("UTC   time: ");
  // Serial.print(asctime(&timeinfo));

  localtime_r(&now, &timeinfo);
  // Serial.print("Local time: ");
  // Serial.print(asctime(&timeinfo));

  char* timeStr = asctime(&timeinfo);

  // 去除末尾的 \n
  size_t len = strlen(timeStr);
  if (len > 0 && timeStr[len - 1] == '\n') {
      timeStr[len - 1] = '\0';
  }
  
  String printStr = "Time: " + String(now) + " " + timeStr;
  //Serial.print(printStr);
  tcp_send_ln(printStr);
  tcp_send_ln("");
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

  String postData = "";
  
  String HTU21D_Data = HTU21D_read(now);
  String BMP180_Data = BMP180_read(now);
  String BH1750_Data = BH1750_read(now);
  String INA226_41 = INA226_read(now,0x41);
  String INA226_44 = INA226_read(now,0x44);

  // tcp_send(HTU21D_Data);
  // tcp_send(BMP180_Data);
  // tcp_send(BH1750_Data);
  // tcp_send(INA226_41);
  // tcp_send(INA226_44);

  postData = HTU21D_Data + BMP180_Data + BH1750_Data + INA226_41 + INA226_44;
  tcp_send_ln(postData);

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  
  if(postData != ""){
    if (http_client.connect(api_serverName, api_port)) { // Replace with your server URL and port
      http_client.println(String("POST ") + api_url + " HTTP/1.1"); // Replace with your API endpoint
      http_client.println(String("Host: ") + api_serverName); // Replace with your server URL
      http_client.println("Content-Type: application/x-www-form-urlencoded");
      http_client.print("Content-Length: ");
      http_client.println(postData.length());
      // 添加Authorization头部
      http_client.print("Authorization: Basic ");
      String base64_auth = base64::encode(api_auth);
      http_client.println(base64_auth);
      http_client.println();
      http_client.print(postData);
    } else {
      // Serial.println("Failed to connect to server");
      tcp_send_ln("Failed to connect to server");
    }

    while (http_client.connected()) {
      if (http_client.available()) {
        String line = http_client.readStringUntil('\n');
        tcp_send_ln(line);
      }
    }

    if (http_client_2.connect(api2_serverName, api2_port)) { // Replace with your server URL and port
      http_client_2.println(String("POST ") + api2_url + " HTTP/1.1"); // Replace with your API endpoint
      http_client_2.println(String("Host: ") + api2_serverName); // Replace with your server URL
      http_client_2.println("Content-Type: application/x-www-form-urlencoded");
      http_client_2.print("Content-Length: ");
      http_client_2.println(postData.length());
      http_client_2.println();
      http_client_2.print(postData);
    } else {
      // Serial.println("Failed to connect to server");
      tcp_send_ln("Failed to connect to server");
    }

    while (http_client_2.connected()) {
      if (http_client_2.available()) {
        String line = http_client_2.readStringUntil('\n');
        tcp_send_ln(line);
      }
    }
  }
  
  tcp_send_ln("");
  tcp_send_ln("=====END=====");
  tcp_send_ln("");
}


