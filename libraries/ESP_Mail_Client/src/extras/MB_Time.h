/*
 * Time helper class v1.0.1
 *
 * Created November 24, 2022
 *
 * The MIT License (MIT)
 * Copyright (c) 2022 K. Suwatchai (Mobizt)
 *
 *
 * Permission is hereby granted, free of charge, to any person returning a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef MB_Time_H
#define MB_Time_H

#if !defined(__AVR__)
#include <vector>
#include <string>
#endif

#include <time.h>
#include <Arduino.h>
#include "./ESP_Mail_FS.h"

#if defined(ESP_MAIL_USE_PSRAM)
#define MB_STRING_USE_PSRAM
#endif

#include "MB_String.h"
#include "MB_List.h"

#if defined(MB_MCU_ATMEL_ARM) || defined(MB_MCU_RP2040)
#include "../wcs/samd/lib/WiFiNINA.h"
#endif

#define ESP_TIME_DEFAULT_TS 1577836800

#if defined(__AVR__) || MB_MCU_TEENSY_ARM
#define MB_TIME_PGM_ATTR
#else
#define MB_TIME_PGM_ATTR PROGMEM
#endif

static const char *mb_months[12] MB_TIME_PGM_ATTR = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
static const char *mb_sdow[7] MB_TIME_PGM_ATTR = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};

class MB_Time
{
public:
  MB_Time()
  {
  }

  ~MB_Time()
  {
    _sv1.clear();
    _sv2.clear();
    _sv3.clear();
  }

  /** Set the system time from the NTP server
   *
   * @param gmtOffset The GMT time offset in hour.
   * @param daylightOffset The Daylight time offset in hour.
   * @param servers Optional. The NTP servers, use comma to separate the server.
   * @return boolean The status indicates the success of operation.
   *
   * @note This requires internet connection
   */
  bool setClock(float gmtOffset, float daylightOffset, const char *servers = "pool.ntp.org,time.nist.gov")
  {

#if defined(ESP32) || defined(ESP8266) || defined(ARDUINO_ARCH_SAMD) || defined(__AVR_ATmega4809__) || defined(ARDUINO_NANO_RP2040_CONNECT)

    TZ = gmtOffset;
    DST_MN = daylightOffset;
    now = time(nullptr);
#if (defined(ARDUINO_ARCH_SAMD) && !defined(ARDUINO_SAMD_MKR1000)) || defined(ARDUINO_NANO_RP2040_CONNECT)
    bool newConfig = TZ != gmtOffset || DST_MN != daylightOffset;
    unsigned long ts = WiFi.getTime();
    if (ts > 0)
    {

      now = ts;
      if (newConfig)
        now += TZ * 3600;
    }
#elif defined(ESP32) || defined(ESP8266)
    bool newConfig = TZ != gmtOffset || DST_MN != daylightOffset;
    if ((millis() - lastSyncMillis > 5000 || lastSyncMillis == 0) && (now < ESP_TIME_DEFAULT_TS || newConfig))
    {

      lastSyncMillis = millis();

      MB_VECTOR<MB_String> tk;
      MB_String sv = servers;

      splitToken(sv, tk, ',');

      switch (tk.size())
      {
      case 1:

        _sv1 = tk[0];
        configTime((TZ)*3600, (DST_MN)*60, _sv1.c_str());
        break;
      case 2:

        _sv1 = tk[0];
        _sv2 = tk[1];
        configTime((TZ)*3600, (DST_MN)*60, _sv1.c_str(), _sv2.c_str());
        break;
      case 3:

        _sv1 = tk[0];
        _sv2 = tk[1];
        _sv3 = tk[2];
        configTime((TZ)*3600, (DST_MN)*60, _sv1.c_str(), _sv2.c_str(), _sv3.c_str());
        break;
      default:

        configTime((TZ)*3600, (DST_MN)*60, "pool.ntp.org", "time.nist.gov");
        break;
      }

      now = time(nullptr);
      uint64_t tmp = now;
      tmp = tmp * 1000;
      msec_time_diff = tmp - millis();
    }

#if defined(ESP32)
    getLocalTime(&timeinfo);
#elif defined(ESP8266)
    localtime_r(&now, &timeinfo);
#endif

#endif

    _clockReady = now > ESP_TIME_DEFAULT_TS;

    if (_clockReady)
    {
      _sv1.clear();
      _sv2.clear();
      _sv3.clear();
    }

#else
    _clockReady = now > ESP_TIME_DEFAULT_TS;
#endif

    return _clockReady;
  }

  /** Get the timestamp from the year, month, date, hour, minute,
   * and second provided.
   *
   * @param year The year.
   * @param mon The month from 1 to 12.
   * @param date The dates.
   * @param hour The hours.
   * @param mins The minutes.
   * @param sec The seconds.
   * @return time_t The value of timestamp.
   */
  time_t getTimestamp(int year, int mon, int date, int hour, int mins, int sec)
  {
    struct tm timeinfo;
    timeinfo.tm_year = year - 1900;
    timeinfo.tm_mon = mon - 1;
    timeinfo.tm_mday = date;
    timeinfo.tm_hour = hour;
    timeinfo.tm_min = mins;
    timeinfo.tm_sec = sec;
    time_t ts = mktime(&timeinfo);
    return ts;
  }

  /** Get the timestamp from the RFC 2822 time string.
   * e.g. Mon, 02 May 2022 00:30:00 +0000 or 02 May 2022 00:30:00 +0000
   *
   * @param gmt Return the GMT time.
   * @return timestamp of time string.
   */
  time_t getTimestamp(const char *timeString, bool gmt = false)
  {
    time_t ts = 0;
    MB_VECTOR<MB_String> tk;
    MB_String s1 = timeString;

    splitToken(s1, tk, ' ');
    int day = 0, mon = 0, year = 0, hr = 0, mins = 0, sec = 0, tz_h = 0, tz_m = 0;

    int tkindex = tk.size() == 5 ? -1 : 0; // No week days?

    // some response may include (UTC) and (ICT)
    if (tk.size() >= 5)
    {
      day = atoi(tk[tkindex + 1].c_str());
      for (size_t i = 0; i < 12; i++)
      {
        if (strcmp_P(mb_months[i], tk[tkindex + 2].c_str()) == 0)
          mon = i;
      }

      // RFC 822 year to RFC 2822
      if (tk[tkindex + 3].length() == 2)
        tk[tkindex + 3].prepend("20");

      year = atoi(tk[tkindex + 3].c_str());

      MB_VECTOR<MB_String> tk2;
      splitToken(tk[tkindex + 4], tk2, ':');
      if (tk2.size() == 3)
      {
        hr = atoi(tk2[0].c_str());
        mins = atoi(tk2[1].c_str());
        sec = atoi(tk2[2].c_str());
      }

      ts = getTimestamp(year, mon + 1, day, hr, mins, sec);

      if (tk[tkindex + 5].length() == 5 && gmt)
      {
        char tmp[6];
        memset(tmp, 0, 6);
        strncpy(tmp, tk[tkindex + 5].c_str() + 1, 2);
        tz_h = atoi(tmp);

        memset(tmp, 0, 6);
        strncpy(tmp, tk[tkindex + 5].c_str() + 3, 2);
        tz_m = atoi(tmp);

        time_t tz = tz_h * 60 * 60 + tz_m * 60;
        if (tk[tkindex + 5][0] == '+')
          ts -= tz; // remove time zone offset
        else
          ts += tz;
      }
    }
    return ts;
  }

  /** Get the current timestamp.
   *
   * @return uint64_t The value of current timestamp.
   */
  uint64_t getCurrentTimestamp()
  {
    getTime();
    return now;
  }

  /** Get the current date time string that valid for Email.
   *
   * @return String The current date time string.
   */
  String getDateTimeString()
  {
    getTime();
    MB_String s;

    s = mb_sdow[timeinfo.tm_wday];

    s += ',';
    s += ' ';
    s += timeinfo.tm_mday;

    s += ' ';
    s += mb_months[timeinfo.tm_mon];

    s += ' ';
    s += timeinfo.tm_year + 1900;

    s += ' ';
    if (timeinfo.tm_hour < 10)
      s += 0;
    s += timeinfo.tm_hour;

    s += ':';
    if (timeinfo.tm_min < 10)
      s += 0;
    s += timeinfo.tm_min;

    s += ':';
    if (timeinfo.tm_sec < 10)
      s += 0;
    s += timeinfo.tm_sec;

    int p = 1;
    if (TZ < 0)
      p = -1;

    float dif = (p * (TZ * 10 - (int)TZ * 10)) * 60.0 / 10;

    s += ' ';

    s += (TZ < 0) ? '-' : '+';

    if ((int)TZ < 10)
      s += 0;
    s += (int)TZ;

    if (dif < 10)
      s += 0;

    s += (int)dif;

    return s.c_str();
  }

  /** get the clock ready state */
  bool clockReady()
  {

#if !defined(__arm__)
    now = time(nullptr);
#endif
    _clockReady = now > ESP_TIME_DEFAULT_TS;
    if (_clockReady)
    {
      _sv1.clear();
      _sv2.clear();
      _sv3.clear();
    }
    return _clockReady;
  }

  time_t now;
  uint64_t msec_time_diff = 0;
  struct tm timeinfo;
  float TZ = 0.0;
  float DST_MN = 0.0;

private:
  void splitToken(MB_String &str, MB_VECTOR<MB_String> &tk, char delim)
  {
    size_t current, previous = 0;
    current = str.find(delim, previous);
    MB_String s;
    while (current != MB_String::npos)
    {

      s = str.substr(previous, current - previous);
      s.trim();

      if (s.length() > 0)
        tk.push_back(s);
      previous = current + 1;
      current = str.find(delim, previous);
    }

    s = str.substr(previous, current - previous);

    s.trim();

    if (s.length() > 0)
      tk.push_back(s);

    s.clear();
  }

  void getTime()
  {
#if defined(ESP32)
    now = time(nullptr);
    getLocalTime(&timeinfo);
#elif defined(ESP8266)
    now = time(nullptr);
    localtime_r(&now, &timeinfo);
#elif (defined(ARDUINO_ARCH_SAMD) && defined(__AVR_ATmega4809__)) || defined(ARDUINO_NANO_RP2040_CONNECT)
    unsigned long ts = WiFi.getTime();
    if (ts > 0)
      now = ts + TZ * 3600;
    localtime_r(&now, &timeinfo);
#endif
  }

  bool _clockReady = false;
  unsigned long lastSyncMillis = 0;
  // in ESP8266 these NTP sever strings should be existed during configuring time.
  MB_String _sv1, _sv2, _sv3;
};

#endif // MB_Time_H
