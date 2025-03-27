#include "includes.h"
#include "bupt_wifi_login.h"
#include "qiangzhi_encode.h"
#include "login_grade.h"
#include "render.h"
#include "get_location.h"
#include "web_config.h"
#include "ota.h"
#include "bitcoin.h"
#include "chatgpt.h"

void only_clock_thread(void *parameters); // 不在校的时候只渲染时钟的线程
void schedule_clock();
void looped_schedule_clock(void *pvParameters);                                                                             // 循环scheduleclock，用这个比较方便
void exam_week_get_grade_summerschool_and_vacation(char *token, int last_teaching_week, bool is_on_vacation_by_empty_week); // 声明一下这个是考试周、查成绩、小学期、放假时用的函数（特殊时期函数） 最后一个参数还用不到
void normal_week(char *token);
void btc_clock();
void setup()
{
  Serial.begin(9600);

  pinMode(CONTROL_BUTTON, INPUT_PULLUP);
  pinMode(AI_BUTTON, INPUT_PULLUP);
  u8g2.begin();
  // render_spectrogram();//频谱仪函数
  u8g2.enableUTF8Print();
  u8g2.setFontDirection(0);
  u8g2.setCursor(10, 10);
  start_loading_animation_thread();
  unsigned int start_pressing_boot = millis();
  while (millis() - start_pressing_boot < 1000)
  {
    if (digitalRead(CONTROL_BUTTON) == LOW)
      break;
    // if (digitalRead(AI_BUTTON) == LOW)
    //   do_check_update = true;
    do_check_update = true; // 不管是不是按下按钮 都要检查更新
  }
  // print_config();
  if (!loadStruct(PREFERENCEKEY) || digitalRead(CONTROL_BUTTON) == LOW)
  {
    stop_loading_animation_thread();
    loop_in_config();
  }
  offline_skip_init = globalConfig.offline_mode;
  i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
  i2s_set_pin(I2S_NUM_0, &i2s_mic_pins);
  i2s_start(I2S_NUM_0);
  if (!offline_skip_init)
  {
    loading_mode = 1;

    bool scan_and_connect_wifi = scan_and_connect_WiFi(&loading_mode);

    if (digitalRead(CONTROL_BUTTON) == LOW)
      offline_skip_init = true;
    if (!scan_and_connect_wifi)
      offline_skip_init = true;
  }
  if (!offline_skip_init)
  {
    if (strcmp(connected_WiFi.c_str(), "BUPT-portal") == 0)
    {
      loading_mode = 2;
      bupt_login(globalConfig.bupt_jw_account, globalConfig.bupt_jw_password);
    }
    if (WiFi.localIP().toString().substring(0, 3) == "10.")
      IS_IN_SCHOOL = true; // 如果是以10.开头的IP肯定是在北邮内网
    if (globalConfig.auto_locate)
    {
      loading_mode = 3;
      while (!get_location())
      {
        if (WiFi.status() != WL_CONNECTED)
        {
          Serial.println("Reconnecting to WiFi...");
          WiFi.disconnect();
          WiFi.reconnect();
        }
        delay(100);
      }
    }
    loading_mode = 4;
    xTaskCreatePinnedToCore(
        time_thread,
        "time_and_schedule_task",
        8192,
        time_thread_handle,
        1,
        NULL,
        1);
    while (!nowtimet)
    {
      if (WiFi.status() != WL_CONNECTED)
      {
        Serial.println("Reconnecting to WiFi...");
        WiFi.disconnect();
        WiFi.reconnect();
      }
    }
    // Serial.println(get_all_sheduled_task());
    push_to_wxbot((char *)("桌面时钟已上线，持续获取数据中...\n" + get_all_sheduled_task()).c_str());
    loading_mode = 11;
    get_province_id(globalConfig.ProvinceName, province_id);
    get_area_id_in_province(province_id, globalConfig.CityName, area_id);
    get_county_id_in_province_and_area(province_id, area_id, globalConfig.CountyName, county_id);
    if (!strlen(province_id) == 5 || !strlen(area_id) == 2 || !strlen(county_id) == 2 || strcmp(province_id, "null") == 0 || strcmp(area_id, "null") == 0 || strcmp(county_id, "null") == 0)
    {
      strcpy(province_id, "10101");
      strcpy(area_id, "00");
      strcpy(county_id, "07");
      strcpy(globalConfig.CountyName, "昌平");
      // 如果任何一个获取错误 直接把位置设置到昌平区去
    }

    // Serial.printf("%s %s %s %s %s %s\n", globalConfig.ProvinceName,globalConfig.CityName,globalConfig.CountyName,province_id, area_id, county_id);

    if (!globalConfig.auto_locate)
    {
      while (!get_lon_lat_using_weather_api(province_id, area_id, county_id, nowtimet, longitude, latitude))
      {
      };
      Serial.println(longitude);
      Serial.println(latitude);
    }
  }
  else
  {
    loading_mode = 8;
    delay(1000);
    // 进入离线 延迟一秒钟 让用户看见
  }
  if (do_check_update && !offline_skip_init)
  {
    loading_mode = 10;
    bool is_need_update = check_if_need_update();
    if (is_need_update)
    {
      stop_loading_animation_thread();
      bool update_success = check_update();
      while (!digitalRead(CONTROL_BUTTON) == LOW && !digitalRead(AI_BUTTON) == LOW)
      {
        vTaskDelay(1 / portTICK_PERIOD_MS);
      }
      while (digitalRead(CONTROL_BUTTON) == LOW || digitalRead(AI_BUTTON) == LOW)
      {
        vTaskDelay(1 / portTICK_PERIOD_MS);
      }
      if (update_success)
        esp_restart();
    }
  }
}

void loop()
{
  if (!offline_skip_init)
  {
    // delay(1000);
    loading_mode = 5;
  }
  // delay(1000);
  loading_mode = 6;
  // delay(1000);
  stop_loading_animation_thread();
  int mode = 0; // 0代表时钟，1代表频谱
  // int last_mode = mode;
  while (1)                           // 主循环
  {                                   // 进入菜单
    int ai_button_hit_time = 0;       // 这两个变量是用来标识一段时间内AI按钮被按了多少次的
    int ai_button_start_hit_time = 0; // 如果在2秒内按了4次直接启动手电筒
    int targetY = 15 * (mode%4);
    static int nowframeY = targetY;
    while (1)
    { // 这里是菜单循环
      if (offline_skip_init && mode == 0)
        mode = 3; // 离线模式不允许使用时钟
      if (offline_skip_init && mode == 1)
        mode = 3; // 离线模式不允许使用问答
      if (offline_skip_init && mode == 2)
        mode = 3; // 离线模式不允许币情监控
      targetY = 15 * (mode%4);
      u8g2.setFont(u8g2_font_wqy12_t_gb2312);
      u8g2.firstPage();
      do
      {
        if (mode / 4 == 0)//第一页菜单
        {
          if (mode == 0)  
          { // 课程时钟模式图标
            if (!IS_IN_SCHOOL)
              u8g2.drawXBMP(8, 8, 48, 48, gImage_weathericon);
            else
              u8g2.drawXBMP(8, 8, 48, 48, gImage_calender);
          }
          else if (mode == 1) // 音乐频谱模式图标
            u8g2.drawXBMP(8, 8, 48, 48, gImage_ailogo);
          else if (mode == 2) // 音乐频谱模式图标
            u8g2.drawXBMP(8, 8, 48, 48, gImage_btc);
          else if (mode == 3)
            u8g2.drawXBMP(8, 8, 48, 48, gImage_music_spec_icon);
          u8g2.setCursor(70, 13); // 绘制12个大小的字体
          if (IS_IN_SCHOOL)
            u8g2.print("课程时钟");
          else
            u8g2.print("天气时钟");
          if (offline_skip_init)
          {
            u8g2.drawLine(67, 7, 120, 7); // 禁止线1
          }
          u8g2.setCursor(70, 28);
          u8g2.print("问机器人");
          if (offline_skip_init)
          {
            u8g2.drawLine(67, 22, 120, 22); // 禁止线2
          }
          u8g2.setCursor(70, 43);
          u8g2.print("币情监控");
          if (offline_skip_init)
          {
            u8g2.drawLine(67, 37, 120, 22); // 禁止线3
          }
          u8g2.setCursor(70, 58);
          u8g2.print("音乐频谱");
        }
        else if (mode / 4 == 1){ //这里是第二页菜单
        if (mode == 4){
          u8g2.drawXBMP(8, 8, 48, 48, gImage_settings);
        }
          u8g2.setCursor(70, 13);
          u8g2.print("系统设置");
        }
        u8g2.drawRFrame(67, nowframeY, 54, 17, 3); // 要求w >= 2*(r+1);h >= 2*(r+1);否则未定义2*(r+1)
        if (offline_skip_init)
        {
          u8g2.setCursor(2, 62);
          u8g2.print("已离线");
        }
      } while (u8g2.nextPage());
      if (nowframeY != targetY)
      {
        float delta = targetY - nowframeY;
        delta = delta / 3 + (delta / abs(delta) / 2);
        if (fabs(delta) < 1)
          delta = 1 * (delta / abs(delta));
        nowframeY += (int)delta; // / abs(delta);
        // int delayTime = 10 + (17 - abs(delta)) * 5; // 先快后慢的延时
        // vTaskDelay(delayTime / portTICK_PERIOD_MS);
      }
      if (digitalRead(CONTROL_BUTTON) == LOW)
      {
        unsigned int start_pressing = millis();
        while (digitalRead(CONTROL_BUTTON) == LOW)
        {
          if (millis() - start_pressing > 1000) // 如果用户按住超过一秒，主动跳出去开始执行任务
            break;
          delay(1);
        }
        if (millis() - start_pressing < 1000) // 如果用户按按键的时间小于1秒，则认为是模式切换
        {
          mode = (mode + 1) % 5;
        }
        else
          break;
      }
      if (digitalRead(AI_BUTTON) == LOW)
      {
        while (digitalRead(AI_BUTTON) == LOW)
        {
        }
        if (ai_button_hit_time == 0)
          ai_button_start_hit_time = millis();
        ai_button_hit_time++;
        if (ai_button_hit_time >= 4 && millis() - ai_button_start_hit_time < 2000)
          break;
        else if (millis() - ai_button_start_hit_time >= 2000)
        {
          ai_button_hit_time = 0;
          ai_button_start_hit_time = 0;
        }
      }
    }

    if (ai_button_hit_time >= 4)
    {
      flash_light();
      ai_button_hit_time = 0;
      ai_button_start_hit_time = 0;
      continue;
    }

    // 退出菜单循环，在主循环中继续
    // 等待用户释放控制按钮
    wait_user_release_button(CONTROL_BUTTON);
    // 下方根据模式启动线程//函数
    if (mode == 0)
    {
      IS_IN_SCHOOL ? looped_schedule_clock(NULL) : only_clock_thread(NULL);
      // schedule_clock_thread_handle
    }
    else if (mode == 1)
    {
      gpt_mainloop();
    }
    else if (mode == 2)
    {
      btc_clock();
    }
    else if (mode == 3)
    {
      render_spectrogram(NULL);
      // spectrogram_thread_handle,
      // delay(5000);
    }
    if (mode == 4)
      loop_in_config();

    wait_user_release_button(CONTROL_BUTTON);

    // 只要用户还没按按键，就死等
    // while (!digitalRead(CONTROL_BUTTON) == LOW)
    // { // 只要用户没按下按键，就等待
    //   delay(10);
    // }
    // // 在这里是用户按下按键跳出循环后根据mode来进行线程的结束。
    // if (mode == 0) // 0是时钟线程
    // {
    //   stop_clock_related_thread();
    //   render_a_thread_switch_animation();
    // }
    // else if (mode == 2) // 2是频谱线程
    // {
    //   stop_spectagram_thread();
    //   render_a_thread_switch_animation();
    // }
  }
}

void only_clock_thread(void *parameters)
{                                      // 不在校的时候只渲染时钟的线程
  char *weather = new char[300];       // 存天气
  char *weather_trend = new char[300]; // 存天气趋势
  while (!digitalRead(CONTROL_BUTTON) == LOW)
  {
    u8g2.firstPage();
    u8g2.setFont(u8g2_font_wqy16_t_gb2312);
    do
    {
      u8g2.setCursor(40, 32);
      u8g2.print("更新中..");
    } while (u8g2.nextPage());
    bool weather_success = get_weather(province_id, area_id, county_id, nowtimet, weather);
    bool trend_success = get_weather_trend_str(nowtimet, weather_trend, latitude, longitude);
    if (!weather_success || !trend_success)
    {
      if (WiFi.status() != WL_CONNECTED)
      {
        Serial.println("Reconnecting to WiFi...");
        WiFi.disconnect();
        WiFi.reconnect();
      }
      continue; // 如果信息都获取不了了，那么就重新来一次，不然会显示出空的东西来
    }
    render_scroll_clock(weather, weather_trend, globalConfig.CountyName);
  }
  delete[] weather;
  delete[] weather_trend;
}
// 流程：查课表->所有课->是否有考试课(周数只有一个元素)
// 是->exam_week_get_grade_loop(下一场考试信息)
// 否->normal_week(char *token)
void looped_schedule_clock(void *parameters)
{
  u8g2.firstPage();
  u8g2.setFont(u8g2_font_wqy16_t_gb2312);
  do
  {
    u8g2.setCursor(22, 30);
    u8g2.print("黑入教务中..");
  } while (u8g2.nextPage());
  vTaskDelay(1000 / portTICK_PERIOD_MS);
  while (!digitalRead(CONTROL_BUTTON) == LOW)
  {
    schedule_clock();
  }
  // vTaskDelete(NULL);
}
void schedule_clock()
{
  char *based_password = new char[300];
  char *token = new char[1024];
  strcpy(token, "null");
  bool is_xiaoxueqi = false;
  bool is_examine_week = false;
  do_encrypt(aeskey, globalConfig.bupt_jw_password, based_password);
  // 获取教务AES加密后的密码
  Serial.println(based_password);
  bool success_get_token = false;
  bool success_query_schedule = false;
  bool is_on_vacation = false;
  int last_teaching_week = -1;
  // 以下一直尝试登录并且获取课表 获取课表的时候会判断是不是考试周和小学期
  // 如果是小学期 is_examine_week, is_xiaoxueqi都会是真
  // 如果是小学期，schedule会是小学期的内容，而且schedule只会在这改一次，后续进循环不会改变
  // 因此小学期整个周(包括周末)都会显示同样的schedule
  while ((token == "null" || last_teaching_week < 0 || (!success_get_token) || !(success_query_schedule)) && !digitalRead(CONTROL_BUTTON) == LOW)
  {
    // if (token == "null") Serial.println("Token is Null");
    // if (last_teaching_week < 0) Serial.println("Cant get teaching week");
    // if (!success_get_token) Serial.println("Cant get token 1");
    // if (!success_query_schedule) Serial.println("Cant query schedule 1");

    vTaskDelay(10 / portTICK_PERIOD_MS);
    success_get_token = wxjw_login(globalConfig.bupt_jw_account, based_password, token);
    last_teaching_week = get_last_teaching_week(token); // 获取学期结束周
    success_query_schedule = query_schedule(token, NULL, &is_examine_week, &is_xiaoxueqi, last_teaching_week, &is_on_vacation);
    // 主要功能是判断是不是考试周和小学期，schedule是一个副产品 是当日查出来的安排 所以置为NULL即可
    if (WiFi.status() != WL_CONNECTED)
    {
      Serial.println("Reconnecting to WiFi...");
      WiFi.disconnect();
      WiFi.reconnect();
    }
  }
  if (!digitalRead(CONTROL_BUTTON) == LOW)
  {
    u8g2.firstPage();
    do
    {
      u8g2.setFont(u8g2_font_wqy16_t_gb2312);
      u8g2.setFontDirection(0);
      u8g2.setCursor(10, 30);
      u8g2.print("获取Token成功");
    } while (u8g2.nextPage());
    // 显示获取了新的token
    // 加载阶段到此结束。根据状态器调用不同函数
    //  is_examine_week = false; // 测试
    // 只要判断是考试周/小学期 就进入这个函数
    // 因为只有考试周和小学期比较特殊
    // 但是只告诉他token，考试周还是小学期，让他自己再查一次，方便自己调整自己的状态
    if (is_examine_week || is_on_vacation)
    {
      // Serial.println("Special weeks.");
      exam_week_get_grade_summerschool_and_vacation(token, last_teaching_week, is_on_vacation);
    }
    else
    {
      // Serial.println("Normal weeks.");
      normal_week(token);
    }
  }
  delete[] based_password;
  delete[] token;
}

void normal_week(char *token)
{
  char *schedule = new char[1024];
  memset(schedule, 0, 1024);
  // schedule是当天所有没上的课
  bool is_on_class = false;         // is_on_class是是否正在上课
  char *on_class = new char[100];   // on_class是正在上的课的名字
  char *next_class = new char[100]; // next_class是下一节课

  memset(on_class, 0, 100);
  memset(next_class, 0, 100);
  int all_class_count = 0;        // allclasscount是当天总课程数
  int finished_class_count = 0;   // 完成的课程数
  int unfinished_class_count = 0; // unfinished_class_count是当天没上完课程数
  time_t time_delta = 0;          // time_delta是距离下一个最近课程的时间差 如果没有 那就是现在的时间
                                  // 如果当前正在上课 那就是还有多久下课
  bool pushedtobot = false;       // 是否已经推送过
  int today_week_day = 0;
  bool finished_all_class = false; // finished_all_class是是否上完了所有的课
  char *weather_buffer = new char[300];
  char *weather_trend_buffer = new char[300];

  // 以下是给线程用的
  char *weather = new char[300]; // 获取存到buffer然后拷贝到这个
  char *weather_trend = new char[300];
  char city_thr[10] = {};              // 存放天气城市给线程
  char *schedule_thr = new char[1024]; // 全部课程
  memset(schedule_thr, 0, 1024);
  bool is_on_class_thr = false;         // is_on_class是是否正在上课
  char *on_class_thr = new char[100];   // on_class是正在上的课的名字
  char *next_class_thr = new char[100]; // next_class是下一节课
  memset(on_class_thr, 0, 100);
  memset(next_class_thr, 0, 100);
  int all_class_count_thr = 0;        // allclasscount是当天总课程数
  int finished_class_count_thr = 0;   // 完成的课程数
  int unfinished_class_count_thr = 0; // unfinished_class_count是当天没上完课程数
  time_t time_delta_thr = 0;
  int today_week_day_thr = 0;
  bool finished_all_class_thr = false; // finished_all_class是是否上完了所有的课

  void *taskParams[12] = {schedule_thr, &all_class_count_thr, &unfinished_class_count_thr, &finished_all_class_thr, &is_on_class_thr, on_class_thr, &time_delta_thr, next_class_thr, &today_week_day_thr, weather, city_thr, weather_trend};
  while (!digitalRead(CONTROL_BUTTON) == LOW)
  {
    bool token_still_valid = normal_week_query_today_class(token, schedule, &is_on_class, on_class, &all_class_count, &finished_class_count, &unfinished_class_count, &time_delta, next_class, &today_week_day);
    finished_all_class = (all_class_count - unfinished_class_count == 0);
    bool weather_success = get_weather(province_id, area_id, county_id, nowtimet, weather_buffer);
    bool trend_success = get_weather_trend_str(nowtimet, weather_trend_buffer, latitude, longitude);
    if ((!token_still_valid) || (!weather_success) || !trend_success)
    {
      // if (!token_still_valid) Serial.println("Token is not valid.");
      // if (!weather_success) Serial.println("Can't get weather.");
      // if (!trend_success) Serial.println("Can't get weather trend.");
      break;
    }
    // Serial.println(time_delta);
    if (time_delta <= 600 && !pushedtobot && !is_on_class)
    {
      bool can_push = push_to_wxbot(next_class);
      if (!can_push)
      {
        break;
      }
      pushedtobot = true;
    }
    else if (time_delta > 600 && time_delta < 3600)
    {
      pushedtobot = false;
    }
    if ((!digitalRead(CONTROL_BUTTON) == LOW))
    {
      strcpy(schedule_thr, schedule);
      strcpy(on_class_thr, on_class);
      strcpy(next_class_thr, next_class);
      strcpy(city_thr, globalConfig.CountyName);
      strcpy(weather, weather_buffer);
      strcpy(weather_trend, weather_trend_buffer);
      all_class_count_thr = all_class_count;
      unfinished_class_count_thr = unfinished_class_count;
      finished_all_class_thr = finished_all_class;
      is_on_class_thr = is_on_class;
      time_delta_thr = time_delta;
      today_week_day_thr = today_week_day;
      // Serial.println("Render class!");
      render_class(taskParams); // 线程函数
      // scollrender_class_handle, // 线程句柄
    }
  }
  Serial.println("LOOP FAILED");
  delete[] schedule;
  delete[] schedule_thr;
  delete[] on_class_thr;
  delete[] next_class_thr;
  delete[] on_class;
  delete[] weather;
  delete[] next_class;
  delete[] weather_buffer;

  delete[] weather_trend;
  delete[] weather_trend_buffer;
}

void exam_week_get_grade_summerschool_and_vacation(char *token, int last_teaching_week, bool is_on_vacation_by_empty_week) // 最后一个参数其实用不到
{
  bool is_examine_week = true; // 这是退出这个函数的标识

  bool is_xiaoxueqi = false;
  bool exam_is_finished = false;

  bool is_xiaoxueqi_thr = false;
  bool exam_is_finished_thr = false;

  bool is_on_vacation = false; // 判断是不是放假了
  bool is_on_vacation_thr = false;

  // push_to_wxbot("成绩机器人重新上线\n准备推送所有成绩。");
  char *grade_thr = new char[1024];
  char *tmpgrade = new char[1024];
  char *schedule = new char[1024];
  char *closest_exam = new char[1024];
  char *schedule_thr = new char[1024];
  char *closest_exam_thr = new char[1024];
  memset(grade_thr, 0, 1024);
  memset(schedule, 0, 1024);
  memset(closest_exam, 0, 1024);
  memset(schedule_thr, 0, 1024);
  memset(closest_exam_thr, 0, 1024);
  char *weather_buffer = new char[300];
  memset(weather_buffer, 0, 200);
  int grade_num_thr = 0;
  float avg_gpa_thr = 0;
  int tmp_grade_num = 0;
  float tmp_avg_gpa = 0;
  char city_thr[10] = {};        // 存放天气城市给线程
  char *weather = new char[300]; // 存天气的字符串
  memset(weather, 0, 300);

  char *weather_trend = new char[300];
  char *weather_trend_buffer = new char[300];

  void *taskParams[11] = {grade_thr, &grade_num_thr, &avg_gpa_thr, &exam_is_finished_thr, schedule_thr, closest_exam_thr, &is_xiaoxueqi_thr, weather, city_thr, &is_on_vacation_thr, weather_trend};

  char *semester = new char[30];
  get_current_term(token, semester);
  while (semester == "null")
    get_current_term(token, semester);
  Serial.println(semester);

  while (!digitalRead(CONTROL_BUTTON) == LOW)
  {
    bool query_schedule_status = query_schedule(token, schedule, &is_examine_week, &is_xiaoxueqi, last_teaching_week, &is_on_vacation);
    // Serial.printf("%s %d %d %d %d", schedule,is_examine_week, is_xiaoxueqi, last_teaching_week, is_on_vacation);
    bool recon = query_if_finished_and_get_all_exam(token, &exam_is_finished, schedule, closest_exam);
    bool query_success = is_on_vacation || query_grade_by_term(token, semester, &tmp_grade_num, &tmp_avg_gpa, tmpgrade); // 逻辑短路，如果是放假了直接不查成绩了
    // 查成绩
    bool weather_success = get_weather(province_id, area_id, county_id, nowtimet, weather_buffer);
    // 查天气
    bool trend_success = get_weather_trend_str(nowtimet, weather_trend_buffer, latitude, longitude);
    if ((!query_success) || (!weather_success) || (!query_schedule_status) || !(recon) || !trend_success)
    {
      if (WiFi.status() != WL_CONNECTED)
      {
        Serial.println("Reconnecting to WiFi...");
        WiFi.disconnect();
        WiFi.reconnect();
      }
      break;
    }
    else if (!is_examine_week)
      break; // 如果我查出来不是考试周或者小学期了，我直接退出
    else if (tmp_grade_num != grade_num_pushed)
    {
      if (push_to_wxbot(tmpgrade, tmp_grade_num))
        grade_num_pushed = tmp_grade_num;
    }
    else if ((!digitalRead(CONTROL_BUTTON) == LOW))
    {
      strcpy(schedule_thr, schedule);
      strcpy(closest_exam_thr, closest_exam);
      strcpy(grade_thr, tmpgrade);
      strcpy(weather, weather_buffer);             // 拷贝天气
      strcpy(weather_trend, weather_trend_buffer); // 拷贝天气趋势
      strcpy(city_thr, globalConfig.CountyName);   // 拷贝城市
      is_on_vacation_thr = is_on_vacation;         // 拷贝是否放假
      grade_num_thr = tmp_grade_num;
      avg_gpa_thr = tmp_avg_gpa;
      is_xiaoxueqi_thr = is_xiaoxueqi;
      exam_is_finished_thr = exam_is_finished;
      render_scoll_thread(taskParams); // 线程函数 scollrender_handle
    }
  }
  delete[] schedule;
  delete[] closest_exam;
  delete[] weather_buffer;
  delete[] semester;
  delete[] grade_thr;
  delete[] weather;
  delete[] tmpgrade;
  delete[] schedule_thr;
  delete[] closest_exam_thr;

  delete[] weather_trend;
  delete[] weather_trend_buffer;
}
/*
void loop() {
  char * newcookie = new char[200];

  jw_login((char*)jw_account, (char*)jw_password, newcookie);
  Serial.println(newcookie);
  while(1){
    String free_heap = String(ESP.getFreeHeap());
    char * grade = new char[200];
    query_grade(newcookie, grade);

    int cursorX = 10;
    int lineHeight = 20; // Adjust based on your font size
    int textHeight = getLineCount(grade) * lineHeight;
    int displayHeight = u8g2.getDisplayHeight();
    int initialY = displayHeight;

    while (1) {
      for (int cursorY = initialY; cursorY >= -textHeight; cursorY -= 1) {
        u8g2.setFont(u8g2_font_wqy12_t_gb2312);
        u8g2.setFontDirection(0);
        u8g2.firstPage();
        do {
          int y = cursorY;
          const char *p = grade;
          while (*p) {
            u8g2.setCursor(cursorX, y);
            while (*p && *p != '\n') {
              u8g2.print(*p++);
            }
            y += lineHeight;
            if (*p == '\n') {
              p++;
            }
          }
        } while (u8g2.nextPage());
        delay(50); // Adjust the delay for smoother or faster scrolling
      }

      Serial.println(grade);
      delay(10000);
    }

    delete[] grade;
  }
  delete[] newcookie;
}
*/