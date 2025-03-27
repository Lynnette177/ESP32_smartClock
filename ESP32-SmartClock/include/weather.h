#include "includes.h"
#define CITY "北京"
#define API "SqVXGTIpzo5UEViHN"
String WeatherURL = "";

bool get_province_id(const char *province_in, char *province_id_out)
{
    HTTPClient http;
    String url = "http://www.weather.com.cn/data/city3jdata/china.html";
    http.begin(url);
    int httpResponseCode = http.GET();
    if (httpResponseCode > 0)
    {
        String responseBody = http.getString();
        JsonDocument doc;
        deserializeJson(doc, responseBody);
        for (JsonPair kv : doc.as<JsonObject>())
        {
            const char *id = kv.key().c_str();                    // 获取键（ID）
            const char *province = kv.value().as<const char *>(); // 获取值（省份）
            if (strcmp(province_in, province) == 0)
            {
                strcpy(province_id_out, id);
                http.end();
                return true;
            }
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

bool get_area_id_in_province(const char *province_id_in, const char *area_name, char *area_id_out)
{
    HTTPClient http;
    String url = "http://www.weather.com.cn/data/city3jdata/provshi/";
    url += province_id_in;
    url += ".html";
    http.begin(url);
    int httpResponseCode = http.GET();
    if (httpResponseCode > 0)
    {
        String responseBody = http.getString();
        JsonDocument doc;
        deserializeJson(doc, responseBody);
        for (JsonPair kv : doc.as<JsonObject>())
        {
            const char *id = kv.key().c_str();                // 获取键（ID）
            const char *area = kv.value().as<const char *>(); // 获取值（省份）
            if (strcmp(area_name, area) == 0)
            {
                strcpy(area_id_out, id);
                http.end();
                return true;
            }
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

bool get_county_id_in_province_and_area(const char *province_id_in, const char *area_id_in, const char *county_name, char *county_id_out)
{
    HTTPClient http;
    String url = "http://www.weather.com.cn/data/city3jdata/station/";
    url += province_id_in;
    url += area_id_in;
    url += ".html";
    http.begin(url);
    int httpResponseCode = http.GET();
    if (httpResponseCode > 0)
    {
        String responseBody = http.getString();
        JsonDocument doc;
        deserializeJson(doc, responseBody);
        for (JsonPair kv : doc.as<JsonObject>())
        {
            const char *id = kv.key().c_str();                  // 获取键（ID）
            const char *county = kv.value().as<const char *>(); // 获取值（省份）
            if (strcmp(county_name, county) == 0)
            {
                strcpy(county_id_out, id);
                http.end();
                return true;
            }
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

bool get_weather(const char *province_id_in, const char *area_id_in, const char *county_id_in, uint64_t timestamp, char *weather_out)
{
    memset(weather_out, 0, 200);
    String weather = "";
    HTTPClient http;
    HTTPClient http_realtime;
    String url = "http://d1.weather.com.cn/dingzhi/";
    String url_realtime_weather = "http://d1.weather.com.cn/sk_2d/";
    String city_id = province_id_in;
    city_id += county_id_in;
    city_id += area_id_in;
    // 连接顺序应该是 province_id + county_id + area_id
    url += city_id;
    url += ".html?_=";
    url += String(timestamp * 1000 - 28800000); // 发送的需要是UTC时间戳 减八小时
    url_realtime_weather += city_id;
    url_realtime_weather += ".html?_=";
    url_realtime_weather += String(timestamp * 1000 - 28800000); // 发送的需要是UTC时间戳 减八小时
    http.begin(url);
    http_realtime.begin(url_realtime_weather);
    String refer_url = "http://www.weather.com.cn/weather1d/" + city_id;
    refer_url += ".shtml";
    http.addHeader("Referer", refer_url);
    http_realtime.addHeader("Referer", refer_url);
    int httpResponseCode = http.GET();
    int httpResponseCode_Realtime = http_realtime.GET();
    if (httpResponseCode > 0 && httpResponseCode_Realtime > 0)
    {
        String responseBody = http.getString();

        int startIndex = responseBody.indexOf('{');
        int endIndex = responseBody.indexOf("};", startIndex);

        String weatherJson = responseBody.substring(startIndex, endIndex + 1);
        static JsonDocument doc_realtime;
        deserializeJson(doc_realtime, weatherJson);

        String max_temperature = doc_realtime["weatherinfo"]["temp"].as<String>();
        String lowest_temperature = doc_realtime["weatherinfo"]["tempn"].as<String>();
        String the_weather = doc_realtime["weatherinfo"]["weather"].as<String>();
        http.end();

        String responseBody_realtime = http_realtime.getString();
        int startIndex_realtime = responseBody_realtime.indexOf('{');
        int endIndex_realtime = responseBody_realtime.indexOf('}');
        String realTimeJson = responseBody_realtime.substring(startIndex_realtime, endIndex_realtime + 1);
        deserializeJson(doc_realtime, realTimeJson);
        String now_temperature = doc_realtime["temp"].as<String>() + "℃";
        String now_weather = doc_realtime["weather"];
        String wind = doc_realtime["WD"].as<String>();
        String wind_speed = doc_realtime["wse"].as<String>();
        String wind_level = doc_realtime["WS"].as<String>();
        wind_speed.remove(wind_speed.indexOf('\\'));
        wind_speed += "(" + wind_level + ")";
        String update_time = doc_realtime["time"];
        String rain_possibility = doc_realtime["rain"];
        String rain_in_24hour = doc_realtime["rain24h"];
        String aqi = doc_realtime["aqi"];
        String aqipm25 = doc_realtime["aqi_pm25"];
        String limit_number = doc_realtime["limitnumber"];
        String humidity = doc_realtime["SD"];
        String visibility = doc_realtime["njd"];
        String atmospheric_pressure = doc_realtime["qy"];
        if (limit_number != "")
            weather = "今日限行" + limit_number + " ";
        weather += "今日天气" + the_weather;
        weather += " 当前：" + now_weather;
        weather += " 当前温度" + now_temperature;
        weather += " 今日最高" + max_temperature;
        weather += " 今日最低" + lowest_temperature;
        weather += " 降雨概率：" + rain_possibility + "% " + rain_in_24hour + "%(24h)";
        weather += " 空气质量：" + aqi + "/" + aqipm25 + "(PM2.5)";
        weather += " 湿度：" + humidity;
        weather += " 能见度" + visibility;
        weather += " 风向：" + wind;
        weather += " 风速：" + wind_speed;
        weather += " 更新时间：" + update_time;
        http_realtime.end();
        strcpy(weather_out, weather.c_str());
        return true;
    }
    else
    {
        http_realtime.end();
        http.end();
        return false;
    }
}

bool get_lon_lat_using_weather_api(const char *province_id_in, const char *area_id_in, const char *county_id_in, uint64_t timestamp, String& longitude, String& latitude)
{
    HTTPClient http;
    static JsonDocument doc;
    String url = "https://d7.weather.com.cn/geong/v1/api/";

    String city_id = province_id_in;
    city_id += county_id_in;
    city_id += area_id_in;

    doc["method"] = "stationinfo";
    doc["areaid"] = city_id;
    doc["category"] = "";
    doc["callback"] = "aa";  //注意这个aa在url中出现了两次
    String jsonStr;
    serializeJson(doc, jsonStr);

    url = url + "?params=" + jsonStr + "&callback=" + "aa" + "&_=" +String(timestamp * 1000 - 28800000);
    http.begin(url);

    String refer_url = "http://m.weather.com.cn";
    http.addHeader("Referer", refer_url);

    int httpResponseCode = http.GET();

    if (httpResponseCode > 0)
    {
        String responseBody = http.getString();

        int startIndex = responseBody.indexOf("({");
        int endIndex = responseBody.indexOf("})", startIndex);

        String weatherJson = responseBody.substring(startIndex + 1, endIndex + 1);
        static JsonDocument doc;
        deserializeJson(doc, weatherJson);

        longitude = doc["location"]["lng"].as<String>();
        latitude = doc["location"]["lat"].as<String>();

        http.end();
        return true;
    }
    else
    {
        http.end();
        return false;
    }
}

bool get_weather_trend_str(uint64_t timestamp, char *weather_trend_out, String latitude, String longitude)
{
    memset(weather_trend_out, 0, 200);
    String weather = "";
    HTTPClient http;

    String url = "http://d3.weather.com.cn/webgis_rain_new/webgis/minute?lat=";
    url += latitude;
    url += "&lon=";
    url += longitude;
    url += "&callback=aa&_=";
    url += String(timestamp * 1000 - 28800000); // 发送的需要是UTC时间戳 减八小时
    http.begin(url);

    String refer_url = "http://m.weather.com.cn";
    http.addHeader("Referer", refer_url);

    int httpResponseCode = http.GET();

    if (httpResponseCode > 0)
    {
        String responseBody = http.getString();

        int startIndex = responseBody.indexOf("({");
        int endIndex = responseBody.indexOf("});", startIndex);

        String weatherJson = responseBody.substring(startIndex + 1, endIndex + 1);
        static JsonDocument doc;
        deserializeJson(doc, weatherJson);

        String trend = doc["msg"].as<String>();
        String update_time = doc["uptime"].as<String>();
        http.end();
        weather = "自" + update_time.substring(11) + "起" + trend;
        strcpy(weather_trend_out, weather.c_str());
        return true;
    }
    else
    {
        http.end();
        return false;
    }
}