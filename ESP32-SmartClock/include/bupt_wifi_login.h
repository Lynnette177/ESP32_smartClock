#pragma once
#include "includes.h"
const char* baidu_host = "baidu.com";
const char* serverUrl = "http://www.baidu.com/";
String redirectLocation;
void bupt_login(String BUPTACC, String BUPTPASS) {
  String postData = "user=" + BUPTACC + "&pass=" + BUPTPASS;
  while (1) {
    String payload;
    String redirectUrl;
    HTTPClient http;
    
    http.begin("https://www.baidu.com/?cmd=redirect&arubalp=12345");
    const char *headerKeys[] = {"Server", "Content-Type", "Date", "Location","Set-Cookie"};
    http.collectHeaders(headerKeys, sizeof(headerKeys) / sizeof(headerKeys[0]));
    int httpCode = http.GET();
    Serial.println(httpCode);   
    for (int i = 0; i < http.headers(); i++) {
      Serial.printf("%s: %s\n", http.headerName(i).c_str(), http.header(i).c_str());
        if (strcmp(http.headerName(i).c_str(), "Location") == 0) {
        redirectLocation = http.header(i);
        if (redirectLocation.length() == 0) {
          Serial.println("You already login so there is no redirection.");
          delay(1000);
          http.end();
          return;
        }
        Serial.printf("Redirect Location: %s\n", redirectLocation.c_str());
      }
    }
    http.end();
    
    http.begin(redirectLocation);
    http.collectHeaders(headerKeys, sizeof(headerKeys) / sizeof(headerKeys[0]));
    httpCode = http.GET();
    String receivedCookies = http.header("Set-Cookie");
    String sessionId;
    int semicolonIndex = receivedCookies.indexOf(';');
    if (semicolonIndex != -1) {
      sessionId = receivedCookies.substring(0, semicolonIndex);
    }
    Serial.printf("Session id: %s\n", sessionId.c_str());
    http.end();
    
    http.begin("http://10.3.8.216/login");
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");
    http.addHeader("Cookie", sessionId);
    http.addHeader("User-Agent", "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/120.0.0.0 Safari/537.36 Edg/120.0.0.0");
    int httpResponseCode = http.POST(postData);
    if (httpResponseCode > 0) {
      Serial.print("HTTP Response code: ");
      Serial.println(httpResponseCode);
      if (httpResponseCode == 200){
        Serial.println("Login Successed.");
      }
    } else {
      Serial.print("HTTP Request failed. Error code: ");
      Serial.println(httpResponseCode);
      Serial.println(http.errorToString(httpResponseCode).c_str());
    }
    http.end();
    break;
  }
}