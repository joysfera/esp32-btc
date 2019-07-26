/********************************************
    ESP32 LCD TFT / OLED display demo

    Prints current time and BTC price in USD

    written by Petr Stehlik in 2019/07/25

    released under the GNU GPL

    https://github.com/joysfera/
*********************************************/

#define HAS_OLED false  // TTGO T-Display by default, change to true for OLED SSD1306

#include <WiFiClientSecure.h>
#include <WiFiUdp.h>
#include <NTPClient.h>  // https://github.com/arduino-libraries/NTPClient
#include <Tasker.h>     // https://github.com/joysfera/arduino-tasker

#if HAS_OLED
# include <Wire.h>
# include <Adafruit_GFX.h>
# include <Adafruit_SSD1306.h>
#else
# include <TFT_eSPI.h>
# include <SPI.h>
#endif

WiFiClientSecure client;
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);

Tasker tasker;

#if HAS_OLED
Adafruit_SSD1306 disp(128, 64, &Wire, -1);
#else
# define TFT_BL          4  // Display backlight control pin
TFT_eSPI disp = TFT_eSPI(135, 240);
#endif

const char* ssid     = "yourssid";     // your network SSID (name of wifi network)
const char* password = "yourpassword"; // your network password
const char* server = "api.coinbase.com";  // Server URL

void setup()
{
    Serial.begin(115200);
    delay(100);

#if HAS_OLED
    Wire.begin(5, 4);
    if (!disp.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3C for 128x64
        Serial.println(F("SSD1306 init failed"));
        for (;;) ; // no display no fun
    }
    disp.setTextColor(WHITE);
#else
    if (TFT_BL > 0) {
        pinMode(TFT_BL, OUTPUT);
        digitalWrite(TFT_BL, HIGH);
    }
    disp.init();
    disp.setRotation(1);
    disp.fillScreen(TFT_BLACK);
    disp.setTextColor(TFT_WHITE, TFT_BLACK);
    disp.setTextDatum(TL_DATUM);
#endif

    disp.print("Connecting to WiFi");
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(ssid);
    WiFi.begin(ssid, password);

    // attempt to connect to Wifi network:
    while (WiFi.status() != WL_CONNECTED) {
        disp.print('.');
        Serial.print('.');
        // wait 1 second for re-trying
        delay(1000);
    }
    Serial.println(" OK");

    timeClient.begin();
    timeClient.setTimeOffset(3600 + 3600); // CEST
    timeClient.update();

    displayBTC();
}

void loop()
{
    timeClient.update();                  // keep time up-to-date
    tasker.loop();
    if (digitalRead(0) == LOW)            // if button on GPIO0 is pressed then
        tasker.setTimeout(displayBTC, 250); // update display almost instantly
}

void displayBTC(void)
{
#if HAS_OLED
    disp.clearDisplay();
#else
    disp.fillScreen(TFT_BLACK);
#endif

    disp.setTextSize(HAS_OLED ? 2 : 3);
    disp.setCursor(0, 0);
    disp.print(timeClient.getFormattedTime());
    disp.setCursor(0, HAS_OLED ? 20 : 30);
    disp.print("BTC price:");
    disp.setTextSize(HAS_OLED ? 3 : 6);
    disp.setCursor(0, HAS_OLED ? 40 : 60);
    disp.print('$');
    disp.print(getBTC());

#if HAS_OLED
    disp.display();
#endif

    tasker.setTimeout(displayBTC, 100000);    // next round in 100 seconds
}

int getBTC(void)
{
    int btc = 0;
    Serial.println("Starting connection to server...");
    if (!client.connect(server, 443)) {
        Serial.println("Connection failed!");
        btc = -1;
    }
    else {
        Serial.println("Connected to server!");
        // Make a HTTP request:
        client.println("GET https://api.coinbase.com/v2/prices/BTC-USD/spot HTTP/1.0");
        client.println("Host: api.coinbase.com");
        client.println("Connection: close");
        client.println();

        while (client.connected()) {
            String line = client.readStringUntil('\n');
            if (line == "\r") break; // end of HTTP headers
        }
        while (client.available()) {
            String line = client.readStringUntil('\n');
            int a = line.indexOf("amount");       // naive parsing of the JSON reply
            if (a > 0) {
                String amount = line.substring(a + 9);
                btc = amount.toInt();
                Serial.print("BTC = $");
                Serial.println(btc);
            }
            break;
        }
        client.stop();
    }
    return btc;
}
