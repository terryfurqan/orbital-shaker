# Revision Log & Hardware Engineering Journal
## Project: DIY Digital Orbital Shaker (Arduino Uno R3 + TMC2209 + LCD Keypad Shield)
**Workspace**: `A:\2. Doku Maaan\[BRAIN]\2. ORbital TERR`  
**Target Hardware**: Arduino Uno R3 (`COM5`), TMC2209 V2.0 (24V VMOT, Vref 0.92V), NEMA 17 17HS4401 (3200 steps/rev), LCD Keypad Shield (Parallel 1602 + Analog Buttons A0).

---

### 📌 Ringkasan Status Hardware
- **Pin D2**: TMC2209 `STEP`
- **Pin D3**: TMC2209 `DIR` (HIGH = Forward)
- **Pin A1**: TMC2209 `ENABLE` (Active LOW: 0V = ON/Lock, 5V = Standby/Free)
- **Pin A0**: Resistor Ladder Tombol Keypad Shield (Analog ADC: Right <65, Up <220, Down <420, Left <680, Select <900, Idle >920)
- **Pin D4–D10**: Bus Paralel LCD 1602 (RS=8, EN=9, D4=4, D5=5, D6=6, D7=7, BL=10)
- **Pin A2**: Buzzer (+)
- **Beban Saat Ini**: Tanpa Beban (Motor bebas / *unloaded test bench*)

---

## 📜 Log Riwayat Revisi Firmware

### [REV 1.0] - Inisialisasi Firmware Baseline (Continuous Software Polling)
- **Tanggal/Waktu**: 2026-09-01
- **Deskripsi**: Firmware dasar menggunakan polling `micros()` dalam `loop()` untuk pulsa langkah, pemindaian tombol A0 setiap 40 ms, dan pembaruan LCD setiap 200 ms.
- **Gejala / Masalah**:
  - Tombol SELECT memicu motor bergerak sesaat lalu mati seketika.
  - Timbul dugaan *bouncing* tombol analog dan benturan delay `lcd.print()` terhadap aliran pulsa `micros()`.

---

### [REV 2.0] - Implementasi Hardware Timer1 CTC Interrupt Engine
- **Tanggal/Waktu**: 2026-09-01
- **Deskripsi**:
  - Mengalihkan pengiriman pulsa langkah ke interrupt perangkat keras Timer1 CTC (`TIMER1_COMPA_vect`) pada frekuensi 1600 Hz (30 RPM, OCR1A = 1249) untuk menghilangkan *jitter*.
  - Menambahkan *software debounce latch* (30 ms filter + 350 ms lockout delay).
- **Gejala / Masalah**:
  - Motor hanya menghentak 1 langkah (*single step / clunk*) saat tombol SELECT ditekan, lalu diam mengunci (*holding torque active*). Tidak berputar kontinu.
- **Analisis**:
  - Tembakan frekuensi instan 0 $\to$ 1600 Hz tanpa akselerasi diduga melampaui *instantaneous pull-in rate* motor stepper (*inertia stall*).

---

### [REV 3.0] - Penambahan Inertia Ramp Engine (Soft Accel & Decel)
- **Tanggal/Waktu**: 2026-09-01
- **Deskripsi**:
  - Menambahkan mesin akselerasi bertahap (*ramp engine* 50 Hz) mulai dari 2 RPM meluncur naik ke 30 RPM (akselerasi 40 RPM/detik).
  - Menambahkan *graceful deceleration* saat tombol STOP ditekan.
- **Gejala / Masalah**:
  - Motor bergerak lebih banyak dari Rev 2.0 (sekitar 2–3 langkah mekanik), namun tetap berhenti setelah itu.
- **Analisis Mendalam**:
  - Terjadi *double-triggering* akibat pantulan saat jari melepas tombol (*release bounce*) pada milidetik ke-350 yang memicu perintah STOP.
  - Terjadi potensi *counter overshoot* pada register `TCNT1` saat `OCR1A` diperbarui secara dinamis pada Timer1.
  - Pins 4, 5, 6, 7 (LCD Data) berbagi port register `PORTD` dengan Pin 2 (STEP) dan Pin 3 (DIR), sehingga manipulasi bit `PORTD` dalam ISR dapat berbenturan dengan operasi `digitalWrite()` pada library `LiquidCrystal`.

---

### [REV 4.0] - Direct Rock-Solid Continuous Engine (Zero-Friction / No Stall)
- **Tanggal/Waktu**: 2026-09-01
- **Perubahan Utama**:
  1. **Direct Continuous Stepping**: Menghilangkan kompleksitas transisi status ramp yang rentan *false stop*. Begitu tombol `SELECT` ditekan, driver langsung aktif dan motor berputar kontinu pada 30 RPM (1600 Hz).
  2. **Guaranteed Pulse Width (8 $\mu$s)**: Menggunakan durasi pulsa langkah 8 $\mu$s di dalam ISR Timer1 agar 100% terbaca oleh buffer input driver TMC2209 tanpa terpengaruh kapasitansi kabel jumper.
  3. **Strict Edge-Trigger Debounce Latch**:
     - Aksi hanya dieksekusi saat transisi `0 (IDLE) -> Tombol`.
     - Ditambahkan proteksi lockout minimal 400 ms.
     - Penahanan tombol tidak akan pernah memicu *repeat trigger* untuk tombol SELECT.
  4. **Lightweight LCD Refresh**: Layar hanya di-render setiap 200 ms dengan buffer baris tetap sehingga bus paralel LCD tidak membebani siklus CPU.
- **Hasil Pengujian**:
  - Kompilasi `arduino-cli`: Berhasil (37% flash, 21% RAM).
  - Flashing ke `COM5`: Berhasil.
  - Uji Serial Monitor: Motor merespons perintah `START` dari PC dan berputar kontinu stabil.
  - **Catatan**: Tombol fisik `SELECT` pada shield belum terpicu karena perbedaan nilai pembacaan ADC analog A0.

---

### [REV 4.1] - Auto-Run 15 Detik Langsung Saat Boot + Real-Time ADC Diagnostic
- **Tanggal/Waktu**: 2026-09-01
- **Gejala / Masalah**: Motor masih belum berputar saat auto-start karena ISR Timer1 conflict / pulsa 8 $\mu$s tidak cukup lebar.

---

### [REV 4.2] - Direct Solid Hardware Pulse Engine (40 $\mu$s High, 585 $\mu$s Low, 15s Pure Stream)
- **Tanggal/Waktu**: 2026-09-01
- **Perubahan Utama**:
  1. **Direct Dedicated Stepping Loop**: Menghilangkan Timer1 interrupt sepenuhnya dan menggunakan teknik yang sama persis seperti pada `hardware_diagnostics.ino` yang terbukti berhasil menggerakkan motor.
  2. **Solid 40 $\mu$s Pulse Width**: Lebar pulsa HIGH dinaikkan ke **40 mikrodetik** dan LOW **585 mikrodetik** (total 625 $\mu$s per step = 1600 Hz = 30 RPM).
  3. **Direct 24,000 Step Stream**: Langsung mengeksekusi 24.000 langkah pulsa murni (tepat 15 detik = 7.5 putaran penuh) seketika setelah inisialisasi boot.
  4. **Post-Spin ADC Diagnostic**: Setelah 15 detik selesai dan motor standby, LCD baris ke-2 langsung aktif menampilkan nilai ADC Pin A0 secara *live*.
- **Hasil Pengujian**:
  - Kompilasi & Flashing ke `COM5`: Berhasil (6298 bytes, 19% flash).

---

### [REV 5.0] - DSH Agent: SELECT Toggle + UP/DOWN Speed Control (+ Ramp Akselerasi)
- **Tanggal/Waktu**: 2026-09-01
- **Status**: Kompilasi OK — Upload ke `COM5` OK — Verifikasi fisik oleh pengguna (belum).
- **Lokasi Kode**: `A:\2. Doku Maaan\[BRAIN]\9. DSH\firmware\orbital_shaker\orbital_shaker.ino`
  (folder kerja agent; detail lengkap di `9. DSH\docs\REVISION_LOG.md`)
- **Perubahan Utama**:
  1. **SELECT = toggle START/STOP**: 1x tekan → motor berputar kontinu & konstan
     pada RPM aktif (default 30); tekan lagi → STOP, driver standby (ENABLE HIGH,
     arus dimatikan agar dingin).
  2. **UP/DOWN = ±5 RPM** (rentang 5–300).
  3. **Direct pulse loop 40 µs** (teknik terbukti Rev 4.2 / hardware_diagnostics)
     dipecah per-chunk 40 ms agar tombol A0 & LCD tetap dipindai SAAT motor
     berputar (menjawab kegagalan Rev 4.0: tombol tak pernah dipindai selama spin).
  4. **Ramp akselerasi start**: 10 RPM → target (+10 RPM / 40 ms ≈ 250 RPM/s) —
     anti stall saat start di RPM tinggi (menjawab bahaya Rev 2.0/3.0).
  5. **Debounce edge-triggered murni** (aksi hanya saat transisi IDLE→tombol) +
     lockout 350 ms per-tombol — menuntaskan bouncing Rev 1.0 & 3.0.
  6. **Dead-zone ADC**: SELECT ≤ 900, idle > 920 (901–920 = NONE) sesuai doc proyek
     (`Select < 900, Idle > 920`) — anti trigger palsu & anti SELECT mati.
  7. **Mode diag ADC** (tombol LEFT): tampil `A0:xxxx TOMBOL` live untuk verifikasi
     nilai ADC SELECT aktual shield ini (isu terbuka sejak Rev 4.0).
- **Hasil Kompilasi**: Flash 6924 bytes (21%), RAM 433 bytes (21%); upload COM5 sukses (exit 0).
- **Catatan Verifikasi Fisik**: uji SELECT start/stop, UP/DOWN ±5 RPM, dan nilai ADC
  SELECT via mode diag (LEFT). Beban inersia 1.5 kg (CONTEXT.md) belum terpasang —
  ramp akselerasi untuk beban penuh perlu penyesuaian sebelum produksi.


