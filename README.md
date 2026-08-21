# Probador de bobinas COP — ESP32-C3 Super Mini

Generador de disparos PWM para probar bobinas de encendido (COP). Un potenciómetro
controla la frecuencia (10–200 Hz ≈ 300–6000 RPM en motor de 4 cilindros) y el
firmware mantiene un **dwell fijo de 3 ms** recalculando el duty. La pantalla
**OLED 128×64 I2C** muestra RPM, frecuencia, dwell y el logo SISU.

Diseñado por **Ramiro** · Logo SISU en la serigrafía inferior de la placa.

## Carcasa imprimible

![Render de la carcasa completa y su tapa](Carcasa%20del%20probador%20de%20bobinas/render_carcasa_completa.png)

Los archivos `carcasa.gcode` y `tapa carcasa.gcode` están listos para impresión
y fueron generados con OrcaSlicer.

## Estructura del repositorio

| Carpeta / archivo | Contenido |
|---|---|
| `Probador_Bobinas.ino` | Firmware para ESP32-C3 (Arduino) |
| `Probador_Bobinas_PCB/` | Proyecto KiCad 10 (esquemático + placa) |
| `Probador_Bobinas_PCB/PARA_FABRICAR/` | **Gerbers ZIP** — se sube tal cual a JLCPCB/PCBWay (2 capas, 1.6 mm, 1 oz) |
| `Probador_Bobinas_PCB/fabricacion/` | STEP 3D de la placa armada, esquemático PDF y BOM CSV |
| `PCB bobina Imprimir/` | PDFs 1:1 para **fabricación casera**: cobre para planchar (2), máscara UV (2), serigrafía (2) y STEP del LCD 20x4 para la carcasa |
| `Carcasa del probador de bobinas/` | G-code de la carcasa y la tapa, más su render conjunto |
| `libreria_kicad_SnapEDA/` | Librería KiCad autocontenida: símbolos, footprints y modelos 3D de todas las partes no estándar |

> Para abrir el proyecto KiCad en otra PC: registrar `libreria_kicad_SnapEDA` como
> librería "SnapEDA" (símbolos y footprints) en las tablas globales.

## La placa

- 61 × 74 × 1.6 mm, doble cara, componentes THT (pensada para soldar a mano)
- Etapa de potencia: **IGBT FGH60N60SFD** (600 V / 60 A, TO-247) con zener 15 V
  gate-emisor — reemplaza al IRLZ44N del prototipo (que avalanchaba a 55 V)
- Alimentación lógica: módulo **LM2596** 12 V → 5 V
- Clemas WAGO 236-402 (12 V y bobina), potenciómetro WH148 al borde (panel),
  header 1x4 para la pantalla OLED I2C
- Reglas de diseño: pista del colector con **2.5 mm de separación** (flyback
  ~400 V), potencia a 2 mm (10 A pulsados), señales a 0.8 mm (aptas para planchado)
- Plano de tierra cuadriculado en ambas caras; taladros M3 de esquina **H1/H2/H4
  con anillo a GND** para aterrizar la carcasa metálica (H3 solo mecánico)
- Rutas de refuerzo con alambre marcadas en la capa `Cmts.User` (retorno GND,
  colector 300 V y VCC de bobina) para la versión casera

## Cableado externo

| Conexión | Detalle |
|---|---|
| Clema J2 | Entrada 12 V (batería/fuente que aguante picos de 5–10 A) + fusible 5 A externo |
| Clema J1 | Primario de la bobina COP |
| Header J3 | OLED 128×64 I2C: 5V · GND · SDA (GPIO8) · SCL (GPIO9) |
| Cuerpo (rosca) de la bujía | GND de potencia — sin esto no hay chispa |

## Compilar y subir (Arduino IDE)

1. Instalar el core ESP32: Preferencias → URLs adicionales →
   `https://espressif.github.io/arduino-esp32/package_esp32_index.json`,
   luego Boards Manager → instalar "esp32 by Espressif Systems".
2. Instalar las librerías **Adafruit GFX** y **Adafruit SSD1306**.
3. Placa: **ESP32C3 Dev Module**, con **USB CDC On Boot: Enabled**.
4. Compilar y subir. Si no entra en modo de programación: mantener BOOT,
   pulsar RESET, soltar BOOT.

El sketch es compatible con el core ESP32 v2.x y v3.x (detecta la API de LEDC
automáticamente).

## Fabricación

**Profesional:** subir `PARA_FABRICAR/Probador_Bobinas_GERBERS.zip` a JLCPCB —
2 capas, 1.6 mm, cobre 1 oz, resto por defecto. (El PCBA no aplica: el ESP32
SuperMini y el módulo LM2596 son módulos de aftermarket que se sueldan a mano.)

**Casera (planchado + máscara UV):** PDFs en `PCB bobina Imprimir/`, impresora
láser a escala 100 % (el contorno debe medir 61 × 74 mm). Los archivos indican
en su nombre si van espejados o no. Las 7 vías se hacen con alambre pasante
soldado por ambos lados. Máscara UV: imprimir 2 acetatos por cara y encimarlos.

## Advertencias

- El primario genera picos de 300–400 V al cortar. El IGBT los soporta con
  margen, pero usar **disipador** en sesiones largas (hay espacio junto a Q1).
- No poner diodo flyback ni TVS en el primario: mata la chispa.
- Mantener los cables de alta tensión lejos del ESP32 (el EMI de la chispa
  puede resetearlo).
- Probar siempre con bujía o chispómetro conectado a la bobina.
