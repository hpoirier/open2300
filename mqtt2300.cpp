/*  open2300 - mqtt2300.cpp
 *
 *  Version 1.11
 *
 *  Control WS2300 weather station
 *
 *  Copyright 2003-2006, Kenneth Lavrsen
 *  This program is published under the GNU General Public license
 */

extern "C"
{
#include "rw2300.h"
}
#include <mosquitto.h>
#include <string>
#include <iostream>

void loginfo(const std::string &error)
{
	std::string s("[INFO] ");
	s += error;
	std::cerr << s << std::endl;
}

void logerror(const std::string &error)
{
	std::string s("[ERROR] ");
	s += error;
	std::cerr << s << std::endl;
}

int publish(struct mosquitto *mqtt, const char *topic, const void *payload, int payload_len)
{
	int *mid = NULL;
	int qos = 0;
	bool retain = false;
	loginfo(std::string("Publishing to ") + topic);
	int errorcode = mosquitto_publish(mqtt, mid, topic, payload_len, payload, qos, retain);
	if (errorcode != MOSQ_ERR_SUCCESS)
	{
		logerror(std::string("Failed to publish to ") + topic + ". Error code: " + std::to_string(errorcode));
	}
	// else
	//{
	//	loginfo(std::string("Published to ") + topic);
	// }
	return errorcode;
}

#define PAYLOAD_MAXLEN 20

int publish(struct mosquitto *mqtt, const char *topic, double data)
{
	char payload[PAYLOAD_MAXLEN + 1];
	snprintf(payload, PAYLOAD_MAXLEN, "%.2f", data);

	return publish(mqtt, topic, payload, strlen(payload));
}

int publish(struct mosquitto *mqtt, const char *topic, long data)
{
	char payload[PAYLOAD_MAXLEN + 1];
	snprintf(payload, PAYLOAD_MAXLEN, "%ld", data);

	return publish(mqtt, topic, payload, strlen(payload));
}
int publish(struct mosquitto *mqtt, const char *topic, int data)
{
	return publish(mqtt, topic, (long)data);
}

/********** MAIN PROGRAM ************************************************
 *
 * This program reads all current and min/max data from a WS2300
 * weather station and write it to standard out. This is the program that
 * the program weatherstation.php uses to display a nice webpage with
 * current weather data.
 *
 * It takes one parameter which is the config file name with path
 * If this parameter is omitted the program will look at the default paths
 * See the open2300.conf-dist file for info
 *
 ***********************************************************************/
int main(int argc, char *argv[])
{
	WEATHERSTATION ws2300;
	// char logline[3000] = "";
	// char tempstring[1000] = "";
	// char datestring[50];     //used to hold the date stamp for the log file
	// const char *directions[]= {"N","NNE","NE","ENE","E","ESE","SE","SSE",
	//                            "S","SSW","SW","WSW","W","WNW","NW","NNW"};
	double winddir[6];
	// char tendency[15];
	// char forecast[15];
	struct config_type config;
	// double tempfloat_min;
	double tempfloat_max;
	int tempint;
	// int tempint_min, tempint_max;
	// struct timestamp time_min;
	struct timestamp time_max;
	// time_t basictime;
	bool ti_enabled = true;
	bool to_enabled = true;
	bool dp_enabled = false;
	bool rhi_enabled = true;
	bool rho_enabled = true;
	bool wind_enabled = false;
	bool rain_enabled = false;
	bool rp_enabled = true;
	bool forecast_enabled = false;

	struct mosquitto *mqtt = NULL;
	bool clean_session = true;
	void *userdata = NULL;
	int port = 1883;
	int keepalive = 60;

	if (argc < 1)
	{
		std::cerr << "Usage: " << argv[0] << " <conf>" << std::endl;
		return 0;
	}
	get_configuration(&config, argv[1]);

	mosquitto_lib_init();
	mqtt = mosquitto_new("open2300", clean_session, userdata);
	if (mqtt == NULL)
	{
		logerror("Out of memory.");
		return 1;
	}
	if (strlen(config.mqtt_username) > 0)
	{
		int errorcode = mosquitto_username_pw_set(mqtt, config.mqtt_username, config.mqtt_password);
		if (errorcode != MOSQ_ERR_SUCCESS)
		{
			logerror("Cannot set MQTT credentials.");
			return 1;
		}
	}
	int errorcode = mosquitto_connect(mqtt, config.mqtt_host, port, keepalive);
	if (errorcode != MOSQ_ERR_SUCCESS)
	{
		logerror(std::string("Cannot connect to MQTT server ") + config.mqtt_host + ":" + std::to_string(port));
		return 1;
	}
	loginfo(std::string("Connected to ") + config.mqtt_host);

	ws2300 = open_weatherstation(config.serial_device_name);

	if (ti_enabled)
	{
		/* READ TEMPERATURE INDOOR */
		double ti = temperature_indoor(ws2300, config.temperature_conv);
		if (ti > -20 && ti < 50)
		{
			if (publish(mqtt, "/ws2300/indoor_temp", ti) != 0)
			{
				return 1;
			}
		}
#if 0
	// Min/Max temperature
	temperature_indoor_minmax(ws2300, config.temperature_conv, &tempfloat_min,
	                          &tempfloat_max, &time_min, &time_max);

	sprintf(tempstring, "Timin %.1f\nTimax %.1f\n"
	                    "TTimin %02d:%02d\nDTimin %04d-%02d-%02d\n"
	                    "TTimax %02d:%02d\nDTimax %04d-%02d-%02d\n",
	        tempfloat_min, tempfloat_max,
	        time_min.hour, time_min.minute, time_min.year, time_min.month, time_min.day,
	        time_max.hour, time_max.minute, time_max.year, time_max.month, time_max.day);
	strcat(logline, tempstring);
#endif
	}
	/* READ TEMPERATURE OUTDOOR */
	if (to_enabled)
	{
		double to = temperature_outdoor(ws2300, config.temperature_conv);
		if (to > -20 && to < 50)
		{
			if (publish(mqtt, "/ws2300/outdoor_temp", to) != 0)
			{
				return 1;
			}
		}

#if 0
	// Min/Max temperature

	temperature_outdoor_minmax(ws2300, config.temperature_conv, &tempfloat_min,
	                           &tempfloat_max, &time_min, &time_max);

	sprintf(tempstring, "Tomin %.1f\nTomax %.1f\n"
	                    "TTomin %02d:%02d\nDTomin %04d-%02d-%02d\n"
	                    "TTomax %02d:%02d\nDTomax %04d-%02d-%02d\n",
	        tempfloat_min, tempfloat_max,
	        time_min.hour, time_min.minute, time_min.year, time_min.month, time_min.day,
	        time_max.hour, time_max.minute, time_max.year, time_max.month, time_max.day);
	strcat(logline, tempstring);
#endif
	}

	if (dp_enabled)
	{
#if 0
	/* READ DEWPOINT */

	sprintf(tempstring, "DP %.1f\n", dewpoint(ws2300, config.temperature_conv) );
	strcat(logline, tempstring);

	dewpoint_minmax(ws2300, config.temperature_conv, &tempfloat_min,
	                &tempfloat_max, &time_min, &time_max);

	sprintf(tempstring, "DPmin %.1f\nDPmax %.1f\n"
	                    "TDPmin %02d:%02d\nDDPmin %04d-%02d-%02d\n"
	                    "TDPmax %02d:%02d\nDDPmax %04d-%02d-%02d\n",
	        tempfloat_min, tempfloat_max,
	        time_min.hour, time_min.minute, time_min.year, time_min.month, time_min.day,
	        time_max.hour, time_max.minute, time_max.year, time_max.month, time_max.day);
	strcat(logline, tempstring);
#endif
	}
	/* READ RELATIVE HUMIDITY INDOOR */
	if (rhi_enabled)
	{
		double rhi = humidity_indoor(ws2300);
		if (rhi < 100)
		{
			if (publish(mqtt, "/ws2300/indoor_humidity", rhi) != 0)
			{
				return 1;
			}
		}
#if 0
	sprintf(tempstring, "RHi %d\n", humidity_indoor_all(ws2300, &tempint_min, &tempint_max,
	                                                    &time_min, &time_max) );
	strcat(logline, tempstring);

	sprintf(tempstring, "RHimin %d\nRHimax %d\n"
	                    "TRHimin %02d:%02d\nDRHimin %04d-%02d-%02d\n"
	                    "TRHimax %02d:%02d\nDRHimax %04d-%02d-%02d\n",
	        tempint_min, tempint_max,
	        time_min.hour, time_min.minute, time_min.year, time_min.month, time_min.day,
	        time_max.hour, time_max.minute, time_max.year, time_max.month, time_max.day);
	strcat(logline, tempstring);
#endif
	}
	if (rho_enabled)
	{
		/* READ RELATIVE HUMIDITY OUTDOOR */
		double rho = humidity_outdoor(ws2300);
		if (rho < 100)
		{
			if (publish(mqtt, "/ws2300/outdoor_humidity", rho) != 0)
			{
				return 1;
			}
		}
#if 0
	sprintf(tempstring, "RHo %d\n", humidity_outdoor_all(ws2300, &tempint_min, &tempint_max,
	                                                  &time_min, &time_max) );
	strcat(logline, tempstring);

	sprintf(tempstring, "RHomin %d\nRHomax %d\n"
	                    "TRHomin %02d:%02d\nDRHomin %04d-%02d-%02d\n"
	                    "TRHomax %02d:%02d\nDRHomax %04d-%02d-%02d\n",
	        tempint_min, tempint_max,
	        time_min.hour, time_min.minute, time_min.year, time_min.month, time_min.day,
	        time_max.hour, time_max.minute, time_max.year, time_max.month, time_max.day);
	strcat(logline, tempstring);
#endif
	}
	/* READ WIND SPEED AND DIRECTION */
	if (wind_enabled)
	{
		double wind_speed = wind_all(ws2300, config.wind_speed_conv_factor, &tempint, winddir);
		if (publish(mqtt, "/ws2300/wind_speed", wind_speed) != 0)
		{
			return 1;
		}
		if (publish(mqtt, "/ws2300/wind_dir", winddir[0]) != 0)
		{
			return 1;
		}

#if 0
	sprintf(tempstring,"WS %.1f\n",
	       wind_all(ws2300, config.wind_speed_conv_factor, &tempint, winddir));
	strcat(logline, tempstring);

	sprintf(tempstring,"DIRtext %s\nDIR0 %.1f\nDIR1 %0.1f\n"
	                   "DIR2 %0.1f\nDIR3 %0.1f\nDIR4 %0.1f\nDIR5 %0.1f\n",
	        directions[tempint], winddir[0], winddir[1],
	        winddir[2], winddir[3], winddir[4], winddir[5]);
	strcat(logline, tempstring);
#endif

		/* WINDCHILL */
		double wind_chill = windchill(ws2300, config.temperature_conv);
		if (publish(mqtt, "/ws2300/wind_chill", wind_chill) != 0)
		{
			return 1;
		}

#if 0
	sprintf(tempstring, "WC %.1f\n", windchill(ws2300, config.temperature_conv) );
	strcat(logline, tempstring);

	windchill_minmax(ws2300, config.temperature_conv, &tempfloat_min,
	                 &tempfloat_max, &time_min, &time_max); 

	sprintf(tempstring, "WCmin %.1f\nWCmax %.1f\n"
	                    "TWCmin %02d:%02d\nDWCmin %04d-%02d-%02d\n"
	                    "TWCmax %02d:%02d\nDWCmax %04d-%02d-%02d\n",
	        tempfloat_min, tempfloat_max,
	        time_min.hour, time_min.minute, time_min.year, time_min.month, time_min.day,
	        time_max.hour, time_max.minute, time_max.year, time_max.month, time_max.day);
	strcat(logline, tempstring);
#endif

		/* READ WINDSPEED MIN/MAX */
#if 0
	wind_minmax(ws2300, config.wind_speed_conv_factor, &tempfloat_min,
	            &tempfloat_max, &time_min, &time_max);

	sprintf(tempstring, "WSmin %.1f\nWSmax %.1f\n"
	                    "TWSmin %02d:%02d\nDWSmin %04d-%02d-%02d\n"
	                    "TWSmax %02d:%02d\nDWSmax %04d-%02d-%02d\n",
	        tempfloat_min, tempfloat_max,
	        time_min.hour, time_min.minute, time_min.year, time_min.month, time_min.day,
	        time_max.hour, time_max.minute, time_max.year, time_max.month, time_max.day);
	strcat(logline, tempstring);
#endif
	}
	if (rain_enabled)
	{

		/* READ RAIN 1H */
		double rain_1h = rain_1h_all(ws2300, config.rain_conv_factor, &tempfloat_max, &time_max);
		if (publish(mqtt, "/ws2300/rain_1h", rain_1h) != 0)
		{
			return 1;
		}
		if (publish(mqtt, "/ws2300/rain_max", tempfloat_max) != 0)
		{
			return 1;
		}
		// get current time and update the date/time field to get the time since epoch as a time_t
		time_t now = time(NULL);
		struct tm tm = *localtime(&now);
		tm.tm_year = time_max.year;
		tm.tm_mon = time_max.month;
		tm.tm_mday = time_max.day;
		tm.tm_hour = time_max.hour;
		tm.tm_min = time_max.minute;
		tm.tm_sec = 0;
		time_t time_max_since_epoch = mktime(&tm);
		if (publish(mqtt, "/ws2300/rain_max_time", time_max_since_epoch) != 0)
		{
			return 1;
		}
#if 0
	sprintf(tempstring, "R1h %.2f\n",
	        rain_1h_all(ws2300, config.rain_conv_factor,
	                    &tempfloat_max, &time_max));
	strcat(logline, tempstring);

	sprintf(tempstring, "R1hmax %.2f\n"
	                    "TR1hmax %02d:%02d\nDR1hmax %04d-%02d-%02d\n",
	        tempfloat_max,
	        time_max.hour, time_max.minute, time_max.year, time_max.month, time_max.day);
	strcat(logline, tempstring);


	/* READ RAIN 24H */

	sprintf(tempstring,"R24h %.2f\n",
	        rain_24h_all(ws2300, config.rain_conv_factor,
	                     &tempfloat_max, &time_max));
	strcat(logline, tempstring);

	sprintf(tempstring,"R24hmax %.2f\n"
	                   "TR24hmax %02d:%02d\nDR24hmax %04d-%02d-%02d\n",
	        tempfloat_max,
	        time_max.hour, time_max.minute, time_max.year, time_max.month, time_max.day);
	strcat(logline, tempstring);


	/* READ RAIN TOTAL */

	sprintf(tempstring,"Rtot %.2f\n",
	        rain_total_all(ws2300, config.rain_conv_factor, &time_max));
	strcat(logline, tempstring);

	sprintf(tempstring,"TRtot %02d:%02d\nDRtot %04d-%02d-%02d\n",
	        time_max.hour, time_max.minute, time_max.year,
	        time_max.month, time_max.day);
	strcat(logline, tempstring);
#endif
	}
	/* READ RELATIVE PRESSURE */
	if (rp_enabled)
	{
		if (publish(mqtt, "/ws2300/pressure", rel_pressure(ws2300, config.pressure_conv_factor)) != 0)
		{
			return 1;
		}
#if 0
	sprintf(tempstring,"RP %.3f\n",
	        rel_pressure(ws2300, config.pressure_conv_factor) );
	strcat(logline, tempstring);


	/* RELATIVE PRESSURE MIN/MAX */

	rel_pressure_minmax(ws2300, config.pressure_conv_factor, &tempfloat_min,
	                    &tempfloat_max, &time_min, &time_max);

	sprintf(tempstring, "RPmin %.3f\nRPmax %.3f\n"
	                    "TRPmin %02d:%02d\nDRPmin %04d-%02d-%02d\n"
	                    "TRPmax %02d:%02d\nDRPmax %04d-%02d-%02d\n",
	        tempfloat_min, tempfloat_max,
	        time_min.hour, time_min.minute, time_min.year, time_min.month, time_min.day,
	        time_max.hour, time_max.minute, time_max.year, time_max.month, time_max.day);
	strcat(logline, tempstring);
#endif
	}
	/* READ TENDENCY AND FORECAST */
	if (forecast_enabled)
	{
#if 0
	tendency_forecast(ws2300, tendency, forecast);
	sprintf(tempstring, "Tendency %s\nForecast %s\n", tendency, forecast);
	strcat(logline, tempstring);


	/* GET DATE AND TIME FOR LOG FILE, PLACE BEFORE ALL DATA IN LOG LINE */

	time(&basictime);
	strftime(datestring, sizeof(datestring),"Date %Y-%b-%d\nTime %H:%M:%S\n",
	         localtime(&basictime));

	// Print out and leave

	printf("%s%s", datestring, logline);
#endif
	}
	close_weatherstation(ws2300);

	mosquitto_destroy(mqtt);
	mosquitto_lib_cleanup();

	return (0);
}
