#ifndef CONFIG_H
#define CONFIG_H

String VER = "1.0";

const char* ssid = "xxx";
const char* password = "xxx";

const char* hostname = "sunpower_esp.lan";
const char* OTAPassword = "00000000";

String api_serverName = "www.xxx.com";
int api_port = 8086;
String api_url = "/write?db=sumpower&precision=s";
String api_auth = "uuu:ppp";

//外网中断时内网备份
String api2_serverName = "192.168.1.134";
int api2_port = 81;
String api2_url = "/post-save/index.php?tag=sumpower";

int loop_delay_Time = 30;

#endif // CONFIG_H