/*
		This sketch base on WiFiWebServer with simple HTTP-like server.
*/
#include <ESP8266WiFi.h>
#include <DHT.h>

String notify(char *); //send message to http server
void getENV(); //read local system information ex: IP, MAC & send the informatio to http server
void connectWiFi();  //connect defined WiFi Access Pinter

#define DHT11_PIN 2
#define DHTTYPE DHT11
DHT dht(DHT11_PIN, DHTTYPE);

// 설정 필요: for access edge service ///////////////////////////////
char *ssid = (char *)"RPi-wifihive-24";
char *password = (char *)"wifihive"; // 와이파이 비번
char *host = (char *)"192.168.200.1"; // 서버IP
int  port = 8117;   // 라즈베리파이 웹서버 포트
char mode = 'C';  // A/B/P : AP mode/Battery/Cable Powered

// for NODE Service ///////////////////////////////
struct  {
  char mac[18];
  char ip[16];
} ENV;
char msg[120];

// edge 서버 데이터 전송
WiFiClient requester;
// 센세 응답
WiFiClient responser;
WiFiServer server(80);

//////////////////////////////////////////////////////
void setup() {
  Serial.begin(115200);
  delay(10);
  Serial.println("Service Mode");
  Serial.flush();

  // Set environments
  connectWiFi();
  getENV();

  // Start the server
  server.begin();
  Serial.println("Server started");

  if (mode == 'B') {
    my_loop();
    Serial.println("Going into deep sleep for 120 seconds");
    ESP.deepSleep(120e6); // 20e6 is 20 microseconds
  }
}

void loop() {
  if (mode != 'B') my_loop();
}


////// LOOP /////////////////
int action (String msg) {
	Serial.print("Action for ");
	Serial.println(msg);
	return 0;
}
String req, rt;
long count = 0;
int dcount;
int	delay_max = 120;
void my_loop() {
  // Check if a responser has connected
  if (WiFi.status() != WL_CONNECTED) connectWiFi();
  //Serial.print("WiFi.status() = ");
  //Serial.println(WiFi.status());

  responser = server.available();
  if (!responser) {
    dcount ++;
    if (dcount > (count < 10 ? count / 10 * delay_max : delay_max) ) { //--
      dcount = 0;
      sprintf(msg, "GET /iot/sensor.php?func=check&c=%ld&t=%.1f&h=%.1f&ip=%s&mac=%s HTTP/1.1",
              count, dht.readTemperature(), dht.readHumidity(), ENV.ip, ENV.mac );
      String received = "";
      int received_timeout = 6;
      while (received == "" && received_timeout > 0) {
      	received = notify(msg);
      	received_timeout--;
      	delay(10000);
      }
      action(received);
      count++;
      Serial.println(msg);
    }
    //Serial.print(".");
    delay(50);
    return;
  }
  dcount = 0;
  // Wait until the responser sends some data
  Serial.println("new response");
  while (!responser.available()) {
    delay(5);
  }

  // Read the first line of the request
  req = responser.readStringUntil('\r');
  Serial.println(req);
  responser.flush();

  // Match the request
  // Prepare the response
  if (req.indexOf("/temp") != -1) {
    sprintf(msg, "%0.1f", dht.readTemperature());
  } else if (req.indexOf("/humy") != -1) {
    sprintf(msg, "%0.1f", dht.readHumidity());
  } else if (req.indexOf("/count") != -1) {
    sprintf(msg, "%ld", count);
  } else if (req.indexOf("/all") != -1) {
    sprintf(msg, "%ld ==== %0.1f C, %0.1f %%", count, dht.readTemperature(), dht.readHumidity());
  } else {
    Serial.println("invalid request");
    responser.stop();
    return;
  }
  rt = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n";
  rt += msg;

  // Send the response to the responser
  responser.print(rt);
  responser.flush();
  delay(1);
  Serial.println("responser disonnected");
  //responser.stop();
  // The responser will actually be disconnected
  // when the function returns and 'responser' object is detroyed
}

void getENV() {
  sprintf(ENV.ip, "%s", WiFi.localIP().toString().c_str());
  Serial.println(ENV.ip);

  //mac2String();
  byte macb[6];
  WiFi.macAddress(macb);
  char *s = ENV.mac;
  for (byte i = 0; i < 6; ++i) {
    sprintf(s, "%2X%s", macb[i], (i < 5) ? ":" : "");
    s += strlen(s);
  }
  Serial.println(ENV.mac);
  sprintf(msg, "GET /iot/sensor.php?func=newnode&mode=humy&ip=%s&mac=%s HTTP/1.1", ENV.ip, ENV.mac );
  notify(msg);
}

void connectWiFi() {
  // Connect to WiFi network
  int waiting = 0;
  Serial.setDebugOutput(true);
  //Serial.println(WiFi.status());
  WiFi.disconnect(true);
  while (WiFi.status() != WL_CONNECTED ) {
    if (waiting == 0) {
      Serial.print("Scan start ... ");
      int n = WiFi.scanNetworks();
      Serial.print(n);
      Serial.println(" network(s) found");
      for (int i = 0; i < n; i++) {
        Serial.println(WiFi.SSID(i));
      }
      Serial.println();
      Serial.print("Connecting to "); Serial.println(ssid);

      WiFi.mode(WIFI_STA);
      //WiFi.setAutoConnect(false);
      //WiFi.setPhyMode(WIFI_PHY_MODE_11G);
      WiFi.begin(ssid, password);
      WiFi.printDiag(Serial);
      Serial.println(WiFi.getPhyMode());
      Serial.print("Connecting");
    }
    delay(500);
    Serial.print(".");
    if (waiting++ > 120) waiting = 0;
  }
  Serial.println("\nWiFi connected");
  Serial.setDebugOutput(false);
}

String notify(char *url) {
  char buf[80];
  Serial.print("RSSI:"); Serial.println(WiFi.RSSI());
  if (requester.connect(host, port)) {
    Serial.println("connected to server");
    Serial.flush();

    // Make a HTTP request:
    requester.println(url);
    sprintf(buf, "Host: %s", host);	 requester.println(buf);
    requester.println("Connection: close");
    requester.println();

    unsigned long timeout = millis();
    while (requester.available() == 0) {
      if (millis() - timeout > 5000) {
        Serial.println(">>> request Timeout !");
        requester.stop();
        return "";
      }
    }

    bool isData = false;
    String received;
    while (requester.available()) {
      String line = requester.readStringUntil('\r');
      line.trim();
      //Serial.println(line);
      if (isData) {
        String a = "{\"data\":\"";
        String b = "\",\"status";
        int fa = line.indexOf(a) + a.length();
        int fb = line.indexOf(b);
        if (fa < 100 || fb < 1000) {
          received = line.substring(fa, fb);
          //Serial.println(line + ">>>>>>>>[" + received + "]");
        }
      }
      if (line.length() == 0) isData = true;
    }

    // if the server's disconnected, stop the responser:
    if (!requester.connected()) {
      //Serial.println("disconnecting from server.");
      Serial.flush();
      requester.stop();
    }
    return received;
  }
  return "";
}
