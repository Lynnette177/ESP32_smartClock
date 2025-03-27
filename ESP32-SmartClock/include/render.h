#include "includes.h"

void render_scroll_clock(char *weather, char *weather_trend, char *city)
{
  const int time_left_x = 34; // 常量 定义时钟数字开始绘制的最左侧的坐标
  const int clock_Y_position = 32;
  int show_clock_time = 50000; // 展示这个时钟的毫秒数
  String now_time_str_all = getFormattedDateTime_For_clock_Chinese(nowtimet);
  int indexofSplit = now_time_str_all.indexOf(';');
  unsigned int start_tick = millis();
  String Y_M_D = now_time_str_all.substring(0, indexofSplit);
  String hour_minute_sec = now_time_str_all.substring(indexofSplit + 1);
  scroll_time_struct last_rendering_time(hour_minute_sec[0], hour_minute_sec[1], hour_minute_sec[3], hour_minute_sec[4], hour_minute_sec[6], hour_minute_sec[7]);
  scroll_time_struct now_rendering_time = last_rendering_time;
  String week_day = parase_weekday_from_wday(localtime(&nowtimet)->tm_wday);
  float scroll_start_pos_f = u8g2.getDisplayHeight();
  float text_scroll_speed = 0.4f; // 文字滚动速度 整个界面进入和退出的那个
  float weatherX_f = 128;         // 从右侧屏幕外渐进
  float weather_trendX_f = 128;   // 天气趋势向左滚动用
  float weather_rolling_speed = 0.4f;
  while ((millis() - start_tick < show_clock_time || scroll_start_pos_f > -64) && (!digitalRead(CONTROL_BUTTON) == LOW)) // 显示15秒钟时间就可以了
  {
    int scroll_start_pos = (int)scroll_start_pos_f;
    bool scrolled = false;
    now_time_str_all = getFormattedDateTime_For_clock_Chinese(nowtimet);
    Y_M_D = now_time_str_all.substring(0, indexofSplit);
    hour_minute_sec = now_time_str_all.substring(indexofSplit + 1);
    now_rendering_time = scroll_time_struct(hour_minute_sec[0], hour_minute_sec[1], hour_minute_sec[3], hour_minute_sec[4], hour_minute_sec[6], hour_minute_sec[7]);
    if (last_rendering_time == now_rendering_time)
    {
      int weatherX = (int)weatherX_f; // 从右侧屏幕外渐进
      int weather_trendX = (int)weather_trendX_f;
      u8g2.firstPage();
      do
      {
        // 以下绘制上方时间和底部天气
        u8g2.setFont(u8g2_font_wqy12_t_gb2312);
        u8g2.setFontDirection(0);
        u8g2.setCursor(0, 10 + scroll_start_pos);
        u8g2.print(Y_M_D + "   " + week_day);

        u8g2.setCursor(weatherX, 62 + scroll_start_pos);
        u8g2.print(weather);

        u8g2.setCursor(weather_trendX, 48 + scroll_start_pos);
        u8g2.print(weather_trend);

        u8g2.setDrawColor(0);
        u8g2.drawBox(0, 36 + scroll_start_pos, 26, 14);
        u8g2.setDrawColor(1);
        u8g2.setCursor(0, 48 + scroll_start_pos);
        u8g2.print(city);

        weatherX_f -= weather_rolling_speed;
        weather_trendX_f -= weather_rolling_speed;
        if (weatherX_f < -u8g2.getUTF8Width(weather))
          weatherX_f = 128;
        if (weather_trendX_f < 24 - u8g2.getUTF8Width(weather_trend))
          weather_trendX_f = 128;

        // 先画天气 后画城市 让城市覆盖天气
        u8g2.setFont(u8g2_font_wqy16_t_gb2312);
        u8g2.setFontDirection(0);
        int curSor_X = 34;
        u8g2.setCursor(curSor_X, clock_Y_position + scroll_start_pos);
        u8g2.print(last_rendering_time.h1);
        curSor_X += u8g2.getStrWidth(String(last_rendering_time.h1).c_str());
        u8g2.setCursor(curSor_X, clock_Y_position + scroll_start_pos);
        u8g2.print(last_rendering_time.h2);
        curSor_X += u8g2.getStrWidth(String(last_rendering_time.h2).c_str());
        u8g2.print(":");
        curSor_X += 4;
        u8g2.setCursor(curSor_X, clock_Y_position + scroll_start_pos);
        u8g2.print(last_rendering_time.m1);
        curSor_X += u8g2.getStrWidth(String(last_rendering_time.m1).c_str());
        u8g2.setCursor(curSor_X, clock_Y_position + scroll_start_pos);
        u8g2.print(last_rendering_time.m2);
        curSor_X += u8g2.getStrWidth(String(last_rendering_time.m2).c_str());
        u8g2.print(":");
        curSor_X += 4;
        u8g2.setCursor(curSor_X, clock_Y_position + scroll_start_pos);
        u8g2.print(last_rendering_time.s1);
        curSor_X += u8g2.getStrWidth(String(last_rendering_time.s1).c_str());
        u8g2.setCursor(curSor_X, clock_Y_position + scroll_start_pos);
        u8g2.print(last_rendering_time.s2);
        curSor_X += u8g2.getStrWidth(String(last_rendering_time.s2).c_str());
      } while (u8g2.nextPage());
    }
    else
    {
      int cursor_Y = clock_Y_position + scroll_start_pos;
      cursor_Y += 15;

      for (; cursor_Y >= clock_Y_position + scroll_start_pos && (scroll_start_pos > -64); cursor_Y -= scrolled ? 2 : 1)
      {
        int weatherX = (int)weatherX_f; // 从右侧屏幕外渐进
        int weather_trendX = (int)weather_trendX_f;
        int startX = time_left_x;
        u8g2.firstPage();
        do
        {
          // 先绘制时间后绘制天气 先覆盖黑色矩形后在上方展示天气

          u8g2.setFont(u8g2_font_wqy16_t_gb2312);
          u8g2.setFontDirection(0);
          u8g2.setCursor(startX, cursor_Y);

          if (last_rendering_time.h1 != now_rendering_time.h1)
          {
            u8g2.print(now_rendering_time.h1);
            u8g2.setCursor(startX, cursor_Y - 15);
            u8g2.print(last_rendering_time.h1);
          }
          else
          {
            u8g2.setCursor(startX, clock_Y_position + scroll_start_pos);
            u8g2.print(last_rendering_time.h1);
          }

          startX += u8g2.getStrWidth(String(last_rendering_time.h1).c_str());
          u8g2.setCursor(startX, cursor_Y);

          if (last_rendering_time.h2 != now_rendering_time.h2)
          {
            u8g2.print(now_rendering_time.h2);
            u8g2.setCursor(startX, cursor_Y - 15);
            u8g2.print(last_rendering_time.h2);
          }
          else
          {
            u8g2.setCursor(startX, clock_Y_position + scroll_start_pos);
            u8g2.print(last_rendering_time.h2);
          }

          startX += u8g2.getStrWidth(String(last_rendering_time.h2).c_str());
          startX += 4;
          u8g2.setCursor(startX, cursor_Y);

          if (last_rendering_time.m1 != now_rendering_time.m1)
          {
            u8g2.print(now_rendering_time.m1);
            u8g2.setCursor(startX, cursor_Y - 15);
            u8g2.print(last_rendering_time.m1);
          }
          else
          {
            u8g2.setCursor(startX, clock_Y_position + scroll_start_pos);
            u8g2.print(last_rendering_time.m1);
          }

          startX += u8g2.getStrWidth(String(last_rendering_time.m1).c_str());
          u8g2.setCursor(startX, cursor_Y);

          if (last_rendering_time.m2 != now_rendering_time.m2)
          {
            u8g2.print(now_rendering_time.m2);
            u8g2.setCursor(startX, cursor_Y - 15);
            u8g2.print(last_rendering_time.m2);
          }
          else
          {
            u8g2.setCursor(startX, clock_Y_position + scroll_start_pos);
            u8g2.print(last_rendering_time.m2);
          }

          startX += u8g2.getStrWidth(String(last_rendering_time.m2).c_str());
          startX += 4;
          u8g2.setCursor(startX, cursor_Y);

          if (last_rendering_time.s1 != now_rendering_time.s1)
          {
            u8g2.print(now_rendering_time.s1);
            u8g2.setCursor(startX, cursor_Y - 15);
            u8g2.print(last_rendering_time.s1);
          }
          else
          {
            u8g2.setCursor(startX, clock_Y_position + scroll_start_pos);
            u8g2.print(last_rendering_time.s1);
          }

          startX += u8g2.getStrWidth(String(last_rendering_time.s1).c_str());
          u8g2.setCursor(startX, cursor_Y);

          // render s2
          if (last_rendering_time.s2 != now_rendering_time.s2)
          {
            u8g2.print(now_rendering_time.s2);
            u8g2.setCursor(startX, cursor_Y - 15);
            u8g2.print(last_rendering_time.s2);
          }
          else
          {
            u8g2.setCursor(startX, clock_Y_position + scroll_start_pos);
            u8g2.print(last_rendering_time.s2);
          }

          // 添加时间上方和下方的蒙版黑色实心矩形 以实现占空间较小的滚动时间效果
          u8g2.setDrawColor(0);
          u8g2.drawBox(time_left_x - 14, clock_Y_position - 28 + scroll_start_pos, 90, 14);
          u8g2.drawBox(time_left_x - 14, clock_Y_position + 2 + scroll_start_pos, 90, 14);
          u8g2.setDrawColor(1);

          // 以下绘制上方时间和底部天气
          u8g2.setFont(u8g2_font_wqy12_t_gb2312);
          u8g2.setFontDirection(0);
          u8g2.setCursor(0, 10 + scroll_start_pos);
          u8g2.print(Y_M_D + "   " + week_day);

          u8g2.setCursor(weatherX, 62 + scroll_start_pos);
          u8g2.print(weather);

          u8g2.setCursor(weather_trendX, 48 + scroll_start_pos);
          u8g2.print(weather_trend);

          u8g2.setDrawColor(0);
          u8g2.drawBox(0, 36 + scroll_start_pos, 26, 14);
          u8g2.setDrawColor(1);
          u8g2.setCursor(0, 48 + scroll_start_pos);
          u8g2.print(city);

          weatherX_f -= weather_rolling_speed;
          weather_trendX_f -= weather_rolling_speed;
          if (weatherX_f < -u8g2.getUTF8Width(weather))
            weatherX_f = 128;
          if (weather_trendX_f < 24 - u8g2.getUTF8Width(weather_trend))
            weather_trendX_f = 128;
          // 先画天气 后画城市 让城市覆盖天气

        } while (u8g2.nextPage());
        vTaskDelay(1 / portTICK_PERIOD_MS);
        if (scroll_start_pos_f > 0)
        {
          scroll_start_pos_f -= text_scroll_speed;
          scrolled = true;
        }
        if (millis() - start_tick >= show_clock_time)
        {
          scroll_start_pos_f -= text_scroll_speed;
          scrolled = true;
        }
      } // for循环结束
      last_rendering_time = now_rendering_time;
    }
    if (scroll_start_pos_f > 0 && !scrolled)
      scroll_start_pos_f -= text_scroll_speed;
    if (millis() - start_tick >= show_clock_time && !scrolled)
      scroll_start_pos_f -= text_scroll_speed;
    vTaskDelay(1 / portTICK_PERIOD_MS);
  }
}
void render_scoll_thread(void *pvParameters)
{
  char **params = (char **)pvParameters;
  char *grade = params[0];
  int *grade_num = (int *)params[1];
  float *avg_gpa = (float *)params[2];
  bool *only_render_grade = (bool *)params[3];
  char *schedule = (char *)params[4];
  char *closest_exam = (char *)params[5];
  bool *is_xiaoxueqi = (bool *)params[6];
  char *weather = (char *)params[7]; // 天气字符串
  char *city = (char *)params[8];    // 天气城市
  bool *is_on_vacation = (bool *)params[9];
  char *weather_trend = (char *)params[10];
  int cursorX = 0;
  int lineHeight = 15;          // 根据字体大小调节
  int absolute_lineHeight = 11; // 根据字体大小调节
  int displayHeight = u8g2.getDisplayHeight();
  int initialY = displayHeight + absolute_lineHeight;

  // Serial.printf("Grade: %s\n", grade);
  // Serial.printf("Grade Number: %d\n", *grade_num);
  // Serial.printf("Average GPA: %.2f\n", *avg_gpa);
  // Serial.printf("Only Render Grade: %s\n", *only_render_grade ? "true" : "false");
  // Serial.printf("Schedule: %s\n", schedule);
  // Serial.printf("Closest Exam: %s\n", closest_exam);
  // Serial.printf("Is Xiaoxueqi: %s\n", *is_xiaoxueqi ? "true" : "false");
  // Serial.printf("Weather: %s\n", weather);
  // Serial.printf("City: %s\n", city);
  // Serial.printf("Is On Vacation: %s\n", *is_on_vacation ? "true" : "false");
  // Serial.printf("Weather Trend: %s\n", weather_trend);

  bool draw_switcher = 1;
  if (!*only_render_grade || *is_xiaoxueqi)
    draw_switcher = !draw_switcher; // 如果有小学期从小学期开始播放 然后成绩 最后时钟 因为0是小学期 1是成绩 成绩结束后进入时钟
  const char *p = NULL;
  if (!*is_on_vacation && (!digitalRead(CONTROL_BUTTON) == LOW))
  {
    int textHeight;
    if (draw_switcher)
    {
      textHeight = (getLineCount(grade) - 2) * lineHeight;
    }
    else
      textHeight = (getLineCount(schedule) - 2) * lineHeight;
    if (textHeight < 64)
      textHeight = 64;  // 防止左边短 右边时间长 导致右边跳上去
    if (textHeight > 0) // 只有内容不为空的时候才绘制，不然直接跳过
    {
      for (int cursorY = initialY; cursorY >= -textHeight && (!digitalRead(CONTROL_BUTTON) == LOW); cursorY -= 1)
      {
        bool time_show_up_together = false;
        bool time_goes_on_together = false;
        if (cursorY > 0)
          time_show_up_together = true;
        if (cursorY + textHeight < 64)
          time_goes_on_together = true;
        u8g2.setFont(u8g2_font_wqy12_t_gb2312);
        u8g2.setFontDirection(0);
        u8g2.firstPage();
        do
        {
          // 打印固定的已出成绩常量信息
          int plus_value = 0; // 控制开始时右侧常驻时间一起滚上来
          int sub_value = 0;  // 控制结束时右侧常驻时间等一起滚动上去
          if (time_show_up_together)
            plus_value = cursorY;
          if (time_goes_on_together)
            sub_value = cursorY + textHeight - 64;
          u8g2.setCursor(105, 10 + sub_value + plus_value);
          if (draw_switcher)
            u8g2.print(String(*grade_num) + "门");
          else
            u8g2.print(String((getLineCount(schedule) - 1) / 4) + "门");
          u8g2.setCursor(100, 23 + sub_value + plus_value);
          u8g2.print(String(*avg_gpa));
          char time_str[50];
          char *time_str_ptr = time_str;
          strcpy(time_str, thetimenow.c_str());
          for (int y_offset = 0; y_offset <= absolute_lineHeight * 2; y_offset += absolute_lineHeight)
          {
            u8g2.setCursor(100, 36 + y_offset + sub_value + plus_value);
            while (*time_str_ptr != '\n' && *time_str_ptr != '\0')
              u8g2.print(*(time_str_ptr++));
            time_str_ptr++;
          }
          u8g2.setCursor(102, 59 + sub_value + plus_value);
          if (draw_switcher)
            u8g2.print("已出");
          else
          {
            if (!*is_xiaoxueqi)
              u8g2.print("待考");
            else
            {
              u8g2.setCursor(92, 59 + sub_value + plus_value);
              u8g2.print("小学期");
            }
          }
          int y = cursorY;
          if (draw_switcher)
            p = grade;
          else
            p = schedule;
          while (*p)
          {
            u8g2.setCursor(cursorX, y);
            while (*p && *p != '\n')
            {
              u8g2.print(*p++);
            }
            y += lineHeight;
            if (*p == '\n')
            {
              p++;
            }
          }
        } while (u8g2.nextPage());
        delay(30); // 可调滚动速度
      }
    }
    if (digitalRead(CONTROL_BUTTON) == LOW)
      return;
    if (draw_switcher || (!draw_switcher && *is_xiaoxueqi && *grade_num == 0)) // 如果成绩页播完了 或者 小学期中 播放的小学期页面 并且成绩门数为0(开学小学期)
    {
      render_scroll_clock(weather, weather_trend, city);
      return;
    }
    else
    {
      u8g2.firstPage();
      bool render_delay = true;
      do
      {
        u8g2.setCursor(10, 10);
        u8g2.print("下一场考试");
        struct tm tm1;
        time_t t1 = 0;
        String time_str = closest_exam;
        time_str.replace("\n", " ");
        int index1 = time_str.indexOf(' ');
        int index2 = time_str.indexOf('~');
        time_str = time_str.substring(index1 + 1, index2 - 1);
        if (strptime(time_str.c_str(), "%Y-%m-%d %H:%M", &tm1) == NULL)
        {
          render_delay = false;
          break;
        }
        tm1.tm_isdst = -1; // 不使用夏令时
        t1 = mktime(&tm1);

        time_t delta = t1 - nowtimet;
        if (delta <= 0)
        {
          WiFi.disconnect();
        }

        // 将delta转换为天数、小时和分钟
        int days = delta / (24 * 3600);
        delta = delta % (24 * 3600);
        int hours = delta / 3600;
        delta = delta % 3600;
        int minutes = delta / 60;

        char timestr[40];
        if (days > 0)
          snprintf(timestr, sizeof(timestr), "%02d小时%02d分钟", hours, minutes);
        else
          snprintf(timestr, sizeof(timestr), "%d天%02d小时%02d分钟", days, hours, minutes);

        int x = 10;
        int y = 20;
        char *e = closest_exam;
        int index = 0;
        while (*e)
        {
          u8g2.setCursor(x, y);
          while (*e && *e != '\n')
          {
            u8g2.print(*e++);
          }
          e++;
          index++;
          if (index == 3)
          {
            x += 75;
          }
          else
            y += 12;
        }
        u8g2.setCursor(10, y);
        u8g2.print(String(timestr));
      } while (u8g2.nextPage());
      if (render_delay)
        vTaskDelay(5000 / portTICK_PERIOD_MS);
    }
    if (!*only_render_grade)
      draw_switcher = !draw_switcher;
  }
  else if (*is_on_vacation && (!digitalRead(CONTROL_BUTTON) == LOW))
  {
    render_scroll_clock(weather, weather_trend, city);
  }
  Serial.println("EXITING..");
}

void render_class(void *pvParameters) // 一定要注意参数必须要读指针
{
  char **params = (char **)pvParameters;
  char *schedule_unfinish = params[0];             // 全部没上完的课
  int *all_class_count = (int *)params[1];         // 今日课程总数
  int *unfinished_class_count = (int *)params[2];  // 没上完的课多少节
  bool *is_finished_all_class = (bool *)params[3]; // 是否上完了所有课
  bool *is_on_class = (bool *)params[4];           // 是否正在上课
  char *on_class_name = (char *)params[5];         // 正在上课的名字
  time_t *time_delta = (time_t *)params[6];        // 时间差。到下一节课或者是到下课
  char *next_class_name = (char *)params[7];       // 下一节课的名字
  int *weekday = (int *)params[8];                 // 今天是周几
  char *weather = (char *)params[9];               // 天气字符串
  char *city = (char *)params[10];                 // 天气城市
  char *weather_trend = (char *)params[11];
  int cursorX = 0;
  int lineHeight = 15;          // 根据字体大小调节
  int absolute_lineHeight = 11; // 根据字体大小调节
  int displayHeight = u8g2.getDisplayHeight();
  int initialY = displayHeight + absolute_lineHeight;
  /*
  Serial.println("1");
  Serial.println(schedule_unfinish);
  Serial.println("2");
  Serial.println(*all_class_count);
  Serial.println("3");
  Serial.println(*unfinished_class_count );
  Serial.println("4");
  Serial.println(*is_finished_all_class);
  Serial.println("5");
  Serial.println(*is_on_class);
  Serial.println("6");
  Serial.println(*on_class_name);
  Serial.println("7");
  Serial.println(*time_delta);
  Serial.println("8");
  Serial.println(next_class_name);
  Serial.println("9");
  Serial.println(* weekday );
  */

  const char *p = NULL;
  if ((!digitalRead(CONTROL_BUTTON) == LOW))
  {
    if (*unfinished_class_count != 0)
    {
      int textHeight;
      String schedule = "";
      if (!*is_on_class)
        schedule = "未上的课：\n" + String(schedule_unfinish);
      else
      {
        schedule = "正在上课：\n" + String(on_class_name);
        if (*unfinished_class_count != 1)
        { // 正在上最后一节课不用加
          schedule += "\n未上的课：\n" + String(schedule_unfinish);
        }
      }
      textHeight = (getLineCount(schedule.c_str()) - 2) * lineHeight; // 这个开学了的时候得看看getLineCount结果是不是需要减去2 --确实是需要的
      if (textHeight < 64)
        textHeight = 64; // 防止左边短 右边时间长 导致右边跳上去
      String week_day_str;
      // 生成星期的字符串
      week_day_str = parase_weekday_from_wday((*weekday) % 7);
      for (float cursorY_f = initialY; cursorY_f >= -textHeight && (!digitalRead(CONTROL_BUTTON) == LOW); cursorY_f -= 0.5) // 文字滚动速度0.5
      {
        int cursorY = (int)cursorY_f;
        bool time_show_up_together = false;
        bool time_goes_on_together = false;
        if (cursorY > 0)
          time_show_up_together = true; // 当cursorY还大于0的时候 说明左侧还在向上出现 需要时间跟着出现
        if (cursorY + textHeight < 64)
          time_goes_on_together = true; // 当左侧到底之后 右侧时间跟着上
        u8g2.setFont(u8g2_font_wqy12_t_gb2312);
        u8g2.setFontDirection(0);
        u8g2.firstPage();
        do
        {
          // draw左侧课程
          int y = cursorY;
          p = schedule.c_str();
          while (*p)
          {
            u8g2.setCursor(cursorX, y);
            while (*p && *p != '\n')
            {
              u8g2.print(*p++);
            }
            y += lineHeight;
            if (*p == '\n')
            {
              p++;
            }
          }

          int plus_value = 0; // 控制开始时右侧常驻时间一起滚上来
          int sub_value = 0;  // 控制结束时右侧常驻时间等一起滚动上去
          if (time_show_up_together)
            plus_value = cursorY;
          else if (time_goes_on_together)
            sub_value = cursorY + textHeight - 64;
          u8g2.setCursor(100, 10 + sub_value + plus_value);
          u8g2.print(week_day_str.c_str()); // 打印今天是周几
          // 打印当日已上课数/课程总数
          u8g2.setCursor(96, 23 + sub_value + plus_value);
          u8g2.print(String(*all_class_count - *unfinished_class_count) + "/" + String(*all_class_count) + "门");
          // 下面是打印时间
          // 先打印最下方的晚上好 防止被吞字符 然后打印上面的内容。
          u8g2.setCursor(92, 61 + sub_value + plus_value);
          int hour_ = thetimenow.substring(6, 8).toInt();
          if (hour_ >= 1 && hour_ <= 6)
            u8g2.print("凌晨好");
          else if (hour_ > 6 && hour_ < 8)
          {
            u8g2.print("早上好");
          }
          else if (hour_ >= 8 && hour_ < 11)
          {
            u8g2.print("上午好");
          }
          else if (hour_ >= 11 && hour_ <= 13)
          {
            u8g2.print("中午好");
          }
          else if (hour_ >= 14 && hour_ < 18)
          {
            u8g2.print("下午好");
          }
          else if (hour_ >= 18 || hour_ < 1)
          {
            u8g2.print("晚上好");
          }

          String hour_str = ""; // 存取一个小时的数值 后面上下午信息的时候用
          char time_str[50];
          char *time_str_ptr = time_str;
          strcpy(time_str, thetimenow.c_str());
          for (int y_offset = 0; y_offset <= absolute_lineHeight * 2; y_offset += absolute_lineHeight)
          {
            u8g2.setCursor(100, 36 + y_offset + sub_value + plus_value);
            while (*time_str_ptr != '\n' && *time_str_ptr != '\0')
              u8g2.print(*(time_str_ptr++));
            time_str_ptr++;
          }
          u8g2.print(" "); // 防止吞字符 不知道为什么
        } while (u8g2.nextPage());
        delay(30); // 可调滚动速度
      }
      if (digitalRead(CONTROL_BUTTON) == LOW)
        return;
      // 下面用来绘制时间差的信息
      if (!*is_on_class && unfinished_class_count != 0)
      {
        u8g2.firstPage();
        bool render_delay = true;
        do
        {
          u8g2.setCursor(10, 10);
          u8g2.print("距下一堂课");
          struct tm tm1;
          time_t t1 = 0;
          time_t delta = *time_delta;
          // 将delta转换为天数、小时和分钟
          int days = delta / (24 * 3600);
          delta = delta % (24 * 3600);
          int hours = delta / 3600;
          delta = delta % 3600;
          int minutes = delta / 60;

          char timestr[40];
          snprintf(timestr, sizeof(timestr), "%d天%02d小时%02d分钟", days, hours, minutes);

          int x = 10;
          int y = 24;
          char *e = next_class_name;
          int index = 0;
          while (*e)
          {
            u8g2.setCursor(x, y);
            while (*e && *e != '\n')
            {
              u8g2.print(*e++);
            }
            e++;
            index++;
            if (index == 3)
            {
              x += 75;
            }
            else
              y += 12;
          }
          u8g2.setCursor(10, y);
          u8g2.print(String(timestr));
        } while (u8g2.nextPage());
      }
      else if (*is_on_class)
      {
        u8g2.firstPage();
        bool render_delay = true;
        do
        {
          u8g2.setCursor(10, 10);
          u8g2.print("正在上课：");
          struct tm tm1;
          time_t t1 = 0;
          time_t delta = *time_delta;
          // 将delta转换为天数、小时和分钟
          int days = delta / (24 * 3600);
          delta = delta % (24 * 3600);
          int hours = delta / 3600;
          delta = delta % 3600;
          int minutes = delta / 60;

          char timestr[40];
          snprintf(timestr, sizeof(timestr), "%02d小时%02d分钟", hours, minutes);
          int x = 10;
          int y = 24;
          char *e = on_class_name;
          int index = 0;
          while (*e)
          {
            u8g2.setCursor(x, y);
            while (*e && *e != '\n')
            {
              u8g2.print(*e++);
            }
            e++;
            index++;
            if (index == 3)
            {
              x += 75;
            }
            else
              y += 12;
          }
          u8g2.setCursor(10, y);
          u8g2.print("距离下课还有：");
          u8g2.setCursor(10, y + 12);
          u8g2.print(String(timestr));
        } while (u8g2.nextPage());
      }
    }
    else
    { // 这里是上完了所有课的
      std::string week_day_str = "";
      // 生成星期的字符串
      switch (*weekday)
      {
      case 1:
        week_day_str = "周一";
        break;
      case 2:
        week_day_str = "周二";
        break;
      case 3:
        week_day_str = "周三";
        break;
      case 4:
        week_day_str = "周四";
        break;
      case 5:
        week_day_str = "周五";
        break;
      case 6:
        week_day_str = "周六";
        break;
      case 7:
        week_day_str = "周日";
        break;
      default:
        week_day_str = "周八";
      }
      u8g2.setFont(u8g2_font_wqy12_t_gb2312);
      u8g2.setFontDirection(0);
      u8g2.firstPage();
      do
      {
        u8g2.setCursor(10, 30);
        u8g2.print("今日已无课"); // 打印测试
        u8g2.setCursor(100, 10);
        u8g2.print(week_day_str.c_str()); // 打印今天是周几
        // 打印当日已上课数/课程总数
        u8g2.setCursor(96, 23);
        u8g2.print(String(*all_class_count - *unfinished_class_count) + "/" + String(*all_class_count) + "门");
        // 下面是打印时间
        String hour_str = ""; // 存取一个小时的数值 后面上下午信息的时候用
        char time_str[50];
        char *time_str_ptr = time_str;
        strcpy(time_str, thetimenow.c_str());
        for (int y_offset = 0; y_offset <= absolute_lineHeight * 2; y_offset += absolute_lineHeight)
        {
          u8g2.setCursor(100, 36 + y_offset);
          while (*time_str_ptr != '\n' && *time_str_ptr != '\0')
            u8g2.print(*(time_str_ptr++));
          time_str_ptr++;
        }
        u8g2.print(" "); // 防止吞字符

        u8g2.setCursor(92, 60);
        int hour_ = thetimenow.substring(6, 8).toInt();
        if (hour_ >= 1 && hour_ <= 6)
          u8g2.print("凌晨好");
        else if (hour_ > 6 && hour_ < 8)
        {
          u8g2.print("早上好");
        }
        else if (hour_ >= 8 && hour_ < 11)
        {
          u8g2.print("上午好");
        }
        else if (hour_ >= 11 && hour_ <= 13)
        {
          u8g2.print("中午好");
        }
        else if (hour_ >= 14 && hour_ < 18)
        {
          u8g2.print("下午好");
        }
        else if (hour_ >= 18 || hour_ < 1)
        {
          u8g2.print("晚上好");
        }
      } while (u8g2.nextPage());
    }

    uint64_t start_rendering_class_pause = millis();
    while (millis() - start_rendering_class_pause < 6000)
    { // 这里是绘制 还有多久上课/还有多久下课/今日已无课 用循环避免不能退出
      vTaskDelay(50 / portTICK_PERIOD_MS);
      if (digitalRead(CONTROL_BUTTON) == LOW)
        break;
    }
    render_scroll_clock(weather, weather_trend, city); // 然后绘制滚动时钟
  }
}

void render_loading(void *pvParameters)
{
  void **params = (void **)pvParameters;
  int *mode = (int *)params[0];
  int lineposLeft = 10;
  int lineposRight = 10;
  int lineSpeedLeft = 1;
  int lineSpeedRight = 2;
  unsigned int scan_wifi_time = 0;
  unsigned int connect_wifi_time = 0;
  while (*mode >= 0)
  {
    if (*mode == 1)
    {
      if (scan_wifi_time == 0)
        scan_wifi_time = millis();
    }
    if (*mode == 9)
    {
      if (connect_wifi_time == 0)
        connect_wifi_time = millis();
    }
    u8g2.setFont(u8g2_font_wqy16_t_gb2312);
    u8g2.setFontDirection(0);
    u8g2.firstPage();
    do
    {
      u8g2.setCursor(22, 30);
      if (*mode == 0)
      {
        u8g2.print("按住来配置..");
      }
      else if (*mode == 1)
      {
        u8g2.print("扫描网络中..");
        u8g2.drawRFrame(14, 42, 100, 6, 2);
        float delta_time = float(millis() - scan_wifi_time);
        float totaltime = 3000.f;
        float progress = delta_time * 100 / totaltime;
        int progress_width = max(1, int(progress));
        progress_width = min(100, int(progress));
        // 带圆角必须 h >= 2*(r+1) w >= 2*(r+1)
        int r = 2;
        if (progress_width < 6)
          r = max(0, progress_width / 2 - 1);
        u8g2.drawRBox(14, 42, progress_width, 6, r);
      }
      else if (*mode == 9)
      {
        // u8g2.setCursor(22, 16);
        u8g2.print("拨号上网中..");
        u8g2.drawRFrame(14, 42, 100, 6, 2);
        float delta_time = float(millis() - connect_wifi_time);
        float totaltime = 15000.f;
        float progress = delta_time * 100 / totaltime;
        int progress_width = max(1, int(progress));
        progress_width = min(100, int(progress));
        // 带圆角必须 h >= 2*(r+1) w >= 2*(r+1)
        int r = 2;
        if (progress_width < 6)
          r = max(0, progress_width / 2 - 1);
        u8g2.drawRBox(14, 42, progress_width, 6, r);
      }
      else if (*mode == 10)
        u8g2.print("检查更新中...");
      else if (*mode == 11)
        u8g2.print("观测天象中..");
      else if (*mode == 2)
        u8g2.print("破解WIFI中..");
      else if (*mode == 3)
        u8g2.print("翻阅地图中..");
      else if (*mode == 4)
        u8g2.print("手表对时中..");
      else if (*mode == 5)
        u8g2.print("翻日程表中..");
      else if (*mode == 6)
        u8g2.print("启动切换器..");
      else if (*mode == 7)
        u8g2.print("线程切换中..");
      else if (*mode == 8)
        u8g2.print("进入离线中..");
      // 带圆角必须 h >= 2*(r+1) w >= 2*(r+1)
      u8g2.drawRBox(lineposLeft, 52, (lineposRight - lineposLeft), 4, 1);
    } while (u8g2.nextPage());

    if (lineposRight < 118 && lineSpeedRight > 0)
    { // 线条正在向右移动
      if (lineposRight - lineposLeft > 50)
        lineSpeedLeft = 3;
      if (lineposRight - lineposLeft < 20)
        lineSpeedLeft = 1;
    }
    else if (lineposLeft > 10 && lineSpeedLeft < 0)
    { // 线条正在向左移动
      if (lineposRight - lineposLeft > 50)
        lineSpeedRight = -3;
      if (lineposRight - lineposLeft < 20)
        lineSpeedRight = -1;
    }
    else if (lineposRight >= 118 && lineSpeedRight > 0)
    { // 移动到最右侧了该变向了
      lineposRight = 118;
      if (lineposLeft < 110)
      {
        lineposLeft += lineSpeedLeft;
      }
      else
      {
        lineposLeft = 110;
        lineSpeedLeft = -2;
        lineSpeedRight = -1;
      }
    }
    else if (lineposLeft <= 10 && lineSpeedLeft < 0)
    { // 移动到最左侧了该变向了
      lineposLeft = 10;
      if (lineposRight > 18)
      {
        lineposRight += lineSpeedRight;
      }
      else
      {
        lineposRight = 18;
        lineSpeedRight = 2;
        lineSpeedLeft = 1;
      }
    }
    lineposRight += lineSpeedRight;
    lineposLeft += lineSpeedLeft;
    vTaskDelay(3 / portTICK_PERIOD_MS);
  }
  vTaskDelete(NULL);
}

void render_spectrogram(void *pvParameters)
{
  int SAMPLES = 64; // This value must be a power of 2
  float last_height[16] = {0};
  float line_down_speed = 0.5;
  double vReal[512];
  double vImag[512];
  ArduinoFFT<double> FFT = ArduinoFFT<double>(vReal, vImag, SAMPLES, SAMPLE_RATE); /* Create FFT object */
  int16_t rawData[512];
  int16_t rawData_tmp[512];

  bool showframe = false;
  unsigned long lastTime = millis();
  int frameCount = 0;
  float fps = 0.0;
  bool aibotton_pressed = false;

  while (!digitalRead(CONTROL_BUTTON) == LOW)
  {

    if (digitalRead(AI_BUTTON) == LOW){
      aibotton_pressed = true;
    }
    else if (aibotton_pressed){
      aibotton_pressed = false;
      showframe = !showframe;
    }

    memset(rawData_tmp, 0, 512);
    size_t bytesRead;
    bytesRead = I2S_Read(rawData_tmp, 1024);
    // Serial.println(bytesRead);

    for (int i = 0; i < SAMPLES && (!digitalRead(CONTROL_BUTTON) == LOW); i++)
    {
      // Serial.printf("%d\n",rawData[i]);
      rawData[i] = rawData[i] + rawData_tmp[i];
      vReal[i] = ((double)rawData[i]) / 2; // > -32768 ? (double)rawData[i] : 0.0;
      vImag[i] = 0.0;
    }

    memcpy(rawData, rawData_tmp, 512);

    FFT.dcRemoval();
    FFT.windowing(FFTWindow::Hamming, FFTDirection::Forward); /* Weigh data */
    FFT.compute(FFTDirection::Forward);                       /* Compute FFT */
    FFT.complexToMagnitude();                                 /* Compute magnitudes */

    u8g2.firstPage();
    do
    {
      for (int i = 0; i < (SAMPLES / 2) / 2; i++)
      {
        int barHeight = (int)(vReal[i] / 10);
        if (barHeight > 64)
          barHeight = 64;
        if (last_height[i] > 64 - barHeight)
          last_height[i] = 64 - barHeight;
        else if (last_height[i] < 63)
          last_height[i] += line_down_speed;
        // u8g2.drawLine(i * 8-8,last_height[i],7,2);
        u8g2.drawLine(i * 8 - 8, (int)last_height[i], i * 8 - 2, last_height[i]);
        u8g2.drawBox(i * 8 - 8, 64 - barHeight, 7, barHeight);
      }

      if (showframe)
      {
        frameCount++;
        unsigned long currentTime = millis();
        if (currentTime - lastTime >= 1000) // Update FPS every second
        {
          fps = frameCount / ((currentTime - lastTime) / 1000.0);
          frameCount = 0;
          lastTime = currentTime;
        }
        char fpsText[16];
        snprintf(fpsText, sizeof(fpsText), "FPS: %.2f", fps);
        u8g2.setFont(u8g2_font_6x10_tr); 
        u8g2.drawStr(0, 10, fpsText); 
      }
    } while (u8g2.nextPage());
    vTaskDelay(1 / portTICK_PERIOD_MS);
  }
}

void wait_user_release_button(int button)
{
  u8g2.setFont(u8g2_font_wqy16_t_gb2312);
  while (digitalRead(button) == LOW)
  { // 避免用户刚才按住还没放手
    static int start_angle = 0;
    static int end_angle = 1;
    u8g2.firstPage();
    do
    {
      u8g2.setCursor(22, 30);
      u8g2.print("请放开按键..");
      u8g2.drawArc(60, 52, 8, start_angle, end_angle);
      u8g2.drawArc(60, 52, 9, start_angle, end_angle);
    } while (u8g2.nextPage());
    if (start_angle == 250)
    {
      end_angle = 1;
      start_angle = 0;
    }
    if (end_angle < 251)
      end_angle += 1;
    else if (end_angle == 251)
      start_angle += 1;
  }
}