#define BLINKER_PRINT Serial
#define BLINKER_MIOT_LIGHT
#define BLINKER_WIFI
#include <Blinker.h>
#include <Adafruit_NeoPixel.h>

// key，在点灯 App 中获取
char auth[] = "57844a09ccf9";
//  DIN PIN (GPIO15, D8)
#define PIN 15
// 灯带 LED 数量
#define NUMPIXELS 22  
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);

// 新建组件对象
BlinkerRGB RGB1("RGB");

// RGB和亮度
int LED_R=0,LED_G=0,LED_B=0,LED_Bright=180;
bool WIFI_Status = true;
void smartConfig()//配网函数
{
  WiFi.mode(WIFI_STA);
  Serial.println("\r\nWait for Smartconfig...");
  // 等待手机端发出的用户名与密码
  WiFi.beginSmartConfig();
  while (1)
  {
    Serial.print(".");
    digitalWrite(LED_BUILTIN, HIGH);  
    delay(1000);                      
    digitalWrite(LED_BUILTIN, LOW);    
    delay(1000);
    // 退出等待
    if (WiFi.smartConfigDone())
    {
      Serial.println("SmartConfig Success");
      Serial.printf("SSID:%s\r\n", WiFi.SSID().c_str());
      Serial.printf("PSW:%s\r\n", WiFi.psk().c_str());
      break;
    }
  }
}
void WIFI_Set()//
{
    //Serial.println("\r\n正在连接");
    int count = 0;
    while(WiFi.status()!=WL_CONNECTED)
    {
        if(WIFI_Status)
        {
            Serial.print(".");
            digitalWrite(LED_BUILTIN, HIGH);  
            delay(500);                       
            digitalWrite(LED_BUILTIN, LOW);    
            delay(500);                 
            count++;
            if(count>=5)//5s
            {
                WIFI_Status = false;
                Serial.println("WiFi连接失败，请用手机进行配网"); 
            }
        }
        else
        {
            // 微信智能配网
            smartConfig();
        }
     }  
    /* Serial.println("连接成功");  
     Serial.print("IP:");
     Serial.println(WiFi.localIP());*/
}

void SET_RGB(int R,int G,int B,int bright)
{
    // 把灯条变色
    for (uint16_t i = 0; i < NUMPIXELS; i++)
    {
        pixels.setPixelColor(i,R,G,B);
    }
    // 设置亮度
    pixels.setBrightness(bright);
    // 送出显示
    pixels.show();
}

//APP RGB颜色设置回调
void rgb1_callback(uint8_t r_value, uint8_t g_value, 
                    uint8_t b_value, uint8_t bright_value)
{
    
    //digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
    BLINKER_LOG("R value: ", r_value);
    BLINKER_LOG("G value: ", g_value);
    BLINKER_LOG("B value: ", b_value);
    BLINKER_LOG("Rrightness value: ", bright_value);
    LED_Bright = bright_value;
    SET_RGB(r_value,g_value,b_value,LED_Bright);
}

void setup() {
    // 初始化串口
    Serial.begin(115200);
    // WS2812初始化
    pixels.begin();
    pixels.show();
    pinMode(LED_BUILTIN, OUTPUT);
    #if defined(BLINKER_PRINT)
        BLINKER_DEBUG.stream(BLINKER_PRINT);
    #endif

    WIFI_Set();
    // 初始化blinker
    Blinker.begin(auth, WiFi.SSID().c_str(), WiFi.psk().c_str());
    // 注册调节颜色的回调函数
    RGB1.attach(rgb1_callback);
}

void loop() {
    Blinker.run();
}
