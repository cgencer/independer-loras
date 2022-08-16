#include <WiFi.h>
#include <HTTPClient.h>

WiFiClient i_wifi_client;
// WiFiClientSecure i_wifi_client; (https)

int c_wifi_server_max_attempts = 500;

boolean i_wifi_connect()
{

    char *char_array_ssid = const_cast<char *>(state_wifi_ssid.c_str());
    char *char_array_pw = const_cast<char *>(state_wifi_pw.c_str());

    WiFi.begin(char_array_ssid, char_array_pw);
    Serial.println("[Should connect to WIFI] START");

    int c_wifi_max_fails = c_wifi_server_max_attempts;

    int wifi_fail_connect_counter = 0;

    /* waiting for WiFi connect */
    while (WiFi.status() != WL_CONNECTED and wifi_fail_connect_counter < c_wifi_max_fails)
    {
        wifi_fail_connect_counter++;
        Serial.print("AW=" + String(wifi_fail_connect_counter) + "/" + String(c_wifi_max_fails) + " ");
        delay(100);
    }

    Serial.println("\n[Should connect to WIFI] STOP");

    if (WiFi.status() == WL_CONNECTED)
    {
        // i_wifi_client.setCACert(i_root_ca);
        return true;
    }
    else
    {
        return false;
    }
}

void i_wifi_disconnect()
{
    WiFi.disconnect(true);
    Serial.println("[Disconnect WIFI]\n");
}

boolean wifi_check_status()
{
    boolean connection_flag = i_wifi_connect();
    i_wifi_disconnect();
    return connection_flag;
}

/*
 * ####################################
 *  Register Device Section
 * ####################################
 */

struct S_WIFI_REGISTER
{
    boolean success;
    String token;
};

S_WIFI_REGISTER wifi_register_device(String device_id, String secret, String serverUrl, int serverPort, int serverTimeout)
{

    char *c_wifi_server_url = const_cast<char *>(serverUrl.c_str());
    int c_wifi_server_port = serverPort;
    int c_wifi_timeout = serverTimeout;

    S_WIFI_REGISTER ret = {false, ""};

    if (i_wifi_connect())
    {
        Serial.println("Connect to server (timeout " + String(c_wifi_timeout) + ")");
        if (!i_wifi_client.connect(c_wifi_server_url, c_wifi_server_port, c_wifi_timeout))
            Serial.println("Connection failed!");
        else
        {
            Serial.println("Connected to server!");

            DynamicJsonDocument doc(1024 * 10);

            doc["id"] = device_id;
            doc["secret"] = secret;

            String body = "";

            serializeJson(doc, body);

            String send_string = String("POST ") + "/v1/register" + " HTTP/1.1\r\n" +
                                 "Host: " + String(c_wifi_server_url) + ":" + String(c_wifi_server_port) + "\r\n" +
                                 "Content-Type: application/json" + "\r\n" +
                                 "Content-Length: " + body.length() + "\r\n\r\n" +
                                 body + "\r\n" +
                                 "Connection: close\r\n\r\n";

            Serial.println("Send String is '" + send_string + "'");

            i_wifi_client.print(send_string);

            int c_mail_max_fails = c_wifi_server_max_attempts;

            int mail_fail_connect_counter = 0;

            Serial.println("[Waiting for response] START");
            while (!i_wifi_client.available() and mail_fail_connect_counter < c_mail_max_fails)
            {
                mail_fail_connect_counter++;
                Serial.print("AM=" + String(mail_fail_connect_counter) + "/" + String(c_mail_max_fails) + " ");
                delay(50);
            }
            Serial.println("\n[Waiting for response] STOP");

            String line = "";
            while (i_wifi_client.available())
            {
                line += i_wifi_client.readStringUntil('\r');
            }
            Serial.print("Response '" + line + "'");

            if (line.indexOf("OK-") > 0)
            {
                ret.success = true;
                ret.token = line.substring(line.indexOf("OK-") + 3);
            }

            /* if the server disconnected, stop the client */
            if (!i_wifi_client.connected())
            {
                Serial.println();
                Serial.println("Server disconnected");
            }

            i_wifi_client.stop();
        }
    }
    else
        Serial.println("Could not send: Not connected");

    i_wifi_disconnect();

    return ret;
}

/*
 * ####################################
 *  Send Chat Message Section
 * ####################################
 */

boolean wifi_send_chat_message(String receiver, String author, String msg, String serverUrl, int serverPort, int serverTimeout, String serverDeviceToken)
{

    char *c_wifi_server_url = const_cast<char *>(serverUrl.c_str());
    int c_wifi_server_port = serverPort;
    int c_wifi_timeout = serverTimeout;

    boolean ret = false;

    if (i_wifi_connect())
    {
        Serial.println("Connect to server (timeout " + String(c_wifi_timeout) + ")");
        if (!i_wifi_client.connect(c_wifi_server_url, c_wifi_server_port, c_wifi_timeout))
            Serial.println("Connection failed!");
        else
        {
            Serial.println("Connected to server!");

            DynamicJsonDocument doc(1024 * 10);

            doc["auth-id"] = author;
            doc["auth-token"] = serverDeviceToken;
            doc["receiver"] = receiver;
            doc["msg"] = msg;

            String body = "";

            serializeJson(doc, body);

            String send_string = String("POST ") + "/v1/writemsg" + " HTTP/1.1\r\n" +
                                 "Host: " + String(c_wifi_server_url) + ":" + String(c_wifi_server_port) + "\r\n" +
                                 "Content-Type: application/json" + "\r\n" +
                                 "Content-Length: " + body.length() + "\r\n\r\n" +
                                 body + "\r\n" +
                                 "Connection: close\r\n\r\n";

            Serial.println("Send String is '" + send_string + "'");

            i_wifi_client.print(send_string);

            int c_mail_max_fails = c_wifi_server_max_attempts;

            int mail_fail_connect_counter = 0;

            Serial.println("[Waiting for response] START");
            while (!i_wifi_client.available() and mail_fail_connect_counter < c_mail_max_fails)
            {
                mail_fail_connect_counter++;
                Serial.print("AM=" + String(mail_fail_connect_counter) + "/" + String(c_mail_max_fails) + " ");
                delay(50);
            }
            Serial.println("\n[Waiting for response] STOP");

            String line = "";
            while (i_wifi_client.available())
            {
                line += i_wifi_client.readStringUntil('\r');
            }
            Serial.print("Response '" + line + "'");

            if (line.endsWith("OK"))
            {
                ret = true;
            }

            /* if the server disconnected, stop the client */
            if (!i_wifi_client.connected())
            {
                Serial.println();
                Serial.println("Server disconnected");
            }

            i_wifi_client.stop();
        }
    }
    else
        Serial.println("Could not send: Not connected");

    i_wifi_disconnect();

    return ret;
}

/*
 * ####################################
 *  Get Chat Messages Section
 * ####################################
 */

String wifi_get_chat_messages(String myId, String serverUrl, int serverPort, int serverTimeout, String serverDeviceToken)
{

    char *c_wifi_server_url = const_cast<char *>(serverUrl.c_str());
    int c_wifi_server_port = serverPort;
    int c_wifi_timeout = serverTimeout;

    String ret = "";

    if (i_wifi_connect())
    {
        Serial.println("Connect to server (timeout " + String(c_wifi_timeout) + ")");
        if (!i_wifi_client.connect(c_wifi_server_url, c_wifi_server_port, c_wifi_timeout))
            Serial.println("Connection failed!");
        else
        {
            Serial.println("Connected to server!");

            DynamicJsonDocument doc(1024 * 10);

            doc["auth-id"] = myId;
            doc["auth-token"] = serverDeviceToken;

            String body = "";

            serializeJson(doc, body);

            String send_string = String("POST ") + "/v1/getmsgs" + " HTTP/1.1\r\n" +
                                 "Host: " + String(c_wifi_server_url) + ":" + String(c_wifi_server_port) + "\r\n" +
                                 "Content-Type: application/json" + "\r\n" +
                                 "Content-Length: " + body.length() + "\r\n\r\n" +
                                 body + "\r\n" +
                                 "Connection: close\r\n\r\n";

            Serial.println("Send String is '" + send_string + "'");

            i_wifi_client.print(send_string);

            int c_mail_max_fails = c_wifi_server_max_attempts;

            int mail_fail_connect_counter = 0;

            Serial.println("[Waiting for response] START");
            while (!i_wifi_client.available() and mail_fail_connect_counter < c_mail_max_fails)
            {
                mail_fail_connect_counter++;
                Serial.print("AM=" + String(mail_fail_connect_counter) + "/" + String(c_mail_max_fails) + " ");
                delay(50);
            }
            Serial.println("\n[Waiting for response] STOP");

            String line = "";
            while (i_wifi_client.available())
            {
                line += i_wifi_client.readStringUntil('\r');
            }
            Serial.print("Response '" + line + "'");

            ret = line;

            /* if the server disconnected, stop the client */
            if (!i_wifi_client.connected())
            {
                Serial.println();
                Serial.println("Server disconnected");
            }

            i_wifi_client.stop();
        }
    }
    else
        Serial.println("Could not send: Not connected");

    i_wifi_disconnect();

    return ret;
}

/*
 * ####################################
 *  Clear Chat Message Section
 * ####################################
 */

boolean wifi_clear_message(String myId, String serverUrl, int serverPort, int serverTimeout, String serverDeviceToken)
{

    char *c_wifi_server_url = const_cast<char *>(serverUrl.c_str());
    int c_wifi_server_port = serverPort;
    int c_wifi_timeout = serverTimeout;

    boolean ret = false;

    if (i_wifi_connect())
    {
        Serial.println("Connect to server (timeout " + String(c_wifi_timeout) + ")");
        if (!i_wifi_client.connect(c_wifi_server_url, c_wifi_server_port, c_wifi_timeout))
            Serial.println("Connection failed!");
        else
        {
            Serial.println("Connected to server!");

            DynamicJsonDocument doc(1024 * 10);

            doc["auth-id"] = myId;
            doc["auth-token"] = serverDeviceToken;

            String body = "";

            serializeJson(doc, body);

            String send_string = String("POST ") + "/v1/clearmsgs" + " HTTP/1.1\r\n" +
                                 "Host: " + String(c_wifi_server_url) + ":" + String(c_wifi_server_port) + "\r\n" +
                                 "Content-Type: application/json" + "\r\n" +
                                 "Content-Length: " + body.length() + "\r\n\r\n" +
                                 body + "\r\n" +
                                 "Connection: close\r\n\r\n";

            Serial.println("Send String is '" + send_string + "'");

            i_wifi_client.print(send_string);

            int c_mail_max_fails = c_wifi_server_max_attempts;

            int mail_fail_connect_counter = 0;

            Serial.println("[Waiting for response] START");
            while (!i_wifi_client.available() and mail_fail_connect_counter < c_mail_max_fails)
            {
                mail_fail_connect_counter++;
                Serial.print("AM=" + String(mail_fail_connect_counter) + "/" + String(c_mail_max_fails) + " ");
                delay(50);
            }
            Serial.println("\n[Waiting for response] STOP");

            String line = "";
            while (i_wifi_client.available())
            {
                line += i_wifi_client.readStringUntil('\r');
            }
            Serial.print("Response '" + line + "'");

            if (line.endsWith("OK"))
            {
                ret = true;
            }

            /* if the server disconnected, stop the client */
            if (!i_wifi_client.connected())
            {
                Serial.println();
                Serial.println("Server disconnected");
            }

            i_wifi_client.stop();
        }
    }
    else
        Serial.println("Could not send: Not connected");

    i_wifi_disconnect();

    return ret;
}