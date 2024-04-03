#include <DHT.h>
#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Inisialisasi pin sensor DHT22
#define DHTPIN 2
#define pinKipas 8
#define pinPemanas 9
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

void setup() {
  Serial.begin(9600);
  dht.begin();
  pinMode(pinKipas, OUTPUT);  // Pin untuk kipas
  pinMode(pinPemanas, OUTPUT);  // Pin untuk pemanas

  lcd.init();
  lcd.backlight();
  lcd.clear();
}

float onKipas, offKipas, onPemanas, offPemanas;

void loop() {
  // Baca nilai suhu dan kelembapan dari DHT22
  // float suhu = dht.readTemperature();
  // float kelembapan = dht.readHumidity();

  // Cek jika gagal membaca nilai dari sensor
  // if (isnan(suhu) || isnan(kelembapan)) {
  //   Serial.println("Gagal membaca dari sensor DHT!");
  //   return;
  // }

  // Menggantikan input dengan potensiometer
  float suhu = map(analogRead(A0),0, 1023, 0, 45);
  float kelembapan =  map(analogRead(A1),0, 1023, 20, 100);

  // Menghitung derajat keanggotaan suhu dan kelembapan
  float sangatDingin = fuzzyTrapezoidal(suhu, 0, 0, 5, 10);
  float dingin = fuzzyTrapezoidal(suhu, 5, 10, 15, 20);
  float sedang = fuzzyTrapezoidal(suhu, 15, 20, 25, 30);
  float panas = fuzzyTrapezoidal(suhu, 25, 30, 35, 40);
  float sangatPanas = fuzzyTrapezoidal(suhu, 35, 40, 45, 45);

  float kering = fuzzyTrapezoidal(kelembapan, 20, 20, 30, 50);
  float lembab = fuzzyTrapezoidal(kelembapan, 30, 50, 70, 90);
  float basah = fuzzyTrapezoidal(kelembapan, 70, 90, 100, 100);

  // Inferance - combination
  bool s_sangatDingin = (suhu >= 0 && suhu <= 10);
  bool s_dingin = (suhu >= 5 && suhu <= 20);
  bool s_sedang = (suhu >= 15 && suhu <= 30);
  bool s_panas = (suhu >= 25 && suhu <= 40);
  bool s_sangatPanas = (suhu >= 35 && suhu <= 45);

  bool s_kering = (kelembapan >= 0 && kelembapan <= 50);
  bool s_lembab = (kelembapan >= 30 && kelembapan <= 90);
  bool s_basah = (kelembapan >= 70 && kelembapan <= 100);

  // Rule status keluaran sistem
  bool statKipas[15] = { 0, 0, 1, 0, 0, 1, 0, 0, 1, 1, 1, 1, 1, 1, 1 };
  bool statPemanas[15] = { 1, 1, 1, 1, 1, 1, 1, 0, 0, 1, 0, 0, 0, 0, 1 };
  float rule[15];

  if (s_sangatDingin || s_basah) rule[0] = min(sangatDingin, basah);
  if (s_sangatDingin || s_lembab) rule[1] = min(sangatDingin, lembab);
  if (s_sangatDingin || s_kering) rule[2] = min(sangatDingin, kering);
  if (s_dingin || s_basah) rule[3] = min(dingin, basah);
  if (s_dingin || s_lembab) rule[4] = min(dingin, lembab);
  if (s_dingin || s_kering) rule[5] = min(dingin, kering);
  if (s_sedang || s_basah) rule[6] = min(sedang, basah);
  if (s_sedang || s_lembab) rule[7] = min(sedang, lembab);
  if (s_sedang || s_kering) rule[8] = min(sedang, kering);
  if (s_panas || s_basah) rule[9] = min(panas, basah);
  if (s_panas || s_lembab) rule[10] = min(panas, lembab);
  if (s_panas || s_kering) rule[11] = min(panas, kering);
  if (s_sangatPanas || s_basah) rule[12] = min(sangatPanas, basah);
  if (s_sangatPanas || s_lembab) rule[13] = min(sangatPanas, lembab);
  if (s_sangatPanas || s_kering) rule[14] = min(sangatPanas, kering);

  for (int i = 0; i < 15; i++) {
    if (rule[i] > 0 && statKipas[i] == 1) onKipas = max(onKipas, rule[i]);
    if (rule[i] > 0 && statKipas[i] == 0) offKipas = max(offKipas, rule[i]);
    if (rule[i] > 0 && statPemanas[i] == 1) onPemanas = max(onPemanas, rule[i]);
    if (rule[i] > 0 && statPemanas[i] == 0) offPemanas = max(offPemanas, rule[i]);
  }

  //Defuzzyfication
  float kipas_M1, kipas_M2, kipas_M3, kipas_M4, kipas_A1, kipas_A2, kipas_A3, kipas_A4, pemanas_M1, pemanas_M2, pemanas_M3, pemanas_M4, pemanas_A1, pemanas_A2, pemanas_A3, pemanas_A4;
  if(onKipas > 0){
    kipas_M1= trapezoidalRule(0, 4.5, [](float z) { return onKipas * z; });
    kipas_M2 = trapezoidalRule(4.5, 5, [](float z) { return ((5 - z) / (5 - 4.5)) * z; });
    kipas_A1= trapezoidalRule(0, 4.5, [](float z) { return onKipas; });
    kipas_A2 = trapezoidalRule(4.5, 5, [](float z) { return ((5 - z) / (5 - 4.5)); });
  } else{
    kipas_M1 = 0;
    kipas_M2 = 0;
    kipas_A1 = 0;
    kipas_A2 = 0;
  }
  if (offKipas > 0){
    kipas_M3 = trapezoidalRule(5, 5.5, [](float z) { return ((z - 5) / (5.5 - 5)) * z; });
    kipas_M4 = trapezoidalRule(5.5, 10, [](float z) { return offKipas * z; });
    kipas_A3 = trapezoidalRule(5, 5.5, [](float z) { return ((z - 5) / (5.5 - 5)); });
    kipas_A4 = trapezoidalRule(5.5, 10, [](float z) { return offKipas; });
  }else{
    kipas_M3 = 0;
    kipas_M4 = 0;
    kipas_A3 = 0;
    kipas_A4 = 0;
  }

  if(onPemanas > 0){
    pemanas_M1= trapezoidalRule(0, 4.5, [](float z) { return onPemanas * z; });
    pemanas_M2 = trapezoidalRule(4.5, 5, [](float z) { return ((5 - z) / (5 - 4.5)) * z; });
    pemanas_A1= trapezoidalRule(0, 4.5, [](float z) { return onPemanas; });
    pemanas_A2 = trapezoidalRule(4.5, 5, [](float z) { return ((5 - z) / (5 - 4.5)); });
  } else{
    pemanas_M1 = 0;
    pemanas_M2 = 0;
    pemanas_A1 = 0;
    pemanas_A2 = 0;
  }
  if (offPemanas > 0){
    pemanas_M3 = trapezoidalRule(5, 5.5, [](float z) { return ((z - 5) / (5.5 - 5)) * z; });
    pemanas_M4 = trapezoidalRule(5.5, 10, [](float z) { return offPemanas * z; });
    pemanas_A3 = trapezoidalRule(5, 5.5, [](float z) { return ((z - 5) / (5.5 - 5)); });
    pemanas_A4 = trapezoidalRule(5.5, 10, [](float z) { return offPemanas; });
  }else{
    pemanas_M3 = 0;
    pemanas_M4 = 0;
    pemanas_A3 = 0;
    pemanas_A4 = 0;
  }

  //Hasil Akhir
  float z_kipas = (kipas_M1 + kipas_M2 + kipas_M3 + kipas_M4)/(kipas_A1 + kipas_A2 + kipas_A3 + kipas_A4);
  float z_pemanas = (pemanas_M1 + pemanas_M2 + pemanas_M3 + pemanas_M4)/(pemanas_A1 + pemanas_A2 + pemanas_A3 + pemanas_A4);
  
  // Set pin berdasarkan keputusan hasil akhir
  digitalWrite(pinKipas, z_kipas <= 5 ? LOW : HIGH);    // Mengontrol kipas
  digitalWrite(pinPemanas, z_pemanas <= 5 ? LOW : HIGH);  // Mengontrol pemanas

  // Tampilkan hasil debug pada Serial Monitor
  Serial.print("Suhu: ");
  Serial.print(suhu);
  Serial.print("°C, Kelembapan: ");
  Serial.print(kelembapan);
  Serial.print("%, Kipas: ");
  Serial.print(z_kipas);
  Serial.print(", Pemanas: ");
  Serial.print(z_pemanas);
  Serial.print(", Kipas: ");
  Serial.print(z_kipas <= 5 ? "ON" : "OFF");
  Serial.print(", Pemanas: ");
  Serial.println(z_pemanas <= 5 ? "ON" : "OFF");

  // Tampilkan hasil debug pada LCD
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Tem:");
  lcd.print(suhu);
  lcd.setCursor(10, 0);
  lcd.print("K:");
  lcd.print(z_kipas);
  lcd.setCursor(0, 1);
  lcd.print("Hum:");
  lcd.print(kelembapan);
  lcd.setCursor(10, 1);
  lcd.print("P:");
  lcd.print(z_pemanas);

  // Reset nilai Inferance - combination
  onKipas = 0;
  offKipas = 0;
  onPemanas = 0;
  offPemanas = 0;

  return;
}
//Fungsi untuk menghitung derajat membership
float fuzzyTrapezoidal(float x, float a, float b, float c, float d) {
  if (x <= a || x >= d) return 0;
  else if (x > a && x <= b) return (x - a) / (b - a);
  else if (x > b && x < c) return 1;
  else if (x >= c && x < d) return (d - x) / (d - c);
}
// Fungsi untuk menghitung Integral
double trapezoidalRule(double a, double b, float (*func)(float)) {
  int n = 1000;
  double h = (b - a) / n;                        // Menghitung lebar trapesium
  double sum = 0.5 * ((*func)(a) + (*func)(b));  // Menambahkan area dari dua trapesium di ujung

  for (int i = 1; i < n; i++) {
    sum += (*func)(a + i * h);  // Menambahkan area dari trapesium di tengah
  }
  return sum * h;  // Mengalikan total area dengan lebar trapesium
}
