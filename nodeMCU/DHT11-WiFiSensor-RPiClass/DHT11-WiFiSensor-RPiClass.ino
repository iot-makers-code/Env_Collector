/*
 *  This sketch demonstrates how to set up a simple HTTP-like server.
 *  The server will set a GPIO pin depending on the request
 *    http://server_ip/gpio/0 will set the GPIO2 low,
 *    http://server_ip/gpio/1 will set the GPIO2 high
 *  server_ip is the IP address of the ESP8266 module, will be 
 *  printed to Serial when the module is connected.
 */

#include <ESP8266WiFi.h>
#include <DHT.h>

char url[80];
const char *ssid = "RPi-wifihive-24";
const char *password = "wifihive";
char host[] = "192.168.200.1";
int  port = 80;

#define DHT11_PIN 2
#define DHTTYPE DHT11   // DHT 11

DHT dht(DHT11_PIN, DHTTYPE); 

WiFiClient client;
WiFiClient client2;

// Create an instance of the server
// specify the port to listen on as an argument
WiFiServer server(80);


void notify(char *url) {
  char buf[80];
  if (client2.connect(host, port)) {
    Serial.println("connected to server");
    // Make a HTTP request:
    client2.println(url);
    sprintf(buf, "Host: %s", host);
    client2.println(buf);
    client2.println("Connection: close");
    client2.println();

    while (client.available()) {
      char c = client.read();
      Serial.write(c);
    }

    // if the server's disconnected, stop the client:
    if (!client.connected()) {
      Serial.println();
      Serial.println("disconnecting from server.");
      client.stop();
    }
  }
}

void setup() {
  Serial.begin(115200);
  delay(10);
 
  // Connect to WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected");
  
  // Start the server
  server.begin();
  Serial.println("Server started");

  // Print the IP address
  Serial.println(WiFi.localIP());

  sprintf(url, "GET /iot/sensor.php?func=newnode&mode=humy&ip=%s HTTP/1.1", WiFi.localIP().toString().c_str() );
  notify(url);
  
}



String req, s;
char v[80];
float temp, humy;
long  count = 0;
int  dcount = 0;
void loop() {
  // Check if a client has connected
  client = server.available();
  if (!client) {   
    dcount ++;
    if (dcount > 20*60) {
      dcount = 0;
      sprintf(url, "GET /iot/sensor.php?func=check&c=%ld&t=%01.f&h=%01.f&&ip=%s HTTP/1.1", 
              count, dht.readTemperature(), dht.readHumidity(), WiFi.localIP().toString().c_str() );
      notify(url);
      count++;
      //Serial.println("");
      Serial.println(url);
    }
    //Serial.print(".");
    delay(10);
    return;  
  }
  dcount = 0;
  // Wait until the client sends some data
  Serial.println("new client");
  while(!client.available()){  delay(5);  }
  
  // Read the first line of the request
  req = client.readStringUntil('\r');
  Serial.println(req);
  client.flush();
  
  // Match the request
  // Prepare the response
  s = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n";
  
  if (req.indexOf("/temp") != -1) {
    sprintf(v,"%0.1f", dht.readTemperature());
  } else if (req.indexOf("/humy") != -1) {
    sprintf(v,"%0.1f", dht.readHumidity());
  } else if (req.indexOf("/count") != -1) {
    sprintf(v,"%ld", count);
  } else if (req.indexOf("/all") != -1) {
    sprintf(v,"%ld ==== %0.1f C, %0.1f %%", count, dht.readTemperature(), dht.readHumidity());
  } else {
    Serial.println("invalid request");
    client.stop();
    return;
  }
  s += v;  
  client.flush();

  // Send the response to the client
  client.print(s);
  delay(1);
  Serial.println("Client disonnected");
  //client.stop();

  // The client will actually be disconnected 
  // when the function returns and 'client' object is detroyed
}

