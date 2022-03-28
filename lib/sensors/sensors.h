#define OW_PIN D5 //one wire communication pin

int readyTempSensor();

int readCuveTemps(float *temps, unsigned long *temp_update_ms); //

int setTempEnabled(byte nSensor, bool value0);

bool getTempEnabled(byte nSensor);

float getMedCuveTemp(float *temps);