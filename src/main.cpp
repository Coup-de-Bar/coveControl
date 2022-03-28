#include <Arduino.h>
#include <ESP8266TimerInterrupt.h>
#include <U8g2lib.h>
#include <OneWire.h>
#include <SPI.h>
#include <string.h>
#include <ESP8266WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

#include "sensors.h"
#include "bitmaps.h"
#include "display.h"

#define ENC_PUSH_PIN D4
#define ENC_A_PIN D7
#define ENC_B_PIN D6

#define HEATER_REALAY_PIN 0
#define PUMP_V0_RELAY_PIN 0
#define PUMP_V1_RELAY_PIN 0
#define PUMP_START_RELAY_PIN 0


#define PUMP_STATUS_OFF 0
#define PUMP_STATUS_STARTING 1
#define PUMP_STATUS_V0 2
#define PUMP_STATUS_v1 3

#define UPDATE_REQUEST 1
#define UPDATE_PUSH 2
#define UPDATE_ENCODER 3

const byte main_menu_n = 4;
const char *main_menu_strings[] = {"Controle", "Planning", "Parametres", "Infos"};
const byte main_menu_choices[] = {1, 2, 3, 4};

const char *ssid     = "BalkanyDidNothingWrong";
const char *password = "orphani-multasse-bracia!-cnidi9";

byte update = UPDATE_REQUEST;
byte position = 0;
int8_t menu_select = 0;
int8_t encoder_delta = 0;
unsigned long lastInput = millis();

U8G2_ST7920_128X64_F_SW_SPI u8g2(U8G2_R2, D3, D1, D0, U8X8_PIN_NONE);
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);

// Functions 
void showStartScreen();
void showMainMenu();
void showControlOverview();
void showManualControl(float *temps, byte selected);
void showPlanningMenu();
void showSettingsMenu();
void showInfos();
void isr_enc_push();
void isr_enc_a();


byte pumpState = 1;
byte heaterPWMState = 0;

float goalTemp = 30;

void setup(void) {
  Serial.begin(9600);
  WiFi.begin(ssid, password);

  pinMode(ENC_PUSH_PIN, INPUT_PULLUP);
  pinMode(ENC_A_PIN, INPUT_PULLUP);
  pinMode(ENC_B_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(ENC_PUSH_PIN), isr_enc_push, FALLING);
  attachInterrupt(digitalPinToInterrupt(ENC_A_PIN), isr_enc_a, FALLING);
  //attachInterrupt(digitalPinToInterrupt(ENC_B_PIN), isr_ENC_B_PIN, FALLING);
  interrupts();
  u8g2.begin();
  showStartScreen();
  timeClient.begin();
  timeClient.setTimeOffset(7200);
}

void loop()
{
  showControlOverview();
}

void showStartScreen()
{
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_fub20_tr);	// choose a suitable font
  u8g2.drawStr(15,42,"Beer OS");	// write something to the internal memory
  u8g2.sendBuffer();
  delay(2000);
}

void showMainMenu()
{
  if(update)
  {
    if(update == UPDATE_PUSH)
    {
      position = main_menu_choices[menu_select];
      update = false;
      menu_select = 0;
      return;
    }

    menu_select += encoder_delta;   //update if encoder is moved
    encoder_delta = 0;
    if(menu_select > 3) menu_select = 0;
    else if(menu_select < 0) menu_select = 3;


    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_ncenR08_te);
    u8g2.drawStr(23, 8, "Menu Principal");
    u8g2.setFont(u8g2_font_profont11_tf);
    for(byte i = 0; i < main_menu_n; i++) //draw menu
    {
      if(i == menu_select)
      {
        u8g2.drawBox(0, i*10 + 12, strlen(main_menu_strings[i])*6+2, 9);
        //u8g2.setFontMode(0);
        u8g2.setDrawColor(0);
        u8g2.drawStr(1, i*10 + 20, main_menu_strings[i]);
        //u8g2.setFontMode(1);
        u8g2.setDrawColor(1);
      }
      else u8g2.drawStr(1, i*10 + 20, main_menu_strings[i]);
    }
    
    u8g2.setCursor(20,62);
    u8g2.print(strlen(main_menu_strings[menu_select]));
    u8g2.sendBuffer();
    update = false;
  }
}

void showControlOverview()
{
  float temps [3];
  unsigned long temps_update_ms = millis();
  uint8_t nFrame = 0;

  readyTempSensor();

  while(1)
  {
    u8g2.clearBuffer();
    timeClient.update();
    if((millis() - temps_update_ms) > 750)
    {
      readCuveTemps(temps, &temps_update_ms);
      readyTempSensor();
    }
    u8g2.setFont(u8g2_font_profont11_tf);   //title font
    u8g2.drawStr(0, 7, "Overview:");

    u8g2.setCursor(80, 7);

    u8g2.print(timeClient.getFormattedTime());

    u8g2.drawXBM(0, 34, 3, 30, tige_bitmap);  //print tige bitmap
    u8g2.setFont(u8g2_font_profont11_tf);   //temp fonts
    
    u8g2.setCursor(5, 43);
    u8g2.print(temps[0], 1);
    u8g2.print("\xb0\103"); //dirty but best way to display °C
    u8g2.setCursor(5,53);
    u8g2.print(temps[1], 1);
    u8g2.print("\xb0\103");
    u8g2.setCursor(5, 63);
    u8g2.print(temps[2], 1);
    u8g2.print("\xb0\103");

    if(heaterPWMState)
    {
      if(nFrame % 2) u8g2.drawXBM(0, 15, heating_width, heating_height, heating_bitmap);
      else u8g2.drawXBM(0, 15, heating_width, heating_height, heatingflipped_bitmap);
    }
    if(pumpState)
    {
      if(nFrame % 2) u8g2.drawXBM(22, 12, pump_width, pump_height, pump2_bitmap);
      else u8g2.drawXBM(22, 12, pump_width, pump_height, pump1_bitmap);
    }
    
    u8g2.drawLine(41, 9, 41, 64);
    u8g2.drawLine(41, 9, 128, 9);
    showManualControl(temps, 1);
    nFrame++;
    u8g2.sendBuffer();
   // delay(200);
  }

  position = 0;
  update = UPDATE_REQUEST;
}

void showManualControl(float *temps, byte selected)
{
  u8g2.setCursor(43, 20);
  u8g2.print("T:");
  u8g2.print(getMedCuveTemp(temps));
  u8g2.print("  ");
  u8g2.drawXBM(86, 12, arrow2_width, arrow2_height, arrow2_bitmap);
  if(selected == 0)
  {
    u8g2.drawBox(96, 12 , 31, 9);
    u8g2.setDrawColor(0);
  }
  u8g2.print(goalTemp);
  u8g2.setDrawColor(1);

  u8g2.setCursor(43, 34);
  u8g2.print("Pump: ");
  if(selected == 1)
  {
    if(pumpState) u8g2.drawBox(78, 26 , 13, 9);
    else u8g2.drawBox(78, 26 , 19, 9);
    u8g2.setDrawColor(0);
  }
  if(pumpState) u8g2.print("ON");
  else u8g2.print("OFF");
  u8g2.setDrawColor(1);

}


void showPlanningMenu()
{
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_ncenR08_te);	// choose a suitable font
  u8g2.drawStr(0,10,"Work in progress");	// write something to the internal memory
  u8g2.sendBuffer();
  delay(500);
  position = 0;
  update = UPDATE_REQUEST;
}

void showSettingsMenu()
{
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_ncenR08_te);	// choose a suitable font
  u8g2.drawStr(0,10,"Work in progress");	// write something to the internal memory
  u8g2.sendBuffer();
  delay(500);
  position = 0;
  update = UPDATE_REQUEST;
}

void showInfos()
{
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_ncenR08_te);	// choose a suitable font
  u8g2.drawStr(0,10,"Work in progress");	// write something to the internal memory
  u8g2.sendBuffer();
  delay(500);
  position = 0;
  update = UPDATE_REQUEST;
}

void setPumpState(byte state)
{
  switch (state)
  {
  case 0:
    /* code */
    break;
  
  default:
    break;
  }
}

IRAM_ATTR void isr_enc_push()
{
  if(millis() - lastInput > 100)
  {
    lastInput = millis();
    update = UPDATE_PUSH;
  }
}

IRAM_ATTR void isr_enc_a()  //Encoder update interrupt-
{
  if(millis() - lastInput > 100)
  {
    if(digitalRead(ENC_B_PIN))
    {
      encoder_delta++;
    }
    else encoder_delta--;
    Serial.println(millis() - lastInput);
    lastInput = millis();
    update = UPDATE_ENCODER;
  }
}