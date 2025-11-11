---

# 🧠 Task 2 — Dual-Core ESP32 Peripheral Control

## 🎯 Tujuan

1. Menghubungkan semua **peripheral** ke **satu ESP32-S3 DevKit**.
2. Membuat **program Task** untuk masing-masing peripheral.
3. Menjalankan masing-masing **I/O pada core berbeda** menggunakan fitur **dual-core (Core 0 & Core 1)** pada ESP32.
4. Mendokumentasikan langkah percobaan dengan **gambar dan video**, lalu menampilkan hasilnya di **GitHub**.

---

## ⚙️ Daftar Peripheral yang Digunakan

| No | Peripheral                   | Fungsi                                      | Jenis  | Core Digunakan  |
| -- | ---------------------------- | ------------------------------------------- | ------ | --------------- |
| 1  | OLED SSD1306 (I²C 128×64)    | Menampilkan string                          | Output | Core 1          |
| 2  | LED                          | Indikator digital (nyala/mati)              | Output | Core 0          |
| 3  | Push Button                  | Input digital (tombol tekan)                | Input  | Core 1          |
| 4  | Buzzer                       | Indikator suara                             | Output | Core 1          |
| 5  | Potentiometer                | Input analog (ADC)                          | Input  | Core 1          |
| 6  | Motor Stepper + Driver A4988 | Gerakan motor presisi                       | Output | Core 1          |
| 7  | Rotary Encoder               | Pembacaan posisi/sudut                      | Input  | Core 0          |
| 8  | Servo Motor                  | Gerakan sudut                               | Output | Core 0          |

---

## 🧰 1. Persiapan Alat dan Bahan

### 🧩 Hardware

* ESP32-S3 DevKitC-1
* LED
* Push Button
* Buzzer
* Potentiometer
* OLED SSD1306 (I²C 128×64)
* Motor Stepper + Driver A4988
* Rotary Encoder
* Servo Motor

### 💻 Software

* **Visual Studio Code** dengan **PlatformIO**
* **Wokwi Simulator** (untuk pengujian virtual)

---

## 🧪 2. Langkah dan Hasil Percobaan

### ⚙️ A. Build dan Upload Program

1. Buka **VSCode** dengan **PlatformIO** dan **Wokwi Simulator**.
2. Pilih board: **ESP32-S3-DevKitC-1**.
3. Pastikan folder proyek berstruktur:

   ```
   .pio
   .vscode
   /include
   /lib
   /src/main.cpp   → kode program utama
   wokwi.toml      → konfigurasi simulasi
   diagram.json    → konfigurasi wiring
   platformio.ini  → konfigurasi build/upload/library
   ```
4. Konfigurasi wiring pada file diagram.json
5. Tulis program pada main.cpp.
6. Lakukan **build PlatformIO** dan **jalankan Wokwi: start sumulation**.

---

### 🔌 B. Wiring Rangkaian & Hasil Pengujian
https://github.com/user-attachments/assets/91bc5ca7-9915-43f0-9cc6-02b4db17d4f1
* **LED**: GPIO 5, 6, 7 
* **Button**: GPIO 4
* **Buzzer**: GPIO 8
* **Potentiometer**: GPIO 1 (ADC1_0)
* **OLED SDA/SCL**: GPIO 10 / GPIO 11
* **Stepper STEP/DIR**: GPIO 18 / GPIO 19
* **Encoder CLK/DT/SW**: GPIO 2 / GPIO 3 / GPIO 15
* **Servo**: GPIO 9
---
