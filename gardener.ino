#include <Bonezegei_DHT11.h>
#include <LiquidCrystal_I2C.h>
#include <IRremote.h>

#define SOIL_SENSOR A7
#define RELAY_PIN 4
#define DHTPIN 8

Bonezegei_DHT11 dht(DHTPIN);
LiquidCrystal_I2C lcd(0x27, 20, 2);

const int IRpin = 2,
          LEFT_BUTTON = 4335,
          RIGHT_BUTTON = 23205,
          UP_BUTTON = 6375,
          DOWN_BUTTON = 19125,
          ASTERICS_BUTTON = 26775;
IRrecv irrecv(IRpin);
decode_results results;


int minSoilMisture = 40;
int statusModeDelay = 200;
long lastRefreshTime = 0, lastSignalTime = 0;
bool needsToUpdateInfo = false, refillingPlant = false, forceUpdate = false, lcdBacklight = true;

void setup() {
  Serial.begin(9600);
  irrecv.enableIRIn();
  dht.begin();
  pinMode(SOIL_SENSOR, INPUT);
  pinMode(RELAY_PIN, OUTPUT);
  Wire.begin();
  lcd.init();
  lcd.backlight();
}

int getSoilMoistureValue() {
  return (int)analogRead(SOIL_SENSOR) / 10;
}

void refillPlant() {
  refillingPlant = true;
  forceUpdate = true;
  digitalWrite(RELAY_PIN, HIGH);
  float soilMoistureCurrent = getSoilMoistureValue();
  if(soilMoistureCurrent > minSoilMisture) {
    digitalWrite(RELAY_PIN, LOW);
    refillingPlant = false;
    forceUpdate = false;
  }
}

bool supportedAction(int value) {
  return value == ASTERICS_BUTTON || value == LEFT_BUTTON || value == UP_BUTTON || value == RIGHT_BUTTON || value == DOWN_BUTTON;
}

void refreshInfo() {
  int soilMoisture = getSoilMoistureValue();
  if (dht.getData()) {
    float temperature = dht.getTemperature();
    int humidity = dht.getHumidity();
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("T ");
    lcd.print(temperature);
    lcd.print("C");
    lcd.print(" H ");
    lcd.print(humidity);
    lcd.print("%");
    lcd.setCursor(0, 1);
    lcd.print("S");
    lcd.print("M ");
    lcd.print(minSoilMisture);
    lcd.print("%");
    lcd.print(" A ");
    lcd.print(soilMoisture);
    lcd.print("%");
    if(refillingPlant) {
      lcd.print(" RF ");
    }
  }
}

void loop() {
  long currentTime = millis();
  if (irrecv.decode(&results)) {
    int value = results.value;
    if (supportedAction(value)) {
      lastSignalTime = millis();
      forceUpdate = true;
      if (value == LEFT_BUTTON || value == DOWN_BUTTON) {
        minSoilMisture--;
      }
      if (value == RIGHT_BUTTON || value == UP_BUTTON) {
        minSoilMisture++;
      }
      if (value == ASTERICS_BUTTON) {
        if(lcdBacklight) {
          lcd.noBacklight();
          lcdBacklight = false;
        } else {
          lcd.backlight();
          lcdBacklight = true;
        }
      }
    }
    irrecv.resume();
  }
  if(currentTime - lastSignalTime > 1000) {
      forceUpdate = false;
      lastSignalTime = currentTime;
  }
  if(lcdBacklight) {
    int timeDelay = 1000;
    if(forceUpdate) {
      timeDelay = 100;
    }
    if(!lcdBacklight) {
      timeDelay = 5000;
    }
    if (forceUpdate || lastRefreshTime == 0 || currentTime - lastRefreshTime > timeDelay) {
        needsToUpdateInfo = true;
    }
    if (needsToUpdateInfo && lcdBacklight) {
      refreshInfo();
      lastRefreshTime = millis();
      needsToUpdateInfo = false;
    }
  }
  float soilMoisture = getSoilMoistureValue();
  if (soilMoisture < minSoilMisture || refillingPlant) {
    refillPlant();
  }
}