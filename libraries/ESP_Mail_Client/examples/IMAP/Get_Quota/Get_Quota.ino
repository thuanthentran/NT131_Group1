/**
 * This example shows how to get the quota root's resource usage and limits.
 *
 * Email: suwatchai@outlook.com
 *
 * Github: https://github.com/mobizt/ESP-Mail-Client
 *
 * Copyright (c) 2022 mobizt
 *
 */

/** For ESP8266, with BearSSL WiFi Client
 * The memory reserved for completed valid SSL response from IMAP is 16 kbytes which
 * may cause your device out of memory reset in case the memory
 * allocation error.
 */

#include <Arduino.h>
#if defined(ESP32)
#include <WiFi.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#else

// Other Client defined here
// To use custom Client, define ENABLE_CUSTOM_CLIENT in  src/ESP_Mail_FS.h.
// See the example Custom_Client.ino for how to use.

#endif

#include <ESP_Mail_Client.h>

// To use only IMAP functions, you can exclude the SMTP from compilation, see ESP_Mail_FS.h.

#define WIFI_SSID "<ssid>"
#define WIFI_PASSWORD "<password>"

/** For Gmail, IMAP option should be enabled. https://support.google.com/mail/answer/7126229?hl=en
 * and also https://accounts.google.com/b/0/DisplayUnlockCaptcha
 *
 * Some Gmail user still not able to sign in using account password even above options were set up,
 * for this case, use "App Password" to sign in instead.
 * About Gmail "App Password", go to https://support.google.com/accounts/answer/185833?hl=en
 *
 * For Yahoo mail, log in to your yahoo mail in web browser and generate app password by go to
 * https://login.yahoo.com/account/security/app-passwords/add/confirm?src=noSrc
 *
 * To use Gmai and Yahoo's App Password to sign in, define the AUTHOR_PASSWORD with your App Password
 * and AUTHOR_EMAIL with your account email.
 */

/* The imap host name e.g. imap.gmail.com for GMail or outlook.office365.com for Outlook */
#define IMAP_HOST "<host>"

/** The imap port e.g.
 * 143  or esp_mail_imap_port_143
 * 993 or esp_mail_imap_port_993
 */
#define IMAP_PORT 993

/* The log in credentials */
#define AUTHOR_EMAIL "<email>"
#define AUTHOR_PASSWORD "<password>"

/* Print the list of mailbox folders */
void printAllMailboxesInfo(IMAPSession &imap);

/* Print the selected folder info */
void printSelectedMailboxInfo(IMAPSession &imap);

/* Declare the global used IMAPSession object for IMAP transport */
IMAPSession imap;

void setup()
{

    Serial.begin(115200);

#if defined(ARDUINO_ARCH_SAMD)
    while (!Serial)
        ;
    Serial.println();
    Serial.println("**** Custom built WiFiNINA firmware need to be installed.****\nTo install firmware, read the instruction here, https://github.com/mobizt/ESP-Mail-Client#install-custom-built-wifinina-firmware");

#endif

    Serial.println();

    Serial.print("Connecting to AP");

    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    while (WiFi.status() != WL_CONNECTED)
    {
        Serial.print(".");
        delay(200);
    }

    Serial.println("");
    Serial.println("WiFi connected.");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
    Serial.println();

    /*  Set the network reconnection option */
    MailClient.networkReconnect(true);

    /** Enable the debug via Serial port
     * 0 for no debugging
     * 1 for basic level debugging
     *
     * Debug port can be changed via ESP_MAIL_DEFAULT_DEBUG_PORT in ESP_Mail_FS.h
     */
    imap.debug(1);

    /* Declare the ESP_Mail_Session for user defined session credentials */
    ESP_Mail_Session session;

    /* Set the session config */
    session.server.host_name = IMAP_HOST;
    session.server.port = IMAP_PORT;
    session.login.email = AUTHOR_EMAIL;
    session.login.password = AUTHOR_PASSWORD;

    /** Declare the IMAP_Config object used for user defined IMAP operating options
     * and contains the IMAP operating result
     */
    IMAP_Config config;

    /* Connect to the server */
    if (!imap.connect(&session /* session credentials */, &config /* operating options and its result */))
        return;

    IMAP_Quota_Root_Info info;

    Serial.println("\nGet quota root...");

    if (!imap.getQuota("", &info))
    {
        Serial.println("Get quota root failed");
    }
    else
    {
        ESP_MAIL_PRINTF("Quota root: %s, Resource name: %s, Usage (k): %d, Limit (k): %d\n", info.quota_root.c_str(), info.name.c_str(), (int)info.usage, (int)info.limit);
    }

    IMAP_Quota_Roots_List quota_roots;

    Serial.println("\nGet quota roots list for INBOX folder...");

    if (!imap.getQuotaRoot("INBOX", &quota_roots))
    {
        Serial.println("Get quota roots list failed");
    }
    else
    {
        for (size_t i = 0; i < quota_roots.size(); i++)
            ESP_MAIL_PRINTF("%d, Quota root: %s, Resource name: %s, Usage (k): %d, Limit (k): %d\n", (int)i + 1, quota_roots[i].quota_root.c_str(), quota_roots[i].name.c_str(), (int)quota_roots[i].usage, (int)quota_roots[i].limit);
    }

    Serial.println("\nSet quota root...");

    info.name = "STORAGE";
    info.limit = 1024 * 1024; // 1048576 k or 1 G.

    if (!imap.setQuota("", &info))
    {
        Serial.println("Set quota root failed");
    }
    else
    {
        Serial.println("Set quota root success");
    }

    ESP_MAIL_PRINTF("Free Heap: %d\n", MailClient.getFreeHeap());
}

void loop()
{
}
