//------------------------HOLD/NAP-ADATOK-LIB----------------------------------------
#include <ESPHTTPClient.h>
#include "SunMoonCalc.h"
#include <time.h>
const float eszaki_szelesseg = 11.111111; //HOLD/NAP ADATOKHOZ TARTOZÓ KOORDINÁTÁK
const float keleti_hosszusag = 22.222222; //HOLD/NAP ADATOKHOZ TARTOZÓ KOORDINÁTÁK

//-------------------------------BLYNK-----------------------------------------------
#include <BlynkSimpleEsp8266.h>
char auth[] = "12345678901234456789456";  //BLYNK TOKEN KÓD

//-------------------------------TFT_eSPI-LIB----------------------------------------
#define FS_NO_GLOBALS
#include <FS.h>
#include <TFT_eSPI.h>
#include <SPI.h>
TFT_eSPI    tft = TFT_eSPI();
TFT_eSprite spr = TFT_eSprite(&tft);

//--------------------------UNIX-KONVERTER-LIB---------------------------------------
#include <UnixEpochToDate.h>
UnixEpochToDate converter;

//----------------------------SHT-HŐ-PÁRA-LIB----------------------------------------
#include <Wire.h>
#include "SHT21.h"
SHT21 SHT21;

//------------------------OPENWEATHERMAP-LIB-----------------------------------------
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <JsonListener.h>
#include <OpenWeatherMapCurrent.h>
#include <OpenWeatherMapForecast.h>
#define WIFI_SSID "SSID" //"WIFI_HÁLÓZATOD_NEVE"
#define WIFI_PASS "PASSWORD"  //"WIFI_HÁLÓZATOD_JELSZAVA"
#define WIFI_HOSTNAME "Weather-station" //EZ LESZ A HÁLÓZATI NEVE AZ ÁLLOMÁSNAK
String OPEN_WEATHER_MAP_APP_ID = "12345678901234456789456"; //GENERÁLNI KELL EGY KÓDOT
const boolean IS_METRIC = true;
String OPEN_WEATHER_MAP_LOCATION_ID = "1234567"; //3043293 VÁROSOD LOCATION ID-JE
String DISPLAYED_CITY_NAME = "Vac"; //VÁROS NEVE(NINCS HATÁSSAL SEMMIRE)
String OPEN_WEATHER_MAP_LANGUAGE = "HU"; //KAPOTT ADATOK NYELVE EZ CSAK A STRINGBEN KAPOTT DESCRIPTION-RE VAN HATÁSSAL (NEM HASZNÁLTAM)
const int UPDATE_INTERVAL_SECS = 30; // 30SEC-ENKÉNT FRISSÍT 15 ALÁ NEM ÉRDEMES ÁLLÍTANI
#define MAX_FORECASTS 24  //ELŐREJELZETT IDŐPONTOK SZÁMA
long lastDownloadUpdate = millis();
OpenWeatherMapCurrentData currentWeather;
OpenWeatherMapForecastData forecasts[MAX_FORECASTS];

//------------------------FÉNYERŐ-ÁLLÍTÁSA-------------------------------------------
int feny = 0;
int blynkgomb;

//------------------------ELŐREJELZÉSHEZ-TARTOZÓ-VÁLTOZÓK----------------------------------------
int ikon[] = {0, 1, 2, 3, 4}; //ELŐREJELZETT IKON INTEGERRÉ ALAKÍTÁSÁNAK VÁLTOZÓI
String elorejelzett_ora_0;
String elorejelzett_ora_1;
String elorejelzett_ora_2;
String elorejelzett_ora_3;
String elorejelzett_ora_4;

//ELŐREJELZÉS FORCIKLUS VÁLTOZÓI
int x;  //X KOORDINÁTA, A KIRAJZOLÁS HELYÉT HATÁROZZA MEG
int x2; //2. X
int y;  //Y
int k;  //1-2-3-4-5

char elorejelzett_homerseklet_1[10];
char elorejelzett_homerseklet_2[10];
char elorejelzett_homerseklet_3[10];
char elorejelzett_homerseklet_4[10];
char elorejelzett_homerseklet_5[10];

//---------------HOLDHOZ-ÉS-NAPHOZ-KAPCSOLÓDÓ-VÁLTOZÓK------------------------------------
String holdkelte; //UNIX KONVERTÁLÁSA UTÁN HOLDKLETE XX:XX
String holdnyugta;  //NYUGTA XX:XX
int holdkora;  //HOLD KORA
String napnyugta;
String napfelkelte;

//-----------------------AKTUÁLIS-DÁTUM-IDŐ-VÁLTOZÓK---------------------------------------
int het_napja; //HÉT NAPJA 1-7IG VASÁRNAP AZ 1
String ido; //KONVERTÁLT PONTOS IDŐ XX:XX
String datum; //KONVERTÁLT DÁTUM YY.MM.DD, 19.12.13
bool nappal; //HA A NAP MÉG NEM MENT LE 1 HA IGEN 0

//-----------------------AKTUÁLIS-ID-PARAMÉTER-VÁLTOZÓK-----------------------------------
char belso_homerseklet[10];
char belso_paratartalom[10];
char kulso_homerseklet[10];
char kulso_paratartalom[10];
char nyomas[10];
char szel[10];
String aktualis_idojaras_leirasa; //MAGYAR NYELVŰ AKTUÁLIS IDŐJÁRÁS LEÍRÁS CSAK SERIAL PRINTNÉL HASZNÁLTAM
int aktualis_idojaras_ikonja; //AKTUÁLIS IDŐ IKONJÁNAK (SZÁM)KÓDJA
int aktualis_idojaras_blynkikonja; //BLYNKHEZ
int aktualis_idojaras_kodja; //3 JEGYŰ SZÁM AMI AZ AKTUÁLIS IDŐJÁRÁS SZERINT VÁLTOZIK
float hoerzet;
float harmatpont;
//-------------------------BETŰTÍPUSOK-DEFINIÁLÁSA------------------------------------------
#define FONT_14 "Calibri-Bold-14"
#define FONT_35 "Calibri-Bold-35"
#define FONT_42 "Calibri-Bold-42"
#define FONT_21 "Calibri-Bold-21"

//-------------------------BETŰSZÍNHEZ-TARTOZÓ-VÁLTOZÓK-------------------------------------
int TFT_KHO;
int TFT_SZEL;
int TFT_BHO;

//-----------------------DEBUGHOZ-----------------------------------------------------------
int teszt; //BLINKBŐL LEHET ÍRNI AZ ÉRTÉKET

//------------------------CSATLAKOZÁS-WIFIHÁLÓZATHOZ----------------------------------------
void csatlakozas_wifihez() {
  if (WiFi.status() == WL_CONNECTED) return;
  Serial.print("Csatlakozas a WiFi halozathoz ");
  Serial.print(WIFI_SSID);
  Serial.print("/");
  Serial.println(WIFI_PASS);
  WiFi.disconnect();
  WiFi.mode(WIFI_STA);
  WiFi.hostname(WIFI_HOSTNAME);
  WiFi.begin(WIFI_SSID, WIFI_PASS); //"SSID", "PW"
  int i = 0;
  while (WiFi.status() != WL_CONNECTED) { //AMÍG A WIFI NEM CSATLAKOZOTT .-KAT ÍR SERIAL MONITORRA
    delay(500);
    if (i > 80) i = 0;
    i += 10;
    Serial.print(".");
    if (i > 70) {
      tft.setCursor(135, 120); //KURZOR POZÍCIÓ VÁLASZTÁS
      tft.setTextColor(TFT_RED, TFT_BLACK); // BETŰ SZÍN ÉS HÁTTÉRSZÍN BEÁLLÍTÁSA
      tft.print("HIBA!");
    }
  }
  Serial.print("Csatlakoztatva...");
  tft.setTextColor(TFT_WHITE, TFT_BLACK); // BETŰ SZÍN ÉS HÁTTÉRSZÍN BEÁLLÍTÁSA
  tft.setCursor(130, 140); //KURZOR POZÍCIÓ VÁLASZTÁS
  tft.print("Csatlakoztatva");
}

BlynkTimer timer;

void setup(void) {
  Serial.begin(115200);
  //TFT
  tft.init();
  tft.setRotation(3); //KIJELZŐ ELFORGATÁS 1, 3 FEKVŐ 2, 4 ÁLLÓ
  spr.setColorDepth(16); //SZÍNMÉLYSÉG
  if (!SPIFFS.begin()) {  //SPIFFS INICIALIZÁLÁSA
    //Serial.println("SPIFFS inicializalas sikertelen!");
    while (1) yield(); // VÁRAKOZIK AMÍG NEM SIKERÜL
  }
  Serial.println("\r\nInicializalas sikeres.");
  pinMode(D8, OUTPUT); //KIJELZŐ HÁTTÉRVILÁGÍTÁSA
  analogWrite(D8, 1023);
  tft.fillScreen(TFT_BLACK); //HÁTTÉRSZÍN FEKETE
  tft.loadFont(FONT_21);  //21-ES MÉRETŰ BETŰ BETÖLTÉSE
  tft.setTextColor(TFT_WHITE, TFT_BLACK); // BETŰ SZÍN ÉS HÁTTÉRSZÍN BEÁLLÍTÁSA
  tft.setCursor(70, 100); //KURZOR POZÍCIÓ VÁLASZTÁS
  tft.print("Csatlakozás WiFi-hez...");
  csatlakozas_wifihez();
  tft.fillScreen(TFT_BLACK); //HÁTTÉRSZÍN FEKETE
  tft.setCursor(75, 100); //KURZOR POZÍCIÓ VÁLASZTÁS
  tft.print("Adatok frissítése...");
  adatok_frissitese();
  tft.fillScreen(TFT_BLACK); //HÁTTÉRSZÍN FEKETE
  tft.loadFont(FONT_21);  //21-ES MÉRETŰ BETŰ BETÖLTÉSE
  tft.setTextColor(TFT_WHITE, TFT_BLACK); // BETŰ SZÍN ÉS HÁTTÉRSZÍN BEÁLLÍTÁSA
  tft.setCursor(115, 50); //KURZOR POZÍCIÓ VÁLASZTÁS
  tft.print("Betöltés...");
  tft.loadFont(FONT_14);  //14-ES MÉRETŰ BETŰ BETÖLTÉSE
  tft.setTextColor(TFT_WHITE, TFT_BLACK); // BETŰ SZÍN ÉS HÁTTÉRSZÍN BEÁLLÍTÁSA
  tft.setCursor(240, 100); //KURZOR POZÍCIÓ VÁLASZTÁS
  tft.print("Betöltés...");
  tft.unloadFont();  //BETŰKÉSZLET ÜRÍTÉSE A MEMÓRIÁBÓL

  //SHT BELSŐ HŐ ÉS PÁRAMÉRŐ
  SHT21.begin();

  //BLYNK
  timer.setInterval(1000L, myTimerEvent); //1000ms A BLYNK FRISSÍTÉSI CIKLUSA
  Blynk.begin(auth, WIFI_SSID, WIFI_PASS);
  betuszinek();
  kirajzolas();
  elorejelzes_kirajzolas();
  hold_kirajzolas();
}

void loop(void) {
  if (millis() - lastDownloadUpdate > 1000 * UPDATE_INTERVAL_SECS) { //ADATOK CIKLIKUS FRISSÍTÉSE/KIRAJZOLÁSA
    adatok_frissitese();
    elorejelzes_kirajzolas();
    hold_kirajzolas();
    lastDownloadUpdate = millis();
  }
  Blynk.run();
  timer.run();
  fenyeroallitas();
  betuszinek();
  kirajzolas();
}

void betuszinek() {
  //----------------SZÉLERŐSÉG-BETŰSZÍN--------------------------------
  float szel_float = currentWeather.windSpeed * 3.6; //ms -> km/h
  if (szel_float < 5) {
    TFT_SZEL = 0x9D55; //GYENGE SZÉL SZÜRKE
  }
  if (szel_float >= 5 && szel_float < 20) {
    TFT_SZEL = 0xFE87; //MÉRSÉKELT SZÉL SÁRGA
  }
  if (szel_float >= 20 && szel_float < 40) {
    TFT_SZEL = 0xF4C4; //ÉLÉNK SZÉL NARANCS
  }
  if (szel_float > 40) {
    TFT_SZEL = 0xF800;  //ERŐS SZÉL VÖRÖS
  }

  //----------------BELSŐHŐMÉRSÉKLET-BETŰSZÍN--------------------------------
  float  belso_homerseklet_float = SHT21.getTemperature();
  if (belso_homerseklet_float >= 30) {
    TFT_BHO = 0xFCF3;  //VILÁGOSPIROS
  }
  if (belso_homerseklet_float < 30 && belso_homerseklet_float > 25) {
    TFT_BHO = 0xFED0;  //VILÁGOSNARANCS
  }
  if (belso_homerseklet_float <= 25 && belso_homerseklet_float > 24) {
    TFT_BHO = 0xDFF1;  //VILÁGOSSÁRGA
  }
  if (belso_homerseklet_float <= 24 && belso_homerseklet_float > 23) {
    TFT_BHO = 0x97F4;  //VILÁGOSZÖLDES
  }
  if (belso_homerseklet_float <= 23 && belso_homerseklet_float > 20) {
    TFT_BHO = 0x9FF9;  //VILÁGOSZÖLDES
  }
  if (belso_homerseklet_float <= 20 && belso_homerseklet_float > 19) {
    TFT_BHO = 0x97FC;  //VILÉGOSKÉKESZÖLD
  }
  if (belso_homerseklet_float <= 19 && belso_homerseklet_float > 18) {
    TFT_BHO = 0x96FF;  //VILÁGOSKÉK
  }
  if (belso_homerseklet_float <= 18) {
    TFT_BHO = 0xACDF;  //VILÁGOSLILA
  }

  //----------------KÜLSŐHŐMÉRSÉKLET-BETŰSZÍN--------------------------------
  float kulso_homerseklet_float = currentWeather.temp;
  if (kulso_homerseklet_float >= 30) {
    TFT_KHO = 0xFCF3;  //VILÁGOSPIROS
  }
  if (kulso_homerseklet_float < 30 && kulso_homerseklet_float > 25) {
    TFT_KHO = 0xFED0;  //VILÁGOSNARANCS
  }
  if (kulso_homerseklet_float <= 25 && kulso_homerseklet_float > 20) {
    TFT_KHO = 0xDFF1;  //VILÁGOSSÁRGA
  }
  if (kulso_homerseklet_float <= 20 && kulso_homerseklet_float > 15) {
    TFT_KHO = 0x97F4;  //VILÁGOSZÖLDES
  }
  if (kulso_homerseklet_float <= 15 && kulso_homerseklet_float > 10) {
    TFT_KHO = 0x9FF9;  //VILÁGOSZÖLDES
  }
  if (kulso_homerseklet_float <= 10 && kulso_homerseklet_float > 0) {
    TFT_KHO = 0x97FC;  //VILÉGOSKÉKESZÖLD
  }
  if (kulso_homerseklet_float <= 0 && kulso_homerseklet_float > -5) {
    TFT_KHO = 0x96FF;  //VILÁGOSKÉK
  }
  if (kulso_homerseklet_float <= -5) {
    TFT_KHO = 0xACDF;  //VILÁGOSLILA
  }

  //----------------HŐÉRZET-ÉS-HARMATPONT-SZÁMOLÁSA-----------------------------
  float kulso_ho_farenheit = (kulso_homerseklet_float * 1.8) + 32;
  float szel_mph = (szel_float * 0.621371192);
  hoerzet = ((35.74 + (0.6215 * kulso_ho_farenheit) - (35.75 * pow(szel_mph, 0.16)) + (0.4275 * kulso_ho_farenheit * pow(szel_mph, 0.16))) - 32) / 1.8;

  float gamma = log(SHT21.getHumidity() / 100) + 17.62 * SHT21.getTemperature() / (243.5 + SHT21.getTemperature());
  harmatpont = 243.5 * gamma / (17.62 - gamma);
}

void kirajzolas() {
  //---------------GRAFIKÁK-KERETEK--KONSTANSOK----------------------------------
  tft.drawLine(0, 126, 218, 126, TFT_DARKGREY); //__________________
  tft.drawLine(0, 166, 320, 166, TFT_DARKGREY); //___________________________
  tft.drawLine(102, 0, 102, 166, TFT_DARKGREY); //       |
  tft.drawLine(218, 0, 218, 166, TFT_DARKGREY); //                  |
  tft.drawLine(218, 72, 320, 72, TFT_DARKGREY); //                  _________

  dtostrf(SHT21.getTemperature(), 1, 1, belso_homerseklet); //FLOAT -> CHAR 1 TIZEDESIG
  dtostrf(SHT21.getHumidity(), 0, 0, belso_paratartalom); //FLOAT -> CHAR 0 TIZEDESIG

  tft.loadFont(FONT_14);
  tft.setTextColor(TFT_WHITE, TFT_BLACK); // BETŰ SZÍN ÉS HÁTTÉRSZÍN BEÁLLÍTÁSA
  tft.setCursor(30, 4);
  tft.print("KÜLSŐ");
  tft.setCursor(250, 4);
  tft.print("BELSŐ");
  tft.unloadFont();

  //----------------------NAPSZAK-MEGHATÁROZÁSA--------------------------------
  //A CURRENT ICON ÉRTÉKEI A NAPSZAKNAK MEGFELELŐEN IS VÁLTOZNAK (PÁROS/PÁRATLAN) ÍGY EZT FELHSZNÁLTAM A NAPPAL ÉS ÉJJSZAKA MEGHATÁROZÁSÁHOZ.
  if (aktualis_idojaras_ikonja % 2 == 0) { //PÁROS PÁRATLAN MEGHATÁROZÁSA MARADÉKOS OSZTÁSSAL
    nappal = 0; //HA PÁROS AKKOR ESTE VAN
  } else {
    nappal = 1; //PÁRATLAN AKKOR NAPPAL
  }

  /*SPR SPRITE, ELÉG NEHÉZ MEGFOGLAMAZNOM HOGY MI EZ, DE VALAHOGY ÍGY ÍRNÁ LE, EZ EGY LÁTHATALAN GARFIKAI KIJELZŐ AMIT A PROCESSZOR A RAMBAN TART.
    GRAFIKÁKAT BEÍRATJUK A SPRITE-BA ÉS ONNAN RAJZOLTATJUK KI A KIJELZŐRE, ÍGY NEM FOG VILLOGNI (flickering) A KIÍRT/RAJZOLT ADAT.
    PONTOSABB ANGOL NYELVŰ LEÍRÁST TALÁLHATSZ A TFT_eSPI HONLAPJÁN.  */

  //---------------BELSŐ-HŐMÉRÉSKLET--------------------------------------------
  spr.createSprite(100, 25); //100X25 PIXEL MÉRETŰ SPRITE LÉTREHOZÁSA
  spr.loadFont(FONT_35); // BETŰMÉRET VÁLTÁS
  spr.setTextDatum(MC_DATUM); // MIDDLE CENTER MINDIG A LÉTREHOZOTT SPRITE KÖZEPÉHEZ LESZNEK IGAZÍTVA A KIÍRT KARAKTEREK.
  spr.setTextColor(TFT_BHO, TFT_BLACK); // BETŰ SZÍN ÉS HÁTTÉRSZÍN BEÁLLÍTÁSA
  spr.fillSprite(TFT_BLACK); //SPRITE KITÖLTÉSE FEKETÉVEL
  spr.drawString(belso_homerseklet + String("°C"), 50, 12);  //BELSŐ HŐMÉRSÉKLET UTÁN RAK °C -JELET ÉS ÍGY ÍRJA KI.
  spr.pushSprite(220, 18);  //A LÉTREHOZOTT SPRITE KIÍRÁSA A KIJELZŐRE 220, 18 KOORDINÁTÁRA
  spr.deleteSprite(); // SPRITE TÖRLÉSE RAM VISSZANYERÉSE MIATT.

  //-------------------BELSŐ-PÁRA----------------------------------------------
  spr.createSprite(100, 25);
  spr.loadFont(FONT_35);
  spr.setTextDatum(MC_DATUM);
  spr.setTextColor(TFT_CYAN, TFT_BLACK);
  spr.fillSprite(TFT_BLACK);
  spr.drawString(belso_paratartalom + String("%"), 50, 12);
  spr.pushSprite(220, 46);
  spr.deleteSprite();

  //-------------------KÜLSŐ-HŐ------------------------------------------------
  spr.createSprite(100, 25);
  spr.loadFont(FONT_35);
  spr.setTextDatum(MC_DATUM);
  spr.setTextColor(TFT_KHO, TFT_BLACK); // BETŰ SZÍN ÉS HÁTTÉRSZÍN BEÁLLÍTÁSA
  spr.fillSprite(TFT_BLACK);
  spr.drawString(kulso_homerseklet + String("°C"), 50, 12);
  spr.pushSprite(0, 18);
  spr.deleteSprite();

  //-------------------KÜLSŐ-PÁRA---------------------------------------------
  spr.createSprite(100, 25);
  spr.loadFont(FONT_35);
  spr.setTextDatum(MC_DATUM);
  spr.setTextColor(TFT_CYAN, TFT_BLACK);
  spr.fillSprite(TFT_BLACK);
  spr.drawString(kulso_paratartalom + String("%"), 50, 12);
  spr.pushSprite(0, 46);
  spr.deleteSprite();

  //-------------------KÜLSŐ-NYOMÁS-SZÉL---------------------------------------
  spr.createSprite(100, 25);
  spr.loadFont(FONT_21);
  spr.setTextDatum(MC_DATUM);
  spr.setTextColor(TFT_YELLOW, TFT_BLACK);
  spr.fillSprite(TFT_BLACK);
  spr.drawString(nyomas + String("hPa"), 50, 12);
  spr.pushSprite(0, 74);
  spr.setTextColor(TFT_SZEL, TFT_BLACK); // BETŰ SZÍN ÉS HÁTTÉRSZÍN BEÁLLÍTÁSA
  spr.fillSprite(TFT_BLACK);
  spr.drawString(szel + String("km/h"), 50, 12);
  spr.pushSprite(0, 99);
  spr.deleteSprite();

  //---------------------------ÓRA----------------------------------------------
  spr.createSprite(100, 30);
  spr.loadFont(FONT_42);
  spr.setTextDatum(MC_DATUM);
  spr.setTextColor(TFT_WHITE, TFT_BLACK);
  spr.fillSprite(TFT_BLACK);
  spr.drawString(ido, 50, 15); //HH:MM
  spr.pushSprite(110, 130);
  spr.deleteSprite();

  //---------------------------NAP-HOLD-FEL-LE-----------------------------------------
  drawBmp("/rise.bmp", 257, 138); //NAPKELTE HOLDKELTE IKON KIRAJZOLÁSA 257, 138 -RA
  spr.createSprite(36, 10);
  spr.loadFont(FONT_14);
  spr.setTextDatum(MC_DATUM);
  spr.setTextColor(TFT_WHITE, TFT_BLACK);
  spr.fillSprite(TFT_BLACK);
  spr.drawString(napfelkelte, 18, 5); //HH:MM
  spr.pushSprite(220, 141);
  spr.deleteSprite();
  spr.createSprite(36, 10);
  spr.drawString(napnyugta, 18, 5); //HH:MM
  spr.pushSprite(220, 151);
  spr.deleteSprite();
  spr.createSprite(36, 10);
  spr.drawString(holdkelte, 18, 5); //HH:MM
  spr.pushSprite(284, 141);
  spr.deleteSprite();
  spr.createSprite(36, 10);
  spr.drawString(holdnyugta, 18, 5); //HH:MM
  spr.pushSprite(284, 151);
  spr.deleteSprite();

  //---------------------------DÁTUM-----------------------------------------
  spr.createSprite(100, 19);
  spr.loadFont(FONT_14);
  spr.setTextDatum(MC_DATUM);
  spr.setTextColor(TFT_WHITE, TFT_BLACK);
  spr.fillSprite(TFT_BLACK);
  spr.drawString(datum, 50, 10); //DÁTUM 20YY.MM.DD
  spr.pushSprite(0, 127);
  spr.deleteSprite();

  //---------------------------HÉTNAPJA---------------------
  spr.createSprite(100, 19);
  spr.loadFont(FONT_14);
  spr.setTextDatum(MC_DATUM);
  spr.setTextColor(TFT_GREENYELLOW, TFT_BLACK);
  spr.fillSprite(TFT_BLACK);
  switch (het_napja) {
    case 1:
      spr.drawString("VASÁRNAP", 50, 10);
      break;
    case 2:
      spr.drawString("HÉTFŐ", 50, 10);
      break;
    case 3:
      spr.drawString("KEDD", 50, 10);
      break;
    case 4:
      spr.drawString("SZERDA", 50, 10);
      break;
    case 5:
      spr.drawString("CSÜTÖRTÖK", 50, 10);
      break;
    case 6:
      spr.drawString("PÉNTEK", 50, 10);
      break;
    case 7:
      spr.drawString("SZOMBAT", 50, 10);
      break;
  }
  spr.pushSprite(0, 146);
  spr.deleteSprite();

  //-------------------------IKON-ÉS-LEÍRÁS-KIJELZŐRE-ÍRÁSA--------------------------------------------
  spr.createSprite(115, 16);
  spr.loadFont(FONT_14);
  spr.setTextDatum(MC_DATUM);
  spr.setTextColor(TFT_WHITE, TFT_BLACK);
  spr.fillSprite(TFT_BLACK);

  switch (aktualis_idojaras_kodja) {
    case 800: //Derült
      spr.drawString("DERÜLT", 58, 9);

      if (nappal == 1) {
        drawBmp("/derult_n.bmp", 103, 20);
        aktualis_idojaras_blynkikonja = 1;
      } else {
        drawBmp("/derult_e.bmp", 103, 20);
        aktualis_idojaras_blynkikonja = 2;
      }
      break;

    case 801: //kevés felhő
      spr.drawString("GYENGÉN FELHŐS", 58, 9);
      if (nappal == 1) {
        drawBmp("/gyengen_n.bmp", 103, 20);
        aktualis_idojaras_blynkikonja = 3;
      } else {
        drawBmp("/gyengen_e.bmp", 103, 20);
        aktualis_idojaras_blynkikonja = 4;
      }
      break;

    case 802: //szórványos felhőzet
      spr.drawString("KÖZEPESEN FELHŐS", 58, 9);
      if (nappal == 1) {
        drawBmp("/mersekelten_n.bmp", 103, 20);
        aktualis_idojaras_blynkikonja = 5;
      } else {
        drawBmp("/mersekelten_e.bmp", 103, 20);
        aktualis_idojaras_blynkikonja = 6;
      }
      break;

    case 803: //Erősen felhős
      spr.drawString("ERŐSEN FELHŐS", 58, 9);
      if (nappal == 1) {
        drawBmp("/erosen_n.bmp", 103, 20);
        aktualis_idojaras_blynkikonja = 7;
      } else {
        drawBmp("/erosen_e.bmp", 103, 20);
        aktualis_idojaras_blynkikonja = 8;
      }
      break;

    case 804: //Borult
      spr.drawString("BORULT", 58, 9);//
      drawBmp("/borult.bmp", 103, 20);
      aktualis_idojaras_blynkikonja = 9;
      break;

    case 771: //Viharos szél
      spr.drawString("VIHAROS SZÉL", 58, 9);//
      drawBmp("/szel.bmp", 103, 20);
      aktualis_idojaras_blynkikonja = 10;
      break;

    case 721: //Pára
      spr.drawString("PÁRA", 58, 9);//
      drawBmp("/kod.bmp", 103, 20);
      aktualis_idojaras_blynkikonja = 11;
      break;

    case 701://köd közepes
      spr.drawString("KÖD", 58, 9);//
      drawBmp("/kod.bmp", 103, 20);
      aktualis_idojaras_blynkikonja = 11;
      break;

    case 741://köd sűrű
      spr.drawString("SŰRŰ KÖD", 58, 9);//
      drawBmp("/kod.bmp", 103, 20);
      aktualis_idojaras_blynkikonja = 11;
      break;

    case 616://eső és hó
    case 615://enyhe eső és hó
    case 613://havaseső szitálás
    case 612: //enyhe havaseső
    case 611://havaseső
      spr.drawString("HAVAS ESŐ", 58, 9);//
      drawBmp("/havaseso.bmp", 103, 20);
      aktualis_idojaras_blynkikonja = 12;
      break;

    case 620: //enyhe Hózápor
    case 621://hózápor
      spr.drawString("HÓZÁPOR", 58, 9);//
      drawBmp("/havazas.bmp", 103, 20);
      aktualis_idojaras_blynkikonja = 13;
      break;

    case 622://erős hózápor
      spr.drawString("ERŐS HÓZÁPOR", 58, 9);//
      drawBmp("/havazas.bmp", 103, 20);
      aktualis_idojaras_blynkikonja = 13;
      break;

    case 601: //Havazás
      spr.drawString("HAVAZÁS", 58, 9);//
      drawBmp("/havazas.bmp", 103, 20);
      aktualis_idojaras_blynkikonja = 13;
      break;

    case 602://erős havazás
      spr.drawString("INTENZÍV HAVAZÁS", 58, 9);//
      drawBmp("/havazas.bmp", 103, 20);
      aktualis_idojaras_blynkikonja = 13;
      break;

    case 600: //Hószállingózás
      spr.drawString("HÓSZÁLLINGÓZÁS", 58, 9);//necces a hely
      drawBmp("/havazas.bmp", 103, 20);
      aktualis_idojaras_blynkikonja = 13;
      break;

    case 313://zápor, szitálás
    case 314://erős zápor, szitálás
    case 321://zápor, szitálás
    case 520://enyhe zápor
    case 521://zápor
    case 522://nagy intenzitásu zápor
    case 531://nagy intenzitásu zápor
      spr.drawString("ZÁPOR", 58, 9);//
      if (nappal == 1) {
        drawBmp("/zapor_n.bmp", 103, 20);
        aktualis_idojaras_blynkikonja = 14;
      } else {
        drawBmp("/zapor_e.bmp", 103, 20);
        aktualis_idojaras_blynkikonja = 15;
      }
      break;

    case 511: //ónos eső
      spr.drawString("ÓNOS ESŐ", 58, 9);//
      drawBmp("/onoseso.bmp", 103, 20);
      aktualis_idojaras_blynkikonja = 16;
      break;

    case 502://nagy intenzitású eső
    case 503://nagyon erős eső
    case 504://szélsőséges eső
      spr.drawString("FELHŐSZAKADÁS", 58, 9);//
      drawBmp("/eso.bmp", 103, 20);
      aktualis_idojaras_blynkikonja = 17;
      break;

    case 212://erős zivatar
    case 221://erős zivatar
      spr.drawString("ERŐS ZIVATAR", 58, 9);//
      drawBmp("/zivatar.bmp", 103, 20);
      aktualis_idojaras_blynkikonja = 18;
      break;

    case 200://zivatar gyenge esővel
    case 201://zivatar esővel
    case 202://zivatar erős esővel
    case 230://zivatar enyhe szitálással
    case 231://zivatar szitálással
    case 232:///mennydörgés erős szitálással
    case 210: //enyhe zivatar
      spr.drawString("ZIVATAR", 58, 9);//
      drawBmp("/zivatar.bmp", 103, 20);
      aktualis_idojaras_blynkikonja = 18;
      break;

    case 312://nagy intenzitású szitáló eső
    case 302://nagy intenzitású szitálás
      spr.drawString("ERŐS SZITÁLÁS", 58, 9);//
      drawBmp("/eso.bmp", 103, 20);
      aktualis_idojaras_blynkikonja = 17;
      break;

    case 300://enyhe Szitálás
    case 301://szitálás
    case 310://szitáló eső
    case 311://szitáló eső
      spr.drawString("SZITÁLÁS", 58, 9);//
      drawBmp("/gyengeeso.bmp", 103, 20);
      aktualis_idojaras_blynkikonja = 19;
      break;

    case 500://enyhe eső
      spr.drawString("GYENGE ESŐ", 58, 9);//
      drawBmp("/gyengeeso.bmp", 103, 20);
      aktualis_idojaras_blynkikonja = 19;
      break;

    case 501://mérsékelt eső
      spr.drawString("ESŐ", 58, 9);//
      drawBmp("/eso.bmp", 103, 20);
      aktualis_idojaras_blynkikonja = 17;
      break;

    case 211://száraz zivatar
      spr.drawString("SZÁRAZ ZIVATAR", 58, 9);// KELLIKON
      drawBmp("/szaraz.bmp", 103, 20);
      aktualis_idojaras_blynkikonja = 20;
      break;

    default: //711  smoke 721 haze 731 sand, dust whirls 751 sand 761 dust 762 volcanic ash 771 squalls 781 tornado
      spr.drawString("ISMERETLEN", 58, 9); //MAGYARORSZÁGON RITKA IDŐJÁRÁSI ESEMÉNYEK MELYEKET NEM DEFINIÁLTAM SZERINTEM FELESELGES.
      drawBmp("/unknow.bmp", 103, 20);
      aktualis_idojaras_blynkikonja = 21;
      break;
  }
  spr.pushSprite(103, 1);
  spr.deleteSprite();
  spr.unloadFont();
}

void hold_kirajzolas() {
  switch (holdkora) {
    case 0:
    case 1:
    case 28:
    case 29:
      drawBmp("/hold/1.bmp", 237, 75);
      break;
    case 2:
      drawBmp("/hold/2.bmp", 237, 75);
      break;
    case 3:
      drawBmp("/hold/3.bmp", 237, 75);
      break;
    case 4:
      drawBmp("/hold/4.bmp", 237, 75);
      break;
    case 5:
      drawBmp("/hold/5.bmp", 237, 75);
      break;
    case 6:
      drawBmp("/hold/6.bmp", 237, 75);
      break;
    case 7:
      drawBmp("/hold/7.bmp", 237, 75);
      break;
    case 8:
      drawBmp("/hold/8.bmp", 237, 75);
      break;
    case 9:
      drawBmp("/hold/9.bmp", 237, 75);
      break;
    case 10:
      drawBmp("/hold/10.bmp", 237, 75);
      break;
    case 11:
      drawBmp("/hold/11.bmp", 237, 75);
      break;
    case 12:
      drawBmp("/hold/12.bmp", 237, 75);
      break;
    case 13:
      drawBmp("/hold/13.bmp", 237, 75);
      break;
    case 14:
    case 15:
      drawBmp("/hold/14.bmp", 237, 75);
      break;
    case 16:
      drawBmp("/hold/15.bmp", 237, 75);
      break;
    case 17:
      drawBmp("/hold/16.bmp", 237, 75);
      break;
    case 18:
      drawBmp("/hold/17.bmp", 237, 75);
      break;
    case 19:
      drawBmp("/hold/18.bmp", 237, 75);
      break;
    case 20:
      drawBmp("/hold/19.bmp", 237, 75);
      break;
    case 21:
      drawBmp("/hold/20.bmp", 237, 75);
      break;
    case 22:
      drawBmp("/hold/21.bmp", 237, 75);
      break;
    case 23:
      drawBmp("/hold/22.bmp", 237, 75);
      break;
    case 24:
      drawBmp("/hold/23.bmp", 237, 75);
      break;
    case 25:
      drawBmp("/hold/24.bmp", 237, 75);
      break;
    case 26:
      drawBmp("/hold/25.bmp", 237, 75);
      break;
    case 27:
      drawBmp("/hold/26.bmp", 237, 75);
      break;
    default:
      //HIBA

      break;
  }
}

void elorejelzes_kirajzolas() {
  //----------------------------ELŐREJELZETT-IKONOK-KIRAJZOLÁSA------------------------------------
  for (k = 0; k < 5; k++) {
    switch (k) {
      case 0:
        x = 6;
        y = 180;
        x2 = 0;
        break;
      case 1:
        x = 70;
        y = 180;
        x2 = 64;
        break;
      case 2:
        x = 134;
        y = 180;
        x2 = 128;
        break;
      case 3:
        x = 198;
        y = 180;
        x2 = 192;
        break;
      case 4:
        x = 262;
        y = 180;
        x2 = 256;
        break;
    }

    switch (ikon[k]) {
      case 1:
        drawBmp("/elore/kderult_n.bmp", x, y);
        break;
      case 2:
        drawBmp("/elore/kderult_e.bmp", x, y);
        break;
      case 3:
        drawBmp("/elore/kgyengen_n.bmp", x, y);
        break;
      case 4:
        drawBmp("/elore/kgyengen_e.bmp", x, y); //moon+few cloud
        break;
      case 5:
        drawBmp("/elore/kerosen_n.bmp", x, y);  //scattered clouds day
        break;
      case 6:
        drawBmp("/elore/kerosen_e.bmp", x, y); //scattered clouds night
        break;
      case 7:
      case 8:
        drawBmp("/elore/kborult.bmp", x, y); //broken clouds day, night
        break;
      case 9:
      case 10:
        drawBmp("/elore/kgyengeeso.bmp", x, y); //shower rain day, night
        break;
      case 11:
      case 12:
        drawBmp("/elore/keso.bmp", x, y); //rain day, night
        break;
      case 13: //thunderstorm day
      case 14: //thunderstorm night
        drawBmp("/elore/kzivatar.bmp", x, y); //13,14 same thunderstorm icon
        break;
      case 15: //snow day
      case 16: //snow night
        drawBmp("/elore/khavazas.bmp", x, y); //15, 16 same snow icon
        break;
      case 17: //mist day
      case 18: //mist night
        drawBmp("/elore/kkod.bmp", x, y); //17,18 same mist icon
        break;
      default:
        //HIBA
        break;
    }
    
    /*ITT KÉT OPCIÓ VAN HA 3 ÓRÁNKÉNTI ELŐREJELZÉST SZERETNÉL KAPNI AKKOR HAGYD ÍGY, HA 2,5 NAPOS ELŐREJELZÉS KELL
    AKKOR TÖRLÖNI KELL AZ EZ A SOR ALATTI KOMMENTEZÉST. ÉS KIKOMMENTEZNI A ELŐREJELZETT-IDŐPONTOK-(ÓRA) RÉSZT, VALAMINT ÁTÍRNI AZ
    1079-AS SORBAN A uint8_t allowedHours[] = {0, 3, 6, 9, 12, 15, 18, 21}; uint8_t allowedHours[] = {12, 21}; RE
    */
    
    //----------------------------ELŐREJELZETT-NAPOK-KIÍRÁSA------------------------------------
    /*
      tft.loadFont(FONT_14);
      tft.setTextColor(TFT_WHITE, TFT_BLACK);
      tft.setCursor(x2, 168);

      switch (elorejelzett_het_napja[k]) {
      case 1: //vasarnap
        spr.createSprite(64, 12);
        spr.loadFont(FONT_14);
        spr.setTextDatum(MC_DATUM);
        if (ikon[k] % 2 == 0) { //ha páros akkor nappal van
          spr.setTextColor(TFT_DARKGREY, TFT_BLACK);
        } else {
          spr.setTextColor(TFT_YELLOW, TFT_BLACK);
        }
        spr.fillSprite(TFT_BLACK);
        spr.drawString("Vasárnap", 32, 6);
        spr.pushSprite(x2, 168);
        spr.deleteSprite();
        break;
      case 2: //hétfő
        spr.createSprite(64, 12);
        spr.loadFont(FONT_14);
        spr.setTextDatum(MC_DATUM);
        if (ikon[k] % 2 == 0) { //ha páros akkor nappal van
          spr.setTextColor(TFT_DARKGREY, TFT_BLACK);
        } else {
          spr.setTextColor(TFT_YELLOW, TFT_BLACK);
        }
        spr.fillSprite(TFT_BLACK);
        spr.drawString("Hétfő", 32, 6);
        spr.pushSprite(x2, 168);
        spr.deleteSprite();
        break;
      case 3: //kedd
        spr.createSprite(64, 12);
        spr.loadFont(FONT_14);
        spr.setTextDatum(MC_DATUM);
        if (ikon[k] % 2 == 0) { //ha páros akkor nappal van
          spr.setTextColor(TFT_DARKGREY, TFT_BLACK);
        } else {
          spr.setTextColor(TFT_YELLOW, TFT_BLACK);
        }
        spr.fillSprite(TFT_BLACK);
        spr.drawString("Kedd", 32, 6);
        spr.pushSprite(x2, 168);
        spr.deleteSprite();
        break;
      case 4: //szerda
        spr.createSprite(64, 12);
        spr.loadFont(FONT_14);
        spr.setTextDatum(MC_DATUM);
        if (ikon[k] % 2 == 0) { //ha páros akkor nappal van
          spr.setTextColor(TFT_DARKGREY, TFT_BLACK);
        } else {
          spr.setTextColor(TFT_YELLOW, TFT_BLACK);
        }
        spr.fillSprite(TFT_BLACK);
        spr.drawString("Szerda", 32, 6);
        spr.pushSprite(x2, 168);
        spr.deleteSprite();
        break;
      case 5: //csütörtök
        spr.createSprite(64, 12);
        spr.loadFont(FONT_14);
        spr.setTextDatum(MC_DATUM);
        if (ikon[k] % 2 == 0) { //ha páros akkor nappal van
          spr.setTextColor(TFT_DARKGREY, TFT_BLACK);
        } else {
          spr.setTextColor(TFT_YELLOW, TFT_BLACK);
        }
        spr.fillSprite(TFT_BLACK);
        spr.drawString("Csütörtök", 32, 6);
        spr.pushSprite(x2, 168);
        spr.deleteSprite();
        break;
      case 6: //péntek
        spr.createSprite(64, 12);
        spr.loadFont(FONT_14);
        spr.setTextDatum(MC_DATUM);
        if (ikon[k] % 2 == 0) { //ha páros akkor nappal van
          spr.setTextColor(TFT_DARKGREY, TFT_BLACK);
        } else {
          spr.setTextColor(TFT_YELLOW, TFT_BLACK);
        }
        spr.fillSprite(TFT_BLACK);
        spr.drawString("Péntek", 32, 6);
        spr.pushSprite(x2, 168);
        spr.deleteSprite();
        break;
      case 7: //szombat
        spr.createSprite(64, 12);
        spr.loadFont(FONT_14);
        spr.setTextDatum(MC_DATUM);
        if (ikon[k] % 2 == 0) { //ha páros akkor nappal van
          spr.setTextColor(TFT_DARKGREY, TFT_BLACK);
        } else {
          spr.setTextColor(TFT_YELLOW, TFT_BLACK);
        }
        spr.fillSprite(TFT_BLACK);
        spr.drawString("Szombat", 32, 6);
        spr.pushSprite(x2, 168);
        spr.deleteSprite();
        break;
      }*/
      
  //----------------------------------ELŐREJELZETT-IDŐPONTOK-(ÓRA)-----------------------------------
    spr.createSprite(64, 12);
    spr.loadFont(FONT_14);
    spr.setTextDatum(MC_DATUM);
    spr.setTextColor(TFT_WHITE, TFT_BLACK);
    spr.fillSprite(TFT_BLACK);
    spr.drawString(elorejelzett_ora_0, 32, 6);
    spr.pushSprite(0, 168);
    spr.deleteSprite();

    spr.createSprite(64, 12);
    spr.loadFont(FONT_14);
    spr.setTextDatum(MC_DATUM);
    spr.setTextColor(TFT_WHITE, TFT_BLACK);
    spr.fillSprite(TFT_BLACK);
    spr.drawString(elorejelzett_ora_1, 32, 6);
    spr.pushSprite(64, 168);
    spr.deleteSprite();

    spr.createSprite(64, 12);
    spr.loadFont(FONT_14);
    spr.setTextDatum(MC_DATUM);
    spr.setTextColor(TFT_WHITE, TFT_BLACK);
    spr.fillSprite(TFT_BLACK);
    spr.drawString(elorejelzett_ora_2, 32, 6);
    spr.pushSprite(128, 168);
    spr.deleteSprite();

    spr.createSprite(64, 12);
    spr.loadFont(FONT_14);
    spr.setTextDatum(MC_DATUM);
    spr.setTextColor(TFT_WHITE, TFT_BLACK);
    spr.fillSprite(TFT_BLACK);
    spr.drawString(elorejelzett_ora_3, 32, 6);
    spr.pushSprite(192, 168);
    spr.deleteSprite();

    spr.createSprite(64, 12);
    spr.loadFont(FONT_14);
    spr.setTextDatum(MC_DATUM);
    spr.setTextColor(TFT_WHITE, TFT_BLACK);
    spr.fillSprite(TFT_BLACK);
    spr.drawString(elorejelzett_ora_4, 32, 6);
    spr.pushSprite(256, 168);
    spr.deleteSprite();
  }
  
  //----------------------------------ELŐREJELZETT-HŐMÉRSÉKLETEK------------------------------------
  spr.createSprite(64, 12);
  spr.loadFont(FONT_14);
  spr.setTextDatum(MC_DATUM);
  spr.setTextColor(TFT_WHITE, TFT_BLACK);
  spr.fillSprite(TFT_BLACK);
  spr.drawString(elorejelzett_homerseklet_1 + String("°C"), 32, 6);
  spr.pushSprite(0, 230);
  spr.deleteSprite();

  spr.createSprite(64, 12);
  spr.loadFont(FONT_14);
  spr.setTextDatum(MC_DATUM);
  spr.setTextColor(TFT_WHITE, TFT_BLACK);
  spr.fillSprite(TFT_BLACK);
  spr.drawString(elorejelzett_homerseklet_2 + String("°C"), 32, 6);
  spr.pushSprite(64, 230);
  spr.deleteSprite();

  spr.createSprite(64, 12);
  spr.loadFont(FONT_14);
  spr.setTextDatum(MC_DATUM);
  spr.setTextColor(TFT_WHITE, TFT_BLACK);
  spr.fillSprite(TFT_BLACK);
  spr.drawString(elorejelzett_homerseklet_3 + String("°C"), 32, 6);
  spr.pushSprite(128, 230);
  spr.deleteSprite();

  spr.createSprite(64, 12);
  spr.loadFont(FONT_14);
  spr.setTextDatum(MC_DATUM);
  spr.setTextColor(TFT_WHITE, TFT_BLACK);
  spr.fillSprite(TFT_BLACK);
  spr.drawString(elorejelzett_homerseklet_4 + String("°C"), 32, 6);
  spr.pushSprite(192, 230);
  spr.deleteSprite();

  spr.createSprite(64, 12);
  spr.loadFont(FONT_14);
  spr.setTextDatum(MC_DATUM);
  spr.setTextColor(TFT_WHITE, TFT_BLACK);
  spr.fillSprite(TFT_BLACK);
  spr.drawString(elorejelzett_homerseklet_5 + String("°C"), 32, 6);
  spr.pushSprite(256, 230);
  spr.deleteSprite();
}

void adatok_frissitese() {
  //--------------------------------DÁTUM-IDŐ-FRISSÍTÉSE----------------------------------------------------
  Serial.println("Ido szinkronizalasa...");
  configTime(0, 0, "pool.ntp.org"); //NTP SZERVER URL
  setenv("TZ", "CET-1CEST,M3.5.0,M10.5.0/3", 0);
  while (time(nullptr) <= 100000) { //AMÍG NEM ÉRKEZIK ÉRTELMEZHETŐ UNIX ADAT ADDIG PONTOKAT ÍR
    //Serial.print(".");
    delay(100);
  }
  Serial.println("Ido szinkronizalva.");
  time_t tnow = time(nullptr); //TNOW VÁLTOZÓBA PONTOS IDŐ BEÍRÁSA
  Serial.println(String(ctime(&tnow)));

  //----------------------------------HOLD-ADATOK-LEKÉRDEZÉSE--------------------------------------------------
  SunMoonCalc smCalc = SunMoonCalc(tnow, eszaki_szelesseg, keleti_hosszusag);
  const SunMoonCalc::Result result = smCalc.calculateSunAndMoonData();
  holdkora = round(result.moon.age);
  //result.sun.transit, result.sun.set, result.sun.azimuth, result.sun.elevation, result.sun.distance, result.moon.rise, result.moon.transit, result.moon.set,
  //result.moon.azimuth, result.moon.elevation, result.moon.distance, result.moon.age, result.moon.illumination * 100, result.moon.phase.name

  //--------------------------------AKTUÁLIS-IDŐJÁRÁSADATOK-LEKÉRÉSE-------------------------------------------
  Serial.println("Adat lekerdezes..");
  OpenWeatherMapCurrent *currentWeatherClient = new OpenWeatherMapCurrent(); //AKTUÁLIS IDŐJÁRÁS
  currentWeatherClient->setMetric(IS_METRIC);  //METRIKUS RENDSZER
  currentWeatherClient->setLanguage(OPEN_WEATHER_MAP_LANGUAGE); //NYELV BEÁLLÍTÁS
  currentWeatherClient->updateCurrentById(&currentWeather, OPEN_WEATHER_MAP_APP_ID, OPEN_WEATHER_MAP_LOCATION_ID);
  delete currentWeatherClient;
  dtostrf(currentWeather.temp, 1, 1, kulso_homerseklet);
  dtostrf(currentWeather.humidity, 0, 0, kulso_paratartalom);
  dtostrf(currentWeather.pressure, 1, 1, nyomas);
  dtostrf(currentWeather.windSpeed * 3.6, 1, 1, szel);
  aktualis_idojaras_leirasa = currentWeather.description;
  aktualis_idojaras_kodja = currentWeather.weatherId;
  currentWeatherClient = nullptr;
  aktualis_idojaras_ikonja = currentWeather.iconMeteoCon.toInt();
  //.temp .icon .country .sunrise .sunset .cityName .rain .tempMin .tempMax .visibility .pressure .humidity .weatherId .main .description .iconMeteoCon .clouds .windSpeed .windDed .observationTimeText

  //--------------------------------ELŐREJELZETT-IDŐJÁRÁSADATOK-LEKÉRÉSE-------------------------------------------
  OpenWeatherMapForecast *forecastClient = new OpenWeatherMapForecast(); //ELŐREJELZETT IDŐJÁRÁS
  forecastClient->setMetric(IS_METRIC); //METRIKUS RENDSZER
  forecastClient->setLanguage(OPEN_WEATHER_MAP_LANGUAGE); //NYELV BEÁLLÍTÁS
  uint8_t allowedHours[] = {0, 3, 6, 9, 12, 15, 18, 21};  //ELŐREJELZÉSEK IDŐPONTJAI KÖZÉP IDŐ SZERINT
  forecastClient->setAllowedHours(allowedHours, sizeof(allowedHours));
  forecastClient->updateForecastsById(forecasts, OPEN_WEATHER_MAP_APP_ID, OPEN_WEATHER_MAP_LOCATION_ID, MAX_FORECASTS);
  delete forecastClient;
  forecastClient = nullptr;
  //.temp .icon .rain .tempMin .tempMax .pressure .pressureSeaLevel .pressureGroundLevel .humidity .weatherId .main .description .iconMeteoCon .clouds .windSpeed .windDed .observationTimeText

  delay(300);
  Serial.printf("Free mem: %d\n",  ESP.getFreeHeap()); //ESP SZABAD MEMÓRIA KIÍRATÁSA
  //Serial.println(aktualis_idojaras_leirasa);

  //------------------------------ELŐREJELZETT-IDŐJÁRÁS-KIÍRÁSÁHOZ-SZÜKÉSGES-ÉRTÉKADÁSOK------------------------------
  ikon[0] = forecasts[0].iconMeteoCon.toInt();
  ikon[1] = forecasts[1].iconMeteoCon.toInt();
  ikon[2] = forecasts[2].iconMeteoCon.toInt();
  ikon[3] = forecasts[3].iconMeteoCon.toInt();
  ikon[4] = forecasts[4].iconMeteoCon.toInt();

  dtostrf(forecasts[0].temp, 1, 1, elorejelzett_homerseklet_1);
  dtostrf(forecasts[1].temp, 1, 1, elorejelzett_homerseklet_2);
  dtostrf(forecasts[2].temp, 1, 1, elorejelzett_homerseklet_3);
  dtostrf(forecasts[3].temp, 1, 1, elorejelzett_homerseklet_4);
  dtostrf(forecasts[4].temp, 1, 1, elorejelzett_homerseklet_5);

  //---------------------------------TÉLI/NYÁRI-IDŐSZÁMÍTÁS-KONVERTER-OFFSETELÉSE--------------------------------------
  converter.setTimeOffset(2); // 1 TÉLI IDŐSZÁMÍTÁS, 2 NYÁRI

  //------------------------------ELŐREJELZETT-IDŐPONTOK-NAPJAINAK-MEGHATÁROZÁSA---------------------------------------
  converter.setUnixEpoch(forecasts[0].observationTime);
  elorejelzett_ora_0 = converter.getFormattedTimeShort();
  converter.setUnixEpoch(forecasts[1].observationTime);
  elorejelzett_ora_1 = converter.getFormattedTimeShort();
  converter.setUnixEpoch(forecasts[2].observationTime);
  elorejelzett_ora_2 = converter.getFormattedTimeShort();
  converter.setUnixEpoch(forecasts[3].observationTime);
  elorejelzett_ora_3 = converter.getFormattedTimeShort();
  converter.setUnixEpoch(forecasts[4].observationTime);
  elorejelzett_ora_4 = converter.getFormattedTimeShort();

  //-----------------------------------SUNRISE/SET-MOONRISE/SET-IDŐK-KONVERTÁLÁSA---------------------------------------
  converter.setUnixEpoch(currentWeather.sunset);
  napnyugta = converter.getFormattedTimeShort();  //18:06
  converter.setUnixEpoch(currentWeather.sunrise);
  napfelkelte = converter.getFormattedTimeShort(); //05:30
  converter.setUnixEpoch(result.moon.set);
  holdnyugta = converter.getFormattedTimeShort();  //18:06
  converter.setUnixEpoch(result.moon.rise);
  holdkelte = converter.getFormattedTimeShort(); //05:30

  //----------------------------------------PONTOS-DÁTUM-IDŐ-KONVERTÁLÁSA------------------------------------------------
  converter.setUnixEpoch(tnow);
  ido = converter.getFormattedTimeShort(); //05:30
  datum = String("20") + converter.getFormattedDate(); //19.05.12
  het_napja = converter.getDayOfWeek(); //Hétfő=2, Kedd=3, Szerda=4, Csütörtök=5, Péntek=6, Szombat=7, Vasárnap=1,
}

void fenyeroallitas() {
  if (nappal == 0) {          //HA NINCS NAPPAL
    feny = 0;                //FÉNYERŐ GYENGE
    //Serial.println("Mar lement a nap");
  } else {                 //HA NAPPAL VAN
    feny = 1;             //FÉNYERŐ ERŐS
    // Serial.println("Mar felkelt a nap");
  }
  switch (blynkgomb) { //HA A BLYNKBEN A VÁLASZTÓ GOMB
    case 1: //1 AKKOR GYENGE A FÉNYERŐ
      analogWrite(D8, 100);
      break;
    case 2: //2 AKKOR AUTOMATIKUS
      if (feny == 0) {
        analogWrite(D8, 100);
      } else {
        analogWrite(D8, 1023);
      }
      break;
    case 3: //AKKOR ERŐS
      analogWrite(D8, 1023);
      break;
  }
}

//--------------------------------ÉRÉTKEK-KÜLDÉSE-BLYNKNEK--------------------------------------
void myTimerEvent() {
  Blynk.virtualWrite(V0, aktualis_idojaras_blynkikonja); //KÜLDÉS BLYNKEK V0 VÁLTOZÓBA
  Blynk.virtualWrite(V1, belso_homerseklet); //KÜLDÉS BLYNKEK V1
  Blynk.virtualWrite(V2, kulso_homerseklet); //KÜLDÉS BLYNKEK V2
  Blynk.virtualWrite(V3, belso_paratartalom); //KÜLDÉS BLYNKEK V3
  Blynk.virtualWrite(V4, kulso_paratartalom); //KÜLDÉS BLYNKEK V4
  Blynk.virtualWrite(V5, szel); //KÜLDÉS BLYNKEK V5
  Blynk.virtualWrite(V6, nyomas); //KÜLDÉS BLYNKEK V6
  Blynk.virtualWrite(V8, aktualis_idojaras_kodja); //KÜLDÉS BLYNKEK V8
  Blynk.virtualWrite(V9, hoerzet); //KÜLDÉS BLYNKEK V9
  Blynk.virtualWrite(V10, harmatpont); //KÜLDÉS BLYNKEK V10
}
//BLYNK_WRITE(V8) {
//  teszt = param.asInt(); //EZT DEBUGHOZ HASZNÁLTAM BÁRMELYIK VÁLTOZÓNAK TETSZŐLEGES ÉRTÉKET TUDTAM VELE ADNI
//Serial.println(teszt);
//}

BLYNK_CONNECTED() {
  Blynk.syncVirtual(V7);  //SZINKRONIZÁLJA A V5 PIN ÁLLAPOTÁT HA LEKAPCSOLÓDNA VAGY ELSŐ INDULÁSNÁL NEM KAPNA AUTOMATIKUSAN ÉRTÉKET.
}
BLYNK_WRITE(V7)
{
  switch (param.asInt()) {
    case 1:
      blynkgomb = 1;
      break;
    case 2:
      blynkgomb = 2;
      break;
    case 3:
      blynkgomb = 3;
      break;
  }
}

/*KÉSZÍTETTE FEHÉR TAMÁS 2020.03.30. A PROGRAM MAGÁNHASZNÁLTRA ÍRÓDOTT, ABBÓL ANYAGI HASZNOT HÚZNI TILOS!
  MEGOSZTÁS ESETÉN KÉRLEK JELÖLD A FORRÁST.
  KÉRDÉS, JAVASLAT, IGÉNY, ESETÉN KERESS: fetomi@totalcar.hu
  AZ ESETLEGES HIBÁS INFORMÁCIÓKÉRT FELELŐSÉGET NEM VÁLLALOK!
*/
