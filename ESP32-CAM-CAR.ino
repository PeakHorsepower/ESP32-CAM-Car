//Things to do
  //Learn HTML and Javascript (Completed)
      //Designing webpages (movement buttons, video streaming etc.) (Completed)
      //Sending and recieving requests to/from ESP32-CAM (Completed)
  //Understand the unknown parts of this program (90% Completed) (Still dont know what some libraries and code do)
  //First use home wifi to control robot, but shift to using ESP32-CAM as an access point (Can be used anywhere)
  //Modify program
    //Controls for 4 motors (Completed as of 07/22/2025)
    //Controls for camera pan/tilt servos (Completed as of 07/31/2025)
  //The list of things to do are in the journal, but I'll put here that I need to figure out how to add the images
  // from the esp32 file system to the webpage. (08/21/2025)


#include "esp_camera.h"
#include <WiFi.h>
#include "esp_timer.h"
#include "img_converters.h"
#include "Arduino.h"             //Unsure what this does
#include "fb_gfx.h"              //Unsure what this does
#include "soc/soc.h"             // disable brownout problems
#include "soc/rtc_cntl_reg.h"    // disable brownout problems
#include "esp_http_server.h"     //Handles webpage operations
#include <ESP32Servo.h>
#include "LittleFS.h"            //ESP32 Filesystem library



// Replace with your network credentials
//const char* ssid = "Verizon_XQ743S";
//const char* password = "tuna7-pleat-did";

//Soft AP credentails
const char* ssid = "ESP32-CAM";
const char* password = "123456789";

//ESP32 CAM soft AP address
//"http://192.168.4.1/"

//Pan servo variables
static const int Ppin = 2;
Servo Pservo;

//Tilt servo variables
static const int Tpin = 1;
Servo Tservo;

#define PART_BOUNDARY "123456789000000000000987654321" //Used in the stream handler for the building the response

//This section defines the model for ESP32 CAM
#define CAMERA_MODEL_AI_THINKER

//#if defined(CAMERA_MODEL_AI_THINKER)
  #define PWDN_GPIO_NUM     32
  #define RESET_GPIO_NUM    -1
  #define XCLK_GPIO_NUM      0
  #define SIOD_GPIO_NUM     26
  #define SIOC_GPIO_NUM     27
  
  #define Y9_GPIO_NUM       35
  #define Y8_GPIO_NUM       34
  #define Y7_GPIO_NUM       39
  #define Y6_GPIO_NUM       36
  #define Y5_GPIO_NUM       21
  #define Y4_GPIO_NUM       19
  #define Y3_GPIO_NUM       18
  #define Y2_GPIO_NUM        5
  #define VSYNC_GPIO_NUM    25
  #define HREF_GPIO_NUM     23
  #define PCLK_GPIO_NUM     22

//Motor pin definitions 
#define MOTOR_1_PIN_1    14
#define MOTOR_1_PIN_2    15
#define MOTOR_2_PIN_1    3
#define MOTOR_2_PIN_2    1

#define MOTOR_3_PIN_1    14
#define MOTOR_3_PIN_2    15
#define MOTOR_4_PIN_1    3
#define MOTOR_4_PIN_2    1

//This section is needed for the video streaming from camera
static const char* _STREAM_CONTENT_TYPE = "multipart/x-mixed-replace;boundary=" PART_BOUNDARY;
static const char* _STREAM_BOUNDARY = "\r\n--" PART_BOUNDARY "\r\n";
static const char* _STREAM_PART = "Content-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n";

httpd_handle_t camera_httpd = NULL;
httpd_handle_t stream_httpd = NULL;

//This is the HTML section of the code for designing the webpage
static const char PROGMEM INDEX_HTML[] = R"rawliteral(
<html>
<head>
    <title>ESP32-CAM Robot</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
        body {
            font-family: Arial;
            text-align: center;
            margin: 0px auto;
            padding-top: 30px;
        }

        table {
            margin-left: auto;
            margin-right: auto;
            empty-cells: show;
        }

        td {
            padding: 8px;
        }

        .button {
            background-color: white;
            border: none;
            color: white;
            background-size: cover;
            text-decoration: none;
            display: inline-block;
            margin: 6px 3px;
            cursor: pointer;
            -webkit-touch-callout: none;
            -webkit-user-select: none;
            -khtml-user-select: none;
            -moz-user-select: none;
            -ms-user-select: none;
            user-select: none;
            -webkit-tap-highlight-color: rgba(0,0,0,0);
            stroke: #000080;
        }

        button:active {
            background: #e5e5e5;
            outline: none;
            -webkit-box-shadow: inset 0px 0px 5px #c1c1c1;
            -moz-box-shadow: inset 0px 0px 5px #c1c1c1;
            box-shadow: inset 0px 0px 5px #c1c1c1;
        }

        img {
            width: 30px;
            height: 30px;
        }

        .web {
            width: 500px;
            height: 300px;
        }

        .Pslider {
            width: 300px;
        }
        .Tslider {
            width: 300px
        }
    </style>
</head>
<body>
    <h1>ESP32-CAM Robot</h1>
    <img src="" id="photo" class="web">
    <table>
        <tr>
            <td><button class="button" onmousedown="toggleCheckbox('forward');" ontouchstart="toggleCheckbox('forward');" onmouseup="toggleCheckbox('stop');" ontouchend="toggleCheckbox('stop');">
                    <svg width ="48" height ="48" viewBox="0 0 256 256"><rect width="256" height="256" fill="none" />
                    <path d="M128,24A104,104,0,1,0,232,128,104.11,104.11,0,0,0,128,24Zm45.66,125.66a8,8,0,0,1-11.32,0L128,115.31,93.66,149.66a8,8,0,0,1-11.32-11.32l40-40a8,8,0,0,1,11.32,0l40,40A8,8,0,0,1,173.66,149.66Z" /></svg>
                </button></td>
            <td></td>
            <td></td>
            <td></td>
            <td></td>
            <td></td>
            <td></td>
            <td></td>
            <td></td>

        </tr>

        <tr>
            <td></td>
            <td></td>
            <td></td>
            <td></td>
            <td></td>
            <td></td>
            <td><button class="button" onmousedown="toggleCheckbox('left');" ontouchstart="toggleCheckbox('left');" onmouseup="toggleCheckbox('stop');" ontouchend="toggleCheckbox('stop');">
                    <svg width ="48" height ="48" viewBox="0 0 256 256"><rect width="256" height="256" fill="none" />
                    <path d="M128,24A104,104,0,1,0,232,128,104.11,104.11,0,0,0,128,24Zm21.66,138.34a8,8,0,0,1-11.32,11.32l-40-40a8,8,0,0,1,0-11.32l40-40a8,8,0,0,1,11.32,11.32L115.31,128Z" />
                 </svg>
                </button></td>
            <td></td>
            <td><button class="button" onmousedown="toggleCheckbox('right');" ontouchstart="toggleCheckbox('right');" onmouseup="toggleCheckbox('stop');" ontouchend="toggleCheckbox('stop');">
                    <svg width ="48" height ="48" viewBox="0 0 256 256"><rect width="256" height="256" fill="none" />
                    <path d="M128,24A104,104,0,1,0,232,128,104.11,104.11,0,0,0,128,24Zm29.66,109.66-40,40a8,8,0,0,1-11.32-11.32L140.69,128,106.34,93.66a8,8,0,0,1,11.32-11.32l40,40A8,8,0,0,1,157.66,133.66Z" /></svg>
                
                </button></td>
        </tr>
        <tr>
            <td><button class="button" onmousedown="toggleCheckbox('backward');" ontouchstart="toggleCheckbox('backward');" onmouseup="toggleCheckbox('stop');" ontouchend="toggleCheckbox('stop');">
                    <svg width ="48" height ="48" viewBox="0 0 256 256"><rect width="256" height="256" fill="none" />
                    <path d="M128,24A104,104,0,1,0,232,128,104.11,104.11,0,0,0,128,24Zm45.66,93.66-40,40a8,8,0,0,1-11.32,0l-40-40a8,8,0,0,1,11.32-11.32L128,140.69l34.34-34.35a8,8,0,0,1,11.32,11.32Z" /></svg>
                </button></td>
            <td></td>
            <td></td>
            <td></td>
            <td></td>
            <td></td>
            <td></td>
            <td></td>
            <td></td>
        </tr>
    </table>
    <p>Pan position: <span id="PanP"></span></p>
    <input type="range" min="1" max="180" class="Pslider" id="PanRange" onchange="Pservo(this.value)" />

    <p>Tilt position: <span id="TiltP"></span></p>
    <input type="range" min="1" max="180" class="Tslider" id="TiltRange" onchange="Tservo(this.value)" />


    <script>

        var Panslider = document.getElementById("PanRange");
        var PanPos = document.getElementById("PanP");
        PanPos.innerHTML = Panslider.value;
        Panslider.oninput = function () {
            Panslider.value = this.value;
            PanPos.innerHTML = this.value;

        }

        var Tiltslider = document.getElementById("TiltRange");
        var TiltPos = document.getElementById("TiltP");
        TiltPos.innerHTML = Tiltslider.value;
        Tiltslider.oninput = function () {
            Tiltslider.value = this.value;
            TiltPos.innerHTML = this.value;
        }

        function toggleCheckbox(x) {
            var xhr = new XMLHttpRequest();
            xhr.open("GET", "/action?go=" + x, true);
            xhr.send();
        }

        function Pservo(Ppos) {
            var xhr = new XMLHttpRequest();
            xhr.open("GET", "/Pslider?value=" + Ppos, true);
            xhr.send();
            }

        function Tservo(Tpos) {
            var xhr = new XMLHttpRequest();
            xhr.open("GET", "/Tslider?value=" + Tpos, true);
            xhr.send();
        }
        window.onload = document.getElementById("photo").src = window.location.href.slice(0, -1) + ":81/stream";

        window.oncontextmenu = function (event) {
        event.preventDefault();
        event.stopPropagation();
        return false;
};
    </script>
</body>
</html>
)rawliteral";


//http request handlers
//These handle the requests and also send responses (500, 400, or 200 etc.)
static esp_err_t index_handler(httpd_req_t *req){
  httpd_resp_set_type(req, "text/html");
  return httpd_resp_send(req, (const char *)INDEX_HTML, strlen(INDEX_HTML));
}
/* This was my previous attempt at getting the webpage images to work, but it doesn't at the moment
static esp_err_t image_handler(httpd_req_t *req){

   File Back = LittleFS.open("Back arrow.jpg", "r");
   const char* Backp = Back.read() ;

  httpd_resp_set_type(req, "image/jpeg");

  return httpd_resp_send(req, Backp , strlen(Backp));

}
*/

//Handles the streaming of video on webpage. I might be able to send the image files in a similar way?
static esp_err_t stream_handler(httpd_req_t *req){
  camera_fb_t * fb = NULL;
  esp_err_t res = ESP_OK;
  size_t _jpg_buf_len = 0;
  uint8_t * _jpg_buf = NULL;
  char * part_buf[64];

  res = httpd_resp_set_type(req, _STREAM_CONTENT_TYPE);
  if(res != ESP_OK){
    return res;
  }

  while(true){
    fb = esp_camera_fb_get();
    if (!fb) {
      Serial.println("Camera capture failed");
      res = ESP_FAIL;
    } else {
      if(fb->width > 400){
        if(fb->format != PIXFORMAT_JPEG){
          bool jpeg_converted = frame2jpg(fb, 80, &_jpg_buf, &_jpg_buf_len);
          esp_camera_fb_return(fb);
          fb = NULL;
          if(!jpeg_converted){
            Serial.println("JPEG compression failed");
            res = ESP_FAIL;
          }
        } else {
          _jpg_buf_len = fb->len;
          _jpg_buf = fb->buf;
        }
      }
    }
    if(res == ESP_OK){
      size_t hlen = snprintf((char *)part_buf, 64, _STREAM_PART, _jpg_buf_len);
      res = httpd_resp_send_chunk(req, (const char *)part_buf, hlen);
    }
    if(res == ESP_OK){
      res = httpd_resp_send_chunk(req, (const char *)_jpg_buf, _jpg_buf_len);
    }
    if(res == ESP_OK){
      res = httpd_resp_send_chunk(req, _STREAM_BOUNDARY, strlen(_STREAM_BOUNDARY));
    }
    if(fb){
      esp_camera_fb_return(fb);
      fb = NULL;
      _jpg_buf = NULL;
    } else if(_jpg_buf){
      free(_jpg_buf);
      _jpg_buf = NULL;
    }
    if(res != ESP_OK){
      break;
    }
    //Serial.printf("MJPG: %uB\n",(uint32_t)(_jpg_buf_len));
  }
  return res;
}

//Handles the movement of the robot with the instructions sent from the webpage buttons
static esp_err_t cmd_handler(httpd_req_t *req){
  char*  buf;
  size_t buf_len;
  char variable[32] = {0,};

  buf_len = httpd_req_get_url_query_len(req) + 1;
  if (buf_len > 1) {
    buf = (char*)malloc(buf_len);
    if(!buf){
      httpd_resp_send_500(req);
      return ESP_FAIL;
    }
    if (httpd_req_get_url_query_str(req, buf, buf_len) == ESP_OK) {
      if (httpd_query_key_value(buf, "go", variable, sizeof(variable)) == ESP_OK) {
      } else {
        free(buf);
        httpd_resp_send_404(req);
        return ESP_FAIL;
      }
    } else {
      free(buf);
      httpd_resp_send_404(req);
      return ESP_FAIL;
    }
    free(buf);
  } else {
    httpd_resp_send_404(req);
    return ESP_FAIL;
  }

  sensor_t * s = esp_camera_sensor_get(); //Not needed
  int res = 0;
  //if(true){Serial.print(variable);};

  //Compares recieved data with command
  //IN1 & IN 3 make the wheels spin backwards, while IN2 & IN4 make it spin forward
  // 15 - IN3 & 14 - IN4
  //  1 - IN1 & 3 - IN2

  if(!strcmp(variable, "forward")) {
    Serial.println("Forward");
    digitalWrite(MOTOR_1_PIN_1, 0); 
    digitalWrite(MOTOR_1_PIN_2, 1);
    digitalWrite(MOTOR_2_PIN_1, 0);
    digitalWrite(MOTOR_2_PIN_2, 1);
    digitalWrite(MOTOR_3_PIN_1, 0);
    digitalWrite(MOTOR_3_PIN_2, 1);
    digitalWrite(MOTOR_4_PIN_1, 0);
    digitalWrite(MOTOR_4_PIN_2, 1);
    
  }
  else if(!strcmp(variable, "left")) {
    Serial.println("Left");
    digitalWrite(MOTOR_1_PIN_1, 0);
    digitalWrite(MOTOR_1_PIN_2, 1);
    digitalWrite(MOTOR_2_PIN_1, 1);
    digitalWrite(MOTOR_2_PIN_2, 0);
    digitalWrite(MOTOR_3_PIN_1, 0);
    digitalWrite(MOTOR_3_PIN_2, 1);
    digitalWrite(MOTOR_4_PIN_1, 1);
    digitalWrite(MOTOR_4_PIN_2, 0);
  }
  else if(!strcmp(variable, "right")) {
    Serial.println("Right");
    digitalWrite(MOTOR_1_PIN_1, 1);
    digitalWrite(MOTOR_1_PIN_2, 0);
    digitalWrite(MOTOR_2_PIN_1, 0);
    digitalWrite(MOTOR_2_PIN_2, 1);
    digitalWrite(MOTOR_3_PIN_1, 1);
    digitalWrite(MOTOR_3_PIN_2, 0);
    digitalWrite(MOTOR_4_PIN_1, 0);
    digitalWrite(MOTOR_4_PIN_2, 1);
  }
  else if(!strcmp(variable, "backward")) {
    Serial.println("Backward");
    digitalWrite(MOTOR_1_PIN_1, 1);
    digitalWrite(MOTOR_1_PIN_2, 0);
    digitalWrite(MOTOR_2_PIN_1, 1);
    digitalWrite(MOTOR_2_PIN_2, 0);
    digitalWrite(MOTOR_3_PIN_1, 1);
    digitalWrite(MOTOR_3_PIN_2, 0);
    digitalWrite(MOTOR_4_PIN_1, 1);
    digitalWrite(MOTOR_4_PIN_2, 0);
  }
  else if(!strcmp(variable, "stop")) {
    Serial.println("Stop");
    digitalWrite(MOTOR_1_PIN_1, 0);
    digitalWrite(MOTOR_1_PIN_2, 0);
    digitalWrite(MOTOR_2_PIN_1, 0);
    digitalWrite(MOTOR_2_PIN_2, 0);
  }
  else {
    res = -1;
  }

  if(res){
    return httpd_resp_send_500(req);
  }

  httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*"); //Unsure what exactly this does
  return httpd_resp_send(req, NULL, 0);
}

//Handles Pan servo movement 
static esp_err_t Pvalue_handler(httpd_req_t *req){
  char*  buf;
  size_t buf_len;
  char variable[32] = {0,};

  buf_len = httpd_req_get_url_query_len(req) + 1;
  if (buf_len > 1) {
    buf = (char*)malloc(buf_len);
    if(!buf){
      httpd_resp_send_500(req);
      return ESP_FAIL;
    }
    if (httpd_req_get_url_query_str(req, buf, buf_len) == ESP_OK) {
      if (httpd_query_key_value(buf, "value", variable, sizeof(variable)) == ESP_OK) {
  
      } else {
        free(buf); 
        httpd_resp_send_404(req);
        return ESP_FAIL;
      }
    } else {
      free(buf); 
      httpd_resp_send_404(req);
      return ESP_FAIL;
    }
    free(buf);
  } else { 
    httpd_resp_send_404(req);
    return ESP_FAIL;
  }
  String str(variable);
  Serial.print("Pan pos:");
  Serial.println(str);

   Pservo.write(str.toInt());

  sensor_t * s = esp_camera_sensor_get();// Not needed
  int res = 0;

 /*
  else {
    res = -1;
  }
*/
  if(res){
    return httpd_resp_send_500(req);
  }

  httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
  return httpd_resp_send(req, NULL, 0);
}

//Handles tilt servo movement
static esp_err_t Tvalue_handler(httpd_req_t *req){
  char*  buf;
  size_t buf_len;
  char variable[32] = {0,};

  buf_len = httpd_req_get_url_query_len(req) + 1;
  if (buf_len > 1) {
    buf = (char*)malloc(buf_len);
    if(!buf){
      httpd_resp_send_500(req);
      return ESP_FAIL;
    }
    if (httpd_req_get_url_query_str(req, buf, buf_len) == ESP_OK) {
      if (httpd_query_key_value(buf, "value", variable, sizeof(variable)) == ESP_OK) {
  
      } else {
        free(buf); 
        httpd_resp_send_404(req);
        return ESP_FAIL;
      }
    } else {
      free(buf); 
      httpd_resp_send_404(req);
      return ESP_FAIL;
    }
    free(buf);
  } else { 
    httpd_resp_send_404(req);
    return ESP_FAIL;
  }
  String str(variable);
  Serial.print("Tilt pos:");
  Serial.println(str);

  Tservo.write(str.toInt());

  sensor_t * s = esp_camera_sensor_get(); //Not needed
  int res = 0;

 /*
  else {
    res = -1;
  }
*/
  if(res){
    return httpd_resp_send_500(req);
  }

  httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
  return httpd_resp_send(req, NULL, 0);
}


void startCameraServer(){
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();
  config.server_port = 80;
  httpd_uri_t index_uri = {
    .uri       = "/",
    .method    = HTTP_GET,
    .handler   = index_handler,
    .user_ctx  = NULL
  };

  httpd_uri_t cmd_uri = {
    .uri       = "/action",
    .method    = HTTP_GET,
    .handler   = cmd_handler,
    .user_ctx  = NULL
  };
  httpd_uri_t stream_uri = {
    .uri       = "/stream",
    .method    = HTTP_GET,
    .handler   = stream_handler,
    .user_ctx  = NULL
  };

   httpd_uri_t Pvalue_uri = {
    .uri       = "/Pslider",
    .method    = HTTP_GET,
    .handler   = Pvalue_handler,
    .user_ctx  = NULL
  };

   httpd_uri_t Tvalue_uri = {
    .uri       = "/Tslider",
    .method    = HTTP_GET,
    .handler   = Tvalue_handler,
    .user_ctx  = NULL
  };
/*
  httpd_uri_t image_uri = {
    .uri       = "/Back arrow.jpg",
    .method    = HTTP_GET,
    .handler   = image_handler,
    .user_ctx  = NULL
  };
*/

  if (httpd_start(&camera_httpd, &config) == ESP_OK) {
    httpd_register_uri_handler(camera_httpd, &index_uri);
    httpd_register_uri_handler(camera_httpd, &cmd_uri);
    httpd_register_uri_handler(camera_httpd, &Tvalue_uri);
    httpd_register_uri_handler(camera_httpd, &Pvalue_uri);
    //httpd_register_uri_handler(camera_httpd, &image_uri);

  }
  config.server_port += 1;
  config.ctrl_port += 1;
  if (httpd_start(&stream_httpd, &config) == ESP_OK) {
    httpd_register_uri_handler(stream_httpd, &stream_uri);
  }
}

void setup() {
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); //disable brownout detector
  
  pinMode(MOTOR_1_PIN_1, OUTPUT);
  pinMode(MOTOR_1_PIN_2, OUTPUT);
  pinMode(MOTOR_2_PIN_1, OUTPUT);
  pinMode(MOTOR_2_PIN_2, OUTPUT);

  
  //Serial.begin(115200);
  //Serial.setDebugOutput(false);
  Pservo.attach(13);
  Tservo.attach(12);

  //Camera settings
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG; 
  
  if(psramFound()){
    config.frame_size = FRAMESIZE_XGA;
    config.jpeg_quality = 10;
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }
  
  // Camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }


  // Wi-Fi connection
  /*WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  
  Serial.print("Camera Stream Ready! Go to: http://");
  Serial.println(WiFi.localIP());
  */

  //Sets the ESP32 CAM to host its own wifi
  WiFi.softAP(ssid,password);

  // Start streaming web server
  startCameraServer();
}

void loop() {
 //Nothing is needed here
}