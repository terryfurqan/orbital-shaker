# Session Checkpoint: DIY Digital Orbital Shaker (Hardware Diagnosis & Firmware Evolution)
**Tanggal**: 2026-09-01  
**Target Hardware**: Arduino Uno R3 (`/dev/ttyACM0`), TMC2209 SilentStepStick (24V VMOT), NEMA 17 17HS4401, LCD Keypad Shield 1602 (Parallel + Analog A0).

---

## 1. Objective & Starting Context
- **Tujuan Awal**:
  1. Menarik pembaruan dari GitHub untuk project orbital-shaker dan repositori konfigurasi (AGY vs DSH).
  2. Membandingkan pendekatan AGY (Rev 4.2 pada branch `main`) vs DSH (Rev 5.0 pada branch `dsh/rev-5.0`).
  3. Memahami dan membedah secara teoritis mekanisme fisik gerak stepper NEMA 17 dan peran driver TMC2209.
  4. Mendiagnosis penyebab motor hanya menyentak sedikit saat tombol SELECT ditekan pada firmware DSH.
  5. Menguji pergerakan terkontrol: dari 10 langkah kasat mata, 1.000 langkah, hingga putaran 360 derajat ($360^\circ$) halus tanpa getaran.
- **Kondisi Awal**:
  - Repositori lokal tertinggal 1 commit dari remote `main`.
  - Remote memiliki branch baru `dsh/rev-5.0` yang dikembangkan oleh agen DSH.
  - Arduino Uno terhubung fisik di `/dev/ttyACM0`.

---

## 2. Completed Work & Artifacts
- **Sinkronisasi Git & Repositori**:
  - `git pull origin main` berhasil (`88008f7` -> `ba42fe4` -> `2e02fc6` -> `ffde1ef` -> `e16edac` -> `553b258` -> `d8e1bcc`).
  - Branch remote `origin/dsh/rev-5.0` di-fetch dan di-track secara lokal.
  - Konfigurasi `dsh-config` dan `antigravity-config` diverifikasi up-to-date.
- **Firmware Pengujian yang Dibuat & Teruji**:
  - [`Arduino code/test_10_steps/test_10_steps.ino`](file:///home/terryfurqan/Documents/[BRAIN]/[1.%20Belajar]/[Orbital%20Shaker]/Arduino%20code/test_10_steps/test_10_steps.ino): Uji 10 Full Steps ($18^\circ$) dengan jeda 600 ms dan audio click sinkron.
  - [`Arduino code/test_1000_steps/test_1000_steps.ino`](file:///home/terryfurqan/Documents/[BRAIN]/[1.%20Belajar]/[Orbital%20Shaker]/Arduino%20code/test_1000_steps/test_1000_steps.ino): Dual-mode 1.000 pulsa microstep ($112,5^\circ$) dan 1.000 full steps (5 putaran penuh).
  - [`Arduino code/test_360_smooth/test_360_smooth.ino`](file:///home/terryfurqan/Documents/[BRAIN]/[1.%20Belajar]/[Orbital%20Shaker]/Arduino%20code/test_360_smooth/test_360_smooth.ino): Uji putaran $360^\circ$ (3.200 langkah) dengan Safe Pull-in Band 6.0 RPM (320 Hz), jeda standstill StealthChop2 200 ms, dan hard lockout anti-flyback 2.5s.
- **Arsip & Dokumentasi Teknis**:
  - [`REVISION_LOG.md`](file:///home/terryfurqan/Documents/[BRAIN]/[1.%20Belajar]/[Orbital%20Shaker]/REVISION_LOG.md): Diperbarui hingga entri **REV 5.1**, **REV 5.2**, **REV 5.3**, dan **REV 5.4**. Seluruh pembaruan di-push ke GitHub.
  - Perizinan serial port `/dev/ttyACM0` dikonfigurasi permanen (grup `dialout`).

---

## 3. Decision Log & Idea Evolution
| Topik / Masalah | Ide / Eksperimen Awal | Alasan Pivot / Kendala | Keputusan / Status Akhir |
| :--- | :--- | :--- | :--- |
| **DSH Rev 5.0 Toggle Stop** | Debounce timer 350 ms + chunk 40 ms | Durasi jari melepas tombol ($300-500$ ms) melampaui 350 ms $\to$ *release-bounce* terbaca sebagai klik kedua (auto-stop). | Diidentifikasi akar masalahnya: release bounce & EMI drop pada pin A0. |
| **Uji 1.000 Langkah (Rev 5.1)** | Menampilkan progres counter LCD tiap 10% di dalam loop langkah | `lcd.print()` memakan waktu $1,5-2$ ms, memotong frekuensi 1600 Hz tiap 10% $\to$ motor bergetar *"brrrp"* bertahap. | Berhasil membuktikan motor bisa berputar, namun mengidentifikasi bahwa I/O LCD tidak boleh ada di dalam loop pulsa. |
| **Uji 360° Ramp Lembut (Rev 5.2)** | Ramp akselerasi dari 7.5 RPM dengan pulsa 20 $\mu$s | Pulsa 20 $\mu$s diredam kapasitansi kabel; kecepatan 7.5 RPM jatuh tepat di zona resonansi mekanis alami NEMA 17 $\to$ *stall* (*"trek! jeda trek"*). | Eksperimen dicatat gagal di log. Pulsa wajib $\ge 40\,\mu\text{s}$. |
| **Uji 360° Murni (Rev 5.3)** | Mengembalikan pulsa 40 $\mu$s / 585 $\mu$s (30 RPM) tanpa I/O LCD | Tembakan frekuensi instan 1.600 Hz dari diam melampaui *instantaneous pull-in rate* inersia diam rotor $\to$ motor menghentak 1 langkah lalu macet. | Disadari bahwa 1.600 Hz adalah *slew rate*, bukan *pull-in rate*. |
| **Uji 360° Safe Pull-in (Rev 5.4)** | Turun ke zona aman 6.0 RPM (320 Hz) + standstill 200 ms + lockout 2.5s | Kode diuji dan di-upload, namun motor di lapangan masih mengalami *"trek ... jeda ... trek"*. | Kode 100% sesuai rencana; masalah bergeser ke **audit sistem kelistrikan/hardware**. |

---

## 4. Current Status
- **Status Kode**: Kode pada [`test_360_smooth.ino`](file:///home/terryfurqan/Documents/[BRAIN]/[1.%20Belajar]/[Orbital Shaker]/Arduino%20code/test_360_smooth/test_360_smooth.ino) **100% sesuai rencana implementasi** (Flash 13%, RAM 18%, upload sukses).
- **Status Perilaku Hardware**: Motor masih mengalami gejala *stall* / sentakan singkat (*"trek ... jeda ... trek"*).
- **Hipotesis Investigasi Sistem (Hardware Root Cause)**:
  1. **Konfigurasi Pin MS1 / MS2 pada Modul TMC2209**: Jika pin MS1/MS2 mengambang (*floating*), driver tidak berada di mode 1/16 microstep, melainkan full-step/half-step, sehingga frekuensi 320 Hz yang dikirim sebenarnya adalah kecepatan tinggi (96 RPM) yang memicu stall seketika.
  2. **Penyetelan Tegangan Referensi (Vref TMC2209)**: Jika trimpot Vref terlalu rendah ($<0.5\text{V}$), arus motor terlalu lemah untuk memutar poros; jika terlalu tinggi ($>1.4\text{V}$), driver langsung masuk proteksi *Thermal Overcurrent Shutdown (OTP)*.
  3. **Integritas Suplai Daya 24V VMOT & Common Ground**: Perlu verifikasi apakah tegangan 24V DC benar-benar stabil mencapai pin VMOT TMC2209 dan ground Arduino terhubung solid dengan ground PSU.
  4. **Urutan Pinout 4 Kabel Kumparan NEMA 17**: Jika kabel fasa A dan fasa B bersilangan/salah pasang, motor tidak akan pernah bisa berputar kontinu dan hanya akan berkedut/mengunci di tempat (*holding lock*).

---

## 5. Next Session Scope & Action Plan
1. **Audit Hardware Interaktif (Langkah Demi Langkah Tanpa Ubah Kode Firmware)**:
   - Ukur tegangan fisik Vref pada titik pot meter TMC2209 menggunakan multimeter (target: $\approx 0.92\text{V}$).
   - Periksa tegangan 24V DC pada terminal VMOT-GND saat motor aktif.
   - Periksa sambungan kabel kumparan NEMA 17 (resistansi fasa A1-A2 $\approx 2-3\,\Omega$, fasa B1-B2 $\approx 2-3\,\Omega$, tanpa korslet antar fasa).
   - Periksa konfigurasi jumper pin MS1 & MS2 (wajib ditarik ke VCC / 5V untuk mode 1/16).
2. **Uji Isolasi Super Minimalis (Ultra-Low Diagnostic 1 Hz)**:
   - Jalankan pulsa 1 Hz (1 denyut per detik) murni untuk melihat apakah kutub magnet berpindah langkah demi langkah tanpa melibatkan inersia.
3. **Penyelesaian Integrasi**:
   - Setelah hardware terverifikasi 100%, gabungkan perbaikan debounce dan kurva akselerasi ke branch utama `orbital-shaker`.

---

## 6. Suggested Skills & Tools for Next Session
- `diagnosing-bugs`: Untuk memandu langkah eliminasi hipotesis hardware secara ketat.
- `multimeter`: Skrip atau prosedur pembacaan kalibrasi voltase Vref dan kontinuitas kabel motor.
- `run_command` & `arduino-cli`: Untuk kompilasi dan pembacaan telemetri serial port `/dev/ttyACM0`.
