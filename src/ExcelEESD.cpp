#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include "Arduino_JSON.h"
#include "ExcelEESD.h"
#include "certs.h"

WiFiClientSecure client;

ExcelEESD::ExcelEESD(const char *ssid, const char *password)
        : _cert(cert_ECDSA) {
    delay(2000);
    Serial.print("Connecting to ");
    Serial.write(ssid);

    WiFi.mode(WIFI_STA);
    WiFiMulti.addAP(ssid, password);

    while ((WiFiMulti.run() != WL_CONNECTED)) {
        Serial.print(".");
        delay(500);
    }

    _connected = true;

    Serial.println("");
    Serial.println("WiFi connected");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());

    configTime(3 * 3600, 0, "pool.ntp.org", "time.nist.gov");

    Serial.print("Waiting for NTP time sync: ");
    time_t now = time(nullptr);
    while (now < 8 * 3600 * 2) {
        delay(500);
        Serial.print(".");
        now = time(nullptr);
    }
    Serial.println("");
    struct tm timeinfo;
    gmtime_r(&now, &timeinfo);
    Serial.print("Current time: ");
    Serial.print(asctime(&timeinfo));

    delay(2000);

    Serial.print("Testing connection to ");
    Serial.println(eesd_host);

    Serial.printf("Using certificate\n");
    client.setTrustAnchors(&_cert);

    if (!client.connect(eesd_host, eesd_port)) {
        Serial.println("Connection failed");
        return;
    } else {
        Serial.println("Connection to server successful.");
    }

    String url = "/";
    Serial.print("Requesting URL: ");
    Serial.println(url);

    client.print(String("GET ") + url + " HTTP/1.1\r\n" + "Host: " + eesd_host + "\r\n" +
                 "User-Agent: BuildFailureDetectorESP8266\r\n" + "Connection: close\r\n\r\n");

    Serial.println("Request sent");
    while (client.available()) {
        String line = client.readStringUntil('\n');
        if (line == "\r") {
            Serial.println("Headers received");
            break;
        }
    }
    String line = client.readStringUntil('\n');
    if (line.startsWith("?!?!?!? what are u doing here bozo")) {
        Serial.println("esp8266/Arduino CI successful!");
    } else {
        Serial.println("esp8266/Arduino CI has failed");
    }

    Serial.flush();

    delay(5000);
}

bool ExcelEESD::connected() {
    return _connected;
}

int ExcelEESD::createExcelFile(String fileName, const String columnNames[], int columns) {
    if ((WiFiMulti.run() != WL_CONNECTED)) {
        Serial.println("Not connected");
        return -1;
    }

    String contentType = "application/json";
    String content = "{\"rows\":[],\"columns\":[";

    for (int i = 0; i < columns; i++) {
        if (i != 0) {
            content.concat(",");
        }
        content.concat("{\"width\":\"250px\"}");
    }
    content.concat("],\"data\":[[");

    for (int i = 0; i < columns; i++) {
        if (i != 0) {
            content.concat(",");
        }
        content.concat("\"" + columnNames[i] + "\"");
    }

    content.concat("]],\"style\":{},\"sheetName\":\"");

    content.concat(fileName);
    content.concat("\",\"mergeCells\":[]}");

    JSONVar postData;
    postData["fileName"] = fileName;
    postData["fileContent"] = content;
    Serial.print("postData.keys() = ");
    Serial.println(postData.keys());

    JSONVar response = postRequest("/api/files/create", postData);

    if (response == null) {
        return -1;
    }

    Serial.print("JSON.typeof(response) = ");
    Serial.println(JSON.typeof(response));  // prints: object

    String msg = ExcelEESD::getMessage(response);

    if (msg == "File created") {
        return 1;
    } else if (msg == "Error") {
        String err = (String) response["data"]["error"];
        if (err == "File already exists") return 0;
    }

    return -1;
}

JSONVar ExcelEESD::readExcelFile(String file) {
    if ((WiFiMulti.run() != WL_CONNECTED)) {
        Serial.println("Not connected");
        return null;
    }

    JSONVar response = getRequest("/api/files/get/" + file);

    if (response == null) {
        return -1;
    }

    Serial.print("JSON.typeof(response) = ");
    Serial.println(JSON.typeof(response));  // prints: object

    String msg = ExcelEESD::getMessage(response);

    if (msg == "File read") {
        Serial.println("Reading file JSON");
        String fileRawText = (String) response["data"]["file"];
        JSONVar file = JSON.parse(fileRawText);
        if (JSON.typeof(file) == "undefined") {
            Serial.println("Parsing JSON failed.");
            return null;
        }

        Serial.print("JSON.typeof(file) = ");
        Serial.println(JSON.typeof(file));

        return file;
    }

    Serial.println("Invalid JSON template received.");
    return null;
}

bool ExcelEESD::writeToExcelFile(String fileName, String data[], int length) {
    if ((WiFiMulti.run() != WL_CONNECTED)) {
        Serial.println("Not connected");
        return false;
    }

    JSONVar postData;
    JSONVar jsonArr;

    for (int i = 0; i < length; i++) {
        jsonArr[i] = data[i];
    }

    Serial.print("jsonArr.length() = ");
    Serial.println(jsonArr.length());

    postData["fileName"] = fileName;
    postData["data"] = jsonArr;


    Serial.print("postData.keys() = ");
    Serial.println(postData.keys());

    JSONVar response = postRequest("/api/files/append", postData);

    if (response == null) {
        return -1;
    }

    Serial.print("JSON.typeof(response) = ");
    Serial.println(JSON.typeof(response));  // prints: object

    String msg = ExcelEESD::getMessage(response);

    if (msg == "File updated") {
        return true;
    }

    return false;
}

String ExcelEESD::getMessage(JSONVar obj) {
    if (obj.hasOwnProperty("data")) {
        Serial.print("Object has data, obj[\"data\"][\"message\"] = ");
        String msg = (String) obj["data"]["message"];
        Serial.println(msg);

        return msg;
    }

    return "Error";
}

JSONVar ExcelEESD::postRequest(String endpoint, JSONVar body) {
    if (!client.connect(eesd_host, eesd_port)) {
        Serial.println("Connection failed");
        return null;
    } else {
        Serial.println("Connection to server successful.");
    }

    String jsonString = JSON.stringify(body);
    String contentType = "application/json";
    String request = "POST " + endpoint + " HTTP/1.1\r\n";
    request += "Host: " + String(eesd_host) + "\r\n";
    request += "Content-Type: " + String(contentType) + "\r\n";
    request += "Content-Length: " + String(jsonString.length()) + "\r\n";
    request += "Connection: close\r\n\r\n";
    request += jsonString;

    Serial.println("making POST request to " + endpoint);
    client.print(request);

    while (client.available()) {
        String line = client.readStringUntil('\n');
        if (line == "\r") {
            break;
        }
    }
    String line = client.readStringUntil('\n');
    JSONVar obj = JSON.parse(line);
    if (JSON.typeof(obj) == "undefined") {
        Serial.println("Parsing JSON failed.");
        return null;
    }

    return obj;
}

JSONVar ExcelEESD::getRequest(String endpoint) {
    if (!client.connect(eesd_host, eesd_port)) {
        Serial.println("Connection failed");
        return null;
    } else {
        Serial.println("Connection to server successful.");
    }

    String contentType = "application/json";
    String request = "GET " + endpoint + " HTTP/1.1\r\n";
    request += "Host: " + String(eesd_host) + "\r\n";
    request += "Connection: close\r\n\r\n";


    Serial.println("making GET request to " + endpoint);
    client.print(request);

    while (client.available()) {
        String line = client.readStringUntil('\n');
        if (line == "\r") {
            break;
        }
    }
    String line = client.readStringUntil('\n');
    JSONVar obj = JSON.parse(line);
    if (JSON.typeof(obj) == "undefined") {
        Serial.println("Parsing JSON failed.");
        return null;
    }

    return obj;
}