# 9. DSH — Folder Kerja Agent (DeepSeek Harness)

Folder ini berisi **seluruh hasil pekerjaan agent** untuk proyek DIY Orbital Shaker,
dipisahkan dari file proyek utama di `2. ORbital TERR` agar mudah dibedakan mana
yang dikerjakan oleh agent dan mana yang bukan.

---

## 📂 Isi Folder

| Path | Isi |
| :--- | :--- |
| `firmware/orbital_shaker/orbital_shaker.ino` | **Firmware Rev 5.0** (deliverable utama): SELECT = START/STOP toggle, UP/DOWN = ±5 RPM (5–300), LCD live status + spinner, mode diag ADC (tombol LEFT). |
| `docs/REVISION_LOG.md` | Jurnal teknis detail pekerjaan agent (Rev 5.0+). |
| `README.md` | File ini. |

---

## 🛠 Cara Compile & Upload (arduino-cli)

```powershell
$cli = "C:\Users\Tora\AppData\Local\Programs\arduino-cli\arduino-cli.exe"
$sketch = "A:\2. Doku Maaan\[BRAIN]\9. DSH\firmware\orbital_shaker"

# Compile
& $cli compile --fqbn arduino:avr:uno $sketch

# Upload ke Arduino Uno COM5
& $cli upload -p COM5 --fqbn arduino:avr:uno $sketch
```

Serial Monitor: 115200 baud.

---

## 🎛 Cara Pakai (Rev 5.0)

- **SELECT** — 1x tekan: motor **RUNNING** (kontinu & konstan pada RPM aktif).
  1x tekan lagi: **STOP** (driver standby, arus dimatikan agar dingin).
- **UP** — naikkan kecepatan **+5 RPM** (maks 300).
- **DOWN** — turunkan kecepatan **−5 RPM** (min 5).
- **LEFT** — toggle mode diagnostik ADC A0 (menampilkan nilai ADC live di baris 2
  LCD, berguna untuk verifikasi ambang tombol SELECT pada shield ini).
- **LCD baris 1** — `RPM:  30 RUNNING` (dengan spinner `| / - \`) atau `RPM:  30 STOPPED`.
- **LCD baris 2** — `SELECT=STOP` (saat jalan) / `SELECT=START` (saat diam).

> Catatan: Teknik pemicu motor = direct pulse loop 40 µs (terbukti bekerja di
> `hardware_diagnostics.ino` & Rev 4.2), dipecah per-chunk 40 ms agar tombol &
> LCD tetap responsif selama motor berputar.

---

## 📌 Status Terakhir

Lihat `docs/REVISION_LOG.md` untuk riwayat lengkap dan hasil uji.
