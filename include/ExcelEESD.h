#ifndef ExcelEESD_h
#define ExcelEESD_h

#include <Arduino.h>
#include <SoftwareSerial.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266WebServer.h>
#include <WiFiClientSecure.h>
#include <Arduino_JSON.h>
#include <WiFiClient.h>

class ExcelEESD {

public:
    ExcelEESD(const char *ssid, const char *password);

    bool connected();

    int createExcelFile(String fileName, const String columnNames[], int columns);

    JSONVar readExcelFile(String fileName);

    bool writeToExcelFile(String fileName, String data[], int length);

private:
    X509List _cert;
    ESP8266WiFiMulti WiFiMulti;
    bool _connected;

    String getMessage(JSONVar obj);

    JSONVar postRequest(String endpoint, JSONVar body);

    JSONVar getRequest(String endpoint);
};

#endif