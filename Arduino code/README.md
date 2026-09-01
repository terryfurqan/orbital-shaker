# Firmware DIY Digital Orbital Shaker (Pro Edition + Jam Digital)

Firmware ini dirancang khusus untuk mengontrol **DIY Digital Orbital Shaker** (Meja 25x30 cm, beban cairan s.d. 1.5 kg, rentang kecepatan 30 – 300 RPM) lengkap dengan tampilan **Jam Digital Real-Time** pada layar LCD 1602.

---

## 🌟 Fitur Tampilan LCD & Jam Digital

### 1. Layar Standby / IDLE (Saat Shaker Diam)
```
+----------------+
|RPM:120  STANDBY| -> Menampilkan target RPM yang disetel
|JAM  06:45:28WIB| -> Menampilkan Jam:Menit:Detik real-time
+----------------+
```

### 2. Layar Running (Saat Shaker Berputar)
- **Mode Timer**:
  ```
  +----------------+
  |RPM:150/150 RUN*| -> RPM aktual/target & animasi putaran
  |06:45 |TMR 14:30| -> Jam saat ini (kiri) | Sisa hitung mundur (kanan)
  +----------------+
  ```
- **Mode Continuous**:
  ```
  +----------------+
  |RPM:150/150 RUN*| -> RPM aktual/target & animasi putaran
  |06:45 |RUN 05:20| -> Jam saat ini (kiri) | Durasi berjalan (kanan)
  +----------------+
  ```

---

## 🕒 Cara Mengatur / Menyetel Jam

### Cara 1: Lewat Keypad 4x4
1. Pada kondisi **STANDBY (IDLE)**, tekan tombol **`D`**.
2. Layar akan menampilkan `Set Jam (0-23):`. Ketik 2 digit jam (misal `0` `7`) lalu tekan **`#`**.
3. Layar akan menampilkan `Set Menit (0-59):`. Ketik 2 digit menit (misal `3` `0`) lalu tekan **`#`**.
4. Jam otomatis tersinkronisasi dan mulai berdetik.

### Cara 2: Otomatis via Serial / Terminal
Setiap kali terhubung ke komputer/terminal, Anda bisa mengirim perintah:
```text
TIME=06:45:00
```
Maka jam pada Arduino langsung terupdate sesuai waktu yang dikirim.

---

## ⌨️ Panduan Tombol & Keypad 4x4

| Tombol Keypad | Fungsi Utama | Keterangan |
| :---: | :--- | :--- |
| **`0` – `9`** | Input Angka Cepat | Ketik angka RPM / Waktu langsung di layar |
| **`A`** | Ganti Mode | Beralih antara **Continuous Mode** dan **Timer Mode** |
| **`B`** | Set Timer (Menit) | Masuk ke menu input durasi hitung mundur (1 - 999 menit) |
| **`C`** | **START / RESUME** | Menjalankan shaker dengan ramp-up lembut |
| **`D`** | **STOP / Set Jam** | Menghentikan shaker saat jalan, atau masuk Menu Set Jam saat IDLE |
| **`*`** | Quick Set RPM / Backspace | Masuk ke menu input RPM atau menghapus angka |
| **`#`** | **Confirm / Toggle** | Konfirmasi angka atau toggle Start/Stop |

---

## 🎛️ Panduan Rotary Encoder KY-040

- **Putar Kiri / Kanan**: Menurunkan / Menaikkan RPM (kelipatan 5 RPM, rentang 30 - 300 RPM).
- **Tekan Singkat (< 0.8 detik)**: Toggle **START** / **STOP**.
- **Tekan & Tahan (> 0.8 detik)**: Toggle Mode (**Continuous** $\leftrightarrow$ **Timer**).
