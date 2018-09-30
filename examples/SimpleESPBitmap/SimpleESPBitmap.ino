
#ifndef ESP8266
    #ERROR This example is for ESP8266 only
#endif

#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266HTTPClient.h>
ESP8266WiFiMulti WiFiMulti;

#include <ESPBitmap.h> //include bitmap library (supports 1, 4, 8, 24 bpp bitmaps)

void setup(void)
{
  Serial.begin(115200);

  for (uint8_t t = 4; t > 0; t--) {
    Serial.printf("[SETUP] WAIT %d...\n", t);
    Serial.flush();
    delay(1000);
  }

  WiFi.mode(WIFI_STA);
  WiFiMulti.addAP("SSID", "PASSWORD");
}

void loop(void)
{ 
   if ((WiFiMulti.run() == WL_CONNECTED)) {
        ESPBitmap bitmap;
        BITMAP_RESULT_t res = bitmap.fetchImageFromUrl("http://....../resonablySmallBitmap.bpm"); //a bitmap that is relitively small (fits in esp8266 memory);
        bitmap.printResult(res);
        if(res == BITMAP_SUCCESS)
        {
            for(int y = 0; y <bitmap.getHeight(); y++){
              for(int x = 0; x < bitmap.getWidth(); x++)
              {
                PIXEL_t pix = bitmap.getPixel(x, y);
                //Do something with the pixel data.
                Serial.printf("[x:%d y:%d {%d,%d,%d}]", x, y, pix.r, pix.g, pix.b);
              }
              Serial.println();
            }

        }
   }
   delay(1000);
}