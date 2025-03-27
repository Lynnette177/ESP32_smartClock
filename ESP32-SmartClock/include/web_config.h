#pragma once
#include "includes.h"
// GPIO0 is the boot button
void handleConfigClient();

void saveStruct(const char *key, const configStruct &data)
{
    preferences.begin("storage", false);
    preferences.putBytes(key, &data, sizeof(configStruct));
    preferences.end();
}

bool loadStruct(const char *key)
{
    preferences.begin("storage", false);
    size_t bytesRead = preferences.getBytes(key, &globalConfig, sizeof(configStruct));
    preferences.end();

    // 如果没有读取到任何字节，则返回一个全零的结构体
    if (bytesRead == 0)
    {
        globalConfig = {};
        return false;
    }
    return true;
}

void print_config()
{
    Serial.printf("Loaded: wifi_ssid1='%s' wifi_pwd1='%s'\nwifi_ssid2='%s' wifi_pwd2='%s'\nwifi_ssid3='%s' wifi_pwd3='%s'\n", globalConfig.wifi_ssid1, globalConfig.wifi_pass1, globalConfig.wifi_ssid2, globalConfig.wifi_pass2, globalConfig.wifi_ssid3, globalConfig.wifi_pass3);
    Serial.printf("Loaded: jw_acc='%s' jw_pwd='%s'\n", globalConfig.bupt_jw_account, globalConfig.bupt_jw_password);

    if (globalConfig.auto_locate)
        Serial.println("自动定位！");
    else
    {
        Serial.printf("Weather: Province:%s City:%s County:%s\n", globalConfig.ProvinceName, globalConfig.CityName, globalConfig.CountyName);
    }

    if (globalConfig.offline_mode)
        Serial.println("离线模式！");

    Serial.println("WeChat Alias: " + String(globalConfig.wechat_alias));
    Serial.println("Pushover Device name: " + String(globalConfig.pushoverDevicename));
    Serial.println("Pushover token: " + String(globalConfig.pushoverToken));
    Serial.println("Pushover userkey: " + String(globalConfig.pushoverUserkey));
    Serial.println("Tencent API key: " + String(globalConfig.tencent_api_key));
    Serial.printf("Weather Province: %s\nCity: %s\nCounty: %s\n", globalConfig.ProvinceName, globalConfig.CityName, globalConfig.CountyName);

    // 打印定时任务设置
    Serial.println("定时任务设置:");
    for (int i = 0; i < globalConfig.schedule_count; i++)
    {
        its_time_to_do task = globalConfig.schedule[i];
        Serial.printf("任务 %d:\n", i + 1);
        Serial.printf("启用: %s\n", task.isEnabled() ? "是" : "否");
        Serial.printf("周几: %d\n", task.getWDay());
        Serial.printf("小时: %d\n", task.getHour());
        Serial.printf("分钟: %d\n", task.getMinutes());
        Serial.printf("命令: %s\n", task.getCommand());
    }
}

// Set web server port number to 80
WiFiServer server(80);
void loop_in_config()
{

    WiFi.disconnect();
    WiFi.mode(WIFI_MODE_AP);
    Serial.println("\nStarting config mode!");
    WiFi.softAP("LynnetteConfig");
    Serial.println(WiFi.softAPIP());
    server.begin();
    int pic_mode = 0;
    while (1)
    {
        u8g2.firstPage();
        do
        {
            if (pic_mode < 50)
                u8g2.drawXBMP(8, 8, 48, 48, gImage_wifiempty);
            else if (pic_mode < 100)
                u8g2.drawXBMP(8, 8, 48, 48, gImage_wifi13);
            else if (pic_mode < 150)
                u8g2.drawXBMP(8, 8, 48, 48, gImage_wifi23);
            else if (pic_mode < 200)
                u8g2.drawXBMP(8, 8, 48, 48, gImage_wififull);
            else
                u8g2.drawXBMP(8, 8, 48, 48, gImage_httpqrcode);
            u8g2.setFont(u8g2_font_wqy12_t_gb2312);
            u8g2.setCursor(64, 14);
            u8g2.print("网页配置中");
            u8g2.setCursor(64, 28);
            u8g2.print("请连接WiFi");
            u8g2.setFont(u8g2_font_3x5im_mr);
            u8g2.setCursor(64, 36);
            u8g2.print("LynnetteConfig");
            u8g2.setFont(u8g2_font_wqy12_t_gb2312);
            u8g2.setCursor(64, 50);
            u8g2.print("扫码或访问");
            u8g2.setFont(u8g2_font_3x5im_mr);
            u8g2.setCursor(64, 60);
            u8g2.print("192.168.4.1");
        } while (u8g2.nextPage());
        if(pic_mode == 249){
            vTaskDelay(2000 / portTICK_PERIOD_MS);
        }
        pic_mode = (pic_mode + 1) % 250;
        handleConfigClient();
    }
}
void handleConfigClient()
{
    String cl_ssid1;
    String cl_pwd1;
    String cl_ssid2;
    String cl_pwd2;
    String cl_ssid3;
    String cl_pwd3;
    String cl_jw_account;
    String cl_jw_password;
    String cl_pushovertoken;
    String cl_pushoveruserkey;
    String cl_pushoverdevicename;
    String cl_tencentapikey;
    String cl_auto_locate;
    String cl_wechat_alias;
    String cl_weather_province;
    String cl_weather_city;
    String cl_weather_county;
    String cl_offline_mode;

    String header;

    WiFiClient client = server.available();
    if (client)
    {
        Serial.println("New Client.");
        String currentLine = "";
        while (client.connected())
        {
            if (client.available())
            {
                char c = client.read();
                header += c;
                if (c == '\n')
                {
                    if (currentLine.length() == 0)
                    {
                        client.println("HTTP/1.1 200 OK");
                        client.println("Content-type:text/html; charset=UTF-8");
                        client.println("Connection: close");
                        client.println();

                        if (header.indexOf("GET /save") >= 0)
                        {
                            Serial.println("Save configuration");
                            Serial.println(header);

                            cl_ssid1 = urlDecode(header.substring(header.indexOf("wifissid1=") + 10, header.indexOf("&wifipassword1")));
                            cl_pwd1 = urlDecode(header.substring(header.indexOf("&wifipassword1=") + 15, header.indexOf("&wifissid2=")));
                            cl_ssid2 = urlDecode(header.substring(header.indexOf("wifissid2=") + 10, header.indexOf("&wifipassword2")));
                            cl_pwd2 = urlDecode(header.substring(header.indexOf("&wifipassword2=") + 15, header.indexOf("&wifissid3=")));
                            cl_ssid3 = urlDecode(header.substring(header.indexOf("wifissid3=") + 10, header.indexOf("&wifipassword3")));
                            cl_pwd3 = urlDecode(header.substring(header.indexOf("&wifipassword3=") + 15, header.indexOf("&jwacc=")));

                            cl_jw_account = urlDecode(header.substring(header.indexOf("&jwacc=") + 7, header.indexOf("&jwpass=")));
                            cl_jw_password = urlDecode(header.substring(header.indexOf("&jwpass=") + 8, header.indexOf("&ptk=")));
                            cl_pushovertoken = urlDecode(header.substring(header.indexOf("&ptk=") + 5, header.indexOf("&puk=")));
                            cl_pushoveruserkey = urlDecode(header.substring(header.indexOf("&puk=") + 5, header.indexOf("&pdn=")));
                            cl_pushoverdevicename = urlDecode(header.substring(header.indexOf("&pdn=") + 5, header.indexOf("&tapi=")));
                            cl_tencentapikey = urlDecode(header.substring(header.indexOf("&tapi=") + 6, header.indexOf("&alias=")));

                            cl_wechat_alias = urlDecode(header.substring(header.indexOf("&alias=") + 7, header.indexOf("&wpro=")));

                            cl_weather_province = urlDecode(header.substring(header.indexOf("&wpro=") + 6, header.indexOf("&wcity=")));
                            cl_weather_city = urlDecode(header.substring(header.indexOf("&wcity=") + 7, header.indexOf("&wcounty=")));

                            int autoLocateIndex = header.indexOf("&autolocate=");
                            int offlineModeIndex = header.indexOf("&offlinemode=");
                            int last_index = header.indexOf("&",  header.indexOf("&wcounty=") + 9);

                            cl_weather_county = urlDecode(header.substring(header.indexOf("&wcounty=") + 9, last_index)); // 这个必须得找到最后一个参数之前

                            if (autoLocateIndex != -1)
                            {
                                cl_auto_locate = header.substring(autoLocateIndex + 12, autoLocateIndex + 13);
                            }
                            if (offlineModeIndex != -1)
                            {
                                cl_offline_mode = header.substring(offlineModeIndex + 13, offlineModeIndex + 14);
                            }

                            Serial.println("Auto LOC");
                            Serial.println(cl_auto_locate);
                            Serial.println("Offline mode");
                            Serial.println(cl_offline_mode);

                            strcpy(globalConfig.wifi_ssid1, cl_ssid1.c_str());
                            strcpy(globalConfig.wifi_pass1, cl_pwd1.c_str());
                            strcpy(globalConfig.wifi_ssid2, cl_ssid2.c_str());
                            strcpy(globalConfig.wifi_pass2, cl_pwd2.c_str());
                            strcpy(globalConfig.wifi_ssid3, cl_ssid3.c_str());
                            strcpy(globalConfig.wifi_pass3, cl_pwd3.c_str());

                            strcpy(globalConfig.bupt_jw_account, cl_jw_account.c_str());
                            strcpy(globalConfig.bupt_jw_password, cl_jw_password.c_str());
                            strcpy(globalConfig.pushoverToken, cl_pushovertoken.c_str());
                            strcpy(globalConfig.pushoverUserkey, cl_pushoveruserkey.c_str());
                            strcpy(globalConfig.pushoverDevicename, cl_pushoverdevicename.c_str());
                            strcpy(globalConfig.tencent_api_key, cl_tencentapikey.c_str());
                            strcpy(globalConfig.ProvinceName, cl_weather_province.c_str());
                            strcpy(globalConfig.CityName, cl_weather_city.c_str());
                            strcpy(globalConfig.CountyName, cl_weather_county.c_str());
                            strcpy(globalConfig.wechat_alias, cl_wechat_alias.c_str());

                            globalConfig.auto_locate = (cl_auto_locate == "1");
                            globalConfig.offline_mode = (cl_offline_mode == "1");

                            for (int i = 0; i < 10; i++)
                            {
                                String enabled = header.indexOf(String("enabled") + i + "=1") >= 0 ? "1" : "0";
                                String wday = urlDecode(header.substring(header.indexOf(String("wday") + i + "=") + 6, header.indexOf("&hour" + String(i))));
                                String hour = urlDecode(header.substring(header.indexOf("&hour" + String(i) + "=") + 7, header.indexOf("&minute" + String(i))));
                                String minute = urlDecode(header.substring(header.indexOf("&minute" + String(i) + "=") + 9, header.indexOf("&command" + String(i))));
                                String command = urlDecode(header.substring(header.indexOf("&command" + String(i) + "=") + 10, header.indexOf("&custom_command" + String(i))));
                                String cl_custom_command;
                                if (i != 9)
                                    cl_custom_command = urlDecode(header.substring(header.indexOf("&customCommand" + String(i) + "=") + 15 + String(i).length(), header.indexOf("&", header.indexOf("&customCommand" + String(i) + "=") + 15 + String(i).length())));
                                else
                                    cl_custom_command = urlDecode(header.substring(header.indexOf("&customCommand" + String(i) + "=") + 15 + String(i).length(), header.indexOf(" HTTP/1.1")));
                                if (cl_custom_command != "")
                                {
                                    globalConfig.schedule[i].setCommand(cl_custom_command);
                                }
                                else
                                {
                                    cl_custom_command = urlDecode(header.substring(header.indexOf("&command" + String(i) + "=") + 9 + String(i).length(), header.indexOf("&", header.indexOf("&command" + String(i) + "=") + 9 + String(i).length())));
                                    if (cl_custom_command != "none")
                                        globalConfig.schedule[i].setCommand(cl_custom_command);
                                    else
                                    {
                                        globalConfig.schedule[i].setCommand("");
                                        enabled = "0";
                                    }
                                }
                                Serial.println("命令 " + globalConfig.schedule[i].getCommand());
                                globalConfig.schedule[i].set_do_time(wday.toInt(),hour.toInt(),minute.toInt());
                                globalConfig.schedule[i].setEnabled(enabled == "1");
                                globalConfig.schedule_enabled[i] = enabled == "1";
                            }

                            globalConfig.schedule_count = 10; // 任务总数是10个任务，实际可以不用这个标志

                            print_config();

                            saveStruct(PREFERENCEKEY, globalConfig);
                            esp_restart();
                        }

                        client.println("<!DOCTYPE html><html>");
                        client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
                        client.println("<link rel=\"icon\" href=\"data:,\">");
                        client.println("<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}");
                        client.println(".button { background-color: #4CAF50; border: none; color: white; padding: 16px 40px;");
                        client.println("text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer;}");
                        client.println(".button2 {background-color: #555555;}</style></head>");

                        client.println("<body><h1>LynnetteClock 配置</h1>");
                        client.println("<h2>版本: " + verloc + "</h2>");
                        client.println("<form action='/save'>");

                        client.printf("Wlan SSID1:<input type='text' name='wifissid1' value='%s'><br>", globalConfig.wifi_ssid1);
                        client.printf("密码1:<input type='text' name='wifipassword1' value='%s'><br>", globalConfig.wifi_pass1);
                        client.printf("Wlan SSID2:<input type='text' name='wifissid2' value='%s'><br>", globalConfig.wifi_ssid2);
                        client.printf("密码2:<input type='text' name='wifipassword2' value='%s'><br>", globalConfig.wifi_pass2);
                        client.printf("Wlan SSID3:<input type='text' name='wifissid3' value='%s'><br>", globalConfig.wifi_ssid3);
                        client.printf("密码3:<input type='text' name='wifipassword3' value='%s'><br>", globalConfig.wifi_pass3);

                        client.printf("教务账号:<input type='text' name='jwacc' value='%s'><br>", globalConfig.bupt_jw_account);
                        client.printf("教务密码:<input type='text' name='jwpass' value='%s'><br>", globalConfig.bupt_jw_password);
                        client.printf("Pushover 令牌:<input type='text' name='ptk' value='%s'><br>", globalConfig.pushoverToken);
                        client.printf("Pushover 用户密钥:<input type='text' name='puk' value='%s'><br>", globalConfig.pushoverUserkey);
                        client.printf("Pushover 设备名称:<input type='text' name='pdn' value='%s'><br>", globalConfig.pushoverDevicename);
                        client.printf("腾讯 API 密钥:<input type='text' name='tapi' value='%s'><br>", globalConfig.tencent_api_key);
                        client.printf("微信备注:<input type='text' name='alias' value='%s'><br>", globalConfig.wechat_alias);
                        client.printf("天气省份:<input type='text' name='wpro' value='%s'><br>", globalConfig.ProvinceName);
                        client.printf("天气城市:<input type='text' name='wcity' value='%s'><br>", globalConfig.CityName);
                        client.printf("天气县:<input type='text' name='wcounty' value='%s'><br>", globalConfig.CountyName);
                        client.printf("自动定位:<input type='checkbox' name='autolocate' value='1'%s><br>", globalConfig.auto_locate ? " checked" : "");
                        client.printf("离线模式:<input type='checkbox' name='offlinemode' value='1'%s><br>", globalConfig.offline_mode ? " checked" : "");

                        client.println("<h2>定时任务设置</h2>");
                        for (int i = 0; i < 10; i++)
                        {
                            client.printf("<h3>任务 %d</h3>", i + 1);
                            client.printf("启用:<input type='checkbox' name='enabled%d' value='1'%s><br>", i, globalConfig.schedule_enabled[i] ? " checked" : "");
                            client.printf("周几 (1-7, -1表示不限制):<input type='text' name='wday%d' value='%d'><br>", i, globalConfig.schedule[i].getWDay());
                            client.printf("小时 (-1表示不限制):<input type='text' name='hour%d' value='%d'><br>", i, globalConfig.schedule[i].getHour());
                            client.printf("分钟 (-1表示不限制):<input type='text' name='minute%d' value='%d'><br>", i, globalConfig.schedule[i].getMinutes());

                            String currentCommand = globalConfig.schedule[i].getCommand().c_str();
                            bool isCustomCommand = (currentCommand != REBOOT_COMMAND && currentCommand != WEATHER_COMMAND && currentCommand != ONLINE_STATUS_COMMAND);

                            client.printf("命令:<select name='command%d' onchange='toggleCustomCommand(this, %d)'>", i, i);
                            client.printf("<option value='none'%s>自定义</option>", isCustomCommand ? " selected" : "");
                            client.printf("<option value='%s'%s>重启</option>", REBOOT_COMMAND, currentCommand == REBOOT_COMMAND ? " selected" : "");
                            client.printf("<option value='%s'%s>天气</option>", WEATHER_COMMAND, currentCommand == WEATHER_COMMAND ? " selected" : "");
                            client.printf("<option value='%s'%s>在线状态</option>", ONLINE_STATUS_COMMAND, currentCommand == ONLINE_STATUS_COMMAND ? " selected" : "");
                            client.println("</select>");

                            client.printf("<input type='text' name='customCommand%d' id='customCommand%d' value='%s' %s><br>",
                                          i, i, isCustomCommand ? currentCommand.c_str() : "",
                                          isCustomCommand ? "" : "style='display:none;'");

                            client.println("<script>");
                            client.println("function toggleCustomCommand(select, index) {");
                            client.println("  var customCommandInput = document.getElementById('customCommand' + index);");
                            client.println("  if (select.value === 'none') {");
                            client.println("    customCommandInput.style.display = 'inline';");
                            client.println("    customCommandInput.value = '';"); // 清空自定义命令
                            client.println("  } else {");
                            client.println("    customCommandInput.style.display = 'none';");
                            client.println("    customCommandInput.value = '';"); // 清空自定义命令
                            client.println("  }");
                            client.println("}");

                            // 在页面加载时显示或隐藏自定义命令输入框
                            client.println("window.onload = function() {");
                            for (int j = 0; j < 10; j++)
                            {
                                String cmd = globalConfig.schedule[j].getCommand();
                                client.printf("  toggleCustomCommand(document.getElementsByName('command%d')[0], %d);", j, j);
                            }
                            client.println("};");
                            client.println("</script>");
                        }

                        client.println("<input type='submit' value='提交'></form>");

                        client.println("<script>");
                        client.println("if (window.location.href.indexOf('/save') > -1) {");
                        client.println("  alert('配置已保存，设备已自动重启');");
                        client.println("}");
                        client.println("</script>");

                        client.println("</body></html>");

                        client.println();
                        break;
                    }
                    else
                    {
                        currentLine = "";
                    }
                }
                else if (c != '\r')
                {
                    currentLine += c;
                }
            }
        }
        header = "";
        client.stop();
        Serial.println("Client disconnected.");
        Serial.println("");
    }
}
