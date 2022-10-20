//ESP WIFI Client Website Server RGB LED WS2816 Demo
//Works with ESP32 not with ESP8266
//For 8266 need correct libraries loaded, esp32 unloaded and specific function adjusted. Not done yet, todo first
//how to compile for esp8266



//#include <ESP8266WiFiGratuitous.h>
//#include <WiFiServerSecure.h>
#include <WiFiClientSecure.h>
//#include <ArduinoWiFiServer.h>
//#include <WiFiClientSecureBearSSL.h>
//#include <ESP8266WiFi.h>
//#include <ESP8266WiFiMulti.h>
#include <WiFiUdp.h>
//#include <ESP8266WiFiType.h>
//#include <CertStoreBearSSL.h>
//#include <ESP8266WiFiAP.h>
#include <WiFiClient.h>
//#include <BearSSLHelpers.h>
#include <WiFiServer.h>
//#include <ESP8266WiFiScan.h>
//#include <WiFiServerSecureBearSSL.h>
//#include <ESP8266WiFiGeneric.h>
//#include <ESP8266WiFiSTA.h>

#include <SoftwareSerial.h>
#include <WiFi.h>
#include <Adafruit_NeoPixel.h>

//LED PIns
#define PIN0            13
#define PIN1            12
#define PIN2            14
// How many NeoPixels are attached to the Arduino?
#define NUMPIXELS      90

//using adafruit pwm helper library
Adafruit_NeoPixel pixels0 = Adafruit_NeoPixel(NUMPIXELS, PIN0, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel pixels1 = Adafruit_NeoPixel(NUMPIXELS, PIN1, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel cam = Adafruit_NeoPixel(NUMPIXELS, PIN2, NEO_GRB + NEO_KHZ800);



int red_value = 0;
int green_value = 0;
int blue_value = 0;

int cam_red_value = 0;
int cam_green_value = 0;
int cam_blue_value = 0;



//Network ID

const char* ssid = "<NETWORK SSID>";
const char* password = "<NETWORK PASSWORD>";


// Set web server port number to 80
WiFiServer server(80);
IPAddress server_ip;

IPAddress static_ip(192,168,178,245);
IPAddress gateway(192,168,178,1);
IPAddress subnet(255,255,255,0);

IPAddress primaryDNS(8,8,8,8);
IPAddress secondaryDNS(4,4,4,4);










/*************************************************************************************************************
 * Define helper structs for data management, http processing and query parsing
 */

//A single Query key value pair inside a linked List with functionalities inside the Query struct
struct QueryNode{
  QueryNode* next;
  String key;
  String value;
};

//The Query, holds functions to check for and retrieve values from the htpp query
struct Query{
  QueryNode* root;
  int count;
  QueryNode* getQueryParamAtIndex(int index){
    QueryNode* curr = root;
    if (curr == NULL){
      return NULL;
    }
    int i=0;
    while(i<index){
      curr = curr->next;
      i++;
    }
    return curr;
  };
  bool containsKey(String key){
    QueryNode* curr = root;
    if (curr == NULL){
      return false;
    }
    do{
      if (curr->key == key){
        return true;
      }
      if (curr->next==NULL){
        return false;
      }
      curr = curr->next;
    }
    while(curr->next!=NULL);
    return false;
  };
  String getValue(String key){
    QueryNode* curr = root;
    if (curr == NULL){
      return "";
    }
    do{
      if (curr->key == key){
        return curr->value;
      }
      curr = curr->next;
    }
    while(curr->next!=NULL);
    return "";
  };
};

//The http header
struct Header{
  String methd;
  String http_version;
  String path;
  String useragent;
  String connection;
  String accepttype;
  Query* query;
};

/*************************************************************************************************************
 * Define global scope variables
 */
// Variable to store the HTTP request
String utf8_header;
long _delta_time=0;
long program_time = 0;


/*************************************************************************************************************
 * Start the program
 */
void setup() {
  Serial.begin(115200);

  initWifi();
  pixels0.begin(); // This initializes the NeoPixel library.
  pixels1.begin(); // This initializes the NeoPixel library.
  cam.begin();
  server.begin();

}

/*************************************************************************************************************
 * Run the program
 */
void loop() { 
  long curr_time = millis();
  long delta_time = curr_time -_delta_time;
  _delta_time = curr_time;
  program_time += delta_time;
 
  char prev_c;

  WiFiClient client = server.available();   // Listen for incoming clients
  if (client) {  
  Serial.println("New Client.");          // print a message out in the serial port
    Header header;
    utf8_header="";
    
    while (client.connected()){
      if (client.available()) {             // if there's bytes to read from the client,
        char c = client.read();             // read a byte, then
        utf8_header += c;
        if (c == '\n'){
          if ( utf8_header != "\r\n" ){ //CR LF (Carriage Return + Line Feed)//utf8_header.indexOf("\r\n")==0
            parseHTTP(&header, utf8_header);
            
            utf8_header="";
          } else {
            //http reading is done
            Serial.println("Reading Done...");
    
            QueryNode* qrynd = header.query->getQueryParamAtIndex(0);
            Serial.println(header.methd);
            Serial.println(header.path);
            Serial.println(header.http_version);

            if (header.methd.indexOf("GET")>=0 && header.path.indexOf("/")>=0){

              client.println("HTTP/1.1 200 OK");
              client.println("Connection: close");
              client.println();

             String page = getLandingPage();
             client.println(page);
             client.println();
              
              
              
              break;
            }
            
            if (header.methd.indexOf("POST")>=0 && header.path.indexOf("/api/rgb_background")>=0){
              if (header.query->containsKey("r") && header.query->containsKey("g") && header.query->containsKey("b")){
                client.println("HTTP/1.1 200 OK");
                client.println("Connection: close");
                client.println();
                int r = String(header.query->getValue("r")).toInt();
                int g = int(String(header.query->getValue("g")).toInt()*1);
                int b = int(String(header.query->getValue("b")).toInt()*1);

                red_value = r;
                green_value = g;
                blue_value = b;
                r = (int)(r);
                g = (int)(g);
                b = (int)(b);
                Serial.println("r; " + header.query->getValue("r"));
                Serial.println("g; " + header.query->getValue("g"));
                Serial.println("b; " + header.query->getValue("b"));
                
              for(int i=0;i<NUMPIXELS;i++)
                {
                  pixels0.setPixelColor(i,r<<16 | g<<8| b); // Moderately bright green color.
                  pixels1.setPixelColor(i,r<<16 | g<<8| b); // Moderately bright green color.
                }
                pixels0.show(); // This sends the updated pixel color to the hardware.
                pixels1.show(); // This sends the updated pixel color to the hardware.
              }
             break;
            }
            if (header.methd.indexOf("POST")>=0 && header.path.indexOf("/api/rgb_camera")>=0){
              if (header.query->containsKey("r") && header.query->containsKey("g") && header.query->containsKey("b")){
                client.println("HTTP/1.1 200 OK");
                client.println("Connection: close");
                client.println();
                int r = String(header.query->getValue("r")).toInt();
                int g = int(String(header.query->getValue("g")).toInt()*0.55);
                int b = int(String(header.query->getValue("b")).toInt()*0.33);

                cam_red_value = r;
                cam_green_value = g;
                cam_blue_value = b;
                
                Serial.println("r; " + header.query->getValue("r"));
                Serial.println("g; " + header.query->getValue("g"));
                Serial.println("b; " + header.query->getValue("b"));
                
              for(int i=0;i<NUMPIXELS;i++)
                {
                  cam.setPixelColor(i,r<<16 | g<<8| b); // Moderately bright green color.
                 
                }
                cam.show(); // This sends the updated pixel color to the hardware.
                
              }
             break;
            }
           
            client.println("HTTP/1.1 400 Bad Request");
            client.println("Connection: close");
            client.println();
            break;
            

            
            
          }
        }
      }
    }
     client.stop();
  }

  if (static_cast<int>(program_time / (1000*3600))==1){
    Serial.println("Rebooting");
    Serial.println(program_time);
    ESP.restart();
  }
}

/*************************************************************************************************************
 * Library functionalities
 */


//Initialize the Wifi Connection when used
void initWifi(){

  if (!WiFi.config(static_ip, gateway, subnet,primaryDNS, secondaryDNS)){
    Serial.println("STA Configuration failed!");
  }
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(50);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  server_ip = WiFi.localIP();
}

//Initialize the Ethernet Connection when used
void initEthernet(){
 
}


String getLandingPage(){
  String page = "";

  page = "<!DOCTYPE html>\n";
  page+= "<head>\n";
  page+= "<style>\n";
  page += ".bg_dark{background-color: #2d2d33;color: #e9e9f2;font-family: Helvetica, 'Trebuchet MS', Verdana, sans-serif;font-weight: 100;font-size: xx-large;font-size: 5em;}\n";
  page+= ".slider {-webkit-appearance: none;width: 100%;height: 50px;background: white;outline: none;opacity: 1;-webkit-transition: .2s;transition: opacity .2s;}\n";
  page+= ".slider::-webkit-slider-thumb {-webkit-appearance: none;appearance: none;width: 50px;height: 50px;background: rgb(90,100,122);cursor: pointer;opacity: 1;border: black;border-style: solid;border-radius: 5px;}\n";
  page+= "</style>\n</head>\n<body class='bg_dark'>\n<div style='text-align: center;padding-top: 2em;'>Ambi-LED</div>\n<div>\n";

  page +=  "<input id='slider01'  value='" + String(red_value) + "' type='range' min='0' max='255' class='slider' style='position: absolute;  top: 70%; background: white; border-radius: 5px; width: 50%;height: 24px; left: 25%;'>\n";
  page +=  "<input id='slider02'  value='" + String(green_value) + "' type='range' min='0' max='255' class='slider' style='position: absolute;  top: 75%; background: white; border-radius: 5px; width: 50%;height: 24px; left: 25%;'>\n";
  page +=  "<input id='slider03'  value='" + String(blue_value) + "' type='range' min='0' max='255' class='slider' style='position: absolute;  top: 80%; background: white; border-radius: 5px; width: 50%;height: 24px; left: 25%;'>\n";
  
  page += "<input id='btn01' type='button' value='W' style='position: absolute;top: 60%; border-style: solid;border-radius: 0px; width: 60px;height: 60px; left: 20%;background: rgb(255,220,200);'>";
  page += "<input id='btn02' type='button' value='W' style='position: absolute;top: 55%; border-style: solid;border-radius: 0px; width: 60px;height: 60px; left: 20%;background: rgb(210,180,160);'>";
  page += "<input id='btn03' type='button' value='W' style='position: absolute;top: 50%; border-style: solid;border-radius: 0px; width: 60px;height: 60px; left: 20%;background: rgb(170,140,115);'>";
  page += "<input id='btn04' type='button' value='W' style='position: absolute;top: 45%; border-style: solid;border-radius: 0px; width: 60px;height: 60px; left: 20%;background: rgb(130,115,90);'>";
  page += "<input id='btn05' type='button' value='W' style='position: absolute;top: 40%; border-style: solid;border-radius: 0px; width: 60px;height: 60px; left: 20%;background: rgb(100,90,75);'>";

  page += "<input id='btn11' type='button' value='W' style='position: absolute;top: 60%; border-style: solid;border-radius: 0px; width: 60px;height: 60px; left: 35%;background: rgb(0,255,255);'>";
  page += "<input id='btn12' type='button' value='W' style='position: absolute;top: 55%; border-style: solid;border-radius: 0px; width: 60px;height: 60px; left: 35%;background: rgb(0,210,210);'>";
  page += "<input id='btn13' type='button' value='W' style='position: absolute;top: 50%; border-style: solid;border-radius: 0px; width: 60px;height: 60px; left: 35%;background: rgb(0,173,130);'>";
  page += "<input id='btn14' type='button' value='W' style='position: absolute;top: 45%; border-style: solid;border-radius: 0px; width: 60px;height: 60px; left: 35%;background: rgb(0,75,75);'>";
  page += "<input id='btn15' type='button' value='W' style='position: absolute;top: 40%; border-style: solid;border-radius: 0px; width: 60px;height: 60px; left: 35%;background: rgb(0,25,25);'>";

  page += "<input id='btn21' type='button' value='W' style='position: absolute;top: 60%; border-style: solid;border-radius: 0px; width: 60px;height: 60px; left: 50%;background: rgb(255,0,255);'>";
  page += "<input id='btn22' type='button' value='W' style='position: absolute;top: 55%; border-style: solid;border-radius: 0px; width: 60px;height: 60px; left: 50%;background: rgb(210,0,210);'>";
  page += "<input id='btn23' type='button' value='W' style='position: absolute;top: 50%; border-style: solid;border-radius: 0px; width: 60px;height: 60px; left: 50%;background: rgb(173,0,173);'>";
  page += "<input id='btn24' type='button' value='W' style='position: absolute;top: 45%; border-style: solid;border-radius: 0px; width: 60px;height: 60px; left: 50%;background: rgb(75,0,75);'>";
  page += "<input id='btn25' type='button' value='W' style='position: absolute;top: 40%; border-style: solid;border-radius: 0px; width: 60px;height: 60px; left: 50%;background: rgb(25,0,25);'>";

  page += "<input id='btn31' type='button' value='W' style='position: absolute;top: 60%; border-style: solid;border-radius: 0px; width: 60px;height: 60px; left: 65%;background: rgb(255,120,0);'>";
  page += "<input id='btn32' type='button' value='W' style='position: absolute;top: 55%; border-style: solid;border-radius: 0px; width: 60px;height: 60px; left: 65%;background: rgb(210,100,0);'>";
  page += "<input id='btn33' type='button' value='W' style='position: absolute;top: 50%; border-style: solid;border-radius: 0px; width: 60px;height: 60px; left: 65%;background: rgb(173,75,0);'>";
  page += "<input id='btn34' type='button' value='W' style='position: absolute;top: 45%; border-style: solid;border-radius: 0px; width: 60px;height: 60px; left: 65%;background: rgb(75,25,0);'>";
  page += "<input id='btn35' type='button' value='W' style='position: absolute;top: 40%; border-style: solid;border-radius: 0px; width: 60px;height: 60px; left: 65%;background: rgb(25,10,0);'>";




  page += "<input id='btn00' type='button' value='B' style='position: absolute;top: 60%; border-style: solid;border-radius: 0px; width: 60px;height: 60px; left: 80%;background: rgb(0,0,0);'>";

  
  page += "</div>\n<script>\n";
  page += "var s01 = \"slider01\";\nvar s02 = \"slider02\";\nvar s03 = \"slider03\";\n";
  page += "var sC01 = \"sliderCam01\";\nvar sC02 = \"sliderCam02\";\nvar sC03 = \"sliderCam03\";\n";
  page += "var btnName01 = 'btn01';";
  page += "var btnName02 = 'btn02';";
  page += "var btnName03 = 'btn03';";
  page += "var btnName04 = 'btn04';";
  page += "var btnName05 = 'btn05';";

  page += "var btnName11 = 'btn11';";
  page += "var btnName12 = 'btn12';";
  page += "var btnName13 = 'btn13';";
  page += "var btnName14 = 'btn14';";
  page += "var btnName15 = 'btn15';";

  page += "var btnName21 = 'btn21';";
  page += "var btnName22 = 'btn22';";
  page += "var btnName23 = 'btn23';";
  page += "var btnName24 = 'btn24';";
  page += "var btnName25 = 'btn25';";

  page += "var btnName31 = 'btn31';";
  page += "var btnName32 = 'btn32';";
  page += "var btnName33 = 'btn33';";
  page += "var btnName34 = 'btn34';";
  page += "var btnName35 = 'btn35';";
  
  page += "var btnName00 = 'btn00';";
  page += "var sliderbg1 = document.getElementById(s01);\nvar sliderbg2 = document.getElementById(s02);\nvar sliderbg3 = document.getElementById(s03);\n";
  page += "var slidercam1 = document.getElementById(sC01);\nvar slidercam2 = document.getElementById(sC02);\nvar slidercam3 = document.getElementById(sC03);\n";
  page += "var button00 = document.getElementById(btnName00);";//background
  
  page += "var button01 = document.getElementById(btnName01);";
  page += "var button02 = document.getElementById(btnName02);";
  page += "var button03 = document.getElementById(btnName03);";
  page += "var button04 = document.getElementById(btnName04);";
  page += "var button05 = document.getElementById(btnName05);";

  page += "var button11 = document.getElementById(btnName11);";
  page += "var button12 = document.getElementById(btnName12);";
  page += "var button13 = document.getElementById(btnName13);";
  page += "var button14 = document.getElementById(btnName14);";
  page += "var button15 = document.getElementById(btnName15);";

  page += "var button21 = document.getElementById(btnName21);";
  page += "var button22 = document.getElementById(btnName22);";
  page += "var button23 = document.getElementById(btnName23);";
  page += "var button24 = document.getElementById(btnName24);";
  page += "var button25 = document.getElementById(btnName25);";

  page += "var button31 = document.getElementById(btnName31);";
  page += "var button32 = document.getElementById(btnName32);";
  page += "var button33 = document.getElementById(btnName33);";
  page += "var button34 = document.getElementById(btnName34);";
  page += "var button35 = document.getElementById(btnName35);";

  page += "const background_sliders = [" + String(red_value) + "," + String(green_value) + "," + String(blue_value) + "];\n";
  page += "const cam_sliders = [" + String(cam_red_value) + "," + String(cam_green_value/0.55) + "," + String(cam_blue_value/0.33) + "];\n";

  
  page += "const sendRGBBG = () => { var e = new XMLHttpRequest();\n";
  page += "e.open('POST', '/api/rgb_background?' + 'r=' +background_sliders[0] + '&g=' + background_sliders[1]+ '&b=' + background_sliders[2]);\n";
  page += "e.send();e.onreadystatechange = () => {if (e.status == XMLHttpRequest.DONE){}}};\n";

  page += "const sendRGBCam = () => { var e = new XMLHttpRequest();\n";
  page += "e.open('POST', '/api/rgb_camera?' + 'r=' +cam_sliders[0] + '&g=' + cam_sliders[1]+ '&b=' + cam_sliders[2]);\n";
  page += "e.send();e.onreadystatechange = () => {if (e.status == XMLHttpRequest.DONE){}}};\n";
  
  page += "var sendBG = false;\nconst sendBackground = () => {sendBG = true;}\n";
  page += "var sendCam = false;\nconst sendCamera= () => {sendCam = true;}\n";

  
  page += "setInterval(()=>{if (!sendBG)return;sendBG=false;sendRGBBG();}, 25 );\n";
  page += "setInterval(()=>{if (!sendCam)return;sendCam=false;sendRGBCam();}, 25);\n";

  
  page += "sliderbg1.oninput = () => {console.log(sliderbg1.value);background_sliders[0] =sliderbg1.value;sendBackground();};\n";
  page += "sliderbg2.oninput = () => {console.log(sliderbg2.value);background_sliders[1] =sliderbg2.value;sendBackground();};\n";
  page += "sliderbg3.oninput = () => {console.log(sliderbg3.value);background_sliders[2] =sliderbg3.value;sendBackground();};\n";

  //page += "slidercam1.oninput = () => {console.log(slidercam1.value);cam_sliders[0] =slidercam1.value;sendCamera();};\n";
  //page += "slidercam2.oninput = () => {console.log(slidercam2.value);cam_sliders[1] =slidercam2.value;sendCamera();};\n";
  //page += "slidercam3.oninput = () => {console.log(slidercam3.value);cam_sliders[2] =slidercam3.value;sendCamera();};\n";

  
  page += "button01.onclick = () => {background_sliders[0] = 255; background_sliders[1] = 100;  background_sliders[2] = 35; sliderbg1.value = background_sliders[0]; sliderbg2.value = background_sliders[1];sliderbg3.value = background_sliders[2];sendBackground();};";
  page += "button02.onclick = () => {background_sliders[0] = 128; background_sliders[1] = 50;   background_sliders[2] = 18; sliderbg1.value = background_sliders[0]; sliderbg2.value = background_sliders[1];sliderbg3.value = background_sliders[2];sendBackground();};";
  page += "button03.onclick = () => {background_sliders[0] = 64;  background_sliders[1] = 25;   background_sliders[2] = 9;  sliderbg1.value = background_sliders[0]; sliderbg2.value = background_sliders[1];sliderbg3.value = background_sliders[2];sendBackground();};";
  page += "button04.onclick = () => {background_sliders[0] = 32;  background_sliders[1] = 13;   background_sliders[2] = 5;  sliderbg1.value = background_sliders[0]; sliderbg2.value = background_sliders[1];sliderbg3.value = background_sliders[2];sendBackground();};";
  page += "button05.onclick = () => {background_sliders[0] = 16;  background_sliders[1] = 6;    background_sliders[2] = 2;  sliderbg1.value = background_sliders[0]; sliderbg2.value = background_sliders[1];sliderbg3.value = background_sliders[2];sendBackground();};"; 

  page += "button11.onclick = () => {background_sliders[0] = 0; background_sliders[1] = 255;  background_sliders[2] = 255; sliderbg1.value = background_sliders[0]; sliderbg2.value = background_sliders[1];sliderbg3.value = background_sliders[2];sendBackground();};";
  page += "button12.onclick = () => {background_sliders[0] = 0; background_sliders[1] = 128;   background_sliders[2] = 128; sliderbg1.value = background_sliders[0]; sliderbg2.value = background_sliders[1];sliderbg3.value = background_sliders[2];sendBackground();};";
  page += "button13.onclick = () => {background_sliders[0] = 0;  background_sliders[1] = 64;   background_sliders[2] = 64;  sliderbg1.value = background_sliders[0]; sliderbg2.value = background_sliders[1];sliderbg3.value = background_sliders[2];sendBackground();};";
  page += "button14.onclick = () => {background_sliders[0] = 0;  background_sliders[1] = 32;   background_sliders[2] = 32;  sliderbg1.value = background_sliders[0]; sliderbg2.value = background_sliders[1];sliderbg3.value = background_sliders[2];sendBackground();};";
  page += "button15.onclick = () => {background_sliders[0] = 0;  background_sliders[1] = 16;    background_sliders[2] = 16;  sliderbg1.value = background_sliders[0]; sliderbg2.value = background_sliders[1];sliderbg3.value = background_sliders[2];sendBackground();};"; 

  page += "button21.onclick = () => {background_sliders[0] = 255; background_sliders[1] = 0;  background_sliders[2] = 255; sliderbg1.value = background_sliders[0]; sliderbg2.value = background_sliders[1];sliderbg3.value = background_sliders[2];sendBackground();};";
  page += "button22.onclick = () => {background_sliders[0] = 128; background_sliders[1] = 0;   background_sliders[2] = 128; sliderbg1.value = background_sliders[0]; sliderbg2.value = background_sliders[1];sliderbg3.value = background_sliders[2];sendBackground();};";
  page += "button23.onclick = () => {background_sliders[0] = 64;  background_sliders[1] = 0;   background_sliders[2] = 64;  sliderbg1.value = background_sliders[0]; sliderbg2.value = background_sliders[1];sliderbg3.value = background_sliders[2];sendBackground();};";
  page += "button24.onclick = () => {background_sliders[0] = 32;  background_sliders[1] = 0;   background_sliders[2] = 32;  sliderbg1.value = background_sliders[0]; sliderbg2.value = background_sliders[1];sliderbg3.value = background_sliders[2];sendBackground();};";
  page += "button25.onclick = () => {background_sliders[0] = 16;  background_sliders[1] = 0;    background_sliders[2] = 16;  sliderbg1.value = background_sliders[0]; sliderbg2.value = background_sliders[1];sliderbg3.value = background_sliders[2];sendBackground();};"; 

  page += "button31.onclick = () => {background_sliders[0] = 255; background_sliders[1] = 50;  background_sliders[2] = 0; sliderbg1.value = background_sliders[0]; sliderbg2.value = background_sliders[1];sliderbg3.value = background_sliders[2];sendBackground();};";
  page += "button32.onclick = () => {background_sliders[0] = 128; background_sliders[1] = 25;   background_sliders[2] = 0; sliderbg1.value = background_sliders[0]; sliderbg2.value = background_sliders[1];sliderbg3.value = background_sliders[2];sendBackground();};";
  page += "button33.onclick = () => {background_sliders[0] = 64;  background_sliders[1] = 12;   background_sliders[2] = 0;  sliderbg1.value = background_sliders[0]; sliderbg2.value = background_sliders[1];sliderbg3.value = background_sliders[2];sendBackground();};";
  page += "button34.onclick = () => {background_sliders[0] = 32;  background_sliders[1] = 6;   background_sliders[2] = 0;  sliderbg1.value = background_sliders[0]; sliderbg2.value = background_sliders[1];sliderbg3.value = background_sliders[2];sendBackground();};";
  page += "button35.onclick = () => {background_sliders[0] = 16;  background_sliders[1] = 3;    background_sliders[2] = 0;  sliderbg1.value = background_sliders[0]; sliderbg2.value = background_sliders[1];sliderbg3.value = background_sliders[2];sendBackground();};"; 


  page += "button00.onclick = () => {background_sliders[0] = 0;background_sliders[1] = 0;background_sliders[2] = 0;sliderbg1.value = background_sliders[0]; sliderbg2.value = background_sliders[1];sliderbg3.value = background_sliders[2];sendBackground();};";
  
  page+= "</script>\n</body>\n";

  return page;
}

/**************************************************************************************************************
 * Functions to work with the HTTP Protocol
 */

void parseHTTP(Header* head, String utf8_h){
  char b[utf8_h.length()];
  utf8_h.toCharArray(b, utf8_h.length());
  if (utf8_h.indexOf("User-Agent:")>=0){
    strtok(b, " ");
    strtok(b, " ");
    utf8_h = "";
    for (char c:b){
      utf8_h += c;
    }
    head->useragent = utf8_h;
  }
  if (utf8_h.indexOf("Accept:")>=0){
    strtok(b, " ");
    strtok(b, " ");
    utf8_h = "";
    for (char c:b){
      utf8_h += c;
    }
    head->accepttype = utf8_h;
  }
  if (utf8_h.indexOf("Connection:")>=0){
    strtok(b, " ");
    strtok(b, " ");
    utf8_h = "";
    for (char c:b){
      utf8_h += c;
    }
    head->connection = utf8_h;
  }
  if (utf8_h.indexOf("HTTP")>=0){
    char c;
    String h="";
    int index=0;
    bool noquery = false;
    Query* query = new Query;
            head->query = query;   
            query->count = 0;
    //read header string, parse http method, http path and http version
    for (int i=0;i<utf8_h.length();i++){
      c = utf8_h.charAt(i);
      
      if (c == ' ' || c == '\n'){
        if (index == 0){
           String methd = String(h);
           head->methd = methd;
        }
        if (index == 1){
          int p=0;
          String path;
            for (int l=0;l<h.length();l++){
           
              char k = h.charAt(l);
              if(k=='?'){
                p=l+1;
                head->path = path;
                break;
              }
              if (k=='\n'){
                head->path = path;
                noquery = true;
                break;
              }
              if (l == h.length()-1){
                path+= k;//add last char
                head->path = path;
                noquery = true;
                break;
              }
              
              path+= k;
            } 
            
          //Parse all query arguments into linked list of key value pairs
          if (!noquery){
           
            
            QueryNode* node = new QueryNode;
            query->root = node;
            for (int l=p;l<h.length();l++){
              String key="";
              String value="";
              for (;l<h.length();l++){
                char o=h.charAt(l);
                if (o == '='){
                  l++;
                  break;
                }
                key+=o;
              }
              for (;l<h.length();l++){
                char o=h.charAt(l);
                if (o == '&'){
                  node->key = key;
                  node->value = value;
                  query->count++;
                  QueryNode* newNode = new QueryNode;
                  node->next = newNode;
                  node = newNode;
                  break;
                }
                if (l<h.length()-2 && o == '%' && h.charAt(l+1) == '2' && h.charAt(l+2)=='0'){
                  o = ' ';
                  l+=2;
                }
                value+=o;
                if (l == h.length()-1){
                  node->key = key;
                  node->value = value;
                }
              }
            }
          }
        }
        if (index == 2){
           String http_version = String(h);
           head->http_version = http_version;
        }
        index ++;
        h = "";
        continue;
      }
      h+=c;
    }
  }
}
