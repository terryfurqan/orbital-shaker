# Wiring Diagram & Skema Pinout DIY Orbital Shaker

Dokumen ini menjelaskan koneksi lengkap kabel, pinout, dan distribusi daya untuk DIY Digital Orbital Shaker berbasis **Arduino Uno R3**, **MKS TMC2209 V2.0 / CNC Shield V3**, **LCD 1602 I2C**, **Rotary Encoder KY-040**, dan **Keypad 4x4**.

---

## 1. Distribusi Catu Daya (Power Distribution)

```
[PLN 220V AC] 
      │
      ▼
[PSU Jaring 24V 5A (120W)]
      ├───[24V DC]───► TMC2209 VMOT (atau Terminal Power CNC Shield 12-36V)
      └───[24V DC]───► Modul Step-Down LM2596 (Input)
                             │
                             ▼ (Disetel Output 12V DC)
                             ├───► Kipas Pendingin DC 12V 5010 (Meniup TMC2209)
                             └───► Pin VIN Arduino Uno (Regulator internal 5V)
```

> **PENTING**:
> 1. Setel trimpot LM2596 ke **12.0V** menggunakan multimeter sebelum disambungkan ke pin VIN Arduino dan Kipas 12V.
> 2. Semua Ground (GND PSU 24V, GND Step-Down, GND Arduino, GND TMC2209) **WAJIB terhubung bersama (Common Ground)**.

---

## 2. Tabel Pinout Arduino Uno R3

| Pin Arduino | Fungsi / Periferal | Keterangan |
| :--- | :--- | :--- |
| **D0 (RX)** | Serial Debugging / USB | Dibiarkan bebas untuk upload & serial monitor |
| **D1 (TX)** | Serial Debugging / USB | Dibiarkan bebas untuk upload & serial monitor |
| **D2** | **TMC2209 STEP** | CNC Shield X.STEP (Pulse timer interrupt) |
| **D3** | **KY-040 CLK** | Hardware Interrupt INT1 (Zero-miss rotation) |
| **D4** | **KY-040 DT** | Direction rotary encoder |
| **D5** | **TMC2209 DIR** | CNC Shield X.DIR (Arah putaran) |
| **D6** | **KY-040 SW** | Tombol tekan encoder (Internal Pull-Up) |
| **D7** | **Keypad Row 1 (R1)** | Baris 1 matriks keypad |
| **D8** | **TMC2209 ENABLE** | CNC Shield EN (Active LOW: LOW=Jalan, HIGH=Off) |
| **D9** | **Keypad Row 2 (R2)** | Baris 2 matriks keypad |
| **D10** | **Keypad Row 3 (R3)** | Baris 3 matriks keypad |
| **D11** | **Keypad Row 4 (R4)** | Baris 4 matriks keypad |
| **D12** | **Keypad Col 1 (C1)** | Kolom 1 matriks keypad |
| **D13** | **Keypad Col 2 (C2)** | Kolom 2 matriks keypad |
| **A0 (D14)** | **Keypad Col 3 (C3)** | Kolom 3 matriks keypad |
| **A1 (D15)** | **Keypad Col 4 (C4)** | Kolom 4 matriks keypad |
| **A2 (D16)** | **Buzzer (+)** | Audio feedback saat tombol ditekan & timer selesai |
| **A3 (D17)** | **Spare / E-Stop** | Cadangan saklar darurat |
| **A4 (D18)** | **LCD I2C SDA** | Jalur data I2C PCF8574 |
| **A5 (D19)** | **LCD I2C SCL** | Jalur clock I2C PCF8574 |

---

## 3. Konfigurasi Jumper Driver TMC2209 (Pada CNC Shield V3)

Jika Anda menggunakan **CNC Shield V3** yang dicolok di atas Arduino Uno:
1. Pasang **1 buah Jumper Microstepping** pada slot X-Axis di bawah driver TMC2209:
   - Pasang jumper pada pin **M0** (atau MS1) untuk mode **1/16 Microstepping** (atau StealthChop standalone mode).
2. Setel **Vref TMC2209** ke $\approx 0.9\text{V} - 1.0\text{V}$:
   $$I_{\text{RMS}} = \frac{V_{\text{ref}}}{1.414 \times 8 \times R_{\text{sense}}} = \frac{0.95}{1.414 \times 8 \times 0.11} \approx 0.76\text{A RMS} \ (1.1\text{A Peak})$$
   *Nilai ini sangat bertenaga untuk NEMA 17 17HS4401 tanpa membuat motor panas berlebih.*

---

## 4. Skema Koneksi Keypad Matriks 4x4

Kabel pita Keypad 4x4 (8 pin dari kiri ke kanan):
```
Pin 1 (R1) ──► Arduino Pin D7
Pin 2 (R2) ──► Arduino Pin D9
Pin 3 (R3) ──► Arduino Pin D10
Pin 4 (R4) ──► Arduino Pin D11
Pin 5 (C1) ──► Arduino Pin D12
Pin 6 (C2) ──► Arduino Pin D13
Pin 7 (C3) ──► Arduino Pin A0
Pin 8 (C4) ──► Arduino Pin A1
```

---

## 5. Skema Koneksi Rotary Encoder KY-040

```
KY-040 GND  ──► Arduino GND
KY-040 +5V  ──► Arduino 5V
KY-040 SW   ──► Arduino Pin D6
KY-040 DT   ──► Arduino Pin D4
KY-040 CLK  ──► Arduino Pin D3 (INT1)
```

---

## 6. Skema Koneksi LCD 1602 I2C

```
LCD GND ──► Arduino GND
LCD VCC ──► Arduino 5V
LCD SDA ──► Arduino Pin A4
LCD SCL ──► Arduino Pin A5
```
