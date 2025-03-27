#pragma once
#include "includes.h"

#include <WiFi.h>
#include <HTTPClient.h>

void render_gpt_answer(char *text, int LINE_HEIGHT = 13, int SCREEN_WIDTH = 128, int char_count_inaline = 30)
{
    char line[128];
    // 找到分割符
    char *separator = strchr(text, '|');
    if (separator == NULL)
    {
        return; // 没有找到分割符，返回
    }

    // 将字符串分割为问题和答案
    *separator = '\0';
    char *question = text;
    char *answer = separator + 1;
    int render_start_dynamic_Y = LINE_HEIGHT;
    int target_now_Y = render_start_dynamic_Y;
    int y = 0;
    bool fast_scroll = false;
    while (!digitalRead(CONTROL_BUTTON) == LOW)
    {
        u8g2.firstPage();
        do
        {
            // 设置字体和初始位置
            u8g2.setFont(u8g2_font_wqy12_t_gb2312);
            y = target_now_Y;

            // 绘制问题
            u8g2.setCursor(0, y);
            u8g2.print("问: ");
            y += LINE_HEIGHT;
            int x = 0;
            int len = strlen(question);
            for (int i = 0; i < len; i += char_count_inaline)
            {
                line[char_count_inaline] = {0};
                strncpy(line, &question[i], char_count_inaline);
                // Serial.println(line);
                u8g2.setCursor(x, y);
                u8g2.print(line);
                y += LINE_HEIGHT;
            }

            // 绘制答案
            u8g2.setCursor(0, y);
            u8g2.print("答: ");
            y += LINE_HEIGHT;
            x = 0;
            len = strlen(answer);
            for (int i = 0; i < len; i += char_count_inaline)
            {
                line[char_count_inaline] = {0};
                strncpy(line, &answer[i], char_count_inaline);
                // Serial.println(line);
                u8g2.setCursor(x, y);
                u8g2.print(line);
                y += LINE_HEIGHT;
            }
        } while (u8g2.nextPage());
        bool switch_line_height = false;
        while ((!digitalRead(AI_BUTTON) == LOW && !digitalRead(CONTROL_BUTTON) == LOW) || target_now_Y != render_start_dynamic_Y)
        {
            if (target_now_Y != render_start_dynamic_Y)
            {
                if (!fast_scroll)
                    target_now_Y += (render_start_dynamic_Y - target_now_Y) / abs(render_start_dynamic_Y - target_now_Y);
                else
                {
                    if (abs(render_start_dynamic_Y - target_now_Y) < 5)
                    {
                        target_now_Y = render_start_dynamic_Y;
                        fast_scroll = false;
                    }
                    else
                        target_now_Y += 5 * (render_start_dynamic_Y - target_now_Y) / abs(render_start_dynamic_Y - target_now_Y);
                }
                break;
            }
            else
                fast_scroll = false;
            vTaskDelay(10 / portTICK_PERIOD_MS);
        }
        while (digitalRead(AI_BUTTON) == LOW)
        {
            switch_line_height = true;
        }
        if (switch_line_height)
        {
            render_start_dynamic_Y -= LINE_HEIGHT;
            if (y < 64)
            {
                render_start_dynamic_Y = LINE_HEIGHT;
                fast_scroll = true;
            }
            if (target_now_Y != render_start_dynamic_Y)
            {
                target_now_Y += (render_start_dynamic_Y - target_now_Y) / abs(render_start_dynamic_Y - target_now_Y);
            }
        }
    }
}

bool sendDataToServer(uint8_t *data, size_t size, int mode = 0)
{
    HTTPClient http;
    http.begin(pi4_server_address + "/upload");
    http.setTimeout(37856);
    http.addHeader("Content-Type", "application/octet-stream");
    http.addHeader("mode", String(mode));
    // 开始POST请求，发送数据
    int httpResponseCode = http.POST((uint8_t *)data, size);

    if (httpResponseCode > 0)
    {
        String response = http.getString();
        Serial.println(httpResponseCode);
        Serial.println(response);
        http.end();
        return true;
    }
    else
    {
        Serial.print("Error on sending POST: ");
        Serial.println(httpResponseCode);
        Serial.println(http.errorToString(httpResponseCode));
        http.end();
        return false;
    }
}
bool getDataFromServer(char *textArray, int mode = 0)
{
    HTTPClient http;
    http.begin(pi4_server_address + "/getanswer");
    http.addHeader("Content-Type", "application/octet-stream");
    http.addHeader("mode", String(mode));
    while (1)
    {
        int httpResponseCode = http.GET();
        if (httpResponseCode > 0)
        {
            String response = http.getString();
            if (response != "WAIT!")
            {
                strcpy(textArray, response.c_str());
                http.end();
                return true;
            }
            vTaskDelay(500 / portTICK_PERIOD_MS);
        }
        else
        {
            Serial.print("Error on sending POST: ");
            Serial.println(httpResponseCode);
            strcpy(textArray, "获取过程中发生了网络错误");
            Serial.println(http.errorToString(httpResponseCode));
            http.end();
            return false;
        }
    }
}

void gpt_mainloop()
{
    int max_record_time = 10;                                                 // 最大允许十秒钟的录音时间
    uint8_t *speechinArray = (uint8_t *)ps_malloc(1048576 * sizeof(uint8_t)); // 分1MB的PSRAM用来存储用户的语音输入
    char *textArray = (char *)ps_malloc(1048576 * sizeof(char));              // 分1MB的PSRAM用来存储用户的语音转换出来的文字
    uint8_t *singletime_record = new uint8_t[1024];                           // 每一次录制1kb
    while (1)
    {
        u8g2.setFont(u8g2_font_wqy12_t_gb2312);

        memset(speechinArray, 0, 1048576); // 清空两个数组
        memset(textArray, 0, 1048576);
        int start_rending_box_pos = 0;
        int plus_or_sub_box_pos = 1;
        while (!digitalRead(AI_BUTTON) == LOW)
        {
            u8g2.firstPage();
            do
            {
                u8g2.drawXBMP(40, 0, 48, 48, gImage_record_animation);
                u8g2.setDrawColor(0); // 绘制反色实心矩形(黑色)然后正常绘制城市字体达成遮挡效果
                u8g2.drawBox(42 + start_rending_box_pos, 4, 44 - start_rending_box_pos, 42);
                u8g2.setDrawColor(1);
                u8g2.setCursor(20, 61);
                u8g2.print("按下AI键来录制...");
            } while (u8g2.nextPage());
            start_rending_box_pos = (start_rending_box_pos + plus_or_sub_box_pos);
            if (start_rending_box_pos == 44 || start_rending_box_pos == 0)
                plus_or_sub_box_pos = -plus_or_sub_box_pos;
            if (digitalRead(CONTROL_BUTTON) == LOW)
            {
                goto end;
            }
            vTaskDelay(10 / portTICK_PERIOD_MS);
        } // 在用户按下按钮之前死等
        unsigned int start_recording_time = millis();
        int index_copy = 0;
        Serial.println("Begin recording");
        u8g2.firstPage();
        do
        {
            u8g2.drawXBMP(40, 0, 48, 48, gImage_wait_record);
            u8g2.setCursor(10, 61);
            u8g2.print("机器人聆听中 至多10s");
        } while (u8g2.nextPage());
        while (digitalRead(AI_BUTTON) == LOW && millis() - start_recording_time < max_record_time * 1000)
        { // 不允许录制超过十秒

            memset(singletime_record, 0, 1024);
            I2S_Read(singletime_record, 1024);
            memcpy(speechinArray + 44 + (index_copy * 1024), singletime_record, 1024);
            index_copy++;
        }
        CreateWavHeader(speechinArray, 1024 * index_copy);
        Serial.println(1024 * index_copy + 44);
        u8g2.firstPage();
        do
        {
            u8g2.drawXBMP(40, 0, 48, 48, gImage_upload);
            u8g2.setCursor(12, 61);
            u8g2.print("正在上传给机器人...");
        } while (u8g2.nextPage());
        bool upload_check = sendDataToServer(speechinArray, 1024 * index_copy + 44, globalConfig.wechat_alias[0]); // 不同的数字代表不同用户 取备注第一个字母的值即可
        bool get_check = false;
        if (upload_check)
        {
            u8g2.firstPage();
            do
            {
                u8g2.drawXBMP(40, 0, 48, 48, gImage_getfromserver);
                u8g2.setCursor(22, 61);
                u8g2.print("机器人思考中...");
            } while (u8g2.nextPage());
            get_check = getDataFromServer(textArray, globalConfig.wechat_alias[0]);
            if (!get_check)
                break;
            // Serial.println(textArray);
            render_gpt_answer(textArray);
        }
        if (!upload_check || !get_check)
        {
            u8g2.firstPage();
            do{
               u8g2.setCursor(10,20);
               u8g2.print("服务器或网络异常");
               u8g2.setCursor(10,40);
               u8g2.print("单击控制按钮重试");
            }while(u8g2.nextPage());
            while (!digitalRead(CONTROL_BUTTON) == LOW)
            {
                vTaskDelay(10 / portTICK_PERIOD_MS);
            }
        }
        while (digitalRead(CONTROL_BUTTON) == LOW)
        {
            vTaskDelay(10 / portTICK_PERIOD_MS);
        }
    }
end:
    free(speechinArray);
    free(textArray);
    delete[] singletime_record; // 删除录制缓冲区
}