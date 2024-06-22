#include <WiFi.h>
#include <DHT.h>
#include <ESP_Mail_Client.h>
#include <WebServer.h>

// WiFi credentials
#define WIFI_SSID "ENTER YOUR WIFI NAME"
#define WIFI_PASSWORD " ENTER YOUR PASSWORD"

// ThingSpeak settings
#define THINGSPEAK_API_KEY "ENTER YOUR API KEY"
#define THINGSPEAK_CHANNEL_ID 2447752

// SMTP server settings
#define SMTP_HOST "smtp.gmail.com"
#define SMTP_PORT 465 // Use 465 for SSL
#define AUTHOR_EMAIL "ENTER YOUR EMAIL"
#define AUTHOR_PASSWORD "ENTER YOUR PASSWORD"

// DHT sensor settings
#define DHTPIN 7
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

// Recipient email address
#define RECIPIENT_EMAIL "ENTER THE RECIPIENT EMAIL "

// Pins
#define FAN_PIN 5
#define SENSOR_PIN_1 A0
#define SENSOR_PIN_2 A2

// Thresholds
const float TEMP_THRESHOLD = 30.0;
const float HUMIDITY_THRESHOLD = 60.0;
float gasThreshold = 1600.0;

SMTPSession smtp;
WiFiServer server(80); // Declare server globally

void handleRoot() {
  WiFiClient client = server.available();
  if (!client) {
    return;
  }

  char msg[1500];p

  // Read sensor values
  float temp = dht.readTemperature();
  float humid = dht.readHumidity();
  int analogValue1 = analogRead(SENSOR_PIN_1);
  int analogValue2 = analogRead(SENSOR_PIN_2);
  float ppm1 = analogToPPM(analogValue1);
  float ppm2 = analogToPPM(analogValue2);

  snprintf(msg, 1500,
           "<html>\
  <head>\
    <meta http-equiv='refresh' content='4'/>\
    <meta name='viewport' content='width=device-width, initial-scale=1'>\
    <title>ESP32 DHT Server</title>\
    <style>\
    html { font-family: Arial; display: inline-block; margin: 0px auto; text-align: center;}\
    h2 { font-size: 3.0rem; }\
    p { font-size: 3.0rem; }\
    .units { font-size: 1.2rem; }\
    .dht-labels{ font-size: 1.5rem; vertical-align:middle; padding-bottom: 15px;}\
    body{ background-color:#caf0f8;}\
    </style>\
  </head>\
  <body>\
      <h2>Warehouse Onion Storage Optimization: Early Spoilage Detection and Alert System</h2>\
      <p>\
        <i class='fas fa-thermometer-half' style='color:#ca3517;'></i>\
        <span class='dht-labels'>Temperature</span>\
        <span>%.2f</span>\
        <sup class='units'>&deg;C</sup>\
      </p>\
      <p>\
        <i class='fas fa-tint' style='color:#00add6;'></i>\
        <span class='dht-labels'>Humidity</span>\
        <span>%.2f</span>\
        <sup class='units'>&percnt;</sup>\
      </p>\
       <p>\
        <i class='fas fa-gas-canister' style='color:#00add6;'></i>\
        <span class='dht-labels'>rack1 ppm</span>\
        <span>%.2f</span>\
        <sup class='units'>ppm</sup>\
      </p>\
       <p>\
        <i class='fas fa-gas-canister' style='color:#00add6;'></i>\
        <span class='dht-labels'>rack2 ppm</span>\
        <span>%.2f</span>\
        <sup class='units'>ppm</sup>\
      </p>\
  </body>\
</html>",
           temp, humid, ppm1, ppm2
          );

  client.print("HTTP/1.1 200 OK\r\n");
  client.print("Content-Type: text/html\r\n");
  client.print("Connection: close\r\n\r\n");
  client.print(msg);
}

void setup() {
  Serial.begin(115200);
  pinMode(FAN_PIN, OUTPUT);
  dht.begin();
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println("\nWiFi connected.");

  smtp.debug(1);
  smtp.callback(smtpCallback);
  server.begin();
}

void loop() {
  WiFiClient client = server.available();
  if (client) {
    handleRoot();
  }

  float temp = dht.readTemperature();
  float humid = dht.readHumidity();
  int analogValue1 = analogRead(SENSOR_PIN_1);
  int analogValue2 = analogRead(SENSOR_PIN_2);
  float ppm1 = analogToPPM(analogValue1);
  float ppm2 = analogToPPM(analogValue2);

  if (isnan(temp) || isnan(humid)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }

  Serial.print("Temperature: ");
  Serial.print(temp);
  Serial.print(" °C, Humidity: ");
  Serial.print(humid);
  Serial.println(" %");

  Serial.println("Gas Sensor 1 PPM: " + String(ppm1));
  Serial.println("Gas Sensor 2 PPM: " + String(ppm2));

  if (ppm1 > gasThreshold) {
    sendEmailAlert("The rack1 is about to spoil, please sell them.");
  }

  if (ppm2 > gasThreshold) {
    sendEmailAlert("The rack2 is about to spoil, please sell them.");
  }

  if (temp > TEMP_THRESHOLD && humid > HUMIDITY_THRESHOLD) {
    turnOnMotor();
    delay(5000);
    Serial.println("Temperature and humidity exceeded thresholds. Turning on the fan.");
  } else {
    turnOffMotor();
    Serial.println("Temperature and humidity are within acceptable range. Fan is off.");
  }

  delay(5000);
}

float analogToPPM(int analogValue) {
  return map(analogValue, 0, 1023, 0, 300);
}

void smtpCallback(SMTP_Status status) {
  if (status.success()) {
    Serial.println("Email sent successfully");
  }
}

void sendEmailAlert(String message) {
  Session_Config config;
  config.server.host_name = SMTP_HOST;
  config.server.port = SMTP_PORT;
  config.login.email = AUTHOR_EMAIL;
  config.login.password = AUTHOR_PASSWORD;

  SMTP_Message email;
  email.sender.name = "ESP32";
  email.sender.email = AUTHOR_EMAIL;
  email.subject = "Alert from Onion Storage System";
  email.addRecipient("Recipient", RECIPIENT_EMAIL);
  email.text.content = message.c_str();

  if (!smtp.connect(&config)) {
   
  Serial.println("Connection error");
  return;
  }

  if (!MailClient.sendMail(&smtp, &email, true)) {
    Serial.println("Error sending email");
  }
}

void turnOnMotor() {
  Serial.println("Turning ON the motor");
  digitalWrite(FAN_PIN, HIGH);
}

void turnOffMotor() {
  Serial.println("Turning OFF the motor");
  digitalWrite(FAN_PIN, LOW);
}
