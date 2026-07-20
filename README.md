# Probador de bobinas — ESP32-C3 Super Mini

Generador de disparos PWM para probar bobinas de encendido. Un potenciómetro
controla la frecuencia (10–200 Hz ≈ 300–6000 RPM en motor de 4 cilindros) y el
firmware mantiene un **dwell fijo de 3 ms** recalculando el duty. LCD 20x4 I2C
opcional (el sketch la detecta solo; si no está, funciona igual y reporta por
Serial).

## Cableado

### Lado lógico (3.3 V)

| Componente | Pin ESP32-C3 |
|---|---|
| Potenciómetro 10 kΩ (cursor) — extremos a 3V3 y GND | GPIO3 |
| Salida PWM → R1 150 Ω → PC817 pin 1 (ánodo) | GPIO4 |
| PC817 pin 2 (cátodo) | GND |
| LCD SDA | GPIO8 |
| LCD SCL | GPIO9 |
| LCD VCC | 5V |
| LCD GND | GND |

### Lado de potencia (12 V)

| Conexión | Detalle |
|---|---|
| PC817 pin 4 (colector) | +12 V |
| PC817 pin 3 (emisor) | → 100 Ω → gate del IRLZ44N |
| Gate → GND | resistencia 560 Ω (pull-down, apagado rápido) |
| Gate → Source | zener 9.1 V, cátodo (banda) al gate — clampea el gate en zona segura |
| Drain IRLZ44N | terminal (–) del primario de la bobina |
| Terminal (+) del primario | +12 V a través de fusible 5 A rápido |
| Source IRLZ44N | GND de potencia |
| Cuerpo (rosca) de la bujía | GND de potencia — sin esto no hay chispa |

**GND común:** unir el GND lógico y el de potencia en UN solo punto (el negativo
de la fuente). No dejar que la corriente de la bobina circule por el GND del micro.

## Lista de materiales

- ESP32-C3 Super Mini
- LCD 20x4 con backpack I2C (PCF8574) — opcional
- Potenciómetro 10 kΩ
- Optoacoplador PC817
- MOSFET IRLZ44N **con disipador** (obligatorio: absorbe la avalancha de la bobina)
- Resistencias: 150 Ω, 100 Ω, 560 Ω
- Zener 9.1 V (gate-source)
- Fusible 5 A rápido con portafusible
- Fuente o batería 12 V (que aguante picos de 5–10 A)
- Capacitores 100 nF + 10 µF en la alimentación del ESP32 (anti-ruido)

## Compilar y subir (Arduino IDE)

1. Instalar el core ESP32: Preferencias → URLs adicionales →
   `https://espressif.github.io/arduino-esp32/package_esp32_index.json`,
   luego Boards Manager → instalar "esp32 by Espressif Systems".
2. Instalar la librería **LiquidCrystal I2C** (Frank de Brabander) desde el
   Library Manager.
3. Placa: **ESP32C3 Dev Module**. Activar **USB CDC On Boot: Enabled** para ver
   el monitor serial por el USB de la placa.
4. Compilar y subir. Si no entra en modo de programación: mantener BOOT,
   pulsar RESET, soltar BOOT.

El sketch es compatible con el core ESP32 v2.x y v3.x (detecta la API de LEDC
automáticamente).

## Advertencias

- El primario de la bobina genera picos de 300–400 V al cortar. El IRLZ44N
  (55 V) los absorbe por avalancha: funciona, pero calienta — usar disipador.
  Para uso intensivo, cambiar por un IGBT de encendido (FGP3040 / ISL9V3040),
  el resto del circuito queda igual.
- No poner diodo flyback ni TVS en el primario: mata la chispa.
- Mantener los cables de alta tensión lejos del ESP32 (el EMI de la chispa
  puede resetearlo).
