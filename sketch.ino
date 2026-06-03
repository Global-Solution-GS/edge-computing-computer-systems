
#include <DHT.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <math.h>

#define DHT_PIN 2
#define DHT_TYPE DHT22

#define DS18B20_PIN 4
#define LDR_PIN A0
#define NTC_PIN A1
#define NDVI_PIN A2
#define FIRE_PIN A3

#define BUZZER_PIN 8
#define RED_PIN 9
#define GREEN_PIN 10
#define BLUE_PIN 11

DHT dht(DHT_PIN, DHT_TYPE);
OneWire oneWire(DS18B20_PIN);
DallasTemperature ds18b20(&oneWire);

float limitar(float valor, float minimo, float maximo) {
  if (valor < minimo) return minimo;
  if (valor > maximo) return maximo;
  return valor;
}

float normalizar(float valor, float minEntrada, float maxEntrada, float minSaida, float maxSaida) {
  valor = limitar(valor, minEntrada, maxEntrada);
  return (valor - minEntrada) * (maxSaida - minSaida) / (maxEntrada - minEntrada) + minSaida;
}

float lerNTC() {
  int leitura = analogRead(NTC_PIN);

  if (leitura <= 0) leitura = 1;
  if (leitura >= 1023) leitura = 1022;

  const float BETA = 3950;
  float temperatura = 1.0 / (log(1.0 / (1023.0 / leitura - 1.0)) / BETA + 1.0 / 298.15) - 273.15;
  return temperatura;
}

float riscoTemperatura(float temperatura) {
  float bruto = exp(0.12 * (temperatura - 20.0)) - 1.0;
  float maximo = exp(0.12 * (42.0 - 20.0)) - 1.0;
  return limitar((bruto / maximo) * 10.0, 0.0, 10.0);
}

float riscoUmidade(float umidade) {
  float desvio = (umidade - 65.0) / 30.0;
  return limitar(desvio * desvio * 10.0, 0.0, 10.0);
}

float riscoVegetacao(float ndvi) {
  float f2 = -2.5 * log(ndvi);
  float f2Min = -2.5 * log(0.80);
  float f2Max = -2.5 * log(0.10);
  return limitar(((f2 - f2Min) / (f2Max - f2Min)) * 10.0, 0.0, 10.0);
}

float penalidadeQueimadas(int queimadas) {
  return limitar((2.0 * log(queimadas + 1.0)) / log(21.0), 0.0, 2.0);
}

void definirRGB(int r, int g, int b) {
  analogWrite(RED_PIN, r);
  analogWrite(GREEN_PIN, g);
  analogWrite(BLUE_PIN, b);
}

String classificarIRC(float irc) {
  if (irc < 3.5) return "BAIXO RISCO";
  if (irc < 6.5) return "RISCO MODERADO";
  return "RISCO CRITICO";
}

void setup() {
  Serial.begin(9600);

  dht.begin();
  ds18b20.begin();

  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(RED_PIN, OUTPUT);
  pinMode(GREEN_PIN, OUTPUT);
  pinMode(BLUE_PIN, OUTPUT);

  definirRGB(0, 0, 0);

  Serial.println("=======================================");
  Serial.println("       ClimaX Edge - Arduino Mega       ");
  Serial.println(" Monitoramento Climatico Urbano / IRC  ");
  Serial.println("=======================================");
}

void loop() {
  float temperaturaAr = dht.readTemperature();
  float umidade = dht.readHumidity();

  ds18b20.requestTemperatures();
  float temperaturaSuperficie = ds18b20.getTempCByIndex(0);
  float temperaturaNTC = lerNTC();

  if (isnan(temperaturaAr)) temperaturaAr = 25.0;
  if (isnan(umidade)) umidade = 65.0;
  if (temperaturaSuperficie < -50 || temperaturaSuperficie > 125) {
    temperaturaSuperficie = temperaturaAr;
  }

  int leituraLDR = analogRead(LDR_PIN);
  int leituraNDVI = analogRead(NDVI_PIN);
  int leituraFire = analogRead(FIRE_PIN);

  float ndvi = normalizar(leituraNDVI, 0, 1023, 0.10, 0.80);
  int queimadas = (int)normalizar(leituraFire, 0, 1023, 0, 20);

  float temperaturaMedia = (temperaturaAr + temperaturaSuperficie + temperaturaNTC) / 3.0;

  float rTemp = riscoTemperatura(temperaturaMedia);
  float rUmid = riscoUmidade(umidade);
  float rVeg = riscoVegetacao(ndvi);
  float rLuz = normalizar(leituraLDR, 0, 1023, 0, 10);
  float pQueimada = penalidadeQueimadas(queimadas);
  float irc = (0.40 * rTemp) + (0.30 * rVeg) + (0.20 * rUmid) + (0.10 * rLuz) + pQueimada;
  irc = limitar(irc, 0.0, 10.0);

  String status = classificarIRC(irc);

  if (irc < 3.5) {
    definirRGB(0, 255, 0);      // verde
    noTone(BUZZER_PIN);
  } else if (irc < 6.5) {
    definirRGB(255, 180, 0);    // amarelo
    noTone(BUZZER_PIN);
  } else {
    definirRGB(255, 0, 0);      // vermelho
    tone(BUZZER_PIN, 1200, 250);
  }

  Serial.println("---------------------------------------");
  Serial.print("Temperatura do ar: ");
  Serial.print(temperaturaAr, 1);
  Serial.println(" C");

  Serial.print("Temperatura superficie DS18B20: ");
  Serial.print(temperaturaSuperficie, 1);
  Serial.println(" C");

  Serial.print("Temperatura NTC: ");
  Serial.print(temperaturaNTC, 1);
  Serial.println(" C");

  Serial.print("Umidade: ");
  Serial.print(umidade, 1);
  Serial.println(" %");

  Serial.print("Luminosidade LDR: ");
  Serial.print(leituraLDR);
  Serial.print(" | Risco luz: ");
  Serial.println(rLuz, 1);

  Serial.print("NDVI simulado: ");
  Serial.print(ndvi, 2);
  Serial.print(" | Risco vegetacao: ");
  Serial.println(rVeg, 1);

  Serial.print("Queimadas simuladas: ");
  Serial.print(queimadas);
  Serial.print(" | Penalidade: ");
  Serial.println(pQueimada, 1);

  Serial.print("IRC final: ");
  Serial.print(irc, 2);
  Serial.print(" / 10 -> ");
  Serial.println(status);

  delay(2000);
}
