//Libraries for WLAN and WebServer
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>

//DDS Library
#include <AD9850.h>

#include "files.h"
#include "config.h"

//DDS Vars
double freq = 10000;
double trimFreq = 124999500;
int phase = 0;
uint8_t mode = MODE_UP;

//Vars needed for Sweeping
uint8_t sweeping = false;
double sweep_max = 1000;
double sweep_min = 500;
unsigned long sweep_delay = 1000;
unsigned long sweep_resolution = 1;
uint8_t sweep_pong = false;
uint8_t sweep_reverse = false;

ESP8266WebServer server(80);

void setup() {
	Serial.begin(115200);

	Serial.println("Connect to WLAN");
	//Connect to WLAN
	WiFi.begin(ssid, password);
	while (WiFi.status() != WL_CONNECTED) {
		delay(500);
		Serial.print(".");
	}
	if (MDNS.begin("esp8266")) {
		Serial.println("MDNS responder started");
	}

	server.on("/", handleRoot);
	server.on("/set", handleSet);
	server.on("/get", handleGet);
	server.on("/sweep", handleSweep);
	server.begin();

	
	Serial.println("StartUp DDS");
	DDS.begin(W_CLK_PIN, FQ_UD_PIN, DATA_PIN, RESET_PIN);
	//DDS.calibrate(trimFreq);
	DDS.setfreq(freq, phase);
	DDS.up();
	DDS.setfreq(freq, phase);

	//activate frequency
	pinMode(LED_R, OUTPUT);
	pinMode(LED_G, OUTPUT);
}

void loop() {
	server.handleClient();

	//If Sweeping is active
	if (sweeping)
	{
		sweep();
	}
}

void handleRoot() {	
	server.send(200, "text/html", index_html);
}

void sweep()
{
	static unsigned long lasttime = 0;
	if ((micros() - lasttime) >= sweep_delay)
	{
		if (sweep_reverse)			//falling frequency
		{
			if (freq - sweep_resolution < sweep_min)
			{
				if (sweep_pong)
				{
					sweep_reverse = !sweep_reverse;
				}
				else
				{
					freq = sweep_max;
				}
			}
			else
			{
					freq = freq - sweep_resolution;
			}
		}
		else
		{
			if (freq + sweep_resolution > sweep_max) //rising frequency
			{
				if (sweep_pong)
				{
					sweep_reverse = !sweep_reverse;
				}
				else
				{
					freq = sweep_min;
				}
			}
			else
			{
				freq = freq + sweep_resolution;
			}
		}
		DDS.setfreq(freq,phase);
		lasttime = micros();
	}

}


void handleSweep()
{
	boolean success = false;

	if (server.hasArg("min"))	//Lower frequneny limit
	{
		String min_str = server.arg("min");
		sweep_min = min_str.toFloat();
		success = true;
	}

	if (server.hasArg("max"))	//Upper limit of frequency
	{
		String max_str = server.arg("max");
		sweep_max = max_str.toFloat();
		success = true;
	}

	if (server.hasArg("delay"))	//delay between two steps
	{
		String delay_str = server.arg("delay");
		sweep_delay = delay_str.toInt();
		success = true;
	}

	if (server.hasArg("resolution")) //width of a step
	{
		String res_string = server.arg("resolution");
		sweep_resolution = res_string.toInt();
		success = true;
	}

	if (server.hasArg("reverse"))	//Should sweep from upper to lower end?
	{
		String reverse_str = server.arg("reverse");
		if (reverse_str == "true" || reverse_str == "1")
		{
			sweep_reverse = true;
			success = true;
		}
		else if (reverse_str == "false" || reverse_str == "0")
		{
			sweep_reverse = false;
			success = true;
		}
		else
		{
			server.send(API_FAIL, CONTENT_TYPE, "Unknown value. Reverse has to be true or false!");
		}
	}

	if (server.hasArg("pong"))	//Should reverse sweep directions on ends?
	{
		String pong_str = server.arg("pong");
		if (pong_str == "true" || pong_str == "1")
		{
			sweep_pong = true;
			success = true;
		}
		else if (pong_str == "false" || pong_str == "0")
		{
			sweep_pong = false;
			success = true;
		}
		else
		{
			server.send(API_FAIL, CONTENT_TYPE, "Unknown value. Pong has to be true or false!");
		}
	}


	if (server.hasArg("mode"))
	{
		String mode_str = server.arg("mode");
		if (mode_str == "on")
		{
			sweeping = 1;
			if (sweep_reverse)
			{
				freq = sweep_max;
			}
			else
			{
				freq = sweep_min;
			}
			DDS.setfreq(freq, phase);
			server.send(API_SUCCESS, CONTENT_TYPE, "Sweeping activated!");
			return;
		}
		else
		{
			sweeping = 0;
			server.send(API_SUCCESS, CONTENT_TYPE, "Sweeping deactivated!");
			return;
		}
	}
	
	if(server.args()==0)
	{
		server.send(API_FAIL, CONTENT_TYPE, "No arguments was given! Set mode=on for activating sweeping!");
	}

	if (success == true)
	{
		server.send(API_SUCCESS, CONTENT_TYPE, "Updating Sweeping Params was successful!");
	}
}

void handleGet()
{
	if (server.hasArg("var"))
	{
		String var = server.arg("var");
		if (var == "version")
		{
			server.send(API_SUCCESS, CONTENT_TYPE, VERSION);
		}
		else if (var == "mode")
		{
			if (mode = MODE_DOWN)
			{
				server.send(API_SUCCESS, CONTENT_TYPE, "down");
			}
			else
			{
				server.send(API_SUCCESS, CONTENT_TYPE, "up");
			}
		}

	}
	else
	{
		server.send(API_FAIL, CONTENT_TYPE, "Specify the Variable you want to query with ?var=");
	}
}


//Handels if /set is called
void handleSet() {
	
	if (mode == MODE_UP && sweeping!=1)	//Allow Freq updates only when AD9850 is up and no sweeping
	{
		//Frequency Update
		if (server.hasArg("freq"))
		{

			String freq_str = server.arg("freq");
			#ifdef DEBUG
			Serial.print("Got " + freq_str + " as Frequency");
			#endif
			double tmp = freq_str.toFloat();
			if (tmp > 0 && tmp < 62500000) //is in Frequency range of
			{
				freq = tmp;
				DDS.setfreq(freq, phase);
			}
			else {
				server.send(API_FAIL, "text/plain", "Frequency invalid. Must be in Range 0 - 62.5MHz. Got: " + freq_str + " Hz");
			}
		}

		//Phase Update
		if (server.hasArg("phase"))
		{
			String phase_str = server.arg("phase");
			#ifdef DEBUG
			Serial.print("Got " + phase_str + " as Phase");
			#endif
			double tmp = phase_str.toFloat();

			if (tmp >= 0 && tmp < 32) //5 bits for Phase. Max Value is 31.
			{
				phase = tmp;
				DDS.setfreq(freq, phase);
			}
			else {
				server.send(API_FAIL, "text/plain", "Phase Invalid. Got: " + phase);
			}
		}

		if (server.hasArg("red"))
		{
			String red_str = server.arg("red");
			if (red_str.indexOf("on") >= 0)
			{
				analogWrite(LED_R, 0);
				digitalWrite(LED_R, HIGH);
			}
			else if (red_str.indexOf("off") >= 0)
			{
				analogWrite(LED_R, 0);
				digitalWrite(LED_R, LOW);
			}
			else
			{
				int red = red_str.toInt();
				if ((red > 0 || red_str.indexOf('0') >= 0) && red <= 1023)
				{
					analogWrite(LED_R, red);
				}
				else
				{
					server.send(API_FAIL, "text/plain", "Red LED PWM has to be between 0 and 1024. Or on or off. Got: " + red_str);
				}
			}
		}
	}	//Closing for if(mode == MODE_UP()
	else if (sweeping)
	{
		server.send(API_FAIL, CONTENT_TYPE, "Sweeping Output is active! You have to deactivate it for changing frequency.");
	}
	else
	{
		server.send(API_FAIL, "text/plain", "DDS Module is down. You has to activate it to update Frequency.");
	}

	if (server.hasArg("green"))
	{
		String green_str = server.arg("green");
		if (green_str.indexOf("on") >= 0)
		{
			analogWrite(LED_G, 0);
			digitalWrite(LED_G, HIGH);
		}
		else if (green_str.indexOf("off") >= 0)
		{
			analogWrite(LED_G, 0);
			digitalWrite(LED_G, LOW);
		}
		else
		{
			int green = green_str.toInt();
			if ((green > 0 || green_str.indexOf('0') >= 0) && green <= 1023)
			{
				analogWrite(LED_G, green);
			}
			else
			{
				server.send(API_FAIL, "text/plain", "Green LED PWM has to be between 0 and 1024. Or on or off. Got: " + green_str);
			}
		}
	}
	
	if (server.hasArg("mode"))	//Function to activate or deactivate the Sinus Output
	{
		String mode_str = server.arg("mode");
		if (mode_str.indexOf("up") >= 0)
		{
			//DDS.up();
			DDS.setfreq(freq, phase);
			mode = MODE_UP;
			server.send(API_SUCCESS, "text/plain", "DDS is now up!");
		}
		else if (mode_str.indexOf("down") >= 0)
		{
			DDS.down();
			mode = MODE_DOWN;
			server.send(API_SUCCESS, "text/plain", "DDS is now down!");
		}
		else
		{
			server.send(API_FAIL,"text/plain","Mode must be either up or down");
			return;
		}
	}
	



	if (server.args() == 0)
	{
		server.send(API_FAIL, "text/plain", "No argument was given. Please submit the values to change via a GET request.");
		return;
	}
		
	server.send(API_SUCCESS, "text/plain", "Update successful!");

}

void handleNotFound() {
	String message = "File Not Found\n\n";
	message += "URI: ";
	message += server.uri();
	message += "\nMethod: ";
	message += (server.method() == HTTP_GET) ? "GET" : "POST";
	message += "\nArguments: ";
	message += server.args();
	message += "\n";
	for (uint8_t i = 0; i<server.args(); i++) {
		message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
	}
	server.send(404, "text/plain", message);
}