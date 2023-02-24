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

    bool connected() const;

    int createExcelFile(const String& fileName, const String columnNames[], int columns);

    JSONVar readExcelFile(const String& fileName);

    bool writeToExcelFile(const String& fileName, String data[], int length);

private:
    X509List _cert;
    ESP8266WiFiMulti WiFiMulti;
    bool _connected;

    static String getMessage(JSONVar obj);

    static JSONVar postRequest(const String& endpoint, const JSONVar& body);

    static JSONVar getRequest(const String& endpoint);
};

#endif