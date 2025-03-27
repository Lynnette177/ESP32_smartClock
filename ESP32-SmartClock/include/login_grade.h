#pragma once
#include "includes.h"
#include <regex>
// CONFIG_ARDUINO_LOOP_STACK_SIZE 8192->102400
String login_url = "https://jwgl.bupt.edu.cn/Logon.do?method=logon&flag=sess";
String login_url_geturl = "https://jwgl.bupt.edu.cn/Logon.do?method=logon";
String grade_url = "https://jwgl.bupt.edu.cn/jsxsd/kscj/cjcx_list?kksj=2023-2024-2";

bool jw_login(char *acc, char *pass, char *newcookie)
{

    HTTPClient http;
    http.begin(login_url);
    const char *headerKeys[] = {"Server", "Content-Type", "Date", "Location", "Set-Cookie"};
    http.collectHeaders(headerKeys, sizeof(headerKeys) / sizeof(headerKeys[0]));
    int httpCode = http.POST("");
    Serial.println(httpCode);
    String cookieString = http.header("Set-Cookie");
    String key = http.getString();
    Serial.println(cookieString);
    Serial.println(key);
    char *output = new char[1024];
    encode(acc, pass, (char *)key.c_str(), output);
    http.end();

    String routeValue;
    String jsessionIdValue;
    int routeIndex = cookieString.indexOf("route=");
    if (routeIndex != -1)
    {
        int endIndex = cookieString.indexOf(';', routeIndex); // 找到；位置
        if (endIndex == -1)
        {
            endIndex = cookieString.length(); // 找长度
        }
        routeValue = cookieString.substring(routeIndex, endIndex);
        Serial.println("route value: " + routeValue);
    }
    else
    {
        Serial.println("route field not found");
    }
    // 获取JSESSIONID
    int jsessionIdIndex = cookieString.indexOf("JSESSIONID=");
    if (jsessionIdIndex != -1)
    {
        int endIndex = cookieString.indexOf(';', jsessionIdIndex); // 找到；位置
        if (endIndex == -1)
        {
            endIndex = cookieString.length(); // 总长度
        }
        jsessionIdValue = cookieString.substring(jsessionIdIndex, endIndex);
        Serial.println("JSESSIONID value: " + jsessionIdValue);
    }
    else
    {
        Serial.println("JSESSIONID field not found");
    }

    http.begin(login_url_geturl);
    http.collectHeaders(headerKeys, sizeof(headerKeys) / sizeof(headerKeys[0]));
    http.addHeader("Host", "jwgl.bupt.edu.cn");
    http.addHeader("Connection", "keep-alive");
    http.addHeader("Content-Length", "106");
    http.addHeader("Cache-Control", "max-age=0");
    http.addHeader("sec-ch-ua", "\"Not/A)Brand\";v=\"8\", \"Chromium\";v=\"126\", \"Microsoft Edge\";v=\"126\"");
    http.addHeader("sec-ch-ua-mobile", "?0");
    http.addHeader("sec-ch-ua-platform", "\"Windows\"");
    http.addHeader("Upgrade-Insecure-Requests", "1");
    http.addHeader("Origin", "https://jwgl.bupt.edu.cn");
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");
    http.addHeader("User-Agent", "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/126.0.0.0 Safari/537.36 Edg/126.0.0.0");
    http.addHeader("Accept", "text/html,application/xhtml+xml,application/xml;q=0.9,image/avif,image/webp,image/apng,*/*;q=0.8,application/signed-exchange;v=b3;q=0.7");
    http.addHeader("Sec-Fetch-Site", "same-origin");
    http.addHeader("Sec-Fetch-Mode", "navigate");
    http.addHeader("Sec-Fetch-User", "?1");
    http.addHeader("Sec-Fetch-Dest", "document");
    http.addHeader("Referer", "https://jwgl.bupt.edu.cn/");
    http.addHeader("Accept-Encoding", "gzip, deflate, br, zstd");
    http.addHeader("Accept-Language", "zh-CN,zh;q=0.9,en;q=0.8,en-GB;q=0.7,en-US;q=0.6");
    String cookie = routeValue + "; " + jsessionIdValue + "; Path=/;";
    Serial.println(cookie);
    http.addHeader("Cookie", cookie);
    String payload = "userAccount=&userPassword=&encoded=" + String(output);
    Serial.println(payload);
    int req = http.POST(payload);
    if (req != 302)
    {
        return false;
    }
    String redirectLocation = http.header("Location");
    Serial.println(redirectLocation);
    http.end();

    http.begin(redirectLocation);
    http.addHeader("Cookie", cookie);
    http.collectHeaders(headerKeys, sizeof(headerKeys) / sizeof(headerKeys[0]));
    http.GET();
    String session_cookie = http.header("Set-Cookie");
    jsessionIdIndex = session_cookie.indexOf("JSESSIONID=");
    if (jsessionIdIndex != -1)
    {
        int endIndex = session_cookie.indexOf(';', jsessionIdIndex); // 重新找到新的jid
        if (endIndex == -1)
        {
            endIndex = session_cookie.length();
        }
        jsessionIdValue = session_cookie.substring(jsessionIdIndex, endIndex);
        Serial.println("JSESSIONID value: " + jsessionIdValue);
    }
    else
    {
        Serial.println("JSESSIONID field not found");
    }
    cookie = routeValue + "; " + jsessionIdValue + "; Path=/;";
    http.end();
    strcpy(newcookie, cookie.c_str());
    delete[] output;
    return true;
}
void query_grade(char *cookie, char *output)
{
    HTTPClient http;
    http.begin(grade_url);
    http.addHeader("Cookie", cookie);
    int status = http.GET();
    if (status <= 0)
    {
        Serial.println("HTTP GET failed");
        http.end(); // 释放 HTTPClient 资源
        return;
    }
    String courseName, score, credits, gpa;

    std::regex reCourses("所修门数:(\\d+)");
    std::regex reCredits("所修总学分:(\\d+)");
    std::string html = http.getString().c_str();
    http.end();
    std::smatch match;
    if (std::regex_search(html, match, reCourses))
    {
        strcpy(output, "所修门数:");
        strcat(output, match[1].str().c_str());
    }
    if (std::regex_search(html, match, reCredits))
    {
        strcat(output, "\n所修总学分:");
        strcat(output, match[1].str().c_str());
    }
    std::regex reCourseInfo("<td align=\"left\">(\\d+)</td>\\s*<td align=\"left\">(.*?)</td>\\s*<td align=\"left\"></td>[\\s\\S]*?<td style=\" \">\\s*(\\d+)\\s*</td>[\\s\\S]*?<td>(\\d+)</td>\\s*<td>(\\d+)</td>[\\s\\S]*?<td>([\\d.]+)</td>");

    std::sregex_iterator iter(html.begin(), html.end(), reCourseInfo);
    std::sregex_iterator end;

    while (iter != end)
    {
        std::smatch match = *iter;
        std::string courseID = match[1].str();
        std::string courseName = match[2].str();
        std::string grade = match[3].str();
        std::string credit = match[4].str();
        std::string hours = match[5].str();
        std::string gpa = match[6].str();
        String result_str = "\n课程名称: " + String(courseName.c_str()) + "\n课程编号: " + String(courseID.c_str()) + "\n成绩: " + String(grade.c_str()) + "\n学分: " + String(credit.c_str()) + "\n学时: " + String(hours.c_str()) + "\n绩点: " + String(gpa.c_str());
        strcat(output, result_str.c_str());
        ++iter;
    }
    return;
}
bool wxjw_login(char *acc, char *encoded_pass, char *token)
{
    HTTPClient http;
    JsonDocument doc; // 分配内存,动态
    String url = "http://jwglweixin.bupt.edu.cn/bjyddx/login";
    String userNo = acc;
    String pwd = encoded_pass;
    String encode = "1";
    String captchaData = "";
    String codeVal = "";
    String postData = "userNo=" + userNo + "&pwd=" + pwd + "&encode=" + encode + "&captchaData=" + captchaData + "&codeVal=" + codeVal;
    http.begin(url);
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");
    int httpResponseCode = http.POST(postData);
    String responseBody = http.getString();
    if (httpResponseCode > 0)
    {
        deserializeJson(doc, responseBody);
        String someValue = doc["data"]["token"].as<String>();
        strcpy(token, someValue.c_str());
        http.end();
        return true;
    }
    else
    {
        http.end();
        return false;
    }
}
bool get_current_term(char *token, char *semester)
{
    HTTPClient http;
    JsonDocument doc; // 分配内存,动态
    String url = "http://jwglweixin.bupt.edu.cn/bjyddx/currentTerm";
    http.begin(url);
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");
    http.addHeader("Token", token);
    int httpResponseCode = http.POST("");
    String responseBody = http.getString();
    if (httpResponseCode > 0)
    {
        deserializeJson(doc, responseBody);
        String someValue = doc["data"][0]["semesterId"].as<String>();
        if (someValue != "null")
        {
            strcpy(semester, someValue.c_str());
            http.end();
            return true;
        }
        http.end();
        return false;
    }
    else
    {
        http.end();
        return false;
    }
}
bool query_grade_by_term(char *token, char *semester, int *grade_num, float *avg_gpa, char *grade, int mode = 1)
{
    // mode = 0为首次， mode = 1为最好， mode = 2为全部
    HTTPClient http;
    JsonDocument doc; // 分配内存,动态
    String url = "http://jwglweixin.bupt.edu.cn/bjyddx/student/termGPA";
    String postData;
    if (mode == 1)
    {
        postData = "semester=" + String(semester) + "&type=1";
    }
    else if (mode == 0)
    {
        postData = "semester=" + String(semester) + "&type=0";
    }
    else
        postData = "semester=" + String(semester) + "&type=";
    http.begin(url);
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");
    http.addHeader("Token", token);
    int httpResponseCode = http.POST(postData);

    String all_grade_str;
    String responseBody = http.getString();
    String cpriority1 = "必修专业课：\n";
    String cpriority2 = "必修非专业课：\n";
    String cpriority3 = "选修专业课：\n";
    String cpriority4 = "选修非专业课：\n";
    bool cat1 = false;
    bool cat2 = false;
    bool cat3 = false;
    bool cat4 = false;
    if (httpResponseCode > 0)
    {
        deserializeJson(doc, responseBody);
        if (doc["code"].as<int>() == 0)
        {
            http.end();
            return false;
        }
        *grade_num = doc["data"][0]["achievement"].size();
        *avg_gpa = doc["data"][0]["pjxfjd"].as<float>();
        for (int i = 0; i < *grade_num; i++)
        {
            String score = doc["data"][0]["achievement"][i]["fraction"].as<String>();
            float gpa = get_gpa_by_score(atoi(score.c_str()));
            String credit = doc["data"][0]["achievement"][i]["credit"].as<String>();
            String curriculumAttributes = doc["data"][0]["achievement"][i]["curriculumAttributes"].as<String>();
            String courseNature = doc["data"][0]["achievement"][i]["courseNature"].as<String>();
            String courseName = doc["data"][0]["achievement"][i]["courseName"].as<String>();
            if (curriculumAttributes == "必修")
            {
                if (courseNature == "专业课")
                {
                    cat1 = true;
                    cpriority1 += courseName + "\n" + score + "分/" + String(gpa, 2) + "\n学分：" + credit + "\n";
                }
                else
                {
                    cat2 = true;
                    cpriority2 += courseName + "\n" + score + "分/" + String(gpa, 2) + "\n学分：" + credit + "\n";
                }
            }
            else
            { // 选修的
                if (courseNature == "专业课")
                {
                    cat3 = true;
                    cpriority3 += courseName + "\n" + score + "分/" + String(gpa, 2) + "\n学分：" + credit + "\n";
                }
                else
                {
                    cat4 = true;
                    cpriority4 += courseName + "\n" + score + "分/" + String(gpa, 2) + "\n学分：" + credit + "\n";
                }
            }
        }
        if (cat1)
            all_grade_str += cpriority1;
        if (cat2)
            all_grade_str += cpriority2;
        if (cat3)
            all_grade_str += cpriority3;
        if (cat4)
            all_grade_str += cpriority4;
        strcpy(grade, all_grade_str.c_str());
        http.end();
        return true;
    }
    else
    {
        http.end();
        return false;
    }
}
int get_last_teaching_week(char * token){
    HTTPClient http;
    JsonDocument doc; // 分配内存,动态
    String url = "http://jwglweixin.bupt.edu.cn/bjyddx/teachingWeek";
    http.begin(url);
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");
    http.addHeader("Token", token);
    int repcode = http.POST("");
    if (repcode > 0){
        String repbody = http.getString();
        deserializeJson(doc, repbody);
        int last_week = doc["data"].size();
        http.end();
        return last_week;
    }
    http.end();
    return -1;
}
bool query_schedule(char *token, char *schedule_out, bool *is_examine_week, bool *is_xiaoxueqi_out,int last_teaching_week, bool* is_on_vacation)
{
    if (last_teaching_week < 0)//如果结束周都没查到直接退出
      return false;
    // 这个函数仅用于判断当前是否是考试周和小学期
    // 输出的schedule是当日的信息 根据周几判断
    String schedule;
    HTTPClient http;
    JsonDocument doc; // 分配内存
    String url = "http://jwglweixin.bupt.edu.cn/bjyddx/student/curriculum";
    http.begin(url);
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");
    http.addHeader("Token", token);
    String post_data = "week=&kbjcmsid="; // 空的代表查询今天
    int httpResponseCode = http.POST(post_data);
    String responseBody = http.getString();

    //Serial.println(responseBody);

    if (httpResponseCode > 0)
    {
        deserializeJson(doc, responseBody);

        String week = doc["data"][0]["week"].as<String>();//当前是第几周
        String first_week_date_str = doc["data"][0]["date"][0]["mxrq"].as<String>();//第一周的时间
        struct tm first_week_time_tm = {0};
        strptime(first_week_date_str.c_str(), "%Y-%m-%d", &first_week_time_tm);
        time_t first_week_time_t = mktime(&first_week_time_tm);

        String weekday = doc["data"][0]["weekday"].as<String>();
        int week_day_num = 0;
        if (weekday == "null")
        {
            http.end();
            return false;
        }
        if (weekday == "一")
        {
            week_day_num = 1;
        }
        else if (weekday == "二")
        {
            week_day_num = 2;
        }
        else if (weekday == "三")
        {
            week_day_num = 3;
        }
        else if (weekday == "四")
        {
            week_day_num = 4;
        }
        else if (weekday == "五")
        {
            week_day_num = 5;
        }
        else if (weekday == "六")
        {
            week_day_num = 6;
        }
        else if (weekday == "日")
        {
            week_day_num = 7;
        }
        int course_num = doc["data"][0]["courses"].size();
        if (course_num == 0)
        {
            *is_on_vacation = true;
            return true;
        }
        bool is_in_examine = false;
        int weektype = 0;
        bool is_xiaoxueqi = false;
        String xiaoxueqi_class_name = doc["data"][0]["bz"];
        bool copy_xiaoxueqi_once = true; // 小学期信息应该只复制一次
        if (xiaoxueqi_class_name == "null")
            is_xiaoxueqi = false;
        for (int i = 0; i < course_num; i++)
        {
            String week_info = doc["data"][0]["courses"][i]["classWeekDetails"].as<String>();
            int tmpvav = check_weektype_by_comma(week_info);
            if (tmpvav < 0)
            {
                http.end();
                return false;
            }
            weektype += tmpvav;
            String class_name = doc["data"][0]["courses"][i]["courseName"].as<String>();
            //Serial.printf("%s %s %d", xiaoxueqi_class_name.c_str(), class_name.c_str(), xiaoxueqi_class_name.indexOf(class_name) );
            if (xiaoxueqi_class_name.indexOf(class_name) != -1) //这个地方用的是 bz包含 class_name即可判断为小学期。因为多个专业的不同小学期都会写在bz里面
                is_xiaoxueqi = true;
            if (week_day_num != doc["data"][0]["courses"][i]["weekDay"].as<int>()) // 不是今日的，不拷贝
                continue;
            if (!is_xiaoxueqi || ((is_xiaoxueqi) && copy_xiaoxueqi_once))
            {
                schedule += doc["data"][0]["courses"][i]["courseName"].as<String>() + "\n地点：\n" + doc["data"][0]["courses"][i]["location"].as<String>() + "\n时间：\n" + doc["data"][0]["courses"][i]["startTime"].as<String>() + "~" + doc["data"][0]["courses"][i]["endTIme"].as<String>() + "\n";
                copy_xiaoxueqi_once = false;
            }
        }
        if (nowtimet - first_week_time_t > last_teaching_week * 604800){//最后一个有事的周结束了之后，置为已经放假
            *is_xiaoxueqi_out = false;//如果是刚过最后一个周，当成考试周再查一个周的成绩
            *is_examine_week = true;
            if (nowtimet - first_week_time_t > (last_teaching_week + 1) * 604800){//如果过了一个周了，就设置为放假，成绩也不查了。就剩一个时钟
             if(is_on_vacation != NULL)
                *is_on_vacation = true;
            }
            http.end();
            return true;
        }
        if(is_on_vacation != NULL)
          *is_on_vacation = false;
        if (is_xiaoxueqi && copy_xiaoxueqi_once)
            is_xiaoxueqi = false; // 如果判断出 这个周是小学期周 但是并没有拷贝过小学期的内容
        // 说明已经到了小学期结束的日子 当日无课 故置为假即可
        if (weektype > 0)
            *is_examine_week = true;
        else if (weektype == 0)
            *is_examine_week = false;
        *is_xiaoxueqi_out = is_xiaoxueqi;
        if (schedule_out != NULL) // 如果不是只查状态（考试周/小学期）那么就把日程拷贝出去
            strcpy(schedule_out, schedule.c_str());
        http.end();
        return true;
    }
    else
    {
        http.end();
        return false;
    }
}
bool query_if_finished_and_get_all_exam(char *token, bool *finished_all_ptr, char *schedule, char *closest)
{
    HTTPClient http;
    JsonDocument doc; // 分配内存
    String url = "http://jwglweixin.bupt.edu.cn/bjyddx/student/examinationArrangement";
    http.begin(url);
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");
    http.addHeader("Token", token);
    int responsecode = http.POST("");
    if (!(responsecode > 0))
    {
        http.end();
        return false;
    }
    deserializeJson(doc, http.getString());
    int exam_num = doc["data"].size();
    if (!exam_num > 0) // 如果查不到任何考试信息 说明考完了
    {                  // 因为一定是判断是小学期或者考试周才能进这个函数，但是查不到任何考试信息，就说明已经结束了。或者是学期开始的小学期
        http.end();
        *finished_all_ptr = true;
        return true;
    }
    String closest_exam = "";
    String all_unfinished_exam = "";
    time_t min_time_delta = nowtimet;
    bool finished_all = true;
    int exam_count = 0;
    Serial.println(http.getString());
    for (int i = 0; i < exam_num; i++)
    {
        if (doc["data"][i]["examinationPlace"] == "_")
        {
            continue; // 忽略老师自主排课的考核方式
        }
        String time_str = doc["data"][i]["time"].as<String>();
        struct tm tm;
        time_t t;
        if (strptime(time_str.c_str(), "%Y-%m-%d %H:%M", &tm) == NULL)
        {
            continue; // 错误处理
        }
        tm.tm_isdst = -1; // 不使用夏令时
        t = mktime(&tm);
        if (nowtimet < t)
        {
            finished_all = false;
            exam_count++;
            if (t - nowtimet < min_time_delta)
            {
                all_unfinished_exam = doc["data"][i]["courseName"].as<String>() + "\n" + doc["data"][i]["time"].as<String>() + "\n" + doc["data"][i]["examinationPlace"].as<String>().substring(doc["data"][i]["examinationPlace"].as<String>().length() - 4) + "\n" + all_unfinished_exam;
                min_time_delta = t - nowtimet;
                closest_exam = doc["data"][i]["courseName"].as<String>() + "\n" + doc["data"][i]["time"].as<String>() + "\n" + doc["data"][i]["examinationPlace"].as<String>().substring(doc["data"][i]["examinationPlace"].as<String>().length() - 4);
            }
            else
                all_unfinished_exam += doc["data"][i]["courseName"].as<String>() + "\n" + doc["data"][i]["time"].as<String>() + "\n" + doc["data"][i]["examinationPlace"].as<String>().substring(doc["data"][i]["examinationPlace"].as<String>().length() - 4) + "\n";
        }
    }
    if (!finished_all)
    {
        all_unfinished_exam.replace(" ", "\n");
        closest_exam.replace(" ", "\n");
        strcpy(schedule, all_unfinished_exam.c_str());
        strcpy(closest, closest_exam.c_str());
    }
    *finished_all_ptr = finished_all;
    http.end();
    return true;
}
bool normal_week_query_today_class(char *token, char *output, bool *is_occupying, char *occupy_class, int *all_count, int *finished_count, int *unfinish_count, time_t *outdelta, char *outnext_class, int *weekday_out)
{
    HTTPClient http;
    JsonDocument doc; // 分配内存
    String url = "http://jwglweixin.bupt.edu.cn/bjyddx/student/curriculum";
    http.begin(url);
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");
    http.addHeader("Token", token);
    String post_data = "week=&kbjcmsid="; // 空的代表查询今天
    int httpResponseCode = http.POST(post_data);
    String responseBody = http.getString();
    deserializeJson(doc, responseBody);
    int week_day_num = 0;
    String weekday = doc["data"][0]["weekday"].as<String>();
    if (weekday == "null")
    {
        http.end();
        return false;
    }
    if (weekday == "一")
    {
        week_day_num = 1;
    }
    else if (weekday == "二")
    {
        week_day_num = 2;
    }
    else if (weekday == "三")
    {
        week_day_num = 3;
    }
    else if (weekday == "四")
    {
        week_day_num = 4;
    }
    else if (weekday == "五")
    {
        week_day_num = 5;
    }
    else if (weekday == "六")
    {
        week_day_num = 6;
    }
    else if (weekday == "日")
    {
        week_day_num = 7;
    }
    if (week_day_num == 0)
    {
        http.end();
        return false;
    }
    int course_num = doc["data"][0]["courses"].size();
    if (course_num == 0)
    {
        http.end();
        return false;
    }

    // week_day_num = 1;//测试！

    int today_course_count = 0;
    int finished_today_course_count = 0;
    int today_unfished_class = 0;
    bool is_on_class = false;
    String on_class_name = "";
    String next_class_name = "";
    String all_unfinish_class_name = "";
    time_t delta = nowtimet;
    time_t time_remain_to_end_class = 0; // 剩余多长时间下课

    for (int i = 0; i < course_num; i++)
    {
        int weekday_of_course = doc["data"][0]["courses"][i]["weekDay"].as<int>();
        if (weekday_of_course != week_day_num)
            continue;
        String start_time_str = doc["data"][0]["courses"][i]["startTime"].as<String>();
        String end_time_str = doc["data"][0]["courses"][i]["endTIme"].as<String>();
        time_t start_timt_t;
        time_t end_time_t;
        struct tm starttm= {0};
        struct tm endtm = {0};
        String date = thetimenow.substring(0, thetimenow.indexOf('\n'));
        String start_full_time = String(localtime(&nowtimet)->tm_year + 1900) + "-" + date + " " + start_time_str;
        String end_full_time = String(localtime(&nowtimet)->tm_year + 1900) + "-" + date + " " + end_time_str;
        start_full_time.replace(".", "-");
        end_full_time.replace(".", "-");

        if (strptime(start_full_time.c_str(), "%Y-%m-%d %H:%M", &starttm) == NULL)
        {
            continue; // 错误处理
        }
        if (strptime(end_full_time.c_str(), "%Y-%m-%d %H:%M", &endtm) == NULL)
        {
            continue; // 错误处理
        }

        starttm.tm_isdst = -1; // 不使用夏令时
        endtm.tm_isdst = -1;   // 不使用夏令时
        start_timt_t = mktime(&starttm);
        end_time_t = mktime(&endtm);

        today_course_count++;
        if (end_time_t < nowtimet)
        {
            finished_today_course_count++;
        }
        else if (start_timt_t < nowtimet && end_time_t > nowtimet)
        {
            is_on_class = true;
            on_class_name = doc["data"][0]["courses"][i]["courseName"].as<String>();
            on_class_name += "\n";
            on_class_name += start_time_str;
            on_class_name += "\n";
            on_class_name += doc["data"][0]["courses"][i]["location"].as<String>();
            time_remain_to_end_class = end_time_t - nowtimet;
        }
        else if (start_timt_t > nowtimet)
        { // 还没上的课
            String name = doc["data"][0]["courses"][i]["courseName"].as<String>();
            String location = doc["data"][0]["courses"][i]["location"].as<String>();
            if (start_timt_t - nowtimet < delta)
            {
                delta = start_timt_t - nowtimet;
                next_class_name = name + "\n" + start_time_str + "\n" + location;
            }
            all_unfinish_class_name += name + "\n" + start_time_str + "\n" + location + "\n";
        }
    }
    today_unfished_class = today_course_count - finished_today_course_count;
    // 把有用的信息拷贝出去
    strcpy(output, all_unfinish_class_name.c_str());
    *is_occupying = is_on_class;
    strcpy(occupy_class, on_class_name.c_str());
    *all_count = today_course_count;
    *finished_count = finished_today_course_count;
    *unfinish_count = today_unfished_class;
    *weekday_out = week_day_num;
    if (is_on_class)
        *outdelta = time_remain_to_end_class;
    else
        *outdelta = delta;
    strcpy(outnext_class, next_class_name.c_str());
    http.end();
    return true;
}
bool push_over(const char *message)
{
    String pushover_url = PUSHOVER_URL;
    JsonDocument doc;
    doc["token"] = globalConfig.pushoverToken;
    doc["message"] = String(message);
    doc["user"] = globalConfig.pushoverUserkey;
    HTTPClient http;
    http.begin(pushover_url);
    http.addHeader("Content-Type", "application/json");
    String requestBody;
    serializeJson(doc, requestBody);
    int responseCode = http.POST(requestBody);
    Serial.println(http.getString());
    if (responseCode > 0)
    {
        http.end();
        return true;
    }
    http.end();
    return false;
}
bool push_to_wxbot(char *message, int num = 0)
{
    String url = strcmp(connected_WiFi.c_str(),"TP-LINK_zky") == 0 ? wxbot_url_inhome : wxbot_url;
    HTTPClient http;
    http.begin(url);
    http.addHeader("Content-Type", "application/json");
    JsonDocument doc;
    doc["to"]["alias"] = globalConfig.wechat_alias;
    if (num != 0)
        doc["data"]["content"] = String("总共出了") + String(num) + "门:\n" + String(message);
    else
        doc["data"]["content"] = String(message);
    String requestBody;
    serializeJson(doc, requestBody);
    int httpResponseCode = http.POST(requestBody);
    if (httpResponseCode == 200)
    {
        String response = http.getString();
        // Serial.println(httpResponseCode);
        // Serial.println(response);
        http.end();
        return true;
    }
    else
    {
        http.end();
        return push_over(doc["data"]["content"].as<String>().c_str());
    }
}