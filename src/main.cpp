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
#include <EEPROM.h>
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

#include <ESP8266WebServer.h>
ESP8266WebServer webserver(80);
void startWebServer();
String urlDecode(String input);
String makePage(String title, String contents);

boolean first_start = true;
String postData = "";
String last_post_time = "null";

int ledPin = 16;
BH1750 lightMeter(0x23); //光照
Adafruit_HTU21DF htu = Adafruit_HTU21DF();
int ina226_address[] = {0x41, 0x44}; //{65, 68} 65太阳能板 68电池
float Battery_Voltage = 0;

// 继电器参数
const int JD1 = D5;
const int JD2 = D6;
const int address = 0; //EEPROM地址

INA226 ina(Wire);
BMP180advanced myBMP(BMP180_ULTRAHIGHRES);

//时间全局变量
struct tm timeinfo;

WiFiClient http_client;
WiFiClient http_client_2;

void INA226_check(int address){
  Serial.println("INA226_check address:" + String(address));
  bool success = ina.begin(address);
  if(!success)
  {
    Serial.println("INA226 error");
    delay(500);
  }else{
    Serial.print("Mode:                  ");
    switch (ina.getMode())
    {
      case INA226_MODE_POWER_DOWN:      Serial.println("Power-Down"); break;
      case INA226_MODE_SHUNT_TRIG:      Serial.println("Shunt Voltage, Triggered"); break;
      case INA226_MODE_BUS_TRIG:        Serial.println("Bus Voltage, Triggered"); break;
      case INA226_MODE_SHUNT_BUS_TRIG:  Serial.println("Shunt and Bus, Triggered"); break;
      case INA226_MODE_ADC_OFF:         Serial.println("ADC Off"); break;
      case INA226_MODE_SHUNT_CONT:      Serial.println("Shunt Voltage, Continuous"); break;
      case INA226_MODE_BUS_CONT:        Serial.println("Bus Voltage, Continuous"); break;
      case INA226_MODE_SHUNT_BUS_CONT:  Serial.println("Shunt and Bus, Continuous"); break;
      default: Serial.println("unknown");
    }
    
    Serial.print("Samples average:       ");
    switch (ina.getAverages())
    {
      case INA226_AVERAGES_1:           Serial.println("1 sample"); break;
      case INA226_AVERAGES_4:           Serial.println("4 samples"); break;
      case INA226_AVERAGES_16:          Serial.println("16 samples"); break;
      case INA226_AVERAGES_64:          Serial.println("64 samples"); break;
      case INA226_AVERAGES_128:         Serial.println("128 samples"); break;
      case INA226_AVERAGES_256:         Serial.println("256 samples"); break;
      case INA226_AVERAGES_512:         Serial.println("512 samples"); break;
      case INA226_AVERAGES_1024:        Serial.println("1024 samples"); break;
      default: Serial.println("unknown");
    }

    Serial.print("Bus conversion time:   ");
    switch (ina.getBusConversionTime())
    {
      case INA226_BUS_CONV_TIME_140US:  Serial.println("140uS"); break;
      case INA226_BUS_CONV_TIME_204US:  Serial.println("204uS"); break;
      case INA226_BUS_CONV_TIME_332US:  Serial.println("332uS"); break;
      case INA226_BUS_CONV_TIME_588US:  Serial.println("558uS"); break;
      case INA226_BUS_CONV_TIME_1100US: Serial.println("1.100ms"); break;
      case INA226_BUS_CONV_TIME_2116US: Serial.println("2.116ms"); break;
      case INA226_BUS_CONV_TIME_4156US: Serial.println("4.156ms"); break;
      case INA226_BUS_CONV_TIME_8244US: Serial.println("8.244ms"); break;
      default: Serial.println("unknown");
    }

    Serial.print("Shunt conversion time: ");
    switch (ina.getShuntConversionTime())
    {
      case INA226_SHUNT_CONV_TIME_140US:  Serial.println("140uS"); break;
      case INA226_SHUNT_CONV_TIME_204US:  Serial.println("204uS"); break;
      case INA226_SHUNT_CONV_TIME_332US:  Serial.println("332uS"); break;
      case INA226_SHUNT_CONV_TIME_588US:  Serial.println("558uS"); break;
      case INA226_SHUNT_CONV_TIME_1100US: Serial.println("1.100ms"); break;
      case INA226_SHUNT_CONV_TIME_2116US: Serial.println("2.116ms"); break;
      case INA226_SHUNT_CONV_TIME_4156US: Serial.println("4.156ms"); break;
      case INA226_SHUNT_CONV_TIME_8244US: Serial.println("8.244ms"); break;
      default: Serial.println("unknown");
    }
    
    Serial.print("Max possible current:  ");
    Serial.print(String(ina.getMaxPossibleCurrent()));
    Serial.println(" A");

    Serial.print("Max current:           ");
    Serial.print(String(ina.getMaxCurrent()));
    Serial.println(" A");

    Serial.print("Max shunt voltage:     ");
    Serial.print(String(ina.getMaxShuntVoltage()));
    Serial.println(" V");

    Serial.print("Max power:             ");
    Serial.print(String(ina.getMaxPower()));
    Serial.println(" W");
  }
}
String INA226_read(int timestamp,int address){
  Serial.println("INA226_read address:" + String(address));
  String result = "";
  bool success = ina.begin(address);
  if(!success)
  {
    Serial.println("INA226 error");
    delay(500);
  }else{
    float Busvoltage_f = ina.readBusVoltage();
    String Busvoltage = String (Busvoltage_f);
    String BusPower = String (ina.readBusPower(), 5);
    String ShuntVoltage = String (ina.readShuntVoltage(), 5);
    String ShuntCurrent = String (ina.readShuntCurrent(), 5);

    Serial.print("Bus voltage:   ");
    Serial.print(Busvoltage);
    Serial.println(" V");

    Serial.print("Bus power:     ");
    Serial.print(BusPower);
    Serial.println(" W");


    Serial.print("Shunt voltage: ");
    Serial.print(ShuntVoltage);
    Serial.println(" V");

    Serial.print("Shunt current: ");
    Serial.print(ShuntCurrent);
    Serial.println(" A");
    
    //{0x41, 0x44} {65, 68} 65太阳能板 68电池
    String type;
    if (address == 0x41){
      type = "SolarPanels";
    }else if (address == 0x44)
    {
      type = "Battery";
      Battery_Voltage = Busvoltage_f;
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
  Serial.println("");
  return result;
}
void INA226_config(){
  for (int i = 0; i < 2; i++) {
    bool success = ina.begin(ina226_address[i]);
    Serial.println("INA226_config address:" + String(ina226_address[i]));
    if(!success)
    {
      Serial.println("INA226 error");
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
  Serial.println("BH1750_read..");
  String result = "";
  if (lightMeter.measurementReady()) {
    float lux = lightMeter.readLightLevel();
    if(lux>=0){
      Serial.println("Light:\t" + String(lux) + " lx");
      result = "Weather,tag=Brightness Brightness=" + String(lux) + " " + timestamp + "\n";
    }else{
      Serial.println("BH1750 error Light: " + String(lux) + " lx");
    }
  }else{
    Serial.println("BH1750 error");
    delay(500);
  }
  Serial.println("");
  return result;
}

String HTU21D_read(int timestamp){
  Serial.println("HTU21D_read..");
  String result = "";
  if (htu.begin()) {
    float temp = htu.readTemperature();
    float rel_hum = htu.readHumidity();
    float dew_point = temp - (100 - rel_hum) / 5; //计算露点
    Serial.println("temp:\t" + String(temp));
    Serial.println("hum:\t" + String(rel_hum));
    Serial.println("dp:\t" + String(dew_point));
    result =  "Weather,tag=Temperature Temperature=" + String(temp) + " " + timestamp + "\n" +
              "Weather,tag=Humidity Humidity=" + String(rel_hum) + " " + timestamp + "\n" +
              "Weather,tag=dewpoint dewpoint=" + String(dew_point) + " " + timestamp + "\n"
              ;
  }else{
    Serial.println("HTU21D error");
  }
  Serial.println("");
  return result;
}

String BMP180_read(int timestamp){
  Serial.println("BMP180_read..");
  String result = "";
  if(myBMP.begin()){
    String BMP_temp = String(myBMP.getTemperature(), 1);
    Serial.println("Temp:\t"+ BMP_temp + " +-1.0C");
    Serial.println("Pa:\t"  + String(myBMP.getPressure())       + " +-100Pa");

    String Pressure_hPa = String(myBMP.getPressure_hPa());
    Serial.println("hPa:\t"  + Pressure_hPa + " +-1hPa");
    Serial.println("mmHg:\t" + String(myBMP.getPressure_mmHg()) + " +-0.75mmHg");
    Serial.println("inHg:\t" + String(myBMP.getPressure_inHg()) + " +-0.03inHg");

    Serial.println("SeaLevel hPa:\t"  + String(myBMP.getSeaLevelPressure_hPa(25))  + " hPa");
    Serial.println("SeaLevel mmHg:\t" + String(myBMP.getSeaLevelPressure_mmHg(25)) + " mmHg");
    Serial.println("SeaLevel inHg:\t" + String(myBMP.getSeaLevelPressure_inHg(25)) + " inHg");

    int Forecast = myBMP.getForecast(25);
    switch (Forecast)
    {
      case 0: Serial.println(F("thunderstorm")); break;
      case 1: Serial.println(F("rain")); break;
      case 2: Serial.println(F("cloudy")); break;
      case 3: Serial.println(F("partly cloudy")); break;
      case 4: Serial.println(F("clear")); break;
      case 5: Serial.println(F("sunny")); break;
    }

    result =  "Weather,tag=Temperature Temperature_2=" + BMP_temp + " " + timestamp + "\n" +
              "Weather,tag=Pressure Pressure=" + Pressure_hPa + " " + timestamp + "\n" +
              "Weather,tag=Forecast Forecast=" + Forecast + " " + timestamp + "\n"
             ;
  }else{
    Serial.println("BMP180 error");
  }
  Serial.println("");
  return result;
}

void i2c_Scanning() {
  byte error, address;
  int deviceCount = 0;

  Serial.println("Scanning...");

  for (address = 1; address < 127; address++) {
    char hexValue[3];
    Wire.beginTransmission(address);
    error = Wire.endTransmission();

    if (error == 0) {
      Serial.print("Device found at address 0x");
      if (address < 16) {
        Serial.print("0");
      }
      sprintf(hexValue, "%02X", address);
      Serial.println(hexValue);
      deviceCount++;
    } else if (error == 4) {
      Serial.print("Unknown error at address 0x");
      if (address < 16) {
        Serial.print("0");
      }
      sprintf(hexValue, "%02X", address);
      Serial.println(hexValue);
    }
  }
  if (deviceCount == 0) {
    Serial.println("No I2C devices found\n");
  } else {
    Serial.println("Scan complete\n");
  }
}

String i2c_Scanning_re() {
  byte error, address;
  int deviceCount = 0;
  String re = "";

  for (address = 1; address < 127; address++) {
    char hexValue[3];
    Wire.beginTransmission(address);
    error = Wire.endTransmission();

    if (error == 0) {
      re = re + "Device found at address 0x";
      if (address < 16) {
        re = re + "0";
      }
      sprintf(hexValue, "%02X", address);
      re = re + hexValue + "\n";
      deviceCount++;
    } else if (error == 4) {
      re = re + "Unknown error at address 0x";
      if (address < 16) {
        re = re + "0";
      }
      sprintf(hexValue, "%02X", address);
      re = re + hexValue + "\n";
    }
  }
  if (deviceCount == 0) {
    re = re + "No I2C devices found\n\n";
  } else {
    re = re + "Scan complete\n\n";
  }
  return re;
}

//继电器状态刷新
void JD_Refresh(int JDX) {
  int i = 0;
  switch (JDX)
  {
  case JD1:i = 0;break;
  case JD2:i = 1;break;
  default:break;
  }

  switch (EEPROM.read(address + i))
  {
  case 0:if (digitalRead(JDX) != HIGH){digitalWrite(JDX, HIGH);}break;
  case 1:if (digitalRead(JDX) != LOW){digitalWrite(JDX, LOW);}break;
  case 2:
    if (Battery_Voltage <= 11.1){
      if (digitalRead(JDX) != HIGH){digitalWrite(JDX, HIGH);};
    }else if (Battery_Voltage >= 12)
    {
      if (digitalRead(JDX) != LOW){digitalWrite(JDX, LOW);};
    }
    break;

  default:break;
  }
}

void setup() {
  // put your setup code here, to run once:
  pinMode(ledPin, OUTPUT);

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
  ArduinoOTA.setPassword(OTAPassword);

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

  for (int i = 0; i < 20; i++) {
    Serial.print("Check Updates|");
    // hasClient ();
    ArduinoOTA.handle();
    delay(500);
  }
  Serial.println("");
  Serial.println("");


  //////////////////////////////////////////// NTP ////
  // Set time via NTP, as required for x.509 validation
  configTime(8 * 3600, 0, "ntp.ntsc.ac.cn", "ntp.aliyun.com");

  Serial.print("Waiting for NTP time sync: ");
  time_t now = time(nullptr);
  while (now < 8 * 3600 * 2) {
    delay(500);
    Serial.print(".");
    now = time(nullptr);
  }
  Serial.println("");
  struct tm timeinfo;
  localtime_r(&now, &timeinfo);
  Serial.println("Current time: " + String (asctime(&timeinfo)));

  Wire.begin();
  ////////////////////////////////// INA226 ////
  INA226_config();
  for (int i = 0; i < 2; i++) {
    INA226_check(ina226_address[i]);
  }
  ////////////////////////////////// HTU21D ////
  if (!htu.begin()) {
    Serial.print("HTU21D error");
    delay(500);
  }
  ////////////////////////////////// BH1750 ////
  if (!lightMeter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE)) {
    Serial.print("BH1750 error");
    delay(500);
  }

  // i2c_Scanning();

  ////////// WEB服务器启动 ////
  startWebServer();

  
  // 继电器初始化
  pinMode(JD1, OUTPUT);
  pinMode(JD2, OUTPUT);
  digitalWrite(JD1, HIGH);
  digitalWrite(JD2, HIGH);

  EEPROM.begin(512);

  int value;
  for (int i = 0; i < 2; i++) {
    value = EEPROM.read(address + i);
    if (value == 255 || value > 2) {
      EEPROM.write(address + i, 2);
    }
  }
  EEPROM.commit();

  if (EEPROM.read(address + 0) == 1){
    digitalWrite(JD1, LOW);
  }
  if (EEPROM.read(address + 1) == 1){
    digitalWrite(JD2, LOW);
  }

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

  Serial.println("================ Version: " + String(VER) + " ================");

  // i2c_Scanning();
  Serial.println(i2c_Scanning_re());

  if (first_start){
    Serial.println("first_start");
    first_start = false;
  }else{
    Serial.print("Delay & Check Updates");
    for (int i = 0; i < loop_delay_Time/2; i++) {
      Serial.print(".");
      webserver.handleClient();  // 处理Web客户端请求
      ArduinoOTA.handle();
      delay(2000);
    }
    // delay(10000);
  }

  Serial.println("execute");
  
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
  Serial.println(printStr);
  Serial.println("");

  last_post_time = timeStr;
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

  postData = "";
  
  String HTU21D_Data = HTU21D_read(now);
  String BMP180_Data = BMP180_read(now);
  String BH1750_Data = BH1750_read(now);
  String INA226_41 = INA226_read(now,0x41);
  String INA226_44 = INA226_read(now,0x44);

  postData = HTU21D_Data + BMP180_Data + BH1750_Data + INA226_41 + INA226_44;
  Serial.println(postData);

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  
  if(postData != ""){
    http_client.setTimeout(15000);
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
      Serial.println("Failed to connect to server");
    }

    http_client_2.setTimeout(15000);
    if (http_client_2.connect(api2_serverName, api2_port)) { // Replace with your server URL and port
      http_client_2.println(String("POST ") + api2_url + " HTTP/1.1"); // Replace with your API endpoint
      http_client_2.println(String("Host: ") + api2_serverName); // Replace with your server URL
      http_client_2.println("Content-Type: application/x-www-form-urlencoded");
      http_client_2.print("Content-Length: ");
      http_client_2.println(postData.length());
      http_client_2.println();
      http_client_2.print(postData);
    } else {
      Serial.println("Failed to connect to server");
    }
  }
  
  Serial.println("");
  Serial.println("=====POST=END=====");
  Serial.println("");

  //继电器状态刷新
  JD_Refresh(JD1);
  JD_Refresh(JD2);

}

 // 启动Web服务器
void startWebServer() {
  Serial.print("startWebServer...");
  webserver.on("/", [](){
    time_t now = time(nullptr);
    localtime_r(&now, &timeinfo);
    String time = String (asctime(&timeinfo));

    String JDstatus = 
    "opi_opt = %d\n"
    "cam_opt = %d"
  ;
  char buffer[50];
  sprintf(buffer, JDstatus.c_str(), EEPROM.read(address + 0), EEPROM.read(address + 1));
  JDstatus = String(buffer);

  String s = "<h1>WxS-info --- Version: " + String(VER) + "</h1>";
    s += "<p>now_get_time: " + time + "</p>";
    s += "<p>last_post_time: " + last_post_time + "</p>";
    s += "<h2>JDstatus</h2>";
    s += "<p style=\"white-space: pre-line;\">" + JDstatus + "</p>";
    s += "<h2>PostData</h2>";
    s += "<p style=\"white-space: pre-line;\">" + postData + "</p>";
    s += "<h2>I2C_Scanning</h2>";
    s += "<p style=\"white-space: pre-line;\">" + i2c_Scanning_re() + "</p>"
  ;
  webserver.send(200, "text/html", makePage("WxS-info", s));
  });
  webserver.on("/gpio", [](){
    String s = "<h1>GPIO-set</h1>"
    "<form action=\"gupset\" method=\"get\">"
      "<label for=\"opi_opt\">OrangePI：</label>"
        "<select id=\"opi_opt\" name=\"opi_opt\">"
          "<option value=\"off\">关</option>"
          "<option value=\"on\">开</option>"
          "<option value=\"auto\">自动</option>"
        "</select><br>"
      "<label for=\"cam_opt\">Camera：</label>"
        "<select id=\"cam_opt\" name=\"cam_opt\">"
          "<option value=\"off\">关</option>"
          "<option value=\"on\">开</option>"
          "<option value=\"auto\">自动</option>"
        "</select><br><br>"
      "<input type=\"submit\" value=\"提交\">"
    "</form>"
    ;
    String js = 
    "<script>"
      "const my_opi_opt = document.getElementById(\"opi_opt\");"
      "my_opi_opt.options[%d].selected = true;"
      "const my_cam_opt = document.getElementById(\"cam_opt\");"
      "my_cam_opt.options[%d].selected = true;"
    "</script>"
    ;

    char buffer[300];
    sprintf(buffer, js.c_str(), EEPROM.read(address + 0), EEPROM.read(address + 1));
    js = String(buffer);

    webserver.send(200, "text/html", makePage("GPIO-set", s+js));
  });
  webserver.on("/gupset", [](){
    String opi_opt = urlDecode(webserver.arg("opi_opt"));
    String cam_opt = urlDecode(webserver.arg("cam_opt"));
    String s = "<h1>set</h1>"
    ;
    int i = 0;
    if (opi_opt != ""){
      if (opi_opt == "off") {
        EEPROM.write(address + i, 0);
      } else if (opi_opt == "on") {
        EEPROM.write(address + i, 1);
      } else if (opi_opt == "auto") {
        EEPROM.write(address + i, 2);
      } else {
        EEPROM.write(address + i, 2);
      }
      EEPROM.commit();
      JD_Refresh(JD1);
      s += "<p>opi_opt: " + opi_opt + "</p>";
      }

    i = 1;
    if (cam_opt != ""){
      if (cam_opt == "off") {
        EEPROM.write(address + i, 0);
      } else if (cam_opt == "on") {
        EEPROM.write(address + i, 1);
      } else if (cam_opt == "auto") {
        EEPROM.write(address + i, 2);
      } else {
        EEPROM.write(address + i, 2);
      }
      EEPROM.commit();
      JD_Refresh(JD2);
      s += "<p>cam_opt: " + cam_opt + "</p>";
    }
    String js = 
    "<script type=\"text/javascript\">"
      "setTimeout(function(){window.location.href = \"/gpio\";}, 2000);"
    "</script>"
    ;

    webserver.send(200, "text/html", makePage("set", s+js));
  });
  webserver.begin(); 
  Serial.println("OK");
}

String makePage(String title, String contents) {
  String s = "<!DOCTYPE html><html><head>";
  s += "<meta name=\"viewport\" content=\"width=device-width,user-scalable=0\">";
  s += "<meta charset=\"UTF-8\">";
  s += "<title>";
  s += title;
  s += "</title></head><body style=\"text-align: center;\">";
  s += contents;
  s += "</body></html>";
  return s;
}

String urlDecode(String input) {
  String s = input;
  s.replace("%20", " ");
  s.replace("+", " ");
  s.replace("%21", "!");
  s.replace("%22", "\"");
  s.replace("%23", "#");
  s.replace("%24", "$");
  s.replace("%25", "%");
  s.replace("%26", "&");
  s.replace("%27", "\'");
  s.replace("%28", "(");
  s.replace("%29", ")");
  s.replace("%30", "*");
  s.replace("%31", "+");
  s.replace("%2C", ",");
  s.replace("%2E", ".");
  s.replace("%2F", "/");
  s.replace("%2C", ",");
  s.replace("%3A", ":");
  s.replace("%3A", ";");
  s.replace("%3C", "<");
  s.replace("%3D", "=");
  s.replace("%3E", ">");
  s.replace("%3F", "?");
  s.replace("%40", "@");
  s.replace("%5B", "[");
  s.replace("%5C", "\\");
  s.replace("%5D", "]");
  s.replace("%5E", "^");
  s.replace("%5F", "-");
  s.replace("%60", "`");
  return s;
}
