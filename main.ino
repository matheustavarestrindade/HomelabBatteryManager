#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <SPI.h>
#include <WiFi.h>

char ssid[] = "Batttery Manager Interface";
char pass[] = "manager123";

int status = WL_IDLE_STATUS;
WiFiServer server(80);

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);


int batteryADCVoltage = 0;
float voltage_value;

const int ADC_PIN = 34;
const int RELAY_PIN = 14;

const int SCREEN_SDA = 21;
const int SCREEN_SCL = 22;

String header;

int mode = 1;

void setup() {

  Serial.begin(115200);

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;)
      ;  // Don't proceed, loop forever
  }
  display.display();
  // attempt to connect to Wifi network:
  WiFi.softAP(ssid, pass);
  server.begin();

  // put your setup code here, to run once:
  pinMode(RELAY_PIN, OUTPUT);
  delay(1000);
}

long currentTime = millis();

void loop() {
  // put your main code here, to run repeatedly:
  if (currentTime + 2000 < millis()) {
    currentTime = millis();
    batteryADCVoltage = analogRead(ADC_PIN);
    Serial.print("Battery Voltage: ");
    voltage_value = (batteryADCVoltage * 3.45 * 11) / (4095);
    Serial.println(voltage_value);
    if (voltage_value < 22 && mode != LOW) {
      mode = LOW;
      digitalWrite(RELAY_PIN, mode);
      Serial.println("Battery is low, turning off.");
    } else if (voltage_value > 26 && mode != HIGH) {
      mode = HIGH;
      Serial.println("Battery is back online! Turning on.");
      digitalWrite(RELAY_PIN, mode);
    }
    drawInfo();
  }
  handleWifi();
}

void handleWifi() {
  WiFiClient client = server.available();  // listen for incoming clients

  if (!client) return;
  Serial.println("new client");
  String currentLine = "";
  while (client.connected()) {
    if (!client.available()) {
      continue;
    }
    char c = client.read();
    header += c;
    if (c != '\n' && c != '\r') {
      currentLine += c;
      continue;
    }

    if (currentLine.length() != 0) {
      currentLine = "";
      continue;
    }

    // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
    // and a content-type so the client knows what's coming, then a blank line:
    client.println("HTTP/1.1 200 OK");
    client.println("Content-type:application/json");
    client.println("Connection: close");
    client.println();
    Serial.println("Sending response...");
    // the content of the HTTP response follows the header:
    client.print("{ \"battery_voltage:\": ");
    client.print(voltage_value);
    client.print(", \"relay_status\": ");
    client.print(mode);
    client.print(" }");

    // The HTTP response ends with another blank line:
    client.println();
    client.println();
    // break out of the while loop:
    break;
  }
  client.stop();
  Serial.println("client disonnected");
  header = "";
  return;
}

void drawInfo() {
  display.clearDisplay();

  display.setTextSize(1);       // Normal 1:1 pixel scale
  display.setTextColor(WHITE);  // Draw white text
  display.setCursor(0, 0);      // Start at top-left corner
  display.cp437(true);          // Use full 256 char 'Code Page 437' font

  // Not all the characters will fit on the display. This is normal.
  // Library will draw what it can and the rest will be clipped.
  display.write("Battery Voltage");
  display.setCursor(0, 10);
  display.write(String(voltage_value).c_str());
  display.write("v");
  display.setCursor(0, 20);
  display.write("Relay status");
  display.setCursor(0, 30);
  if (mode == 1)
    display.write("ON");
  if (mode == 0)
    display.write("OFF");

  display.setCursor(0, 40);
  display.write("Soft AP IP");
  display.setCursor(0, 50);
  display.write(WiFi.softAPIP().toString().c_str());

  display.display();
  delay(2000);
}
