#include <SPI.h>
#include <WiFiNINA.h>
#include "wifi.h"

char ssid[] = SECRET_SSID;
char pass[] = SECRET_PASS;
int status = WL_IDLE_STATUS;

WiFiClient client;

IPAddress server(192,168,129,18);
int port = 3000;

const int RED_PIN = 9;
const int GREEN_PIN = 10;
const int BLUE_PIN = 11;

const int RED_BTN_PIN = 2;
const int BLUE_BTN_PIN = 3;

int redScore = 0;
int blueScore = 0;
// 0 = GREEN
// 1 = RED
// 2 = BLUE
int currentColor = 0;
int redHoldingTime = 0;
int blueHoldingTime = 0;

int ms = 0;

const int SCORE_STEP = 5;
const int SCORE_WIN_AMOUNT = 1500;
const int CAPTURE_TIME = 5000;

void setup() {
  Serial.begin(9600);
  while (!Serial) {
    ;
  }

  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println("Communication with WiFi module failed!");
    // don't continue
    while (true);
  }

  while (status != WL_CONNECTED) {
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(ssid);
    // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
    status = WiFi.begin(ssid, pass);
    // wait 10 seconds for connection:
    delay(10000);
  }

  printWifiStatus();

  pinMode(RED_PIN, OUTPUT);
  pinMode(GREEN_PIN, OUTPUT);
  pinMode(BLUE_PIN, OUTPUT);

  pinMode(RED_BTN_PIN, INPUT);
  pinMode(BLUE_BTN_PIN, INPUT);

  digitalWrite(GREEN_PIN, HIGH);

  httpRequest("{ \"redScore\": \"" + String(redScore) + "\", \"blueScore\": \"" + String(blueScore) + "\"}");
  
  Serial.print("Battle House Domination cube ready for action.");
}

void loop() {
  bool isRedPushed = digitalRead(RED_BTN_PIN);
  bool isBluePushed = digitalRead(BLUE_BTN_PIN);

  bool holdingRed = isRedPushed && !isBluePushed;
  bool holdingBlue = !isRedPushed && isBluePushed;

  if (holdingRed || holdingBlue) {
    if (holdingRed && currentColor != 1 && !holdingBlue) {
      redHoldingTime += 2;
      Serial.print("Red capturing...");
      Serial.println(redHoldingTime);
    } 
    if (holdingBlue && currentColor != 2 && !holdingRed) {
      blueHoldingTime += 2;
      Serial.print("Blue capturing...");
      Serial.println(blueHoldingTime);
    }

    if (redHoldingTime >= CAPTURE_TIME && currentColor != 1) {
      changeToRed();
    }

    if (blueHoldingTime >= CAPTURE_TIME && currentColor != 2) {
      changeToBlue();
    }
  } else {
    redHoldingTime = 0;
    blueHoldingTime = 0;
  }

  if (redScore >= SCORE_WIN_AMOUNT) {
      Serial.println("RED WINS!!!");
      digitalWrite(BLUE_PIN, LOW);
      while (true) {
        digitalWrite(RED_PIN, HIGH);
        delay(1000);
        digitalWrite(RED_PIN, LOW);
        delay(1000);
      }
    }

  if (blueScore >= SCORE_WIN_AMOUNT) {
    Serial.println("BLUE WINS!!!");
    digitalWrite(RED_PIN, LOW);
    while (true) {
      digitalWrite(BLUE_PIN, HIGH);
      delay(1000);
      digitalWrite(BLUE_PIN, LOW);
      delay(1000);
    }
  }

  if (ms % 1000 == 0) {
    if (currentColor != 0) {
      incrementScore();
      String requestData = "{ \"redScore\": \"" + String(redScore) + "\", \"blueScore\": \"" + String(blueScore) + "\"}";
      httpRequest(requestData);
    }
  }

  ms++;
  delay(1);
}

void changeToRed() {
  digitalWrite(RED_PIN, HIGH);
  digitalWrite(BLUE_PIN, LOW);
  digitalWrite(GREEN_PIN, LOW);
  currentColor = 1;
  Serial.println("Red captured cube.");
}

void changeToBlue() {
  digitalWrite(RED_PIN, LOW);
  digitalWrite(BLUE_PIN, HIGH);
  digitalWrite(GREEN_PIN, LOW);
  currentColor = 2;
  Serial.println("Blue captured cube.");
}

void incrementScore() {
  if (currentColor == 1) {
    redScore += SCORE_STEP;
  } 
  if (currentColor == 2) {
    blueScore += SCORE_STEP;
  }
  printScore();
}

void printScore() {
  Serial.print("RED: ");
  Serial.print(redScore);
  Serial.print(" | BLUE: ");
  Serial.println(blueScore);
}

void printWifiStatus() {
  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());
  // print your board's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);
  // print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
}

void httpRequest(String body) {
  // close any connection before send a new request.
  // This will free the socket on the Nina module
  client.stop();

  // if there's a successful connection:
  if (client.connect(server, port)) {
    Serial.println("connecting...");
    // send the HTTP PUT request:
    client.println("POST / HTTP/1.1"); 
    client.print("Host: ");
    client.println(server);
    client.println("Content-type: application/json");
    client.println("Accept: */*");
    client.println("Cache-Control: no-cache");
    client.print("Host: ");
    client.println(server);
    client.println("Accept-Encoding: gzip, deflate");
    client.print("Content-Length: ");
    client.println(body.length());
    client.println("Connection: close");
    client.println();
    client.println(body);

    Serial.println("request sent");
  } else {
    // if you couldn't make a connection:
    Serial.println("connection failed");
  }
}
