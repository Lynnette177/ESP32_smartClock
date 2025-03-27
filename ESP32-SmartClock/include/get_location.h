#include "includes.h"
bool get_location(){
    String url = "https://apis.map.qq.com/ws/location/v1/ip?ip=&key=";
    url += globalConfig.tencent_api_key;
    HTTPClient http;
    JsonDocument doc; // 分配内存
    http.begin(url);
    int responseCode = http.GET();
    String responseBody = http.getString();
    if (responseCode < 0){
      http.end();
      return false;
    }
    deserializeJson(doc, responseBody);
    if (doc["message"].as<String>() != "Success"){
      http.end();
      return false;
    }
    String province = doc["result"]["ad_info"]["province"].as<String>();
    String city = doc["result"]["ad_info"]["city"].as<String>();
    String district = doc["result"]["ad_info"]["district"].as<String>();
    longitude = doc["result"]["location"]["lng"].as<String>();
    latitude = doc["result"]["location"]["lat"].as<String>();
    Serial.println(longitude);
    Serial.println(latitude);
    my_ip_address = doc["result"]["ip"].as<String>();
    Serial.println(my_ip_address);
    strcpy(globalConfig.ProvinceName,province.substring(0,6).c_str());
    strcpy(globalConfig.CityName,city.substring(0,6).c_str());
    strcpy(globalConfig.CountyName,district.substring(0,6).c_str());
    http.end();
    return true;
}