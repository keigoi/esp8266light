// Arduino Studio の設定方法：
// http://trac.switch-science.com/wiki/esp_dev_arduino_ide use esp8266 version 2.2

#include <Servo.h>
#include <ESP8266WiFi.h>

#define SERVO_OFF_PIN 4
#define SERVO_ON_PIN 5

#define SERVO_UPPER_PIN 5
#define SERVO_LOWER_PIN 4

#define SWOFF_PIN 15
#define SWON_PIN 16

/**
 * 部屋毎の 端末のカスタマイズ (FIXME...)
 * CHANGE HERE
 */
#define LOCATION_E424
// #define LOCATION_E425
// #define LOCATION_E405

#define WIFI_SSID "ctwlan_g"
#define WIFI_PASS "*************"


#define API_KEY "*************"

#define SERVICE_PORT 18080

#ifdef LOCATION_E425
  #define SERVO_LAYOUT_VER2
#endif

#ifdef LOCATION_E424
  #define SERVO_LAYOUT_VER3
  #define HAS_HW_SW
  #define HAS_REMOTE
#endif

#ifdef LOCATION_E405
  #define SERVO_LAYOUT_VER4
#endif


#define REMOTE_HOST "192.168.0.74"
#define REMOTE_PORT SERVICE_PORT


WiFiServer server(SERVICE_PORT);
WiFiClient client;

IPAddress gateway(192,168,0,64);
IPAddress subnet(255,255,255,0);


void turnoff();
void turnon();
void turnoff_remote();
void turnon_remote();


void setup() {
  pinMode(SWON_PIN, INPUT);
  pinMode(SWOFF_PIN, INPUT);
  
  Serial.begin(115200);
  
  WiFi.printDiag(Serial);
  WiFi.disconnect(true);
  digitalWrite(26,0);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  
#if defined(LOCATION_E405)
  WiFi.config(IPAddress(192,168,0,72), gateway, subnet);
#elif defined(LOCATION_E424)
  WiFi.config(IPAddress(192,168,0,73), gateway, subnet);
#elif defined(LOCATION_E425)
  WiFi.config(IPAddress(192,168,0,74), gateway, subnet);
#endif
    
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.printf("Connection status: %d\n", WiFi.status());
  }

  Serial.println("WiFi connected");

  server.begin();
  Serial.println("Server started");
  
  Serial.print(WiFi.localIP());
  Serial.println(" obtained");
  
}


bool react() {
  WiFiClient client = server.available();
  if (!client) {
    return false;
  }
  Serial.println("new client");
  client.setTimeout(500);
  int wait = 0;
  while (!client.available()) {
      delay(1);
      wait++;
      if(wait>1000) {
        Serial.println("too many available() calls");
        return false;
      }
  }
  Serial.println("client becomes available");
  
  String line = client.readStringUntil('\r');
  Serial.println(line);

  if (line.indexOf("GET /") != -1) {
    client.print(F("HTTP/1.1 200 OK\r\n"));
    client.print(F("Content-Type:text/plain\r\n"));
    client.print(F("Connection:close\r\n\r\n"));
  }

  if (line.indexOf("GET /turnon?key=" API_KEY) != -1) {
    turnon();
    client.print(F("Light turned on\r\n\r\n"));
  } else if(line.indexOf("GET /turnoff?key=" API_KEY) != -1) {
    turnoff();
    client.print(F("Light turned off\r\n\r\n"));
  } else {
    client.print(F("Do nothing. The request was:\r\n"));
    client.print(line);
    client.print(F("\r\n\r\n"));
  }

  client.flush();
  delay(1000);
  client.stop();
  delay(1000);

  return true;

}

bool on_pressing = false;
bool off_pressing = false;

bool clicked_(int PIN, bool* prev) {
  bool pressed = digitalRead(PIN) == HIGH;
  bool ret = !(*prev) && pressed;
  *prev = pressed;
  return ret;
}


bool swon_clicked() {
  return clicked_(SWON_PIN, &on_pressing);
}

bool swoff_clicked() {
  return clicked_(SWOFF_PIN, &off_pressing);
}

int cnt = 0;

void loop() {
#ifdef HAS_HW_SW
  if (cnt % 100000 == 0) {
    if (on_pressing) {
      Serial.println("on_presssing");
    } else if (off_pressing) {
      Serial.println("off_pressing");
    } else {
      Serial.println("none");
    }
    Serial.flush();
    cnt = 0;
  }
  cnt++;
#endif

  if (react()) {
    delay(1000);
  }

#ifdef HAS_HW_SW
  if (swon_clicked()) {
    turnon();
  } else if (swoff_clicked()) {
    turnoff();
  }
#endif // HAS_SW
}

#if defined(SERVO_LAYOUT_VER2) || defined(SERVO_LAYOUT_VER3)
Servo servo_off; // left
Servo servo_on; // right (ON)
#elif defined(SERVO_LAYOUT_VER4)
Servo servo_upper;
Servo servo_lower;
#endif


void servo_work(Servo& servo, int up, int down, int middle, int sleep) {
  servo.write(up);
  delay(sleep);
  servo.write(down);
  delay(sleep);
  servo.write(middle);
  delay(sleep);
}

void sw_remote(bool on) {
  WiFiClient client;
  int count = 0;
  while (!client.connect(REMOTE_HOST, REMOTE_PORT) && count < 5) {
    Serial.println("connection failed");
    Serial.println("wait 500 msec...");
    delay(500);
    count++;
  }
  if (!client.connected()) {
    return;
  }
  if(on) {
    client.println("GET /turnon?key=" API_KEY);
  } else {    
    client.println("GET /turnoff?key=" API_KEY);
  }
  delay(1);
}

void turnoff_remote() {
  sw_remote(false);
}

void turnon_remote() {
  sw_remote(true);
}

void turnoff() {
  Serial.println("turnoff called");
  
#if defined(SERVO_LAYOUT_VER2)
  servo_off.attach(SERVO_OFF_PIN);
  servo_work(servo_off, 20, 150, 90, 1000); // ver. 2
  servo_off.detach();
#elif defined(SERVO_LAYOUT_VER3)
  servo_off.attach(SERVO_OFF_PIN);
  servo_work(servo_off, 70, 110, 90, 500); // ver. 3
  servo_off.detach();
#elif defined(SERVO_LAYOUT_VER4)
  servo_upper.attach(SERVO_UPPER_PIN);
  servo_upper.write(110);
  delay(500);
  servo_upper.write(90);
  delay(500);
  servo_upper.detach();
  servo_lower.attach(SERVO_LOWER_PIN);
  servo_lower.write(70);
  delay(500);
  servo_lower.write(90);
  delay(500);
  servo_lower.detach();
#endif


#ifdef HAS_REMOTE
  turnoff_remote();
#endif
}


void turnon() {
  Serial.println("turnon called");
  
#if defined(SERVO_LAYOUT_VER2)
  servo_on.attach(SERVO_ON_PIN);
  servo_work(servo_on, 170, 25, 90, 1000); // ver. 2
  servo_on.detach();
#elif defined(SERVO_LAYOUT_VER3)
  servo_on.attach(SERVO_ON_PIN);
  servo_work(servo_on, 110, 70, 90, 500); // ver. 3
  servo_on.detach();
#elif defined(SERVO_LAYOUT_VER4)
  servo_upper.attach(SERVO_UPPER_PIN);
  servo_upper.write(65);
  delay(500);
  servo_upper.write(90);
  delay(500);
  servo_upper.detach();
  servo_lower.attach(SERVO_LOWER_PIN);
  servo_lower.write(120);
  delay(500);
  servo_lower.write(90);
  delay(500);
  servo_lower.detach();
#endif

#ifdef HAS_REMOTE
  turnon_remote();
#endif
}
