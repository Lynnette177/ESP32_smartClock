#include "includes.h"
// 不再需要直接请求API。通过主机转发一次 避免因为IP问题访问不了
// const char* binance_api_key = "NnnY5XLz9Suso27G6P6lbqn72eEMoTW84z2alcxQahjKorTt1E3xx4DZsBggoEhS";
// const char* binance_secret_key = "31UeH8ppmCHaeQy7COeXFm8iYXA2aNT9por7v3xiDzGd6f0MwBf3xGXbwQkapXb8";
// const char* binance_base_url = "http://binance.rigelow.uk";
// #define price_change_in_24h_url  "/fapi/v1/ticker/24hr"
// #define latest_price_url  "/fapi/v2/ticker/price"

// bool get_latest_price(const char *coin_type, char *result)
// {
//     HTTPClient http;
//     http.addHeader("X-MBX-APIKEY", binance_api_key);
//     String url = binance_base_url;
//     url += latest_price_url;

//     String query_string = "symbol=";
//     query_string += coin_type;
//     query_string += "&timestamp=";
//     query_string += String(nowtimet * 1000);
//     String query_sign = calculateHMAC(binance_secret_key, query_string);
//     query_string += "&signature=" + query_sign;

//     http.begin(url + "?" + query_string);
//     int httpResponseCode = http.GET();
//     if (httpResponseCode > 0)
//     {
//         String responseBody = http.getString();
//         Serial.println(responseBody);
//         JsonDocument doc;
//         deserializeJson(doc, responseBody);
//     }

//     http.end();
//     return false;
// }
bool get_latest_price(const char *coin_type, char *result)
{
    HTTPClient http;

    String url = pi4_server_address;
    url += "/latest_price";

    JsonDocument doc;
    doc["token"] = "Lynnette";
    doc["cointype"] = coin_type;
    String jsonStr;
    serializeJson(doc, jsonStr);
    http.begin(url);
    int httpResponseCode = http.POST(jsonStr);
    if (httpResponseCode > 0)
    {
        String responseBody = http.getString();
        JsonDocument doc_rep;
        deserializeJson(doc_rep, responseBody);
        String Pricestring = doc_rep["price"].as<String>();
        if (Pricestring == "null")
        {
            http.end();
            return false;
        }
        else
        {
            if (result != NULL)
            {
                strcpy(result, Pricestring.c_str());
            }
            http.end();
            return true;
        }
    }
    http.end();
    return false;
}

void btc_clock()
{
    char *price = new char[256];
    String quote_asset = "USDT";
    float all_price[12] = {0};
    int pointX[12] = {0};
    int pointY[12] = {0};
    String base_asset = "";
    int locked_type = -1;
    u8g2.setFont(u8g2_font_wqy12_t_gb2312);
    while (digitalRead(CONTROL_BUTTON) != LOW)
    {
        for (int i = 0; i < 3; i++)
        {
            if (locked_type >= 0 && locked_type != i) // 如果不是用户锁定的币种 直接跳过
                continue;
            memset(price, 0, 256);
            memset(pointX, 0, sizeof(int) * 12);
            memset(pointY, 0, sizeof(int) * 12);
            if (locked_type == -1)
            {
                memset(all_price, 0, sizeof(float) * 12);//如果用户不锁定币种，清空所有价格数据，这样开启新的货币循环
            }
            else
            {
                for (int lefti = 0; lefti < 11; lefti++) //如果用户锁定了币种，那么应该保留好历史记录。左移，把最早的一个值弹出去，新的值当成最后一个
                {
                    all_price[lefti] = all_price[lefti + 1];
                }
                all_price[11] = 0;
            }
            switch (i)
            {
            case 0:
                base_asset = "BTC";
                break;
            case 1:
                base_asset = "ETH";
                break;
            case 2:
                base_asset = "SOL";
                break;
            }
            float maxPrice = 0;
            float minPrice = 0;
            if (locked_type != -1)// 如果用户锁定了币种，那么得重新计算最大最小值。因为不锁定的时候，为了节省性能最大最小值是在获取每一个的值的时候单独获取的
            { // 在锁定的情况下 只要添加最后一个新的数据，同时数组最开始的值弹出去了，最大最小值需要重新计算
                minPrice = all_price[0];
                maxPrice = all_price[0];
                for (int j = 1; j < 12; j++)
                {
                    if (all_price[j] < 1.f)
                        continue;
                    if (all_price[j] < minPrice)
                        minPrice = all_price[j];
                    if (all_price[j] > maxPrice)
                        maxPrice = all_price[j];
                }
            }
            for (int j = 0; j < 12;) // 每个绘制12个点
            {
                if (locked_type != -1) //如果用户锁定了币种，那么直接跳过，到最后一个来执行。就好像新添加最后一个元素一样。但是没有写死，是为了鲁棒性
                {
                    while (all_price[j] != 0 && j != 11)
                    {
                        j++;
                    }
                }
                if (get_latest_price((base_asset + quote_asset).c_str(), price))
                {
                    float priceF = std::atof(price);
                    all_price[j] = priceF;
                    if (j == 0)
                    {
                        maxPrice = priceF;
                        minPrice = priceF;
                    }
                    else
                    {
                        if (priceF > maxPrice)
                            maxPrice = priceF;
                        if (priceF < minPrice)
                            minPrice = priceF;
                    }
                    // Serial.printf("%f %f %f\n", priceF, maxPrice, minPrice);
                    if (j == 0)
                    {
                        pointY[j] = 28;
                        pointX[j] = 10;
                    }
                    else
                    {
                        for (int z = 0; z < 12; z++)
                        {
                            if (all_price[z] == 0)
                                break;
                            pointX[z] = 10 * z + 10;
                            if (maxPrice - minPrice > 0.0000001f)
                                pointY[z] = 14 + (maxPrice - all_price[z]) / (maxPrice - minPrice) * 32;
                            else
                                pointY[z] = 28;
                        }
                    }
                    j++;
                    float lineDX = 0;
                    float lineDY = 0;
                    unsigned int start_time = millis();
                    int interval = 5000;
                    bool pressed_button = false;
                    while (true) // 循环一次一秒，每五秒我们才更新一次
                    {
                        if (digitalRead(AI_BUTTON) == LOW && !pressed_button)
                        {
                            pressed_button = true;
                            if (locked_type == -1)
                                locked_type = i;
                            else
                                locked_type = -1;
                        }
                        else if (!digitalRead(AI_BUTTON) == LOW)
                        {
                            pressed_button = false;
                        }
                        if (digitalRead(CONTROL_BUTTON) == LOW) break;
                        u8g2.firstPage();
                        do
                        {
                            int z = 0;
                            for (; z < 12; z++)
                            {
                                if (all_price[z] == 0)
                                    break;
                                if (z != 11 && all_price[z + 1] != 0)
                                    u8g2.drawDisc(pointX[z], pointY[z], 2);
                                else
                                    u8g2.drawCircle(pointX[z], pointY[z], 2);
                            }
                            for (int z1 = 0; z1 < z - 2; z1++)
                            { // z是0， z-1是最后一个 画z-2到z-1
                                u8g2.drawLine(pointX[z1], pointY[z1], pointX[z1 + 1], pointY[z1 + 1]);
                            }
                            if (z - 2 >= 0)
                            { // 需要渐变来画z-2到z-1
                                float all_dx = pointX[z - 1] - pointX[z - 2] - lineDX;
                                float all_dy = pointY[z - 1] - pointY[z - 2] - lineDY;
                                float length_dxy = sqrtf(all_dx * all_dx + all_dy * all_dy);

                                if (abs(all_dx) > 0.1f)
                                    lineDX += all_dx / length_dxy / 4.f;
                                if (abs(all_dy) > 0.1f)
                                    lineDY += all_dy / length_dxy / 4.f;
                                u8g2.drawLine(pointX[z - 2], pointY[z - 2], pointX[z - 2] + int(lineDX), pointY[z - 2] + int(lineDY));
                            }
                            u8g2.setCursor(0, 60);
                            if (locked_type == -1)
                                u8g2.print(base_asset + "合约");
                            else
                                u8g2.print(base_asset + "合约√");
                            u8g2.setCursor(75, 60);
                            u8g2.printf("%.2f", priceF);

                            String now_time_str_all = getFormattedDateTime_For_clock_Chinese(nowtimet);
                            int indexofSplit = now_time_str_all.indexOf(';');
                            String Y_M_D = now_time_str_all.substring(0, indexofSplit);
                            String hour_minute_sec = now_time_str_all.substring(indexofSplit + 1).substring(0, 5); // 只到分钟
                            u8g2.setCursor(0, 10);
                            u8g2.print(Y_M_D);
                            u8g2.setCursor(95, 10);
                            u8g2.print(hour_minute_sec);
                        } while (u8g2.nextPage());
                        if (millis() - start_time > interval)
                            break;
                    }
                    if (digitalRead(CONTROL_BUTTON) == LOW) break;
                }
                else
                    vTaskDelay(500 / portTICK_PERIOD_MS);
            }
            if (digitalRead(CONTROL_BUTTON) == LOW) break;
        }
    }
    delete price;
}