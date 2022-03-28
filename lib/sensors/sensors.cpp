#include <Arduino.h>
#include <OneWire.h>
#include "sensors.h"

OneWire ds(OW_PIN);

const byte probes_uid[3][8] = //Temperature probes oneWire uid
{{0x28, 0xFF, 0x64, 0x1E, 0x85, 0x95, 0x83, 0x6E}, //20cm
{0x28, 0xFF, 0x64, 0x1E, 0x84, 0x75, 0x9E, 0xD0},  //10cm
{0x28, 0xFF, 0x64, 0x1E, 0x84, 0x68, 0x0B, 0x9B}}; //0cm (fond)

byte tempEnabledMask = 0xFF;

int readyTempSensor()
{
  ds.reset();
  ds.write(0xCC);   //"Skip Rom" command -> adressed to all devices
  ds.write(0x44);   //"Convert T" command
  return 0;
}

int readCuveTemps(float *temps, unsigned long *temp_update_ms)
{
  byte data[12];

  for(byte i = 0; i < 3; i++)
  {
    ds.reset();
    ds.select(probes_uid[i]);
    ds.write(0xBE);  //"Read Scratchpad"
    for(byte j = 0; j < 9; j++)
    {
      data[j] = ds.read();
    }
    temps[i] = ( (data[1] << 8) + data[0] )*0.0625;
    
  }
  *temp_update_ms = millis();
  return 0;
}

int setTempEnabled(byte nSensor, bool value)
{
  if(value) tempEnabledMask = tempEnabledMask | (1 << nSensor); //set 
  else tempEnabledMask = tempEnabledMask & (~(1 << nSensor)); //clear
  return 0;
}

bool getTempEnabled(byte nSensor)
{
  if(tempEnabledMask & (1 << nSensor)) return true;
  return false;
}

float getMedCuveTemp(float *temps)
{
    float sum = 0;
    byte enabled = 0;
    Serial.println(tempEnabledMask);
    Serial.println();
    for(byte i = 0; i < 3; i++)
    {
        sum += temps[i]*getTempEnabled(i);
        enabled += getTempEnabled(i);
        Serial.println(sum);
    }
    return sum/enabled;
}