# 📚 Multimeter Domain Glossary & Architecture Rules

Domain concepts, safety invariants, and technical definitions for digital multimeters (DMM), voltage calibration, and test equipment architecture in the Orbital Shaker and Edge AI Hardware projects.

---

## 1. Domain Glossary & Terminology

**Display Counts (Resolusi Layar)**:
* Definisi: Jumlah unit kuantisasi maksimum yang dapat ditampilkan oleh konverter Analog-ke-Digital (ADC) multimeter sebelum berganti skala.
* *Contoh*: Multimeter **6000 Counts** dapat menampilkan angka hingga `5.999V` dengan 3 digit di belakang koma ($1\text{ mV}$ step), sedangkan **9999 Counts** dapat menampilkan hingga `9.999V` dengan presisi $0.1\text{ mV}$ ($100\,\mu\text{V}$).
* _Hindari_: "Digit layar biasa", "Ketajaman LCD".

**True RMS (Root Mean Square)**:
* Definisi: Kemampuan sirkuit kalkulasi multimeter untuk mengukur nilai efektif tegangan atau arus bolak-balik pada bentuk gelombang non-sinusoidal (seperti riak *switching* power supply, output PWM inverter, dan beban non-linear).
* _Hindari_: "Average responding meter", "RMS biasa".

**Burden Voltage (Tegangan Beban Alat Ukur)**:
* Definisi: Penurunan tegangan (*voltage drop*) yang terjadi melintasi resistor *shunt* internal multimeter saat mengukur arus seri dalam sebuah rangkaian.
* *Dampak*: Pada pengukuran arus modul SBC (seperti Raspberry Pi / Jetson / Orange Pi), burden voltage yang terlalu tinggi dapat memicu *under-voltage throttling* atau *system reboot*.
* _Hindari_: "Hambatan probe".

**Instant Latching Continuity Buzzer**:
* Definisi: Fitur pendeteksi kontinuitas sirkuit listrik yang dilengkapi rangkaian komparator cepat dengan penahan nada (*latched oscillator*), sehingga bunyi *beep* terdengar instan ($< 30\text{ ms}$) saat kedua ujung probe bersentuhan tanpa jeda waktu *scratchy*.
* _Hindari_: "Buzzer biasa", "Tes kabel berisik".

**Needle Probe / Gold-Plated Fine Tip (Jarum Runcing Emas)**:
* Definisi: Kabel uji instrumen dengan ujung jarum tembaga berlapis emas berdiameter $\le 0.5\text{ mm}$ yang dirancang khusus untuk menyentuh kaki komponen SMD (*Surface Mount Device*), test pad, dan trimpot mikro tanpa risiko meleset (*slip*) ke jalur konduktor sebelahnya.
* _Hindari_: "Jarum pentul", "Kabel tusuk biasa".

**Non-Contact Voltage (NCV) & Live Wire Detection**:
* Definisi: Sensor kapasitif/induktif di ujung atas multimeter yang mampu mendeteksi keberadaan medan listrik AC ($90\text{V} - 1000\text{V}$) tanpa kontak fisik langsung dengan inti tembaga konduktor.
* _Hindari_: "Tespen colok".

---

## 2. Safety Invariants & Rules (CAT Rating & Fuse Architecture)

### ⚠️ Invarian Keselamatan Mutlak (Safety Rules)

1. **Aturan Colokan Jack Probe**:
   * Probe Merah **WAJIB** berada di jack **`V/Ω/Hz/°C`** saat mengukur tegangan (DC atau AC 220V).
   * **LARANGAN KERAS**: Menancapkan probe merah di jack **`A`** atau **`mA/uA`** saat mengukur stopkontak 220V PLN atau terminal output PSU, karena akan menciptakan sirkuit *dead short* berenergi tinggi.

2. **Kategori Isolasi Tegangan (Overvoltage Categories)**:
   * **CAT II**: Beban portabel fasa tunggal dan stopkontak rumah tangga standar ($220\text{V}$ AC).
   * **CAT III**: Instalasi distribusi daya gedung, sakelar utama, panel MCB, dan beban industri ringan ($400\text{V} - 600\text{V}$).
   * **CAT IV**: Titik awal saluran utilitas luar ruangan dan meteran listrik PLN sebelum MCB utama.

3. **Arsitektur Sekring (HRC vs Glass Tube)**:
   * **HRC Ceramic Fuse (High Rupture Capacity)**: Sekring tabung keramik berisi pasir kuarsa peredam busur api (*arc flash*), mampu memutus arus lonjakan hingga puluhan kiloampere ($kA$) dengan aman. Wajib untuk pengukuran lingkungan CAT III / IV.
   * **Glass Fuse (Sekring Kaca 250V)**: Hanya aman untuk sirkuit elektronik DC tegangan rendah dan beban arus mikro.

---

## 3. Decision Matrix & Hardware Pairing Table

| Skenario Pengujian | Target Komponen | Mode DMM yang Dipilih | Tipe Probe yang Wajib Digunakan | Batas Toleransi Normal |
| :--- | :--- | :--- | :--- | :--- |
| **Kalibrasi Driver Stepper** | Trimpot `Vref` TMC2209 | $V_{=}$ (DC mV / 2V range) | **Jarum Emas Runcing (Needle Tip)** | $0.95\,\text{V} \pm 0.05\,\text{V}$ |
| **Kalibrasi Regulator Buck** | Output LM2596 | $V_{=}$ (DC 20V range) | Needle Tip atau Probe Standar | $12.00\,\text{V} \pm 0.10\,\text{V}$ |
| **Verifikasi Stepper Coil** | 4-Pin NEMA 17 Header | $\Omega$ / Continuity Buzzer | Probe Standar atau Capit Buaya | $2.0\,\Omega - 4.0\,\Omega$ per fasa |
| **Verifikasi Catu Daya AC** | Stopkontak PLN / Input PSU | $V_{\sim}$ (AC 750V range) | **Probe Standar Tebal (Insulated)** | $210\,\text{V} - 235\,\text{V AC}$ |
| **Monitoring Termal SoC/AI** | Heatsink NPU / Driver | Temp Mode ($^\circ\text{C}$) | K-Type Thermocouple Probe | $< 70^\circ\text{C}$ di bawah beban |
