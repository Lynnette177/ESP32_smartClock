#pragma once

#define verloc String("3.1.0") // 固件版本标志
#define CONTROL_BUTTON 38
#define AI_BUTTON 39 // 按钮pin规定

#include <string.h>
#include <WiFi.h>
#include <esp_ping.h>
#include <esp_wifi.h>
#include <HTTPClient.h>
#include <TimeLib.h>
#include <NTPClient.h>
#include <Arduino.h>
#include <WiFiClientSecure.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include <U8g2lib.h>
#include "mbedtls/aes.h"
#include "mbedtls/base64.h"
#include "mbedtls/md.h"
#include <ArduinoJson.h>
#include "weather.h"
#include "schedule_task.h"
#include "driver/i2s.h"
#include "i2s_api.h"
#include "icons.h"
#include <arduinoFFT.h>
#include <Preferences.h>
#include "wav.h"
#include <Update.h>
#include "Freenove_WS2812_Lib_for_ESP32.h"

extern bool push_to_wxbot(char *message, int num);

Preferences preferences; // 用来存储动态的设置
#define PREFERENCEKEY "globalConfig"

// 以下是全局变量定义
#define LEDS_COUNT 1
#define LEDS_PIN 48 // 48号脚默认为板载RGB
#define LED_CHANNEL 0
Freenove_ESP32_WS2812 LED = Freenove_ESP32_WS2812(LEDS_COUNT, LEDS_PIN, LED_CHANNEL, TYPE_GRB); // 初始化LED灯

// 用来存储时间戳和字符串格式时间的全局变量
String thetimenow;
time_t nowtimet;

String connected_WiFi; // 用来存储当前连接的Wi-Fi ssid

bool IS_IN_SCHOOL = false; // 判断是否在北邮校内

// 天气函数尽量不要用String做返回值会有大问题 内存重复释放的大问题

const char *wxbot_url_inhome = "http://192.168.2.101:3001/webhook/msg/v2?token=Lynnette";
const char *wxbot_url = "YOUR_WECHAT_BOT_URL";

char province_id[10] = {};      // 获取天气中省份ID用
char area_id[5] = {};           // 获取天气中城市ID用
char county_id[5] = {};         // 获取天气中县城ID用
String longitude;               // 经度 用来获取天气趋势
String latitude;                // 纬度 用来获取天气趋势
bool offline_skip_init = false; // 根据设置和连接状况改变在线/离线模式
bool do_check_update = false;   // 检测用户是否需要进行更新，如果需要，那么会在全部初始化完成之后进行检查更新。将在setup一开始进行赋值，并在render中显示
// 以下是定时执行命令
#define WEATHER_COMMAND "1"
#define ONLINE_STATUS_COMMAND "2"
#define REBOOT_COMMAND "3"

class its_time_to_do
{
private:
  int w_day;
  int hour;
  int minutes;
  bool excuted_task;
  char command[90];
  bool enabled;

public:
  its_time_to_do(int w_day = -1, int hour = -1, int minutes = -1, char *command_in = (char *)"", bool enabled = false)
      : w_day(w_day), hour(hour), minutes(minutes), excuted_task(false), enabled(enabled)
  {
    strcpy(command, command_in);
  }

  bool if_should_do(time_t time_now)
  {
    if (!enabled)
      return false;

    struct tm *timeinfo;
    timeinfo = localtime(&time_now);
    // int year = timeinfo->tm_year + 1900;
    // int month = timeinfo->tm_mon + 1;
    int wdaynow = timeinfo->tm_wday;
    wdaynow = wdaynow == 0 ? 7 : wdaynow; // 周天视为7
    // int daynow = timeinfo->tm_mday;
    int hournow = timeinfo->tm_hour;
    int minutenow = timeinfo->tm_min;
    if ((wdaynow == w_day || w_day < 0) && (hour == hournow || hour < 0) && (minutes == minutenow || minutes < 0))
    {
      if (!excuted_task)
      {
        excuted_task = true;
        return true;
      }
    }
    else
    {
      // 当时间条件不满足时才重置任务标志
      excuted_task = false;
    }
    return false;
  }

  void excute()
  {
    if (String(command) == String(REBOOT_COMMAND))
      esp_restart();
    else if (String(command) == String(WEATHER_COMMAND))
    {
      char weather_str[300] = {};
      get_weather(province_id, area_id, county_id, nowtimet, weather_str);
      push_to_wxbot(weather_str, 0);
    }
    else if (String(command) == String(ONLINE_STATUS_COMMAND))
      push_to_wxbot((char *)"时钟正常在线", 0);
  }
  void set_do_time(int wday_in, int hour_in, int minute_in)
  {
    this->w_day = wday_in;
    this->hour = hour_in;
    this->minutes = minute_in;
  }
  int getWDay() { return w_day; }
  int getHour() { return hour; }
  int getMinutes() { return minutes; }
  String getCommand() { return String(command); }
  bool isEnabled() { return enabled; }

  void setCommand(String cmd) { strcpy(command, cmd.c_str()); }
  void setEnabled(bool en) { enabled = en; }
};

struct configStruct
{
  char wifi_ssid1[50];
  char wifi_pass1[50];
  char wifi_ssid2[50];
  char wifi_pass2[50];
  char wifi_ssid3[50];
  char wifi_pass3[50];

  bool auto_locate;
  char ProvinceName[50];
  char CityName[50];
  char CountyName[50];

  char bupt_jw_account[128];
  char bupt_jw_password[128];

  char wechat_alias[100];

  char pushoverToken[100];
  char pushoverUserkey[100];
  char pushoverDevicename[100];

  char tencent_api_key[256];
  bool offline_mode;
  its_time_to_do schedule[10];
  bool schedule_enabled[10];
  int schedule_count;
};
configStruct globalConfig = {0};

String my_ip_address = "";                                   // 将从天气调用腾讯api时获取公网IP
String pi4_server_address = "http://YOUR_SERVER_URL:51156"; // 树莓派4的地址，用来更新和chatgpt服务 以及币安API
const unsigned char aeskey[] = "qzkj1kjghd=876&*";           // 这是北邮登录时加密用的AES密钥
const char *PUSHOVER_URL = "https://api.pushover.net/1/messages.json";
// int fail_count = 0;       // 错误次数。如果多次发生错误证明用户正在尝试使用企业微信，则进行一次休眠。
// 已弃用。因为每轮时钟循环时间较长，不再需要统计错误次数进行休眠。

int grade_num_pushed = 0; // 已经推送过的成绩个数

TaskHandle_t time_thread_handle = NULL; // 时间处理线程的句柄

TaskHandle_t render_circle_thread = NULL;      // 加载阶段显示线程句柄
TaskHandle_t spectrogram_thread_handle = NULL; // 音乐频谱句柄

// TaskHandle_t schedule_clock_thread_handle = NULL; // 日程时钟主循环句柄
// TaskHandle_t scollrender_handle = NULL;           // 绘制小学期的成绩的句柄
// TaskHandle_t scollrender_class_handle = NULL;     // 绘制课程的句柄

int loading_mode = 0;                       // 加载阶段不同状态的标识符
void *load_taskParams[1] = {&loading_mode}; // 给启动提示动画的参数

//U8G2_SSD1306_128X64_NONAME_F_4W_SW_SPI u8g2(U8G2_R0, /* clock=*/12, /* data=*/11, /* cs=*/47, /* dc=*/16, /* reset=*/15);
U8G2_SSD1306_128X64_NONAME_F_4W_HW_SPI u8g2(U8G2_R0, 47, 16, 15);


WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "ntp6.aliyun.com", 60 * 60 * 8, 30 * 60 * 1000);
extern void render_loading(void *pvParameters);
// 启动加载动画


void print_mac_by_bssi(const uint8_t *bssid){
  // 格式化并打印MAC地址
  char macAddr[18]; // 格式化后的MAC地址
  snprintf(macAddr, sizeof(macAddr), "%02X:%02X:%02X:%02X:%02X:%02X",bssid[0], bssid[1], bssid[2], bssid[3], bssid[4], bssid[5]);
  Serial.print("MAC: ");
  Serial.println(macAddr);
}

bool connectToWiFi(const char *ssid, const char *password,const uint8_t* bssid,int32_t channel)
{
  Serial.print("Connecting to ");
  Serial.println(ssid);
  // WiFi.mode(WIFI_STA);
  // uint8_t newMACAddress[] = {0xAA, 0xB8, 0xCC, 0x11, 0x22, 0x38}; //  测试校园网：设置新的MAC地址，
  // esp_wifi_set_mac(WIFI_IF_STA, &newMACAddress[0]);
  WiFi.begin(ssid, password, channel, bssid);
  // Serial.println(WiFi.macAddress());
  unsigned long startTime = millis();

  while (WiFi.status() != WL_CONNECTED)
  {
    if (millis() - startTime > 15000)
    { // 10秒超时
      Serial.println("Connection timeout");
      return false;
    }
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  return true;
}

bool scan_and_connect_WiFi(int *mode)
{
  int n = WiFi.scanNetworks(false, false, false, 100);
  Serial.println("Scan done");

  if (n == 0)
  {
    Serial.println("No networks found");
  }
  else
  {
    Serial.print(n);
    Serial.println(" networks found");
    for (int i = 0; i < n; ++i)
    {
      String ssid = WiFi.SSID(i);
      Serial.print(i + 1);
      Serial.print(": ");
      Serial.print(ssid);
      Serial.print(" (");
      Serial.print(WiFi.RSSI(i));
      Serial.print(")");
      //print_mac_by_bssi(WiFi.BSSID(i));
    }
    Serial.print("\n");
    int max_rssi_index = -1;
    int32_t max_rssi = -1000;
    for (int i = 0; i < n; ++i)
    {
      String ssid = WiFi.SSID(i);
      int32_t rssi = WiFi.RSSI(i);

      if (ssid.equals(globalConfig.wifi_ssid1) || ssid.equals(globalConfig.wifi_ssid2) || ssid.equals(globalConfig.wifi_ssid3))
      {
        if (rssi > max_rssi)
        {
          max_rssi_index = i;
          max_rssi = rssi;
        }
      }
    }
    Serial.printf("Connecting to: %d\n", max_rssi_index + 1);
    if (max_rssi_index != -1)
    {
      *mode = 9;
      connected_WiFi = WiFi.SSID(max_rssi_index);
      const uint8_t* bssid = WiFi.BSSID(max_rssi_index);
      int32_t channel = WiFi.channel(max_rssi_index); // 获取信道
      //print_mac_by_bssi(bssid);

      if (connected_WiFi.equals(globalConfig.wifi_ssid1))
        return connectToWiFi(connected_WiFi.c_str(), globalConfig.wifi_pass1, bssid, channel);
      if (connected_WiFi.equals(globalConfig.wifi_ssid2))
        return connectToWiFi(connected_WiFi.c_str(), globalConfig.wifi_pass2, bssid, channel);
      if (connected_WiFi.equals(globalConfig.wifi_ssid3))
        return connectToWiFi(connected_WiFi.c_str(), globalConfig.wifi_pass3, bssid, channel);
    }
  }
  return false;
}

String urlDecode(String input)
{
  String decoded = "";
  char temp[] = "0x00";
  unsigned int i, len = input.length();
  for (i = 0; i < len; i++)
  {
    if (input[i] == '%')
    {
      temp[2] = input[i + 1];
      temp[3] = input[i + 2];
      decoded += (char)strtol(temp, NULL, 16);
      i += 2;
    }
    else if (input[i] == '+')
    {
      decoded += ' ';
    }
    else
    {
      decoded += input[i];
    }
  }
  return decoded;
}

void start_loading_animation_thread()
{
  xTaskCreatePinnedToCore(
      render_loading,
      "render_loading",
      4096,
      load_taskParams,
      1,
      &render_circle_thread,
      0);
}
// 停止加载动画
void stop_loading_animation_thread()
{
  loading_mode = -1;
  if (render_circle_thread != NULL)
  {
    while (eTaskGetState(render_circle_thread) != eDeleted)
    {
      delay(100); // vTaskDelete(render_circle_thread);
    }
    render_circle_thread = NULL;
  }
}
// 绘制一个一秒长度的线程切换动画
void render_a_thread_switch_animation()
{
  stop_loading_animation_thread(); // 避免还在运行
  loading_mode = 7;
  start_loading_animation_thread();
  delay(1000);
  stop_loading_animation_thread();
}

// U8G2_SSD1306_128X64_NONAME_F_SW_I2C u8g2(U8G2_R0, /* clock=*/ 12, /* data=*/ 11, /* reset=*/ 47);
void checkMemory()
{
  Serial.print("Free PSRAM: ");
  Serial.println(ESP.getFreePsram());
  Serial.printf("Free heap: %u\n", ESP.getFreeHeap());
  Serial.printf("Max allocable heap: %u\n", ESP.getMaxAllocHeap());
}
int getLineCount(const char *str)
{
  int count = 1;
  while (*str)
  {
    if (*str == '\n')
    {
      count++;
    }
    str++;
  }
  return count;
}
float get_gpa_by_score(int score)
{
  if (score < 60)
    return 0;
  if (score == 60)
    return 1.f;
  if (score == 100)
    return 4.f;
  float fscore = score;
  float delta = 3 * (100 - fscore) * (100 - fscore) / 1600;
  float rdelta = roundf(delta * 100) / 100;
  return 4.f - rdelta;
}

time_t getUpdatedEpochTime()
{
  timeClient.update();
  unsigned long getrighttime = timeClient.getEpochTime();
  while (!(getrighttime > 1000000))
  {
    timeClient.update();
    getrighttime = timeClient.getEpochTime();
    delay(100);
  }
  delay(1);
  // Serial.println("Got time!");
  return timeClient.getEpochTime();
}
String getFormattedDateTime(time_t now)
{
  struct tm *timeinfo;
  timeinfo = localtime(&now);
  // 获取年月日时分秒
  int year = timeinfo->tm_year + 1900;
  int month = timeinfo->tm_mon + 1;
  int day = timeinfo->tm_mday;
  int hour = timeinfo->tm_hour;
  int minute = timeinfo->tm_min;
  // 格式化为字符串
  char datetimeString[25];
  snprintf(datetimeString, sizeof(datetimeString), "%02d.%02d\n%02d:%02d", month, day, hour, minute);
  return String(datetimeString);
}
String getFormattedDateTime_For_clock_Chinese(time_t now)
{
  struct tm *timeinfo;
  timeinfo = localtime(&now);
  // 获取年月日时分秒
  int year = timeinfo->tm_year + 1900;
  int month = timeinfo->tm_mon + 1;
  int day = timeinfo->tm_mday;
  int hour = timeinfo->tm_hour;
  int minute = timeinfo->tm_min;
  int seconds = timeinfo->tm_sec;
  // 格式化为字符串
  char datetimeString[30];
  snprintf(datetimeString, sizeof(datetimeString), "%04d年%02d月%02d日;%02d:%02d:%02d", year, month, day, hour, minute, seconds);
  return String(datetimeString);
}
void time_thread(void *pvParameters)
{
  char weather_str[300] = {0};
  while (1)
  {
    nowtimet = getUpdatedEpochTime(); // 1719233700;
    thetimenow = getFormattedDateTime(nowtimet);
    vTaskDelay(1000 / portTICK_PERIOD_MS); // 延迟1秒
    if (!offline_skip_init)
    {
      for (its_time_to_do &a_task : globalConfig.schedule) // 注意这里的& 用引用 才能确保执行之后保存下来！
      {
        if (a_task.isEnabled() && a_task.if_should_do(nowtimet))
          a_task.excute();
      }
    }
  }
}

// 如果是错误，返回-1。如果是考试周，返回1，如果是普通周，返回0
int check_weektype_by_comma(String str)
{
  int firstComma = str.indexOf(',');
  if (firstComma == -1)
  {
    return -1;
  }
  int secondComma = str.indexOf(',', firstComma + 1);
  if (secondComma == -1)
  {
    return -1;
  }
  int thirdComma = str.indexOf(',', secondComma + 1);
  if (thirdComma == -1)
    return 1;
  return 0;
}
String parase_weekday_from_wday(int weekday)
{
  switch (weekday)
  {
  case 0:
    return "周日";
  case 1:
    return "周一";
  case 2:
    return "周二";
  case 3:
    return "周三";
  case 4:
    return "周四";
  case 5:
    return "周五";
  case 6:
    return "周六";
  default:
    return "周八";
  }
}
String get_all_sheduled_task()
{
  String return_str = "";
  int task_count = 0;
  int index = 0;
  for (its_time_to_do a_task : globalConfig.schedule)
  {
    if (a_task.isEnabled())
    {
      return_str += "\n任务" + String(index + 1) + "已经启用：\n";
      int weekday = a_task.getWDay();
      int hour = a_task.getHour();
      int minute = a_task.getMinutes();
      if (weekday < 0)
        return_str += "每天 ";
      else
        return_str += "每周" + parase_weekday_from_wday(weekday - 1);
      if (hour < 0)
        return_str += "每小时 ";
      else
        return_str += hour + String("点");
      return_str += minute + String("分\n执行任务 ");
      if (a_task.getCommand() == WEATHER_COMMAND)
        return_str += "推送天气\n";
      else if (a_task.getCommand() == REBOOT_COMMAND)
        return_str += "自动重启\n";
      else if (a_task.getCommand() == ONLINE_STATUS_COMMAND)
        return_str += "推送在线状态\n";
      else
        return_str += String("发送信息 ") + a_task.getCommand();
      index++;
    }
  }
  return_str = "总共有" + String(index) + String("个任务被启用\n") + return_str;
  return return_str;
}
class scroll_time_struct
{
public:
  char h1;
  char h2;
  char m1;
  char m2;
  char s1;
  char s2;
  scroll_time_struct(const char &h1, const char &h2, const char &m1,
                     const char &m2, const char &s1, const char &s2)
      : h1(h1), h2(h2), m1(m1), m2(m2), s1(s1), s2(s2) {}
  void show_time()
  {
    Serial.printf("%c%c:%c%c:%c%c\n", h1, h2, m1, m2, s1, s2);
  }
  String get_time_string()
  {
    char time_str[20];
    sprintf(time_str, "%c%c:%c%c:%c%c", h1, h2, m1, m2, s1, s2);
    return String(time_str);
  }
  bool operator==(const scroll_time_struct &other) const
  {
    return h1 == other.h1 && h2 == other.h2 && m1 == other.m1 &&
           m2 == other.m2 && s1 == other.s1 && s2 == other.s2;
  }
  bool operator!=(const scroll_time_struct &other) const
  {
    return h1 != other.h1 || h2 != other.h2 || m1 != other.m1 ||
           m2 != other.m2 || s1 != other.s1 || s2 != other.s2;
  }
  scroll_time_struct &operator=(const scroll_time_struct &other)
  {
    if (this != &other)
    { // 防止自我赋值
      h1 = other.h1;
      h2 = other.h2;
      m1 = other.m1;
      m2 = other.m2;
      s1 = other.s1;
      s2 = other.s2;
    }
    return *this;
  }
};

// 该函数用来自动颜色变化模式的颜色生成
void updateColor(uint8_t &r, uint8_t &g, uint8_t &b)
{
  static int r_flag = 1, g_flag = -1, b_flag = 1; // 目标颜色
  static uint8_t rStep = 1, gStep = 1, bStep = 1; // 颜色变化步长

  if (r == 0 || r == 255)
    r_flag = -r_flag;
  if (g == 0 || g == 255)
    g_flag = -g_flag;
  if (b == 0 || b == 255)
    b_flag = -b_flag;

  r += r_flag * rStep;
  g += g_flag * gStep;
  b += b_flag * bStep;
}

void flash_light()
{
  static bool inited_led = false;
  int swift_color = 0;
  uint8_t r = 255, g = 255, b = 255; // 当前颜色

  if (!inited_led)
  {
    inited_led = true;
    LED.begin();
  }
  LED.setBrightness(255);
  u8g2.setFont(u8g2_font_wqy12_t_gb2312);
  while (1)
  {
    if (digitalRead(CONTROL_BUTTON) == LOW)
    {
      while (digitalRead(CONTROL_BUTTON) == LOW)
      {
      }
      break;
    }
    if (digitalRead(AI_BUTTON) == LOW)
    {
      while (digitalRead(AI_BUTTON) == LOW)
      {
      }
      swift_color = (swift_color + 1) % 3;
      if (swift_color == 1)
      {
        r = random(0, 256), g = random(0, 256), b = random(0, 256); // 当前颜色
      }
      else
      {
        r = 255;
        g = 255;
        b = 255;
      }
    }
    u8g2.firstPage();
    do
    {
      if (swift_color == 0)
      {
        u8g2.setCursor(40, 10);
        u8g2.print("手电筒");
      }
      else if (swift_color == 1)
      {
        u8g2.setCursor(20, 10);
        u8g2.print("手电筒 自动颜色");
      }
      else if (swift_color == 2)
      {
        u8g2.setCursor(20, 10);
        u8g2.print("手电筒 闪光模式");
      }
      u8g2.setCursor(10, 22);
      u8g2.print("轻点控制按钮以退出");

      // Serial.printf("rgb: %d %d %d\n",r,g,b);
      if (swift_color == 1)
        updateColor(r, g, b);
      if (swift_color == 2)
      {
        if (r == 255)
        {
          r = 0;
          g = 0;
          b = 0;
        }
        else
        {
          r = 255;
          g = 255;
          b = 255;
        }
        vTaskDelay(300 / portTICK_PERIOD_MS);
      }
      LED.setLedColor(0, r, g, b);
      u8g2.drawBox(15, 28, ((float)r / 255 * 90), 8);
      u8g2.drawBox(15, 40, ((float)g / 255 * 90), 8);
      u8g2.drawBox(15, 52, ((float)b / 255 * 90), 8);

      u8g2.setCursor(5, 36);
      u8g2.print("R");
      u8g2.setCursor(107, 36);
      u8g2.print(r);

      u8g2.setCursor(5, 46);
      u8g2.print("G");
      u8g2.setCursor(107, 46);
      u8g2.print(g);

      u8g2.setCursor(5, 58);
      u8g2.print("B");
      u8g2.setCursor(107, 58);
      u8g2.print(b);
    } while (u8g2.nextPage());
  }
  LED.setLedColor(0, 0, 0, 0);
  LED.setBrightness(0);
}

String calculateHMAC(const String &key, const String &message) {
    // 转换 String 到 C 风格的字符串
    const char *key_cstr = key.c_str();
    const char *message_cstr = message.c_str();
    
    uint8_t hmac_output[32]; // HMAC-SHA256 输出长度为 32 字节

    // 创建 mbedTLS 上下文
    mbedtls_md_context_t ctx;
    const mbedtls_md_info_t *info = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);

    mbedtls_md_init(&ctx);

    String result = "";
    if (mbedtls_md_setup(&ctx, info, 1) == 0) { // 1 表示启用 HMAC
        // 开始计算 HMAC
        mbedtls_md_hmac_starts(&ctx, (const uint8_t *)key_cstr, strlen(key_cstr));
        mbedtls_md_hmac_update(&ctx, (const uint8_t *)message_cstr, strlen(message_cstr));
        mbedtls_md_hmac_finish(&ctx, hmac_output);

        // 将 HMAC 输出转换为十六进制字符串
        char hex_output[65]; // 64 字节十六进制表示 + 终止符
        for (int i = 0; i < 32; i++) {
            sprintf(&hex_output[i * 2], "%02x", hmac_output[i]);
        }
        hex_output[64] = '\0'; // 确保字符串以空字符结束

        // 转换为 String 返回
        result = String(hex_output);
    }

    // 清理上下文
    mbedtls_md_free(&ctx);

    // 返回结果
    return result;
}
