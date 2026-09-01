# Revision Log — Pekerjaan Agent (DeepSeek Harness)
## Project: DIY Digital Orbital Shaker (Arduino Uno R3 + TMC2209 + LCD Keypad Shield)
**Workspace Agent**: `A:\2. Doku Maaan\[BRAIN]\9. DSH`
**Rujukan**: Log utama proyek di `A:\2. Doku Maaan\[BRAIN]\2. ORbital TERR\REVISION_LOG.md` (Rev 1.0–4.2)

> Folder ini memisahkan seluruh hasil kerja agent dari file proyek utama.
> Penomoran revisi dilanjutkan dari log utama (Rev 5.0 dst).

---

### [REV 5.0] - DSH Agent: SELECT Toggle Engine + UP/DOWN Speed Control (Lanjutan Rev 4.2)
- **Tanggal/Waktu**: 2026-09-01
- **Status**: ✅ Kompilasi OK — ✅ Ter-upload ke `COM5` — ⏳ Verifikasi fisik oleh pengguna (belum)
- **Lokasi Kode**: `firmware/orbital_shaker/orbital_shaker.ino`

#### 🎯 Tujuan (sesuai handoff)
1. **SELECT** = saklar START/STOP toggle:
   - Tekan 1x → motor berputar **kontinu & konstan pada 30 RPM**.
   - Tekan lagi → **STOP**: driver standby, arus dimatikan agar dingin.
2. **UP / DOWN** = ±5 RPM (rentang 5–300, default 30).
3. **LCD 1602** = `RPM: 30  RUNNING` (spinner `| / - \`) / `RPM: 30  STOPPED`.

#### ⚙️ Perubahan / Desain Utama
1. **Direct Pulse Loop (bukti Rev 4.2 & hardware_diagnostics)**: pulsa STEP 40 µs HIGH
   + (period−40) µs LOW, dengan period dihitung dari RPM:
   `periodUs = 60,000,000 / rpm / 3200` (30 RPM → 625 µs → 1600 Hz).
2. **Chunked UI-servicing**: motor jalan 40 ms per-slice (`CHUNK_MS`), lalu tombol A0
   dan LCD dipindai. Motor bebas beban → jitter antar-chunk (±1–2%) tidak masalah,
   tetapi tombol SELECT tetap responsif SAAT motor berputar (masalah lama Rev 4.0:
   tombol tidak pernah dipindai selama 15 detik spin).
3. **Debounce edge-triggered + lockout**: 3× pembacaan ADC identik berurutan baru
   dianggap stabil; transisi IDLE→tombol = satu event; lockout 350 ms anti
   *double-trigger* (bouncing saat tekan/lepas — akar masalah Rev 1.0 & 3.0).
4. **ENABLE aktif-LOW benar**: LOW = driver ON saat RUNNING, HIGH = standby saat
   STOP (driver dingin). DIR HIGH = forward.
5. **LCD live**: baris 1 `RPM: %3d RUNNING/STOPPED`, baris 2 petunjuk tombol +
   spinner kolom 16; refresh tiap 200 ms.
6. **Mode Diag ADC (tombol LEFT)**: menampilkan `A0:xxxx TOMBOL` live — untuk
   verifikasi ambang ADC tombol SELECT pada shield ini (isu terbuka dari Rev 4.0:
   nilai SELECT belum pernah terverifikasi).

#### ✅ Hasil Kompilasi (arduino-cli 1.5.2-rc.1, FQBN arduino:avr:uno)
- **V1 (sebelum fix)**: Flash 6814 bytes (21%), RAM 410 bytes (20%).
- **V2 (setelah fix review, ter-upload)**: Flash **6924 bytes (21%)**, RAM **433 bytes (21%)** (sisa 1615 bytes).
- Upload ke **COM5**: **SUCCESS** (exit 0, tanpa error avrdude).

#### 🔬 Hasil Review Kode (Agent paralel) — Verdict: PASS-WITH-FIXES
- **CRITICAL**: tidak ada.
- **W1 (diperbaiki)** — Dead-zone ADC SELECT vs idle tidak ada (dua-duanya 940).
  Fix: `ADC_SELECT_HI = 900`, `ADC_IDLE_HI = 920` (901–920 = NONE, hysteresis)
  sesuai doc proyek: Select < 900, Idle > 920.
- **W2 (diperbaiki)** — Tidak ada ramp akselerasi; start diam di RPM tinggi (mis.
  300) berisiko stall (bahaya yang sama dengan Rev 2.0/3.0).
  Fix: ramp start **10 RPM → target, +10 RPM per chunk (40 ms)** ≈ 250 RPM/s.
- **S3 (diperbaiki)** — Edge-trigger kini murni: aksi hanya saat transisi
  IDLE → tombol (tombol→tombol & tombol→IDLE hanya memperbarui state).
- **S4 (diperbaiki)** — Lockout 350 ms kini **per-tombol** (sebelumnya semua tombol
  diblokir 350 ms setelah aksi apa pun — urutan cepat SELECT→UP bisa hilang).
- **S5 (diperbaiki)** — `ENABLE` (A1) di-set `pinMode`+`digitalWrite(HIGH)` pada
  baris PALING AWAL `setup()` → driver tidak mengambang/nyala di 700 ms pertama.
- **S6 (diperbaiki)** — `periodUs` kini dibulatkan (bukan truncate) →
  RPM aktual lebih dekat ke nominal di kecepatan tinggi (30 RPM = 625 µs,
  300 RPM = 63 µs). Overhead `digitalWrite` +`delayMicroseconds` tetap membuat
  RPM aktual beberapa % di bawah nominal di 300 RPM — tidak bermasalah untuk
  shaker tanpa beban.

#### 📋 Yang Perlu Diverifikasi Fisik oleh Pengguna
1. Tekan `SELECT` → motor berputar 30 RPM kontinu; tekan lagi → berhenti & bebas.
2. Tekan `UP`/`DOWN` → RPM berubah ±5 (cek di LCD & putaran motor).
3. Jika `SELECT` tidak terdeteksi: tekan `LEFT` untuk mode diag, catat nilai ADC A0
   saat SELECT ditekan, lalu sesuaikan ambang di firmware (atau laporkan nilainya).
4. Pastikan beban shaker tidak dinaikkan sebelum ramp akselerasi ditambahkan
   (beban inersia 1.5 kg membutuhkan ramp-up — sesuai CONTEXT.md).

---
