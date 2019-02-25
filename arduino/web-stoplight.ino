#include <Ethernet.h>
#include <SPI.h>

byte mac[] = {0x90, 0xa2, 0xda, 0x0e, 0x98, 0x34};
IPAddress ip(192, 168, 254, 33);
EthernetServer server(80);
int redLED = 7;
int yellowLED = 6;
int greenLED = 5;
char c;

void setup() {
  Serial.begin(9600);
  Ethernet.begin(mac, ip);
  pinMode(redLED, OUTPUT);
  pinMode(yellowLED, OUTPUT);
  pinMode(greenLED, OUTPUT);
  digitalWrite(redLED, LOW);
  digitalWrite(yellowLED, LOW);
  digitalWrite(greenLED, LOW);
  delay(1000);
  Serial.print("The server is on IP address: ");
  Serial.println(Ethernet.localIP());
  Serial.println("v2");
}

void loop() {
  EthernetClient client = server.available();
  if (client.connected())
  {
    while (client.available())
    {
      c = client.read();
      if (c == '?')
      {
        c = client.read();
        switch (c)
        {
          case '1':
            Serial.println("Activate Red LED");
            digitalWrite(redLED, HIGH);
            digitalWrite(yellowLED, LOW);
            digitalWrite(greenLED, LOW);
            break;
          case '2':
            Serial.println("Activate Yellow LED");
            digitalWrite(redLED, LOW);
            digitalWrite(yellowLED, HIGH);
            digitalWrite(greenLED, LOW);
            break;
          case '3':
            Serial.println("Activate Green LED");
            digitalWrite(redLED, LOW);
            digitalWrite(yellowLED, LOW);
            digitalWrite(greenLED, HIGH);
            break;
        }
      }
    }


    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: text/html");
    client.println("Connection: close");
    client.println();
    client.println("<!DOCTYPE html>");
    client.println("<html>");
    client.println("<head>");
    client.println("<title>Arduino Controller</title>");
    client.println("</head>");
    client.println("<body>");
    client.println("<h2>Arduino Controller</h2>");
    client.print("<a href=\"");
    client.println("/?1\">Activate Red LED</a><br />");
    client.print("<a href=\"");
    client.println("/?2\">Activate Yellow LED</a><br />");
    client.print("<a href=\"");
    client.println("/?3\">Activate Green LED</a><br />");
    client.println("</body>");
    client.println("</html>");
    delay(10);
    client.stop();
  }
}