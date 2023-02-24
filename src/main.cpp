#include <Arduino.h>
#include <ESP8266WebServer.h>
#include "ExcelEESD.h"

ESP8266WebServer server(80);

// Replace with your network credentials
const char *ssid = "PEACE";
const char *pass = "4255887227";
const String deviceId = "data-test-2";

const String columns[] = {
        "Time (UTC)",
        "E Humidity",
        "E Temperature",
        "E Heat Index",
        "S Moisture",
        "S Temperature",
        "UV Light",
        "VIS Light",
        "IR Light"

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

    int a = excel->createExcelFile(deviceId, columns, 9);
    if (a == 0) {
        Serial.println("Excel file already exists :D");
    } else if (a == 1) {
        Serial.println("Created excel file \"" + deviceId + ".xlsx\"");
    } else if (a == -1) {
        Serial.println("Could not create excel file, network/server error.");
    }

    Serial.println("Setup complete");

    // append fake data to test
    // example data:
    // "Item[]" : Fri Feb 24 09:59:16 2023
    // "Item[]" : 40
    // "Item[]" : 20
    // "Item[]" : 19.1
    // "Item[]" : 742
    // "Item[]" : 18.88
    // "Item[]" : 0.01
    // "Item[]" : 5
    // "Item[]" : 19

    String data[] = {
            ExcelEESD::getTime(),
            "40",
            "20",
            "19.1",
            "742",
            "18.88",
            "0.01",
            "5",
            "19"
    };
    excel->writeToExcelFile(deviceId, data, 9);
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
            String uVK = getValue(ardData, ';', 5);
            String vIK = getValue(ardData, ';', 6);
            String iRK = getValue(ardData, ';', 7);


            String eH = getValue(eHK, '=', 1);
            String eT = getValue(eTK, '=', 1);
            String eHI = getValue(eHIK, '=', 1);
            String sM = getValue(sMK, '=', 1);
            String sT = getValue(sTK, '=', 1);
            String uV = getValue(uVK, '=', 1);
            String vI = getValue(vIK, '=', 1);
            String iR = getValue(iRK, '=', 1);

            eH.trim();
            eT.trim();
            eHI.trim();
            sM.trim();
            sT.trim();
            uV.trim();
            vI.trim();
            iR.trim();

            String data[] = {
                    ExcelEESD::getTime(),
                    eH,
                    eT,
                    eHI,
                    sM,
                    sT,
                    uV,
                    vI,
                    iR
            };
            excel->writeToExcelFile(deviceId, data, 9);

            ardData = "";
        } else {
            ardData.concat((char) incomingByte);
        }
    }
}