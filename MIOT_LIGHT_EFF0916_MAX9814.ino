/* *****************************************************************

   Download latest Blinker library here:
   https://github.com/blinker-iot/blinker-library/archive/master.zip


   Blinker is a cross-hardware, cross-platform solution for the IoT.
   It provides APP, device and server support,
   and uses public cloud services for data transmission and storage.

  /* *****************************************************************

   Download latest Blinker library here:
   https://github.com/blinker-iot/blinker-library/archive/master.zip


   Blinker is a cross-hardware, cross-platform solution for the IoT.
   It provides APP, device and server support,
   and uses public cloud services for data transmission and storage.
   It can be used in smart home, data monitoring and other fields
   to help users build Internet of Things projects better and faster.

   Make sure installed 2.5.0 or later ESP8266/Arduino package,
   if use ESP8266 with Blinker.
   https://github.com/esp8266/Arduino/releases

   Make sure installed 1.0.2 or later ESP32/Arduino package,
   if use ESP32 with Blinker.
   https://github.com/espressif/arduino-esp32/releases

   Docs: https://doc.blinker.app/
         https://github.com/blinker-iot/blinker-doc/wiki

 * *****************************************************************

   Blinker 库下载地址:
   https://github.com/blinker-iot/blinker-library/archive/master.zip

   Blinker 是一套跨硬件、跨平台的物联网解决方案，提供APP端、设备端、
   服务器端支持，使用公有云服务进行数据传输存储。可用于智能家居、
   数据监测等领域，可以帮助用户更好更快地搭建物联网项目。

   如果使用 ESP8266 接入 Blinker,
   请确保安装了 2.5.0 或更新的 ESP8266/Arduino 支持包。
   https://github.com/esp8266/Arduino/releases

   如果使用 ESP32 接入 Blinker,
   请确保安装了 1.0.2 或更新的 ESP32/Arduino 支持包。
   https://github.com/espressif/arduino-esp32/releases

   文档: https://doc.blinker.app/
         https://github.com/blinker-iot/blinker-doc/wiki

 * *****************************************************************/




/*首先，谢谢您的支持！下面有几个小细节需要说明一下：
   1.注意60位（或较长的）的WS2812可能会造成开发板供电不足，不断重启，所以建议灯带额外供电。具体看开发板的支持情况，当然也可以用其它位数的灯带。
   2，开发板可以选择ESP32 ESP8266 ESP8266 01S ESP8266-12F模块（需焊接），注意使用的引脚定义。灯带WS2812 WS2812B，在按照SDK时ESP32与8266系列的有冲突。
   3.长灯带有分输入输出端，开发板和灯带需要在同一供电设备下，否则会对控制信号异常干扰。
   4.灯光效果目前的循环模式是，每循环一次效果检测一下blinker的新命令，所以会有blinker app按键不能及时同步的情况，同样语音助手也可能迟钝。blinker频次限制暂时做不到随时中断灯效下的循环。
   5.代码同时使用了fastled和adafruit_nexopixel库。大家可以看自己的喜好添加效果。
   6.某些模式需要配合调色盘和两个滑条使用。
   7.创建设备时选择阿里云，赋值auth，并在下面代码中更改。智能配网方法：烧录成功之后在blinker app 左上角选项--开发者--开发工具-ESPtouch/smartconfig进行配网。具体的流程：blinker app设置控件--关联智能家具--编译、上传代码到开发板
   8.相关教程参考点灯科技的官方开发文档与教程，也欢迎交流qq群1147147694。哔哩哔哩 https://space.bilibili.com/45978823 ，如果转载请勿修改此行，谢谢。

   更新日志20200916,呼吸灯效果，简化音乐效果（注意在效果程序中修改引脚），新版点灯科技库支持

*/
#define BLINKER_WIFI
#define BLINKER_MIOT_LIGHT
//#define BLINKER_ESP_SMARTCONFIG
#include <FastLED.h>

#include <Blinker.h>



int sig_max;
int st_si;
int st_count;


/////////////////////////////////////修改blinker设备的授权码、设备所在环境的wifi、wifi密码
char auth[] = "xxxxxx";
char ssid[] = "xxxxx";
char pswd[] = "xxxx";

////////////////////////////////////////五个开关按键 一个滑条 一个调色盘
BlinkerButton Button1("ButtonKey1");
BlinkerButton Button2("ButtonKey2");
BlinkerButton Button3("ButtonKey3");
BlinkerButton Button4("ButtonKey4");
BlinkerButton Button5("ButtonKey5");
BlinkerSlider Slider1("SliderKey");
BlinkerRGB RGBWS2812("RGBKey");


int openState = 0;/////////////////////////////灯效模式，默认关灯
int freq_flash = 25;/////////////////////////灯效节奏
uint8_t colorR, colorG, colorB, colorW;
bool wsState;
uint8_t wsMode = BLINKER_CMD_MIOT_DAY;
int brt_set = 100;

#include <Adafruit_NeoPixel.h>

#define PIN            15                             /////ws2812 DAT 接的引脚编号，注意开发板不同，请更改
#define NUMPIXELS      9                            ////ws2812 灯数
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);//////////////////////Adafruit_NeoPixel


CRGB leds[NUMPIXELS];//////////////FASTLED
//////////////////////////////////////颜色写入



void pixelShow()
{
  pixels.setBrightness(colorW);

  for (int i = 0; i < NUMPIXELS; i++) {
    pixels.setPixelColor(i, colorR, colorG, colorB);
  }
  pixels.show();
}
/////////////////////////////////////////频率读取


void tip_led()
{
  digitalWrite(2, LOW);
  digitalWrite(2, HIGH);
  delay(50);
  digitalWrite(2, LOW);
  Blinker.vibrate();
}

void slider1_callback(int32_t value)
{
  tip_led();
  Blinker.vibrate();///震动
  BLINKER_LOG("get slider value: ", value);
  freq_flash = value;
}


////////////////////////////////////////////////调色盘

void ws2812_callback(uint8_t r_value, uint8_t g_value, uint8_t b_value, uint8_t bright_value)
{
  tip_led();
  button_clear();
  BLINKER_LOG("R value: ", r_value);
  BLINKER_LOG("G value: ", g_value);
  BLINKER_LOG("B value: ", b_value);
  BLINKER_LOG("Rrightness value: ", bright_value);
  /////颜色和亮度赋值，但不写入
  colorR = r_value;
  colorG = g_value;
  colorB = b_value;
  colorW = bright_value;
  openState = 6;
  //Text1.print("灯光模式：" , "单颜色");
}



void button_clear()
{
  Button1.print("off");
  Button2.print("off");
  Button3.print("off");
  Button4.print("off");
  Button5.print("off");
  Button1.color("#008000");
  Button2.color("#008000");
  Button3.color("#008000");
  Button4.color("#008000");
  Button5.color("#008000");
}

///////////////////////////////////////////////////////////////////btn1
void button1_callback(const String & state)
{
  tip_led();
  BLINKER_LOG("get button state: ", state);

  if (state == BLINKER_CMD_ON) {
    BLINKER_LOG("日光模式开启");
    tip_led();
    button_clear();
    Button1.color("#DC143C");
    Button1.print("on");
    openState = 1;
  }
  else if (state == BLINKER_CMD_OFF) {
    BLINKER_LOG("日光模式关闭!");
    tip_led();
    button_clear();
    Button1.color("#008000");
    openState = 0;
  }

}




//////////////////////////////////////////////////////////btn2

void button2_callback(const String & state)
{
  tip_led();
  BLINKER_LOG("get button state: ", state);

  if (state == BLINKER_CMD_ON) {
    BLINKER_LOG("月光模式开启 music模式");
    tip_led();
    button_clear();
    Button2.print("on");
    Button2.color("#DC143C");
    openState = 2;
  }
  else if (state == BLINKER_CMD_OFF) {
    BLINKER_LOG("月光模式关闭!music模式");
    tip_led();
    button_clear();
    openState = 0;
  }

}


///////////////////////////////////////////////////////////////////3
void button3_callback(const String & state)
{
  tip_led();
  BLINKER_LOG("get button state: ", state);

  if (state == BLINKER_CMD_ON) {
    BLINKER_LOG("温馨模式开启");
    tip_led();
    button_clear();
    Button3.print("on");
    Button3.color("#DC143C");
    openState = 3;
    brt_set = colorW;
  }
  else if (state == BLINKER_CMD_OFF) {
    BLINKER_LOG("温馨模式关闭!");
    tip_led();
    button_clear();
    openState = 0;
  }

}

////////////////////////////////////////////////////////////////////////4

void button4_callback(const String & state)
{
  tip_led();
  BLINKER_LOG("get button state: ", state);

  if (state == BLINKER_CMD_ON) {
    BLINKER_LOG("电脑模式开启");
    tip_led();
    button_clear();
    Button4.print("on");
    Button4.color("#DC143C");
    openState = 4;
  }
  else if (state == BLINKER_CMD_OFF) {
    BLINKER_LOG("电脑模式关闭!");
    tip_led();
    button_clear();
    openState = 0;
  }

}

////////////////////////////////////////////////////////////////////////
void button5_callback(const String & state)
{
  tip_led();
  BLINKER_LOG("get button state: ", state);

  if (state == BLINKER_CMD_ON) {
    BLINKER_LOG("电视模式开启");
    tip_led();
    button_clear();
    Button5.print("on");
    Button5.color("#DC143C");
    openState = 5;
  }
  else if (state == BLINKER_CMD_OFF) {
    BLINKER_LOG("电视模式关闭!");
    tip_led();
    button_clear();
    openState = 0;

  }

}

void dataRead(const String & data)
{
  BLINKER_LOG("Blinker readString: ", data);

  Blinker.vibrate();

  uint32_t BlinkerTime = millis();

  Blinker.print("millis", BlinkerTime);
}

uint32_t getColor()
{

  uint32_t color = colorR << 16 | colorG << 8 | colorB;

  return color;
}

void miotPowerState(const String & state)
{
  BLINKER_LOG("need set power state: ", state);

  if (state == BLINKER_CMD_ON) {
    tip_led();
    BlinkerMIOT.powerState("on");
    BlinkerMIOT.print();

    wsState = true;

    if (colorW == 0) colorW = 255;
    openState = 6;
  }
  else if (state == BLINKER_CMD_OFF) {
    tip_led();

    BlinkerMIOT.powerState("off");
    BlinkerMIOT.print();

    wsState = false;
    openState = 0;
  }
}

void miotColor(int32_t color)
{
  BLINKER_LOG("need set color: ", color);

  colorR = color >> 16 & 0xFF;
  colorG = color >>  8 & 0xFF;
  colorB = color       & 0xFF;

  BLINKER_LOG("colorR: ", colorR, ", colorG: ", colorG, ", colorB: ", colorB);
  openState = 6;
  //pixelShow();

  BlinkerMIOT.color(color);
  BlinkerMIOT.print();
}

void miotMode(uint8_t mode)
{
  BLINKER_LOG("need set mode: ", mode);

  if (mode == BLINKER_CMD_MIOT_DAY) {
    // Your mode function
    button_clear();
    Button1.print("on");
    Button1.color("#DC143C");
    openState = 1;

  }
  else if (mode == BLINKER_CMD_MIOT_NIGHT) {
    // Your mode function
    button_clear();
    Button2.print("on");
    Button2.color("#DC143C");
    openState = 2;
  }
  else if (mode == BLINKER_CMD_MIOT_COLOR) {
    // Your mode function
    button_clear();
    openState = 6;

  }
  else if (mode == BLINKER_CMD_MIOT_WARMTH) {
    // Your mode function
    button_clear();
    Button3.print("on");
    Button3.color("#DC143C");
    openState = 3;
  }
  else if (mode == BLINKER_CMD_MIOT_TV) {
    // Your mode function
    button_clear();
    Button5.print("on");
    Button5.color("#DC143C");
    openState = 5;
  }
  else if (mode == BLINKER_CMD_MIOT_READING) {
    // Your mode function
  }
  else if (mode == BLINKER_CMD_MIOT_COMPUTER) {
    // Your mode function
    button_clear();
    Button4.print("on");
    Button4.color("#DC143C");
    openState = 4;
  }

  wsMode = mode;

  BlinkerMIOT.mode(mode);
  BlinkerMIOT.print();
}

void miotBright(const String & bright)
{
  BLINKER_LOG("need set brightness: ", bright);

  colorW = bright.toInt();

  BLINKER_LOG("now set brightness: ", colorW);

  pixelShow();

  BlinkerMIOT.brightness(colorW);
  BlinkerMIOT.print();
  openState = 6;
}

void miotColoTemp(int32_t colorTemp)
{
  BLINKER_LOG("need set colorTemperature: ", colorTemp);

  BlinkerMIOT.colorTemp(colorTemp);
  BlinkerMIOT.print();
}

void miotQuery(int32_t queryCode)
{
  BLINKER_LOG("MIOT Query codes: ", queryCode);

  switch (queryCode)
  {
    case BLINKER_CMD_QUERY_ALL_NUMBER :
      BLINKER_LOG("MIOT Query All");
      BlinkerMIOT.powerState(wsState ? "on" : "off");
      BlinkerMIOT.color(0);
      BlinkerMIOT.mode(0);
      BlinkerMIOT.colorTemp(1000);
      BlinkerMIOT.brightness(1);
      BlinkerMIOT.print();
      break;
    case BLINKER_CMD_QUERY_POWERSTATE_NUMBER :
      BLINKER_LOG("MIOT Query Power State");
      BlinkerMIOT.powerState(wsState ? "on" : "off");
      BlinkerMIOT.print();
      break;
    case BLINKER_CMD_QUERY_COLOR_NUMBER :
      BLINKER_LOG("MIOT Query Color");
      BlinkerMIOT.color(0);
      BlinkerMIOT.print();
      break;
    case BLINKER_CMD_QUERY_MODE_NUMBER :
      BLINKER_LOG("MIOT Query Mode");
      BlinkerMIOT.mode(0);
      BlinkerMIOT.print();
      break;
    case BLINKER_CMD_QUERY_COLORTEMP_NUMBER :
      BLINKER_LOG("MIOT Query ColorTemperature");
      BlinkerMIOT.colorTemp(1000);
      BlinkerMIOT.print();
      break;
    case BLINKER_CMD_QUERY_BRIGHTNESS_NUMBER :
      BLINKER_LOG("MIOT Query Brightness");
      BlinkerMIOT.brightness(1);
      BlinkerMIOT.print();
      break;
    default :
      BlinkerMIOT.powerState(wsState ? "on" : "off");
      BlinkerMIOT.color(0);
      BlinkerMIOT.mode(0);
      BlinkerMIOT.colorTemp(1000);
      BlinkerMIOT.brightness(1);
      BlinkerMIOT.print();
      break;
  }
}

/**************************灯光效果程序**************************
 * **************************************************
 * **************************************************/

////////////////////////////////////////////////彩虹灯
uint32_t Wheel(byte WheelPos) {
  WheelPos = 255 - WheelPos;
  if (WheelPos < 85) {
    return pixels.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  }
  if (WheelPos < 170) {
    WheelPos -= 85;
    return pixels.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
  WheelPos -= 170;
  return pixels.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
}

void rainbow(uint8_t wait) {
  uint16_t i, j;

  for (j = 0; j < 256; j++) {
    for (i = 0; i < pixels.numPixels(); i++) {
      pixels.setPixelColor(i, Wheel((i + j) & 255));
    }
    pixels.show();
    delay(freq_flash);
  }
}



////////////////////////////////////////////////////////////////
void colorWipe(uint32_t c, uint8_t wait) {
  for (uint16_t i = 0; i < pixels.numPixels(); i++) {
    pixels.setPixelColor(i, c);
    pixels.show();
    delay(wait);
  }
}

void colorScan()
{
  colorWipe(pixels.Color(255, 0, 0), freq_flash); // Red
  colorWipe(pixels.Color(0, 255, 0), freq_flash); // Green
  colorWipe(pixels.Color(0, 0, 255), freq_flash);
  colorWipe(pixels.Color(0, 255, 255), freq_flash);
  colorWipe(pixels.Color(255, 0, 255), freq_flash);
  colorWipe(pixels.Color(255, 0, 0), freq_flash);

}

///////////////////////////////////FASTLEDS的示例效果
void fadeall() {
  for (int i = 0; i < NUMPIXELS; i++) {
    leds[i].nscale8(250);
  }
}


void cylon() {
  static uint8_t hue = 0;

  // First slide the led in one direction
  for (int i = 0; i < NUMPIXELS; i++) {
    // Set the i'th led to red
    leds[i] = CHSV(hue++, 255, 255);
    // Show the leds
    FastLED.show();
    // now that we've shown the leds, reset the i'th led to black
    // leds[i] = CRGB::Black;
    fadeall();
    // Wait a little bit before we loop around and do it again
    delay(freq_flash);
  }

  // Now go in the other direction.
  for (int i = (NUMPIXELS) - 1; i >= 0; i--) {
    // Set the i'th led to red
    leds[i] = CHSV(hue++, 255, 255);
    // Show the leds
    FastLED.show();
    // now that we've shown the leds, reset the i'th led to black
    // leds[i] = CRGB::Black;
    fadeall();
    // Wait a little bit before we loop around and do it again
    delay(freq_flash);
  }
}


void breath()
{
  wsState = true;


  for (int brt = 1; brt < brt_set + 1; brt++) {
    colorW = brt;
    pixelShow();
    delay(freq_flash * 2);
  }
  for (int brt = brt_set; brt > 0 ; brt--) {
    colorW = brt;
    pixelShow();
    delay(freq_flash * 2);

  }

}
void musicWrite()//音乐（月光模式）
{
  int sig = analogRead(0);//out引脚


  if (sig > sig_max)
  {
    sig_max = sig;

  }
  int si = map(sig, int(sig_max / 2), sig_max, 0, NUMPIXELS);
  //Serial.println(si);
  pixels.clear();
  pixels.show();
  for (int i = 0; i < si + 1; i++)
  {
    pixels.setPixelColor(i, pixels.Color(colorR, colorG, colorB));

    pixels.show();   //

    delay(freq_flash); //
  }
}


void setup()
{
  Serial.begin(115200);
  BLINKER_DEBUG.stream(Serial);
  LEDS.addLeds<WS2812, PIN, RGB>(leds, NUMPIXELS);
  LEDS.setBrightness(84);
  Blinker.begin(auth, ssid, pswd);
  pixels.begin();
  Blinker.attachData(dataRead);

  pinMode(14, OUTPUT);
  digitalWrite(14, HIGH);
  pinMode(15, OUTPUT);
  digitalWrite(15, HIGH);
  BlinkerMIOT.attachPowerState(miotPowerState);
  BlinkerMIOT.attachColor(miotColor);
  BlinkerMIOT.attachMode(miotMode);
  BlinkerMIOT.attachBrightness(miotBright);
  BlinkerMIOT.attachColorTemperature(miotColoTemp);
  BlinkerMIOT.attachQuery(miotQuery);
  Slider1.attach(slider1_callback);
  RGBWS2812.attach(ws2812_callback);
  Button1.attach(button1_callback);
  Button2.attach(button2_callback);
  Button3.attach(button3_callback);
  Button4.attach(button4_callback);
  Button5.attach(button5_callback);
  pixels.setBrightness(0);
  button_clear();
}






void mode_1()
{

  cylon();
}

void mode_2()
{

  musicWrite();
}

void mode_3()
{
  breath();
}

void mode_4()
{
  rainbow(freq_flash);
  colorW = 50;
}


void mode_5()
{
  colorW = 50;
  colorScan();
}








void loop()
{

  Blinker.run();


  switch (openState)
  {
    case 0:
      wsState = false;
      openState = 0;
      colorR = 255;
      colorG = 255;
      colorB = 255;
      colorW = 0;
      pixelShow();
      break;
    case 1:
      mode_1();
      break;

    case 2:
      mode_2();
      break;
    case 3:
      mode_3();
      break;
    case 4:
      mode_4();
      break;
    case 5:
      mode_5();
      break;
    case 6:
      pixelShow();
      break;

    default:

      break;




  }



}
