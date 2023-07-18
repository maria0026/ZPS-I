#include <WiFi.h>
#include <ESPAsyncWebSrv.h>
 
const char *ssid = "esp322";
const char *password = "12345678";

AsyncWebServer server(80);
#include <VL53L0X.h>
#define PIN_SCL   21 
#define PIN_SDA   17  

#define LOX1_ADDRESS 0x30 // address we will assign if dual sensor is present
#define LOX2_ADDRESS 0x31
#define LOX3_ADDRESS 0x32
int sensor1,sensor2,sensor3;

#define SHT_LOX1 34 // set the pins to shutdown
#define SHT_LOX2 36
#define SHT_LOX3 38
#define EEP 3  //  silnik
#define IN4 5
#define IN3 7
#define IN2 9
#define IN1 11
#define sensor1R 1   // defining the reflective sensors
#define sensor2R 6
#define sensor3R 2 
#define sensor4R 13

// objects for the vl53l0x
VL53L0X lox1;
VL53L0X lox2;
VL53L0X lox3;

// this holds the measurement
bool buttonPressed = false;
bool button2Pressed = false;
bool button3Pressed = false;

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <title>ESP32 Web Server</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    html {font-family: Arial; display: inline-block; text-align: center;}
    button {background-color: #4CAF50; border: none; color: white; padding: 10px 24px; text-align: center; text-decoration: none; display: inline-block; font-size: 24px; margin: 4px 2px; cursor: pointer;}
  </style>
</head>
<body>
  <h2>ESP32 Web Server</h2>
  <button onclick="sendData()">Start</button>
  <button onclick="sendData2()">Stop</button>
  <button onclick="sendData3()">Taktyka 2</button>
  <script>
    function sendData() {
      var xhttp = new XMLHttpRequest();
      xhttp.open("GET", "/send", true);
      xhttp.send();
    }
      function sendData2() {
      var xhttp = new XMLHttpRequest();
      xhttp.open("GET", "/send2", true);
      xhttp.send();
    }
      function sendData3() {
      var xhttp = new XMLHttpRequest();
      xhttp.open("GET", "/send3", true);
      xhttp.send();
    }
  
  </script>
</body>
</html>
)rawliteral";

void setID() {
  // all reset
  digitalWrite(SHT_LOX1, LOW);    
  digitalWrite(SHT_LOX2, LOW);
  digitalWrite(SHT_LOX3, LOW);
  delay(10);
  // all unreset
  digitalWrite(SHT_LOX1, HIGH);
  digitalWrite(SHT_LOX2, HIGH);
  digitalWrite(SHT_LOX3, HIGH);
  delay(10);

  // activating LOX1 and reseting LOX2
  digitalWrite(SHT_LOX1, HIGH);
  digitalWrite(SHT_LOX2, LOW);
  digitalWrite(SHT_LOX3, LOW);

  // initing LOX1
  lox1.setAddress(LOX1_ADDRESS);
  if(!lox1.init()) {
    Serial.println(F("Failed to boot first VL53L0X"));
  }
  lox1.setTimeout(50);
  delay(10);

  // activating LOX2
  digitalWrite(SHT_LOX2, HIGH);
  delay(10);

  //initing LOX2
  lox2.setAddress(LOX2_ADDRESS);
  
  if(!lox2.init()) {
    Serial.println(F("Failed to boot second VL53L0X"));
  }
  lox2.setTimeout(50);
  
  // activating LOX3
  digitalWrite(SHT_LOX3, HIGH);
  delay(10);

  //initing LOX3
  lox3.setAddress(LOX3_ADDRESS);
  if(!lox3.init()) {
    Serial.println(F("Failed to boot third VL53L0X"));
  }
  lox3.setTimeout(50);

}

int Dyst_LP() {
  return lox1.readRangeSingleMillimeters();
}

int Dyst_PP() {
  return lox2.readRangeSingleMillimeters();
}
  
int Dyst_T() {
  return lox3.readRangeSingleMillimeters();
}

int Linia_PP() {
  return analogRead(sensor1R);
}
int Linia_LP() {
  return analogRead(sensor2R);
}
int Linia_LT() {
  return analogRead(sensor3R);
}
int Linia_PT() {
  return analogRead(sensor4R);
}

void forward() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);
}

void backward() {
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
}

void rotate_right(){
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);  
}

void rotate_left(){
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
}

void stop_motors() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
  button2Pressed=false;
}

void stand() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
}

void stop() {
  digitalWrite(EEP, LOW);
}

void start() {
  digitalWrite(EEP, HIGH);
}

bool starter=true;
void setup() {
  Wire.begin(PIN_SDA, PIN_SCL);
  // Serial port for debugging
  Serial.begin(9600);
 
  // Set up the access point
  WiFi.softAP(ssid, password);
  Serial.print("Access point started. IP address: ");
  Serial.println(WiFi.softAPIP());

  pinMode(SHT_LOX1, OUTPUT);
  pinMode(SHT_LOX2, OUTPUT);
  pinMode(SHT_LOX3, OUTPUT);

  pinMode(sensor1R, INPUT);
  pinMode(sensor2R, INPUT);
  pinMode(sensor3R, INPUT);
  pinMode(sensor4R, INPUT);
  pinMode(LED_BUILTIN, OUTPUT);

  pinMode(EEP, OUTPUT);
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);

  digitalWrite(EEP, LOW);
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);

  Serial.println("Shutdown pins inited...");

  digitalWrite(SHT_LOX1, LOW);
  digitalWrite(SHT_LOX2, LOW);
  digitalWrite(SHT_LOX3, LOW);

  Serial.println("Both in reset mode...(pins are low)"); 
  Serial.println("Starting...");
  setID();
  
  // Serve the main page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/html", index_html);
  });


  // Send the message over the serial port when the button is pressed
  server.on("/send", HTTP_GET, [](AsyncWebServerRequest *request) {
    //if (!buttonPressed) {
    Serial.println("Button pressed. Message sent.");
    buttonPressed = true;

    request->send(200, "text/plain", "Message sent");
    //}
  });
  server.on("/send2", HTTP_GET, [](AsyncWebServerRequest *request) {
    //if (!button2Pressed) {
    Serial.println("Button2 pressed. Message2 sent.");
    button2Pressed = true;
    buttonPressed = false;
    button3Pressed = false;
    starter= true;
    stop_motors();
    request->send(200, "text/plain", "Message sent");
    //}
  });

    server.on("/send3", HTTP_GET, [](AsyncWebServerRequest *request) {
    //if (!buttonPressed) {
    Serial.println("Button3 pressed. Message sent.");
    button3Pressed = true;

    request->send(200, "text/plain", "Message sent");
    //}
  });
 
  // Start the server
  server.begin();
  delay(5000);
  for(int i=0; i <1; i++){
    Serial.print("Access point started. IP address: ");
    Serial.println(WiFi.softAPIP());
    delay(2000);
  }
  
  start();
}

void odczyty() {
  static unsigned long lastTime = 0;
  const long interval = 1000;
  unsigned long now = millis();

  if ( now - lastTime > interval ) {
    lastTime = now;
    /*
    Serial.print("Dyst_LP:");
    Serial.println(Dyst_LP());
    Serial.print("Dyst_PP:");
    Serial.println(Dyst_PP());
    Serial.print("Dyst_T:");
    Serial.println(Dyst_T());
    */
    Serial.print("Linia_LP:");
    Serial.println(Linia_LP());
    Serial.print("Linia_PP:");
    Serial.println(Linia_PP());
    Serial.print("Linia_LT:");
    Serial.println(Linia_LT());
    Serial.print("Linia_PT:");
    Serial.println(Linia_PT());
  }
}

const unsigned long Delay = 150;
unsigned long now = 0;
unsigned long LastSeen = 0;
bool seen = true;
bool seenline = false;

void loop() {
  
  now = millis();

  //odczyty();

  if(buttonPressed){

    if(starter){
      //delay(5000);
      forward();
      starter = false;
    }

    bool LineF = Linia_PP() < 6200 || Linia_LP() < 6200;
    
    if(LineF && !seenline){
      seenline = true;
      LastSeen = millis();
      backward();
      return;
    }

    bool LineB = Linia_PT() < 6200 || Linia_LT() < 6200;

    if(LineB && !seenline){
      seenline = true;
      LastSeen = millis();
      forward();
      return;
    }

    if(seen && seenline){
      rotate_left();
      delay(150);
      backward();
      seen = false;
      seenline = false;
      return;
    }

    if(seenline && millis() - LastSeen > Delay){
      seenline = false;
    }
    
    if(!seenline){
      int16_t PP = Dyst_PP();
      int16_t LP = Dyst_LP();
      //bool SeenF = LP < 500 || PP < 500;

      bool SeenF = LP < 200 || PP < 200;
      
      if(SeenF && !seen){
        seen = true;
        forward();
        return;
      }

      int16_t T = Dyst_T();
      bool SeenB = T < 200;

      if(SeenB && !seen){
        seen = true;
        backward();
        return;
      }
      if(!SeenF && !SeenB && seen){
        seen = false;
        rotate_right();
        return;
      }
    }
  }

  if(button3Pressed){

    if(starter){
      forward();
      starter = false;
      return;
      }
  
    bool LineF = Linia_PP() < 6200 || Linia_LP() < 6200;
   
    if(LineF && !starter){
      rotate_right();
      delay(450);
      starter = true;
      return;
    }
  }
}
