#pragma once

#ifndef _config_h
#define _config_h

//Pin Defines
const int W_CLK_PIN = 14;
const int FQ_UD_PIN = 12;
const int DATA_PIN = 16;
const int RESET_PIN = 13;
const int LED_R = 4;
const int LED_G = 5;

//WLAN
const char* ssid = "****";			//Enter here your WiFi-Credentials
const char* password = "******";

//API
#define API_SUCCESS 200
#define API_FAIL 400

#define MODE_DOWN 0
#define MODE_UP 1

const String VERSION = "0.1";

#define CONTENT_TYPE "text/plain"

#endif