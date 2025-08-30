#include <Arduino.h>
#include <Preferences.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <string.h>
#include <base64.h>
#include <ArduinoJson.h>

#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include "wifi_credentials.h"
#include "client_secrets.h"

String clientId = CLIENT_ID;
String clientSecret = CLIENT_SECRET;

char code[500];
String redirectUri = "http://localhost:8080/callback";

bool sentAuth = false;
bool codeReceived = false;

String base = "api.spotify.com";
String tokenUrl = "/api/token";

String accessToken;
String refreshToken;

// Song details
String currName = "Spotify Controller";

char* spotify_root_cert = spotify_CA;


/*  OLED STUFF  */
#define SCREEN_WIDTH 128        // OLED display width in pixels
#define SCREEN_HEIGHT 64        // OLED display height in pixels
#define OLED_RESET -1           // Reset pin (-1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C     // I2C address of the OLED display


Preferences preferences;
WiFiClientSecure client;
JsonDocument doc;
base64 BASE64;

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

void wifiConnect(){
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(WIFI_SSID);

    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    // Connect to the SSID
    while(WiFi.status() != WL_CONNECTED){
        Serial.print(".");
        delay(1000);
    }

    Serial.println();
    Serial.println("Connected to the network!");
}

void clientConnect(char* server){

    // Set the client certificate
    client.setCACert(spotify_root_cert);
    Serial.println("Attempting to connect to the server...");
    // 443 <- Default port for HTTPS
    while (!client.connect(server, 443)) {
        Serial.println("Connection failed! Retrying in 5s...");
        delay(5000);
    }
    Serial.println("Connected successfully!");
}

void reqAccess(const String& authCode){
    // & <- just passes it by refrence instead of making a copy
    // Like a pointer but not a pointer

    clientConnect("accounts.spotify.com");
    // Request body
    String body = "grant_type=authorization_code&code=" + authCode + "&redirect_uri=" + redirectUri;
    String auth = BASE64.encode(clientId + ":" + clientSecret);

    // Raw HTTP request
    client.println("POST /api/token HTTP/1.1");
    client.println("Host: accounts.spotify.com");
    client.println("Content-Type: application/x-www-form-urlencoded");
    client.print("Authorization: Basic ");
    client.println(auth);
    client.print("Content-Length: ");
    client.println(body.length());
    client.println("Connection: close");
    client.println();
    client.println(body);


    // Read the response
    int statusCode = -1;
    bool headersDone = false;
    String resBody;

    while( client.available()){
        // Read line by line
        String line = client.readStringUntil('\n');
        line.trim();

        if(!headersDone){
            // Get the status code -> HTTP/1.1 200 OK
            if(line.startsWith("HTTP/1.1")){
                int space1 = line.indexOf(" ");
                int space2 = line.indexOf(" ", space1+1);

                if(space1 > 0 and space2 > space1){
                    statusCode =  line.substring(space1+1,space2).toInt();
                }

            }

            // blank line = end of headers
            if(line.length() == 0){
                headersDone = true;
            }
        }else{
            resBody += line + "\n";
        }
   }

    client.stop();
    delay(50);

    Serial.print("Status code: ");
    Serial.println(statusCode);
    Serial.println("Response: ");
    Serial.println(resBody);

    // Deserialize and fetch access and request tokens
    if(statusCode == 200){
        deserializeJson(doc, resBody);
        refreshToken = (String) doc["refresh_token"];
        accessToken = (String) doc["access_token"];

        // Initialize and open storage space
        preferences.begin("spotify", false);
        preferences.putString("refreshToken", refreshToken);
        delay(50);
        preferences.end();
    }
}

void refresh(){

    clientConnect("accounts.spotify.com");
    preferences.begin("spotify", true);
    refreshToken = preferences.getString("refreshToken");

    String body = "grant_type=refresh_token&refresh_token="+refreshToken;
    String auth = BASE64.encode(clientId + ":" + clientSecret);
    // Raw HTTP request

    client.println("POST /api/token HTTP/1.1");
    client.println("Host: accounts.spotify.com");
    client.println("Content-Type: application/x-www-form-urlencoded");
    client.print("Authorization: Basic ");
    client.println(auth);
    client.print("Content-Length: ");
    client.println(body.length());
    client.println("Connection: close");
    client.println();
    client.println(body);

    // Reading the response
    int statusCode = -1;
    bool headersDone = false;
    String resBody;

    while(client.available()){
        String line = client.readStringUntil('\n');
        line.trim();

        if(!headersDone){
            if(line.startsWith("HTTP/1.1")){
                int space1 = line.indexOf(" ");
                int space2 = line.indexOf(" ", space1+1);

                if(space1 > 0 and space2 > space1){
                    statusCode =  line.substring(space1+1,space2).toInt();
                }
            }
            if(line.length() == 0){
                headersDone = true;
            }
        }else{
            resBody += line + "\n";
        }
    }

    client.stop();
    delay(50);

    // Deserialize and fetch access token
    if(statusCode == 200){
        deserializeJson(doc, resBody);
        accessToken = (String) doc["access_token"];
    }else{
        Serial.print("Status Code: ");
        Serial.println(statusCode);
        Serial.println(resBody);
    }

    preferences.end();
}

void getCurrPlaying(){

    clientConnect("api.spotify.com");

    client.println("GET /v1/me/player/currently-playing?market=IN HTTP/1.1");
    client.println("Host: api.spotify.com");
    client.print("Authorization: Bearer ");
    client.println(accessToken);
    client.println("Connection: close");
    client.println();

    int statusCode = -1;
    bool headersDone = false;
    String resBody;

    delay(500);
    while(client.available()){
        String line = client.readStringUntil('\n');
        line.trim();

        if(!headersDone){
            if(line.startsWith("HTTP/1.1")){
                int space1 = line.indexOf(" ");
                int space2 = line.indexOf(" ", space1+1);

                if(space1 > 0 and space2 > space1){
                    statusCode =  line.substring(space1+1,space2).toInt();
                }
            }
            if(line.length() == 0){
                headersDone = true;
            }
        }else{
            resBody += line + "\n";
        }
    }

    client.stop();
    delay(50);

    if(statusCode == 200){
        deserializeJson(doc, resBody);
        currName = (String) doc["item"]["name"];
        Serial.println();
        Serial.println(currName);
        Serial.println();
    }else if(statusCode == 204){
        currName = "Nothing is currently playing";
        Serial.println("Nothing is currently playing....");
    }else if(statusCode == 401){
        refresh();
        getCurrPlaying();
    }else{
        currName = "Error";
        Serial.print("Status Code: ");
        Serial.println(statusCode);
        Serial.println(resBody);
    }
}

void setup(){
    Serial.begin(115200);

    // Initialize OLED display 
    if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)){
        Serial.println("SSD1306 allocation failed!");
        // loop forever if display initialization fails
        while (true){}
    }

    wifiConnect();
    Serial.println(WiFi.RSSI());

    display.clearDisplay();
    display.setTextSize(2);
    display.setTextColor(WHITE);
    display.setCursor(0,0);
    display.println(currName);
    display.display();

    preferences.begin("spotify", true);
    // If already have refresh token use it
    // If not get one
    if(!preferences.isKey("refreshToken")){
        preferences.end();

        // Only when data received
        while(Serial.available() > 0){
            int len = Serial.readBytesUntil('\n', code, sizeof(code)-1);
            code[len] = '\0';
            codeReceived = true;
        }
        String authCode = code;

        if(codeReceived && !sentAuth){
            reqAccess(authCode);
            sentAuth = true;
        }
    }else{
        refresh();
    }
}

void loop(){

    preferences.begin("spotify", true);
    if(preferences.isKey("refreshToken")){
        getCurrPlaying();
    }

    display.clearDisplay();
    display.setTextSize(2);
    display.setTextColor(WHITE);
    display.setCursor(0,0);
    display.println(currName);
    display.display();

    preferences.end();
    delay(15000);
}