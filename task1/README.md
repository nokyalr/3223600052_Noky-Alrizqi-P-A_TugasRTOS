---

# 🧠 Task 1 — Dual-Core ESP32 Peripheral Control

## 🎯 Tujuan

1. Menghubungkan semua **peripheral** ke **ESP32-S3 DevKit**.
2. Membuat **program terpisah** untuk setiap **Input/Output (I/O)**.
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
| 5  | Potentiometer                | Input analog (ADC)                          | Input  | Core 0          |
| 6  | Motor Stepper + Driver A4988 | Gerakan motor presisi                       | Output | Core 1          |
| 7  | Rotary Encoder               | Pembacaan posisi/sudut                      | Input  | Core 0          |
| 8  | Servo Motor                  | Gerakan sudut                               | Output | Core 1          |

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

#### 1️⃣ Rangkaian LED
https://github.com/user-attachments/assets/3856fb74-bbb9-40ef-b482-be925d324d97
* **Pin**: GPIO 5, 6, 7
* **Mode**: OUTPUT
* **Pengujian**: LED akan menyala selama tick yang diberikan.   

#### 2️⃣ Rangkaian Push Button
https://github.com/user-attachments/assets/bd2390b0-a956-4118-9dbe-0848223c1dbd
* **Pin**: GPIO 4
* **Mode**: INPUT_PULLUP
* **Pengujian**: serial monitor merespons saat tombol ditekan.

#### 3️⃣ Rangkaian Buzzer
https://github.com/user-attachments/assets/1dcda10a-49b8-434b-82ef-b7fd011e48ef
* **Pin**: GPIO 1
* **Mode**: OUTPUT
* **Pengujian**: Menghasilkan bunyi sesuai kondisi program.

#### 4️⃣ Rangkaian Potentiometer
https://github.com/user-attachments/assets/ca349e56-08a5-49bf-a4f9-87e6bb7d5769
* **Pin**: GPIO 1 (ADC1_0)
* **Mode**: INPUT ANALOG
* **Pengujian**: Nilai ADC tampil di serial monitor.

#### 5️⃣ Rangkaian OLED SSD1306
https://github.com/user-attachments/assets/2ab0e7ed-8a4b-4a6a-aafb-cbebeb8aeb74
* **Pin SDA/SCL**: GPIO 10 / GPIO 11
* **Protokol**: I2C
* **Pengujian**: Menampilkan teks.

#### 6️⃣ Rangkaian Motor Stepper + A4988
https://github.com/user-attachments/assets/d5495d0d-1419-4f0c-9fcb-b51a1677b185
* **Pin STEP/DIR**: GPIO 12 / GPIO 13
* **Pengujian**: Motor berputar sesuai perintah.

#### 7️⃣ Rangkaian Rotary Encoder
https://github.com/user-attachments/assets/72b634ee-a377-4d25-8c9f-97c18020b938
* **Pin CLK/DT/SW**: GPIO 2 / GPIO 3 / GPIO 4
* **Pengujian**: Menampilkan nilai posisi atau perubahan arah.

#### 8️⃣ Rangkaian Servo Motor
https://github.com/user-attachments/assets/6d602c24-783f-45cb-95dd-d6efa239d204
* **Pin**: GPIO 9
* **Pengujian**: Servo bergerak ke posisi sudut tertentu.

---
