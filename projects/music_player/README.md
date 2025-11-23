# 🧠 Projects — Music Player (Multicore, Queue, Mutex)

---

## 🎯 Tujuan

1. Menjalankan dua **task** terpisah pada **dua core** ESP32-S3: satu untuk pemutaran musik (PlayerTask) dan satu untuk tampilan OLED (OledTask).
2. Menggunakan **ISR** untuk tombol (dua tombol) dan mengirim perintah ke PlayerTask lewat **Queue**.
3. Melindungi akses I2C/OLED dengan **Semaphore (mutex)**.

---

## ⚙️ Perangkat yang Digunakan

| No |                Peripheral |                             Fungsi |  Jenis |                   Core Digunakan |
| -- | ------------------------: | ---------------------------------: | -----: | -------------------------------: |
| 1  | OLED SSD1306 (I2C 128×64) |    Menampilkan status (song/state) | Output |                           Core 0 |
| 2  |                    Buzzer |         Menghasilkan nada / melodi | Output |                           Core 1 |
| 3  |    Button PLAY (BTN_PLAY) | ISR → kirim CMD_PLAYPAUSE ke queue |  Input | ISR (any) → PlayerTask di Core 1 |
| 4  |    Button NEXT (BTN_NEXT) |      ISR → kirim CMD_NEXT ke queue |  Input | ISR (any) → PlayerTask di Core 1 |

---

## 🔌 Pin Mapping

* `BTN_PLAY`  → GPIO 4  (INPUT_PULLUP)
* `BTN_NEXT`  → GPIO 5  (INPUT_PULLUP)
* `BUZZ_PIN`  → GPIO 8  (OUTPUT / tone)
* `SDA_PIN`   → GPIO 10 (I2C SDA)
* `SCL_PIN`   → GPIO 11 (I2C SCL)

---

## 🧩 Ringkasan Arsitektur Perangkat Lunak

* **RTOS primitives** dibuat pada `setup()`:

  * `cmdQ` (Queue) — untuk menerima perintah dari ISR.
  * `i2cMtx` (Semaphore/MUTEX) — untuk melindungi akses I2C/OLED.
* **ISR**:

  * `isrPlay()` mengirim `CMD_PLAYPAUSE` ke queue dari ISR (minimal work in ISR).
  * `isrNext()` mengirim `CMD_NEXT` ke queue dari ISR.
* **PlayerTask (pinned to Core 1)** — bertanggung jawab memutar lagu: membaca `songs[]`, memanggil `playTone(f,d)`, mengecek queue non-blok untuk perintah penghentian atau lompat lagu.
* **OledTask (pinned to Core 0)** — inisialisasi I2C dan OLED, mengambil `i2cMtx` sebelum menggambar layar, menampilkan `currentSong` dan `State` (PLAY / PAUSE).
* **Tone handling** — fungsi `playTone(int freq,int dur)` memakai `tone()` dan `noTone()` sederhana; bila frekuensi 0 → jeda.

---

## 📁 Struktur Proyek

```
.pio/
.vscode/
/include/
/lib/
/src/main.cpp   ← sketch utama (kode program)
wokwi.toml      ← konfigurasi simulasi
diagram.json    ← konfigurasi wiring
platformio.ini  ← konfigurasi board & lib (PlatformIO)
README.md       ← instruksi & dokumentasi percobaan
```

---

## ⚙️ Langkah Build & Upload (PlatformIO)

1. Buka VSCode → PlatformIO → Open Project (folder proyek).
2. Pastikan `platformio.ini` mengarah ke board `esp32-s3-devkitc-1` dan menyertakan library Adafruit SSD1306 & GFX. Contoh dependency:

```ini
[env:esp32-s3-devkitc-1]
platform = espressif32
board = esp32-s3-devkitc-1
framework = arduino
monitor_speed = 115200
lib_deps =
  adafruit/Adafruit SSD1306@^2.5.7
  adafruit/Adafruit GFX Library@^1.11.4
```

3. Build (`PlatformIO: Build`) lalu Upload (`PlatformIO: Upload`).
4. Buka Serial Monitor untuk debugging (115200).

> Untuk simulasi: konfigurasi `wokwi.toml` dan `diagram.json` sesuai pinmap di atas lalu jalankan simulasi Wokwi.

---

## 🧪 Hasil Pengujian

* Tekan **PLAY** → tombol memicu ISR `isrPlay()` → perintah dikirim ke queue → PlayerTask mulai memutar melodi; OLED berubah menampilkan `State: PLAY`.
* Tekan **PLAY** saat sedang memutar → mengirim `CMD_PLAYPAUSE` → PlayerTask menghentikan pemutaran; OLED menampilkan `State: PAUSE`.
* Tekan **NEXT** → kirim `CMD_NEXT` → PlayerTask berpindah ke lagu berikutnya.
* Buzzer mengeluarkan nada sesuai data `songs[]` sampai terima perintah pause/next.
* OLED selalu diakses dengan mutex untuk menghindari kondisi balapan I2C.

---

https://github.com/user-attachments/assets/5c9a5541-f502-4456-9705-cc73caaf3b6f
