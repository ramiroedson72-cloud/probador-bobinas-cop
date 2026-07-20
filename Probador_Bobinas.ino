/*
 * Probador de bobinas de encendido COP
 * ESP32-C3 Super Mini + PC817 + IGBT FGH60N60SFD (PCB v2)
 *
 * El potenciometro controla la frecuencia de disparo (10-200 Hz).
 * El dwell (tiempo de carga de la bobina) se mantiene fijo en 3 ms
 * recalculando el duty en cada cambio, con tope de seguridad del 40%
 * (a mas de ~133 Hz el tope reduce el dwell gradualmente).
 *
 * Conexiones (segun PCB v2):
 *   GPIO3  <- cursor del potenciometro WH148 (extremos a 3V3 y GND)
 *   GPIO4  -> R1 150R -> pin 1 PC817 (pin 2 a GND logico)
 *   GPIO8  -> SDA de la LCD 20x4 (backpack PCF8574, alimentada a 5V)
 *   GPIO9  -> SCL de la LCD
 *
 * Compatible con el core ESP32 de Arduino v2.x y v3.x.
 */

#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// ---------- Configuracion ----------
const int   PIN_POT   = 3;     // entrada ADC del potenciometro
const int   PIN_PWM   = 4;     // salida hacia el opto/MOSFET
const int   PIN_SDA   = 8;
const int   PIN_SCL   = 9;

const int   FREQ_MIN  = 10;    // Hz (= 300 RPM en motor 4 cil.)
// Con el IGBT FGH60N60SFD (600 V) el flyback de la bobina ya no se disipa
// en avalancha: el rango completo es seguro. (Con el IRLZ44N del prototipo
// v1 habia que limitar a 60 Hz porque el MOSFET clampeaba a 55 V y ardia.)
const int   FREQ_MAX  = 200;   // Hz (= 6000 RPM en motor 4 cil.)
// 3 ms: dwell tipico de bobina COP (primario 3.2 ohms -> ~3 A de pico).
const float DWELL_MS  = 3.0;   // tiempo de carga de la bobina
const float DUTY_MAX  = 0.40;  // tope de seguridad de duty (40%)

// 14 bits: con el reloj de 80 MHz el LEDC acepta de 4.8 Hz a 4.8 kHz,
// asi todo el rango 10-200 Hz es valido. Con 12 bits el minimo era
// ~19 Hz y las frecuencias bajas fallaban en silencio.
const int   RES_BITS  = 14;
const uint32_t DUTY_FULL = (1UL << RES_BITS) - 1;

#if !defined(ESP_ARDUINO_VERSION_MAJOR) || ESP_ARDUINO_VERSION_MAJOR < 3
  #define CORE_V2                // API antigua de LEDC (canales)
  const int PWM_CHANNEL = 0;
#endif

LiquidCrystal_I2C *lcd = nullptr;
bool hayLcd = false;

int  freqActual = 0;           // frecuencia aplicada al LEDC
uint32_t lastLcd = 0;

// Busca la LCD en las dos direcciones tipicas del backpack PCF8574
uint8_t detectarLcd() {
  const uint8_t dirs[] = {0x27, 0x3F};
  for (uint8_t d : dirs) {
    Wire.beginTransmission(d);
    if (Wire.endTransmission() == 0) return d;
  }
  return 0;
}

// Devuelve la frecuencia que el LEDC esta generando de verdad
uint32_t freqReal() {
#ifdef CORE_V2
  return ledcReadFreq(PWM_CHANNEL);
#else
  return ledcReadFreq(PIN_PWM);
#endif
}

// Reconfigura el LEDC. Devuelve false si el driver rechazo la frecuencia.
bool aplicarPwm(int freq, uint32_t duty) {
#ifdef CORE_V2
  bool ok = ledcChangeFrequency(PWM_CHANNEL, freq, RES_BITS) != 0;
  ledcWrite(PWM_CHANNEL, duty);
#else
  bool ok = ledcChangeFrequency(PIN_PWM, freq, RES_BITS) != 0;
  ledcWrite(PIN_PWM, duty);
#endif
  return ok;
}

uint32_t dutyParaDwell(int freq) {
  float dutyFrac = DWELL_MS * freq / 1000.0f;
  if (dutyFrac > DUTY_MAX) dutyFrac = DUTY_MAX;
  return dutyFrac * DUTY_FULL;
}

void setup() {
  Serial.begin(115200);

  Wire.begin(PIN_SDA, PIN_SCL);
  uint8_t dir = detectarLcd();
  if (dir != 0) {
    lcd = new LiquidCrystal_I2C(dir, 20, 4);
    lcd->init();
    lcd->backlight();
    lcd->setCursor(0, 0);
    lcd->print("Probador de bobinas");
    hayLcd = true;
    Serial.printf("LCD encontrada en 0x%02X\n", dir);
  } else {
    Serial.println("LCD no detectada, sigo solo con Serial");
  }

  analogReadResolution(12);

#ifdef CORE_V2
  ledcSetup(PWM_CHANNEL, FREQ_MIN, RES_BITS);
  ledcAttachPin(PIN_PWM, PWM_CHANNEL);
  bool pwmOk = true;
#else
  bool pwmOk = ledcAttach(PIN_PWM, FREQ_MIN, RES_BITS);
#endif
  if (!pwmOk) {
    Serial.println("ERROR: fallo la configuracion del PWM (LEDC)");
    if (hayLcd) { lcd->setCursor(0, 1); lcd->print("ERROR config PWM!"); }
    while (true) delay(1000);   // no tiene caso seguir
  }

  freqActual = FREQ_MIN;
  aplicarPwm(FREQ_MIN, dutyParaDwell(FREQ_MIN));
}

void loop() {
  // Promedio de 16 lecturas para estabilizar el pot
  long suma = 0;
  for (int i = 0; i < 16; i++) suma += analogRead(PIN_POT);
  int adc = suma / 16;

  int freq = map(adc, 0, 4095, FREQ_MIN, FREQ_MAX);

  // Solo reconfigurar el LEDC cuando el cambio es real (>= 2 Hz).
  // Reconfigurar en cada vuelta del loop reinicia el temporizador
  // antes de completar un periodo y el PWM sale glitcheado.
  if (abs(freq - freqActual) >= 2) {
    freqActual = freq;
    if (!aplicarPwm(freqActual, dutyParaDwell(freqActual))) {
      Serial.printf("ERROR: LEDC rechazo %d Hz\n", freqActual);
    }
  }

  // Refresco de pantalla y serial cada 200 ms
  if (millis() - lastLcd >= 200) {
    lastLcd = millis();
    int rpm = freqActual * 30;   // motor 4 tiempos, 4 cilindros
    int dutyPct = int(100.0f * dutyParaDwell(freqActual) / DUTY_FULL);

    if (hayLcd) {
      char linea[21];
      snprintf(linea, sizeof(linea), "Frec: %3d Hz        ", (int)freqReal());
      lcd->setCursor(0, 1); lcd->print(linea);
      snprintf(linea, sizeof(linea), "RPM (4cil): %5d    ", rpm);
      lcd->setCursor(0, 2); lcd->print(linea);
      snprintf(linea, sizeof(linea), "Dwell:%.1fms D:%2d%%   ", DWELL_MS, dutyPct);
      lcd->setCursor(0, 3); lcd->print(linea);
    }
    // adc crudo para diagnosticar el pot: debe ir de ~0 a ~4095 al girarlo
    Serial.printf("adc=%4d  f_obj=%3d Hz  f_real=%3u Hz  duty=%d%%\n",
                  adc, freqActual, freqReal(), dutyPct);
  }
}
