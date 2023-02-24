#include <Arduino.h>
#include <ESP8266WebServer.h>
#include "ExcelEESD.h"

ESP8266WebServer server(80);

// Replace with your network credentials
const char *ssid = "PEACE";
const char *pass = "4255887227";
const String deviceId = "aryan";

const String columns[] = {
        "E Humidity",
        "E Temperature",
        "E Heat Index",
        "S Moisture",
        "S Temperature"
};

ExcelEESD *excel;

void webpage() {
    server.send(200, "text/html", "hello!");
}

void setup() {
    // Open serial communications and wait for port to open:
    Serial.begin(9600);
    while (!Serial) { ;  // wait for serial port to connect. Needed for native USB port only
    }
    excel = new ExcelEESD(ssid, pass);
    while (!excel->connected()) { ;
    }

    server.on("/", webpage);
    server.begin();

    int a = excel->createExcelFile(deviceId, columns, 5);
    if (a == 0) {
        Serial.println("Excel file already exists :D");
    } else if (a == 1) {
        Serial.println("Created excel file \"" + deviceId + ".xlsx\"");
    } else if (a == -1) {
        Serial.println("Could not create excel file, network/server error.");
    }
}

String getValue(const String &data, char separator, int index) {
    int found = 0;
    int strIndex[] = {0, -1};
    int maxIndex = data.length() - 1;

    for (int i = 0; i <= maxIndex && found <= index; i++) {
        if (data.charAt(i) == separator || i == maxIndex) {
            found++;
            strIndex[0] = strIndex[1] + 1;
            strIndex[1] = (i == maxIndex) ? i + 1 : i;
        }
    }
    return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}

String ardData = "";

void loop() {
    server.handleClient();
    if (Serial.available()) {
        int incomingByte = Serial.read();

        if (((char) incomingByte) == '@') {
            Serial.println(ardData);
            //writing to excel
            String eHK = getValue(ardData, ';', 0);
            String eTK = getValue(ardData, ';', 1);
            String eHIK = getValue(ardData, ';', 2);
            String sMK = getValue(ardData, ';', 3);
            String sTK = getValue(ardData, ';', 4);


            String eH = getValue(eHK, '=', 1);
            String eT = getValue(eTK, '=', 1);
            String eHI = getValue(eHIK, '=', 1);
            String sM = getValue(sMK, '=', 1);
            String sT = getValue(sTK, '=', 1);

            eH.trim();
            eT.trim();
            eHI.trim();
            sM.trim();
            sT.trim();

            String data[] = {
                    eH,
                    eT,
                    eHI,
                    sM,
                    sT
            };
            excel->writeToExcelFile(deviceId, data, 5);

            ardData = "";
        } else {
            ardData.concat((char) incomingByte);
        }
    }
}