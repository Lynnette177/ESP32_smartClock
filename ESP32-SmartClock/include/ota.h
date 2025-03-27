#include "includes.h"

String global_server_ver = "";
void progressCallBack(size_t currSize, size_t totalSize)
{
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_wqy12_t_gb2312);
    u8g2.setCursor(10, 10);
    u8g2.print("正在更新固件...");
    u8g2.setCursor(10, 24);
    u8g2.print("更新进度: ");
    // 计算进度百分比
    int progress = (currSize * 100) / totalSize;
    // 绘制进度条背景
    u8g2.drawRFrame(10, 30, 90, 10,3);
    // 绘制进度条
    int r = 3;
    if ((progress * 0.9) < 7) r = max(0,(int)((progress * 0.9)/2-1));
    u8g2.drawRBox(10, 30, (progress * 0.9), 10,r);
    // 显示百分比
    u8g2.setCursor(105, 39);
    u8g2.print(progress);
    u8g2.print("%");
    u8g2.setCursor(10, 51);
    u8g2.print(currSize);
    u8g2.print("/");
    u8g2.print(totalSize);
    u8g2.print("Bytes");
    u8g2.setCursor(10, 61);
    u8g2.print(verloc);
    u8g2.print(" -> ");
    u8g2.print(global_server_ver);
    u8g2.sendBuffer();
}

bool check_update() //只有成功更新 返回真 而 不需要更新/连接失败 都是假
{
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_wqy12_t_gb2312);
    u8g2.setCursor(10, 15);
    u8g2.print("当前版本 ");
    u8g2.setCursor(100, 15);
    u8g2.print(verloc.c_str());
    u8g2.setCursor(10, 30);
    u8g2.print("连接更新服务器..");
    u8g2.sendBuffer();
    vTaskDelay(1000/portTICK_PERIOD_MS);

    HTTPClient http;
    http.begin(pi4_server_address + "/firmware");
    int httpCode = http.GET();

    if (httpCode == HTTP_CODE_OK)
    {
        String ver_on_server = http.getString();
        if (ver_on_server != verloc)
        {
            global_server_ver = ver_on_server;
            u8g2.clearBuffer();
            u8g2.setCursor(10, 10);
            u8g2.print("已请求更新");
            u8g2.setCursor(10, 24);
            u8g2.print("远端版本: ");
            u8g2.print(ver_on_server);
            u8g2.setCursor(10, 38);
            u8g2.print("本地版本: ");
            u8g2.print(verloc);
            u8g2.setCursor(10, 52);
            u8g2.print("轻触控制按钮以更新");
            u8g2.sendBuffer();
            while (!digitalRead(CONTROL_BUTTON) == LOW && !digitalRead(AI_BUTTON) == LOW)
            {
                vTaskDelay(1 / portTICK_PERIOD_MS);
            }
            while (digitalRead(CONTROL_BUTTON) == LOW || digitalRead(AI_BUTTON) == LOW)
            {
                if (digitalRead(CONTROL_BUTTON) == LOW)
                    break;
                if (digitalRead(AI_BUTTON) == LOW){
                    http.end();
                    return false;
                }
                vTaskDelay(1 / portTICK_PERIOD_MS);
            }
            // Request firmware update
            WiFiClient client;
            http.begin(client, pi4_server_address + "/firmware.bin");
            int httpCode = http.GET();

            if (httpCode == HTTP_CODE_OK)
            {
                int len = http.getSize();
                uint8_t buff[128] = {0};
                WiFiClient *stream = http.getStreamPtr();

                Update.begin(len);
                Update.onProgress(progressCallBack);
                while (http.connected() && (len > 0 || len == -1))
                {
                    size_t size = stream->available();
                    if (size)
                    {
                        int c = stream->readBytes(buff, ((size > sizeof(buff)) ? sizeof(buff) : size));
                        Update.write(buff, c);
                        if (len > 0)
                        {
                            len -= c;
                        }
                    }
                    delay(1);
                }

                if (Update.end(true))
                {
                    u8g2.clearBuffer();
                    u8g2.setCursor(10, 15);
                    u8g2.print("系统已更新");
                    u8g2.setCursor(10, 30);
                    u8g2.print(" Ver: " + verloc);
                    u8g2.print(" -> ");
                    u8g2.print(ver_on_server);
                    u8g2.setCursor(10, 45);
                    u8g2.print("轻点控制按钮来重启");
                    u8g2.sendBuffer();
                    Serial.println("Update finished!");
                    http.end();
                    return true;
                }
                else
                {
                    u8g2.clearBuffer();
                    u8g2.setCursor(10, 15);
                    u8g2.print("更新失败 请重试");
                    u8g2.setCursor(10, 30);
                    u8g2.print("轻点控制按钮来继续");
                    u8g2.sendBuffer();
                    Serial.println("Update error!");
                    Serial.println(Update.getError());
                }
            }
        }
        else
        {
            // u8g2.clearBuffer();
            // u8g2.setCursor(10, 15);
            // u8g2.print("已是最新版本");
            // u8g2.setCursor(10, 30);
            // u8g2.print("Local Ver: ");
            // u8g2.print(verloc);
            // u8g2.setCursor(10, 45);
            // u8g2.print("轻点控制按钮来继续");
            // u8g2.sendBuffer();
            http.end();
            return false;
        }
    }
    else
    {
        u8g2.clearBuffer();
        u8g2.setCursor(10, 15);
        u8g2.print("连接失败");
        u8g2.setCursor(10, 30);
        u8g2.print("轻点控制按钮来继续");
        u8g2.sendBuffer();
    }
    http.end();
    return false;
}

bool check_if_need_update(){
    HTTPClient http;
    http.begin(pi4_server_address + "/firmware");
    int httpCode = http.GET();

    if (httpCode == HTTP_CODE_OK)
    {
        String ver_on_server = http.getString();
        if (ver_on_server != verloc){
            http.end();
            return true;
        }
    }
    http.end();
    return false;
}