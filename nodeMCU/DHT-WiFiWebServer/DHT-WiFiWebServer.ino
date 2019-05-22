/*
		This sketch demonstrates how to set up a simple HTTP-like server.
*/

//#include <ESP8266WiFi.h>
#include <DHT.h>
#include <EEPROM.h>
#include <ESP8266WebServer.h>


String notify(char *); //send message to http server
void getENV(); //read local system information ex: IP, MAC & send the informatio to http server
void connectWiFi();  //connect defined WiFi Access Pinter

#define DHT11_PIN 2
#define DHTTYPE DHT11
DHT dht(DHT11_PIN, DHTTYPE);

// for EEPROM service ///////////////////////////////
typedef struct _EdgeService {
  char status;
  char ssid[33];
  char password[33];
  char host[33];
  int  port;
} defEdgeService ;

typedef struct _APConfig {
  char ssid[33];
  char password[33];
  IPAddress ip;
  long time;
} defAPConfig;

defEdgeService EdgeService;
defAPConfig APConfig;

void check_configuration(char mode);
void set_edge( char *ssid, char *password, char *host, int port);
void print_edge(defEdgeService *pEdgeService );
void print_ap(defAPConfig *pAPConfig );
void set_mode( char mode);

// for NODE Service ///////////////////////////////
struct  {
  char url[120];
  char mac[18];
  char ip[16];
} ENV;

// global arguemets
WiFiClient responser;
WiFiClient requester;

// Create an instance of the server
// specify the port to listen on as an argument
//ESP8266WebServer server(80);
WiFiServer server(80);


//////////////////////////////////////////////////////
void handleRoot();


void startAP() {
  // start AP
  Serial.setDebugOutput(true);
  //Serial.println(WiFi.status());
  WiFi.disconnect(true);

  WiFi.mode(WIFI_AP_STA);
  WiFi.softAPConfig(APConfig.ip, APConfig.ip, IPAddress (255,255,255,0));
  WiFi.softAP(APConfig.ssid, APConfig.password);
 IPAddress myIP = WiFi.softAPIP();
  Serial.print("AP IP address: "); Serial.println(myIP);
  WiFi.printDiag(Serial);
  Serial.println(WiFi.getPhyMode());
  Serial.print("Connecting");

  delay(500);
  Serial.print(".");

  Serial.println("\nWiFi ap started");
  Serial.setDebugOutput(false);
}


void setup() {
  Serial.begin(115200);
  delay(10);
  Serial.flush();

  EEPROM.begin(512);
  //check_configuration('F'); // factory setting
  set_mode('C');
  check_configuration('N'); // normal operation
  delay(10);

  if ( EdgeService.status == 'C') {
    Serial.println("Service Mode");
    Serial.flush();
    //APserver.stop(); 	delay(100);

    connectWiFi();
    getENV();
    // Start the server
    server.begin();
    Serial.println("Server started");
  } else {
    startAP();
    server.begin();
    /*
      ESP8266WebServer APserver(8080);
      Serial.println("Install Mode");
      Serial.flush();
      //server.stop(); delay(100);
      Serial.print("Configuring access point...");
      // You can remove the password parameter if you want the AP to be open.
      Serial.print("SSID: ");     Serial.println(APConfig.ssid);
      Serial.print("password: ");     Serial.println(APConfig.password);
      WiFi.softAP(APConfig.ssid, APConfig.password);

      IPAddress myIP = WiFi.softAPIP();
      Serial.print("AP IP address: ");	Serial.println(myIP);
      APserver.on("/", handleRoot);
      APserver.begin();
      Serial.println("HTTP server started");
    */
  }
}

////// LOOP /////////////////
String req, rt;
long count = 0;
int dcount;
int	delay_max = 120;
void loop() {

  if ( EdgeService.status == 'C') {
    // Check if a responser has connected
    if (WiFi.status() != WL_CONNECTED) connectWiFi();

    responser = server.available();
    if (!responser) {
      dcount ++;
      if (dcount > (count < 10 ? count / 10 * delay_max : delay_max) ) { //--
        dcount = 0;
        sprintf(ENV.url, "GET /iot/sensor.php?func=check&c=%ld&t=%.1f&h=%.1f&ip=%s&mac=%s HTTP/1.1",
                count, dht.readTemperature(), dht.readHumidity(), ENV.ip, ENV.mac );
        notify(ENV.url);
        count++;
        Serial.println(ENV.url);
      }
      //Serial.print(".");
      delay(50);
      return;
    }
    dcount = 0;
  }
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
  char msg[80];
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
  byte macb[6];

  sprintf(ENV.ip, "%s", WiFi.localIP().toString().c_str());
  Serial.println(ENV.ip);

  //mac2String();
  WiFi.macAddress(macb);
  char *s = ENV.mac;
  for (byte i = 0; i < 6; ++i) {
    sprintf(s, "%2X%s", macb[i], (i < 5) ? ":" : "");
    s += strlen(s);
  }
  Serial.println(ENV.mac);
  sprintf(ENV.url, "GET /iot/sensor.php?func=newnode&mode=humy&ip=%s&mac=%s HTTP/1.1", ENV.ip, ENV.mac );
  notify(ENV.url);
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

      Serial.println();
      Serial.print("Connecting to "); Serial.println(EdgeService.ssid);

      //WiFi.mode(WIFI_STA);
      WiFi.setAutoConnect(false);
      WiFi.setPhyMode(WIFI_PHY_MODE_11G);
      WiFi.begin(EdgeService.ssid, EdgeService.password);
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
  if (requester.connect(EdgeService.host, EdgeService.port)) {
    Serial.println("connected to server");

    // Make a HTTP request:
    requester.println(url);
    sprintf(buf, "Host: %s", EdgeService.host);	 requester.println(buf);
    requester.println("Connection: close");
    requester.println();

    unsigned long timeout = millis();
    while (requester.available() == 0) {
      if (millis() - timeout > 5000) {
        Serial.println(">>> responser Timeout !");
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
        received = line.substring(fa, fb);
        Serial.println(line + ">>>>>>>>[" + received + "]");
      }
      if (line.length() == 0) isData = true;
    }

    // if the server's disconnected, stop the responser:
    if (!requester.connected()) {
      Serial.println("disconnecting from server.");
      Serial.flush();
      requester.stop();
    }
    return received;
  }
  return "";
}

//EEPROM modules
void check_configuration(char mode) {
  int addr;
  char flag;
  flag = EEPROM.read(0);

  if (flag == 'C' && mode != 'F') { // get current information C:commited, F:factory set
    addr = 0;
    ///////////////////////////////////////
    EEPROM.get(addr, EdgeService);
    Serial.println("get configuration from : EEPROM");
    print_edge(&EdgeService );
    addr += sizeof(EdgeService);
    /////////////////////////////////
    EEPROM.get(addr, APConfig);
    print_ap(&APConfig);
    addr += sizeof(APConfig);

  } else  {  // factory set or none defined
    Serial.println("get configuration from : FACTORY");
    addr = 0;

    ////////////////////////////////
    EdgeService.status = 'F';
    strcpy (EdgeService.ssid, "RPi-wifihive-24");
    strcpy (EdgeService.password, "wifihive");
    strcpy (EdgeService.host, "192.168.200.1");
    EdgeService.port = 8117;

    EEPROM.put(addr, EdgeService);
    addr += sizeof (EdgeService);

    ///////////////////////////////////
    strcpy (APConfig.ssid, "Welcome-to-hive");
    strcpy (APConfig.password, "12345678");
    APConfig.ip = IPAddress(10, 10, 10, 1);
    APConfig.time = millis();

    EEPROM.put(addr, APConfig);
    addr += sizeof (APConfig);

    ////////////////////////////////////
    EEPROM.commit();
    
  }
  return;
}

void set_edge( char *ssid, char *password, char *host, int port) {
  int addr;

  addr = 0;
  EdgeService.status = 'C';
  strcpy (EdgeService.ssid, ssid);
  strcpy (EdgeService.password, password);
  strcpy (EdgeService.host, host);
  EdgeService.port = port;

  EEPROM.put(addr, EdgeService);
  EEPROM.commit();
}

void set_mode( char mode) {
  EEPROM.write(0, mode);
  EEPROM.commit();
}

void print_edge(defEdgeService *pEdgeService ) {
  Serial.print("EdgeService.status:");  Serial.println(pEdgeService->status);
  Serial.print("EdgeService.ssid:");  Serial.println(pEdgeService->ssid);
  Serial.print("EdgeService.password:");  Serial.println(pEdgeService->password);
  Serial.print("EdgeService.host:");  Serial.println(pEdgeService->host);
  Serial.print("EdgeService.port:");  Serial.println(pEdgeService->port);
  Serial.println();
  Serial.flush();
}

void print_ap(defAPConfig *pAPConfig ) {
  Serial.print("APConfig.ssid:");  Serial.println(pAPConfig->ssid);
  Serial.print("APConfig.password:");  Serial.println(pAPConfig->password);
  Serial.print("APConfig.ip:");  Serial.println(pAPConfig->ip);
  Serial.println();
  Serial.flush();
}

//////////////////////////////
void handleRoot() {
  //APserver.send(200, "text/html", "<h1>You are connected</h1>");
}