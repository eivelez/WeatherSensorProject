#include <Arduino.h>
#include "ESP32_MailClient.h"
#include "DHT.h"
#include <String.h>
#include <WiFi.h>
#define DHTPIN 4
#define DHTTYPE DHT11
//add here the wifi credentials, change mailing parameters in code.
#define WIFI_SSID "XXXXXXXXXXXX"
#define WIFI_PASSWORD "XXXXXXXXXXX"
int MAX_TEMP;
int MIN_TEMP;
int NOTIF_MAX;
int NOTIF_MIN;
WiFiServer server(80);
SMTPData smtpData;
DHT dht(DHTPIN, DHTTYPE);
void sendCallback(SendStatus info);

void setup()
{
  NOTIF_MAX=0;
  NOTIF_MIN=0;
  MAX_TEMP=28;
  MIN_TEMP=20;
  Serial.begin(9600);
  dht.begin();
  Serial.print("Connecting to AP");
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(200);
  }
  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  server.begin();
}

void loop()
{
  WiFiClient client = server.available();
  if (client) {
    String currentLine = "";
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        //Serial.write(c);
        if (c == '\n') {
          if (currentLine.length() == 0) {
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println();
            client.print("min:"+(String)MIN_TEMP+" max:"+(String)MAX_TEMP+"<br>");
            client.print("Click <a href=\"/upH\">HERE</a> to turn up the max temperature.<br>");
            client.print("Click <a href=\"/downH\">HERE</a> to turn down the max temperature.<br>");
            client.print("Click <a href=\"/upL\">HERE</a> to turn up the min temperature.<br>");
            client.print("Click <a href=\"/downL\">HERE</a> to turn down the min temperature.<br>");
            client.print("Click <a href=\"/reset\">HERE</a> to reset alarm and receive mails again.<br>");
            client.println();
            // break out of the while loop:
            break;
          } else {    // if you got a newline, then clear currentLine:
            currentLine = "";
          }
        } else if (c != '\r') {  // if you got anything else but a carriage return character,
          currentLine += c;      // add it to the end of the currentLine
        }

        // Check to see if the client request was "GET /H" or "GET /L":
        if (currentLine.endsWith("GET /upH")) {
          MAX_TEMP=MAX_TEMP+1;               // GET /H turns the LED on
        }
        if (currentLine.endsWith("GET /downH")) {
          MAX_TEMP=MAX_TEMP-1;                // GET /L turns the LED off
        }
        if (currentLine.endsWith("GET /upL")) {
          MIN_TEMP=MIN_TEMP+1;               // GET /H turns the LED on
        }
        if (currentLine.endsWith("GET /downL")) {
          MIN_TEMP=MIN_TEMP-1;                // GET /L turns the LED off
        }
        if (currentLine.endsWith("GET /reset")) {
          NOTIF_MAX=0;
          NOTIF_MIN=0;
        }
      }
    }
    // close the connection:
    client.stop();
  }
  delay(2000);
  float h = dht.readHumidity();
  float t = dht.readTemperature();
  float f = dht.readTemperature(true);
  if (isnan(h) || isnan(t) || isnan(f)) {
    Serial.println(F("Failed to read from DHT sensor!"));
    return;
  }
  float hif = dht.computeHeatIndex(f, h);
  float hic = dht.computeHeatIndex(t, h, false);
  Serial.println(hic);
  if (hic>MAX_TEMP && NOTIF_MAX==0)
  {
    Serial.println("Sending email...");
    //edit here the sender mail credentials
    smtpData.setLogin("smtp.gmail.com", 587, "yourMail@mail.com", "yourPassword");
    smtpData.setSender("ESP32", "SOME_EMAIL_ACCOUNT@SOME_EMAIL.com");
    smtpData.setPriority("High");
    String msg="Alerta alta temperatura ["+(String)MAX_TEMP+"°C]";
    smtpData.setSubject("Alerta alta temperatura");
    smtpData.setMessage(msg,true);
    smtpData.addRecipient("alarmRecipient@mail.com");
    smtpData.setSendCallback(sendCallback);
    if (!MailClient.sendMail(smtpData))
    {
      Serial.println("Error sending Email, " + MailClient.smtpErrorReason());
    }
    else
    {
      NOTIF_MAX=1;
    }
    smtpData.empty();
  }
  if (hic<MIN_TEMP && NOTIF_MIN==0)
  {
    Serial.println("Sending email...");
    smtpData.setLogin("smtp.gmail.com", 587, "senderMail@mail.com", "yourPassword");
    smtpData.setSender("ESP32", "SOME_EMAIL_ACCOUNT@SOME_EMAIL.com");
    smtpData.setPriority("High");
    String msg="Alerta baja temperatura ["+(String)MIN_TEMP+"°C]";
    smtpData.setSubject("Alerta baja temperatura");
    smtpData.setMessage(msg,true);
    smtpData.addRecipient("recipientMail@mail.com");
    smtpData.setSendCallback(sendCallback);
    if (!MailClient.sendMail(smtpData))
    {
      Serial.println("Error sending Email, " + MailClient.smtpErrorReason());
    }
    else
    {
      NOTIF_MIN=1;
    }
    smtpData.empty();
  }
}
void sendCallback(SendStatus msg)
{
  Serial.println(msg.info());
  if (msg.success())
  {
    Serial.println("----------------");
  }
}
