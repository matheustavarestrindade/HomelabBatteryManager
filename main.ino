#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <SPI.h>
#include <WiFi.h>

char ssid[] = "Rede trindade repetidor";
char pass[] = "";

int status = WL_IDLE_STATUS;
WiFiServer server(80);

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1

#define ADC_PIN 34
#define RELAY_PIN 14
#define SCREEN_SDA 21
#define SCREEN_SCL 22

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);


int battery_adc_voltage = 0;
float voltage_value;

String header;
int mode = -1;

void setup() {

  Serial.begin(115200);

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;)
      ;  // Don't proceed, loop forever
  }
  display.display();
  // attempt to connect to Wifi network:
  WiFi.mode(WIFI_STA); 
  WiFi.begin(ssid, pass);

  Serial.println("\nConnecting");

  while(WiFi.status() != WL_CONNECTED){
      Serial.print(".");
      delay(100);
  }

  Serial.println("\nConnected to the WiFi network");
  Serial.print("Local ESP32 IP: ");
  Serial.println(WiFi.localIP());

  server.begin();

  // put your setup code here, to run once:
  pinMode(RELAY_PIN, OUTPUT);
  delay(1000);

}

long currentTime = millis();
float last_battery_values[12] = {0,0,0,0,0,0,0,0,0,0,0,0};
int last_battery_value_index = 0;

void loop() {

  if(currentTime + 5000 > millis()) { handleWifi();  return; }
  currentTime = millis();
  
  calculateBatteryVoltage();

  if (voltage_value < 22 && mode != 0) {
      mode = 0;
      Serial.println("Battery is low, turning off.");
      digitalWrite(RELAY_PIN, LOW);
  } else if (voltage_value > 25 && mode != 1) {
    mode = 1;
    Serial.println("Battery is back online! Turning on.");
    digitalWrite(RELAY_PIN, HIGH);
  }

  drawInfo();
  handleWifi();
}

void calculateBatteryVoltage() {
  battery_adc_voltage = analogRead(ADC_PIN);
  Serial.print("Battery Voltage: ");
  float read_voltage = (battery_adc_voltage * 3.47 * 11) / (4095);
  Serial.println(read_voltage);

  last_battery_values[last_battery_value_index] = read_voltage;
  if(last_battery_value_index == 11) last_battery_value_index = 0;
  else last_battery_value_index++;

  int i;
  float total = 0;
  Serial.print("Last Values: ");
  for(i = 0; i < 12; i++){
    total += last_battery_values[i];
    Serial.print(last_battery_values[i]);
    Serial.print(", ");
  }
  voltage_value = (total / 12);
  Serial.print("Battery calculated Voltage 60s: ");
  Serial.println(voltage_value);


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
  display.write(WiFi.localIP().toString().c_str());

  display.display();
  delay(2000);
}
