/*  Sensor Gerak PIR HC-SR501 ESP32 CAM Menggunakan Telegram
    Create By ASEP SUCIPTO INDRA SUKMA

    GITHUB: https://github.com/aurroracell/Sensor-Gerak-PIR-HC-SR501-ESP32-CAM
*/

/* ======================================== Memasukan Library. */
#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include "esp_camera.h"
#include <UniversalTelegramBot.h>
#include <ArduinoJson.h>
#include <EEPROM.h>
/* ======================================== */

/* ======================================== Variabel untuk jaringan. */
// GANTI DENGAN KREDENSIAL JARINGAN ANDA
const char* ssid = "NAMA WIFI ANDA"; //--> Masukkan SSID / nama jaringan WiFi Anda.
const char* password = "PASSWORD WIFI ANDA"; //--> Masukkan kata sandi WiFi Anda.
/* ======================================== */

/* ======================================== Variabel untuk token bot telegram. */
String BOTtoken = "123456789:KODE TOKEN BOT ANDA";  //--> Token Bot Anda (Dapatkan dari Botfather).
/* ======================================== */

/* ======================================== @myidbot ID */
// Gunakan @myidbot untuk mengetahui ID chat individu atau grup.
// Perhatikan juga bahwa Anda perlu mengklik "mulai" pada bot sebelum bot dapat mengirimi Anda pesan.
String CHAT_ID = "123456789"; // ID chat Anda pada Telegram
/* ======================================== */

/* ======================================== Inisialisasi WiFiClientSecure. */
WiFiClientSecure clientTCP;
/* ======================================== */

/* ======================================== Inisialisasi UniversalTelegramBot. */
UniversalTelegramBot bot(BOTtoken, clientTCP);
/* ======================================== */

/* ======================================== Mendefinisikan GPIO kamera pada ESP32 Cam. */
// KAMERA_MODEL_AI_THINKER
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27

#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22
/* ======================================== */

/* ======================================== Mendefinisikan TINGGI dan RENDAH dengan ON dan OFF (untuk LED FLash). */
#define ON HIGH
#define OFF LOW
/* ======================================== */

#define FLASH_LED_PIN   4           //--> LED Flash PIN (GPIO 4)
#define PIR_SENSOR_PIN  13          //--> PIR SENSOR PIN (GPIO 12)

#define EEPROM_SIZE     2           //--> Tentukan jumlah byte yang ingin Anda akses

/* ======================================== Variabel untuk millis (Memeriksa pesan baru). */
// Memeriksa pesan baru setiap 1 detik (1000 ms).
int botRequestDelay = 1000;
unsigned long lastTimeBotRan;
/* ======================================== */

/* ======================================== Variabel untuk millis (untuk menstabilkan Sensor PIR). */
int countdown_interval_to_stabilize_PIR_Sensor = 1000;
unsigned long lastTime_countdown_Ran;
byte countdown_to_stabilize_PIR_Sensor = 30;
/* ======================================== */

bool sendPhoto = false;             //--> Variabel untuk pemicu pengiriman foto.

bool PIR_Sensor_is_stable = false;  //--> Variabel yang menyatakan waktu stabilisasi sensor PIR telah selesai.

bool boolPIRState = false;

/* ________________________________________________________________________________ String berfungsi untuk mengurai String */
// Fungsi ini digunakan untuk mendapatkan status pengiriman foto di json.
String getValue(String data, char separator, int index) {
  int found = 0;
  int strIndex[] = { 0, -1 };
  int maxIndex = data.length() - 1;
  
  for (int i = 0; i <= maxIndex && found <= index; i++) {
    if (data.charAt(i) == separator || i == maxIndex) {
      found++;
      strIndex[0] = strIndex[1] + 1;
      strIndex[1] = (i == maxIndex) ? i+1 : i;
    }
  }
  return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}
/* ________________________________________________________________________________ */

/* ________________________________________________________________________________ Subrutin pengiriman pesan feedback ketika foto berhasil atau gagal terkirim ke Telegram. */
void FB_MSG_is_photo_send_successfully (bool state) {
  String send_feedback_message = "";
  if(state == false) {
    send_feedback_message += "From the ESP32-CAM :\n\n";
    send_feedback_message += "ESP32-CAM failed to send photo.\n";
    send_feedback_message += "Suggestion :\n";
    send_feedback_message += "- Please try again.\n";
    send_feedback_message += "- Reset ESP32-CAM.\n";
    send_feedback_message += "- Change FRAMESIZE (see Drop down frame size in void configInitCamera).\n";
    Serial.print(send_feedback_message);
    send_feedback_message += "\n\n";
    send_feedback_message += "/start : to see all commands.";
    bot.sendMessage(CHAT_ID, send_feedback_message, "");
  } else {
    if(boolPIRState == true) {
      Serial.println("Successfully sent photo.");
      send_feedback_message += "From the ESP32-CAM :\n\n";
      send_feedback_message += "The PIR sensor detects objects and movements.\n";
      send_feedback_message += "Photo sent successfully.\n\n";
      send_feedback_message += "/start : to see all commands.";
      bot.sendMessage(CHAT_ID, send_feedback_message, ""); 
    }
    if(sendPhoto == true) {
      Serial.println("Successfully sent photo.");
      send_feedback_message += "From the ESP32-CAM :\n\n";
      send_feedback_message += "Photo sent successfully.\n\n";
      send_feedback_message += "/start : to see all commands.";
      bot.sendMessage(CHAT_ID, send_feedback_message, ""); 
    }
  }
}
/* ________________________________________________________________________________ */

/* ________________________________________________________________________________ Berfungsi untuk membaca nilai sensor PIR (HIGH/1 OR LOW/0) */
bool PIR_State() {
  bool PRS = digitalRead(PIR_SENSOR_PIN);
  return PRS;
}
/* ________________________________________________________________________________ */

/* ________________________________________________________________________________ Subrutin untuk menghidupkan atau mematikan LED Flash. */
void LEDFlash_State(bool ledState) {
  digitalWrite(FLASH_LED_PIN, ledState);
}
/* ________________________________________________________________________________ */

/* ________________________________________________________________________________ Subrutin untuk mengatur dan menyimpan pengaturan di EEPROM untuk mode "mengambil foto dengan LED Flash". */
void enable_capture_Photo_With_Flash(bool state) {
  EEPROM.write(0, state);
  EEPROM.commit();
  delay(50);
}
/* ________________________________________________________________________________ */

/* ________________________________________________________________________________ Berfungsi untuk membaca pengaturan di EEPROM untuk mode "mengambil foto dengan LED Flash". */
bool capture_Photo_With_Flash_state() {
  bool capture_Photo_With_Flash = EEPROM.read(0);
  return capture_Photo_With_Flash;
}
/* ________________________________________________________________________________ */

/* ________________________________________________________________________________ Subrutin untuk mengatur dan menyimpan pengaturan di EEPROM untuk mode "mengambil foto dengan Sensor PIR". */
void enable_capture_Photo_with_PIR(bool state) {
  EEPROM.write(1, state);
  EEPROM.commit();
  delay(50);
}
/* ________________________________________________________________________________ */

/* ________________________________________________________________________________ Berfungsi untuk membaca pengaturan di EEPROM untuk mode "mengambil foto dengan Sensor PIR".*/
bool capture_Photo_with_PIR_state() {
  bool capture_Photo_with_PIR = EEPROM.read(1);
  return capture_Photo_with_PIR;
}
/* ________________________________________________________________________________ */

/* ________________________________________________________________________________ Subrutin untuk konfigurasi kamera. */
void configInitCamera(){
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;

  /* ---------------------------------------- init dengan spesifikasi tinggi untuk mengalokasikan buffer yang lebih besar terlebih dahulu. */
  if(psramFound()){
    config.frame_size = FRAMESIZE_UXGA; //--> FRAMESIZE_ + UXGA|SXGA|XGA|SVGA|VGA|CIF|QVGA|HQVGA|QQVGA
    config.jpeg_quality = 10;  
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 12;  
    config.fb_count = 1;
  }
  /* ---------------------------------------- */

  /* ---------------------------------------- camera init. */
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    Serial.println();
    Serial.println("Restart ESP32 Cam");
    delay(1000);
    ESP.restart();
  }
  /* ---------------------------------------- */

  /* ---------------------------------------- Drop down frame size untuk kecepatan frame awal yang lebih tinggi (Tetapkan ukuran dan kualitas frame di sini) */
  /*
   *Jika foto yang dikirim oleh ESP32-CAM rusak atau ESP32-CAM gagal mengirimkan foto, untuk mengatasinya ikuti langkah-langkah di bawah ini:
   * - Pengaturan UKURAN BINGKAI:
   * > Ubah "s->set_framesize(s, FRAMESIZE_UXGA);" untuk menurunkan ukuran bingkai, seperti FRAMESIZE_VGA, FRAMESIZE_CIF dan seterusnya.
   *
   *Jika ukuran frame sudah diperkecil namun foto yang dikirim melalui ESP32-CAM masih rusak atau ESP32-CAM masih gagal mengirimkan foto,
   * lalu ubah setting "s->set_quality(s, 30);".
   * - set_pengaturan kualitas :
   * > Kualitas gambar (set_quality) dapat berupa angka antara 0 dan 63.
   * > Angka yang lebih tinggi berarti kualitas yang lebih rendah.
   * > Angka yang lebih rendah berarti kualitas yang lebih tinggi.
   * > Angka kualitas gambar yang sangat rendah, terutama pada resolusi yang lebih tinggi dapat menyebabkan ESP32-CAM mogok atau tidak mengambil foto dengan benar.
   * > Jika GAMBAR YANG DITERIMA RUSAK ATAU FOTO GAGAL, ​​coba gunakan nilai yang lebih besar pada "s->set_quality(s, 30);", misalnya 25, 30 dan seterusnya hingga 63.
   *
   * Di ESP32-CAM saya, jika menggunakan "FRAMESIZE_UXGA", nilai set_quality adalah 30.
   *Setelah saya uji, settingan di atas cukup stabil untuk pengambilan foto di dalam ruangan, di luar ruangan, dalam kondisi dengan kualitas pencahayaan yang baik maupun dalam kondisi cahaya yang buruk.
   */

  /*
   * UXGA   = 1600 x 1200 pixels
   * SXGA   = 1280 x 1024 pixels
   * XGA    = 1024 x 768  pixels
   * SVGA   = 800 x 600   pixels
   * VGA    = 640 x 480   pixels
   * CIF    = 352 x 288   pixels
   * QVGA   = 320 x 240   pixels
   * HQVGA  = 240 x 160   pixels
   * QQVGA  = 160 x 120   pixels
   */
  sensor_t * s = esp_camera_sensor_get();
  s->set_framesize(s, FRAMESIZE_SXGA);  //--> FRAMESIZE_ + UXGA|SXGA|XGA|SVGA|VGA|CIF|QVGA|HQVGA|QQVGA
  /* ---------------------------------------- */
}
/* ________________________________________________________________________________ */

/* ________________________________________________________________________________ Subrutin untuk menangani apa yang harus dilakukan setelah pesan baru tiba. */
void handleNewMessages(int numNewMessages) {
  Serial.print("Handle New Messages: ");
  Serial.println(numNewMessages);

  /* ---------------------------------------- "For Loop" untuk memeriksa isi pesan yang baru diterima. */
  for (int i = 0; i < numNewMessages; i++) {
    /* ::::::::::::::::: Cek ID (ID didapat dari IDBot/@myidbot). */
    /*
     * Jika chat_id berbeda dengan ID chat Anda (CHAT_ID), berarti ada seseorang (bukan Anda) yang mengirimkan pesan ke bot Anda.
     * Jika demikian, abaikan pesan tersebut dan tunggu pesan berikutnya.
     */
    String chat_id = String(bot.messages[i].chat_id);
    if (chat_id != CHAT_ID){
      bot.sendMessage(chat_id, "Unauthorized user", "");
      Serial.println("Unauthorized user");
      Serial.println("------------");
      continue;
    }
    /* ::::::::::::::::: */

    /* ::::::::::::::::: Print the received message. */
    String text = bot.messages[i].text;
    Serial.println(text);
    /* ::::::::::::::::: */

    /* ::::::::::::::::: Cek kondisi berdasarkan perintah yang dikirimkan dari BOT telegram anda. */
    // Jika menerima pesan "/start", kami akan mengirimkan perintah yang valid untuk mengontrol ESP. Ini berguna jika Anda lupa perintah apa untuk mengontrol board Anda.
    String send_feedback_message = "";
    String from_name = bot.messages[i].from_name;
    if (text == "/start") {
      send_feedback_message += "From the ESP32-CAM :\n\n";
      send_feedback_message += "Welcome , " + from_name + "\n";
      send_feedback_message += "Use the following commands to interact with the ESP32-CAM.\n\n";
      send_feedback_message += "/capture_photo : takes a new photo\n\n";
      send_feedback_message += "Settings :\n";
      send_feedback_message += "/enable_capture_Photo_With_Flash : takes a new photo with LED FLash\n";
      send_feedback_message += "/disable_capture_Photo_With_Flash : takes a new photo without LED FLash\n";
      send_feedback_message += "/enable_capture_Photo_with_PIR : takes a new photo with PIR Sensor\n";
      send_feedback_message += "/disable_capture_Photo_with_PIR : takes a new photo without PIR Sensor\n\n";
      send_feedback_message += "Setting status :\n";
      if(capture_Photo_With_Flash_state() == ON) {
        send_feedback_message += "- Capture Photo With Flash = ON\n";
      }
      if(capture_Photo_With_Flash_state() == OFF) {
        send_feedback_message += "- Capture Photo With Flash = OFF\n";
      }
      if(capture_Photo_with_PIR_state() == ON) {
        send_feedback_message += "- Capture Photo With PIR = ON\n";
      }
      if(capture_Photo_with_PIR_state() == OFF) {
        send_feedback_message += "- Capture Photo With PIR = OFF\n";
      }
      if(PIR_Sensor_is_stable == false) {
        send_feedback_message += "\nPIR Sensor Status:\n";
        send_feedback_message += "The PIR sensor is being stabilized.\n";
        send_feedback_message += "Stabilization time is " + String(countdown_to_stabilize_PIR_Sensor) + " seconds away. Please wait.\n";
      }
      bot.sendMessage(CHAT_ID, send_feedback_message, "");
      Serial.println("------------");
    }
    
    // The condition if the command received is "/capture_photo".
    if (text == "/capture_photo") {
      sendPhoto = true;
      Serial.println("New photo request");
    }
    
    // Syarat jika perintah yang diterima adalah "/enable_capture_Photo_With_Flash".
    if (text == "/enable_capture_Photo_With_Flash") {
      enable_capture_Photo_With_Flash(ON);
      send_feedback_message += "From the ESP32-CAM :\n\n";
      if(capture_Photo_With_Flash_state() == ON) {
        Serial.println("Capture Photo With Flash = ON");
        send_feedback_message += "Capture Photo With Flash = ON\n\n";
      } else {
        Serial.println("Failed to set. Try again.");
        send_feedback_message += "Failed to set. Try again.\n\n"; 
      }
      Serial.println("------------");
      send_feedback_message += "/start : to see all commands.";
      bot.sendMessage(CHAT_ID, send_feedback_message, "");
    }
    
    // Syarat jika perintah yang diterima adalah “/disable_capture_Photo_With_Flash”.
    if (text == "/disable_capture_Photo_With_Flash") {
      enable_capture_Photo_With_Flash(OFF);
      send_feedback_message += "From the ESP32-CAM :\n\n";
      if(capture_Photo_With_Flash_state() == OFF) {
        Serial.println("Capture Photo With Flash = OFF");
        send_feedback_message += "Capture Photo With Flash = OFF\n\n";
      } else {
        Serial.println("Failed to set. Try again.");
        send_feedback_message += "Failed to set. Try again.\n\n"; 
      }
      Serial.println("------------");
      send_feedback_message += "/start : to see all commands.";
      bot.sendMessage(CHAT_ID, send_feedback_message, "");
    }

    // Syarat jika perintah diterima adalah "/enable_capture_Photo_with_PIR".
    if (text == "/enable_capture_Photo_with_PIR") {
      enable_capture_Photo_with_PIR(ON);
      send_feedback_message += "From the ESP32-CAM :\n\n";
      if(capture_Photo_with_PIR_state() == ON) {
        Serial.println("Capture Photo With PIR = ON");
        send_feedback_message += "Capture Photo With PIR = ON\n\n";
        botRequestDelay = 2000;
      } else {
        Serial.println("Failed to set. Try again.");
        send_feedback_message += "Failed to set. Try again.\n\n"; 
      }
      Serial.println("------------");
      send_feedback_message += "/start : to see all commands.";
      bot.sendMessage(CHAT_ID, send_feedback_message, "");
    }

    // Syarat jika perintah diterima adalah “/disable_capture_Photo_with_PIR”.
    if (text == "/disable_capture_Photo_with_PIR") {
      enable_capture_Photo_with_PIR(OFF);
      send_feedback_message += "From the ESP32-CAM :\n\n";
      if(capture_Photo_with_PIR_state() == OFF) {
        Serial.println("Capture Photo With PIR = OFF");
        send_feedback_message += "Capture Photo With PIR = OFF\n\n";
        botRequestDelay = 1000;
      } else {
        Serial.println("Failed to set. Try again.");
        send_feedback_message += "Failed to set. Try again.\n\n"; 
      }
      Serial.println("------------");
      send_feedback_message += "/start : to see all commands.";
      bot.sendMessage(CHAT_ID, send_feedback_message, "");
    }
    /* ::::::::::::::::: */
  }
  /* ---------------------------------------- */
}
/* ________________________________________________________________________________ */

/* ________________________________________________________________________________ Subrutin untuk proses pengambilan dan pengiriman foto. */
String sendPhotoTelegram() {
  const char* myDomain = "api.telegram.org";
  String getAll = "";
  String getBody = "";

  /* ---------------------------------------- Proses pengambilan foto. */
  Serial.println("Taking a photo...");

  /* ::::::::::::::::: Menyalakan LED FLash jika pengaturannya adalah "enable_capture_Photo_With_Flash(ON);". */
  if(capture_Photo_With_Flash_state() == ON) {
    LEDFlash_State(ON);
  }
  delay(1500);
  /* ::::::::::::::::: */
  
  /* ::::::::::::::::: Mengambil foto. */ 
  camera_fb_t * fb = NULL;
  fb = esp_camera_fb_get();  
  if(!fb) {
    Serial.println("Camera capture failed");
    Serial.println("Restart ESP32 Cam");
    delay(1000);
    ESP.restart();
    return "Camera capture failed";
  }  
  /* ::::::::::::::::: */

  /* ::::::::::::::::: Matikan LED Flash setelah berhasil mengambil foto. */
  if(capture_Photo_With_Flash_state() == ON) {
    LEDFlash_State(OFF);
  }
  /* ::::::::::::::::: */
  Serial.println("Successful photo taking.");
  /* ---------------------------------------- */
  

  /* ---------------------------------------- Proses pengiriman foto. */
  Serial.println("Connect to " + String(myDomain));

  if (clientTCP.connect(myDomain, 443)) {
    Serial.println("Connection successful");
    Serial.print("Send photos");
    
    String head = "--Esp32Cam\r\nContent-Disposition: form-data; name=\"chat_id\"; \r\n\r\n";
    head += CHAT_ID; 
    head += "\r\n--Esp32Cam\r\nContent-Disposition: form-data; name=\"photo\"; filename=\"esp32-cam.jpg\"\r\nContent-Type: image/jpeg\r\n\r\n";
    String tail = "\r\n--Esp32Cam--\r\n";

    /* ::::::::::::::::: Jika Anda hanya menggunakan framesize rendah, seperti CIF, QVGA, HQVGA dan QQVGA, maka gunakan variabel di bawah ini untuk menghemat lebih banyak memori. */
    //uint16_t imageLen = fb->len;
    //uint16_t extraLen = head.length() + tail.length();
    //uint16_t totalLen = imageLen + extraLen;
    /* ::::::::::::::::: */

    uint32_t imageLen = fb->len;
    uint32_t extraLen = head.length() + tail.length();
    uint32_t totalLen = imageLen + extraLen;
  
    clientTCP.println("POST /bot"+BOTtoken+"/sendPhoto HTTP/1.1");
    clientTCP.println("Host: " + String(myDomain));
    clientTCP.println("Content-Length: " + String(totalLen));
    clientTCP.println("Content-Type: multipart/form-data; boundary=Esp32Cam");
    clientTCP.println();
    clientTCP.print(head);
  
    uint8_t *fbBuf = fb->buf;
    size_t fbLen = fb->len;
    
    for (size_t n=0;n<fbLen;n=n+1024) {
      if (n+1024<fbLen) {
        clientTCP.write(fbBuf, 1024);
        fbBuf += 1024;
      }
      else if (fbLen%1024>0) {
        size_t remainder = fbLen%1024;
        clientTCP.write(fbBuf, remainder);
      }
    }  
    
    clientTCP.print(tail);
    
    esp_camera_fb_return(fb);
    
    int waitTime = 10000;   //--> batas waktu 10 detik (Untuk mengirim foto.)
    long startTimer = millis();
    boolean state = false;
    
    while ((startTimer + waitTime) > millis()){
      Serial.print(".");
      
      while (clientTCP.available()) {
        char c = clientTCP.read();
        if (state==true) getBody += String(c);        
        if (c == '\n') {
          if (getAll.length()==0) state=true; 
          getAll = "";
        } 
        else if (c != '\r')
          getAll += String(c);
        startTimer = millis();
      }
      if (getBody.length()>0) break;
    }
    clientTCP.stop();
    Serial.println(getBody);

    /* ::::::::::::::::: Syarat untuk mengecek apakah foto berhasil dikirim atau gagal. */
    // Jika foto berhasil atau gagal terkirim, pesan feedback akan dikirimkan ke Telegram.
    if(getBody.length() > 0) {
      String send_status = "";
      send_status = getValue(getBody, ',', 0);
      send_status = send_status.substring(6);
      
      if(send_status == "true") {
        FB_MSG_is_photo_send_successfully(true);  //--> Foto berhasil terkirim dan terkirim pesan informasi bahwa foto berhasil terkirim ke telegram.
      }
      if(send_status == "false") {
        FB_MSG_is_photo_send_successfully(false); //--> Foto gagal terkirim dan mengirimkan pesan informasi bahwa foto gagal terkirim ke telegram.
      }
    }
    if(getBody.length() == 0) FB_MSG_is_photo_send_successfully(false); //--> Foto gagal terkirim dan mengirimkan pesan informasi bahwa foto gagal terkirim ke telegram.
    /* ::::::::::::::::: */
  }
  else {
    getBody="Connected to api.telegram.org failed.";
    Serial.println("Connected to api.telegram.org failed.");
  }
  Serial.println("------------");
  return getBody;
  /* ---------------------------------------- */
}
/* ________________________________________________________________________________ */

/* ________________________________________________________________________________ VOID SETTUP() */
void setup(){
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); //--> Nonaktifkan detektor brownout.

   /* ---------------------------------------- Kecepatan komunikasi serial init (baud rate). */
  Serial.begin(115200);
  delay(1000);
  /* ---------------------------------------- */

  Serial.println();
  Serial.println();
  Serial.println("------------");

  /* ---------------------------------------- Memulai EEPROM, menulis dan membaca pengaturan yang disimpan di EEPROM. */
  EEPROM.begin(EEPROM_SIZE);

  /* ::::::::::::::::: Menulis pengaturan ke EEPROM. */
  /*
   * Aktifkan baris kode di bawah ini untuk 1 kali saja.
   * Setelah anda mengupload kode, lalu "beri komentar" pada baris kode di bawah ini, lalu upload kembali kode tersebut.
   */
  enable_capture_Photo_With_Flash(OFF);
  enable_capture_Photo_with_PIR(OFF);
  delay(500);
  /* ::::::::::::::::: */

  Serial.println("Setting status :");
  if(capture_Photo_With_Flash_state() == ON) {
    Serial.println("- Capture Photo With Flash = ON");
  }
  if(capture_Photo_With_Flash_state() == OFF) {
    Serial.println("- Capture Photo With Flash = OFF");
  }
  if(capture_Photo_with_PIR_state() == ON) {
    Serial.println("- Capture Photo With PIR = ON");
    botRequestDelay = 2000;
  }
  if(capture_Photo_with_PIR_state() == OFF) {
    Serial.println("- Capture Photo With PIR = OFF");
    botRequestDelay = 1000;
  }
  /* ---------------------------------------- */

  /* ---------------------------------------- Atur LED Flash sebagai output dan matikan status awal LED Flash. */
  pinMode(FLASH_LED_PIN, OUTPUT);
  LEDFlash_State(OFF);
  /* ---------------------------------------- */

  /* ---------------------------------------- Konfigurasi dan inisiasi kamera. */
  Serial.println();
  Serial.println("Start configuring and initializing the camera...");
  configInitCamera();
  Serial.println("Successfully configure and initialize the camera.");
  Serial.println();
  /* ---------------------------------------- */

  /* ---------------------------------------- Hubungkan ke Wi-Fi. */
  WiFi.mode(WIFI_STA);
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  clientTCP.setCACert(TELEGRAM_CERTIFICATE_ROOT); //--> Tambahkan sertifikat root untuk api.telegram.org

  /* ::::::::::::::::: Proses menghubungkan ESP32 CAM dengan WiFi Hotspot / WiFi Router. */
  /*
   * Batas waktu proses menghubungkan ESP32 CAM dengan WiFi Hotspot / WiFi Router adalah 20 detik.
   * Jika dalam waktu 20 detik CAM ESP32 belum berhasil tersambung ke WiFi, maka CAM ESP32 akan restart.
   * Kondisi ini saya buat karena di ESP32-CAM saya ada kalanya seperti tidak bisa konek ke WiFi sehingga perlu direstart agar bisa terkoneksi ke WiFi.
   */
  int connecting_process_timed_out = 20; //--> 20 = 20 seconds.
  connecting_process_timed_out = connecting_process_timed_out * 2;
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    LEDFlash_State(ON);
    delay(250);
    LEDFlash_State(OFF);
    delay(250);
    if(connecting_process_timed_out > 0) connecting_process_timed_out--;
    if(connecting_process_timed_out == 0) {
      delay(1000);
      ESP.restart();
    }
  }
  /* ::::::::::::::::: */
  
  LEDFlash_State(OFF);
  Serial.println();
  Serial.print("Successfully connected to ");
  Serial.println(ssid);
  Serial.print("ESP32-CAM IP Address: ");
  Serial.println(WiFi.localIP());
  Serial.println();
  Serial.println("The PIR sensor is being stabilized.");
  Serial.printf("Stabilization time is %d seconds away. Please wait.\n", countdown_to_stabilize_PIR_Sensor);
  
  Serial.println("------------");
  Serial.println();
  /* ---------------------------------------- */
}
/* ________________________________________________________________________________ */

/* ________________________________________________________________________________ VOID LOOP() */
void loop() {
  /* ---------------------------------------- Ketentuan pengambilan dan pengiriman foto. */
  if(sendPhoto) {
    Serial.println("Preparing photo...");
    sendPhotoTelegram(); 
    sendPhoto = false; 
  }
  /* ---------------------------------------- */

  /* ---------------------------------------- Kondisi untuk memeriksa apakah ada pesan baru yang masuk. */
  /*
   * Memeriksa pesan baru setiap 1 detik (lihat variabel "botRequestDelay") jika pengambilan foto dengan PIR tidak diaktifkan/MATI.
   * 
   * Jika pengambilan foto dengan PIR aktif, pesan baru akan diperiksa setiap 20 detik. Saya membuat kondisi ini karena
   * Pengecekan pesan baru membutuhkan waktu sekitar 2-4 detik (tergantung kecepatan jaringan internet) dan dapat menunda proses pembacaan sensor PIR.
   * Jadi dengan memeriksa pesan baru setiap 20 detik, pembacaan sensor PIR dapat diprioritaskan.
   */
  if(millis() > lastTimeBotRan + botRequestDelay) {
    int numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    while (numNewMessages) {
      Serial.println();
      Serial.println("------------");
      Serial.println("got response");
      handleNewMessages(numNewMessages);
      numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    }
    lastTimeBotRan = millis();
  }
  /* ---------------------------------------- */
  
  /* ---------------------------------------- Kondisi untuk menstabilkan Sensor PIR. */
  /*
   * Kondisi ini tercipta karena pada saat sensor PIR dihidupkan memerlukan waktu stabilisasi sekitar 30-60 detik,
   * sehingga dapat mendeteksi dengan benar atau mendeteksi dengan sedikit noise.
   *
   * Dengan kondisi ini, sensor hanya dapat bekerja setelah waktu stabilisasi selesai.
   *
   * Waktu stabilisasi yang digunakan dalam proyek ini adalah 30 detik (Waktu 30 detik mungkin tidak akurat karena terganggu oleh proses lain).
   * Anda dapat mengubahnya dengan mengubah nilai variabel "countdown_to_stabilize_PIR_Sensor".
   */
  if(PIR_Sensor_is_stable == false) {
    if(millis() > lastTime_countdown_Ran + countdown_interval_to_stabilize_PIR_Sensor) {
      if(countdown_to_stabilize_PIR_Sensor > 0) countdown_to_stabilize_PIR_Sensor--;
      if(countdown_to_stabilize_PIR_Sensor == 0) {
        PIR_Sensor_is_stable = true;
        Serial.println();
        Serial.println("------------");
        Serial.println("The PIR Sensor stabilization time is complete.");
        Serial.println("The PIR sensor can already work.");
        Serial.println("------------");
        String send_Status_PIR_Sensor = "";
        send_Status_PIR_Sensor += "From the ESP32-CAM :\n\n";
        send_Status_PIR_Sensor += "The PIR Sensor stabilization time is complete.\n";
        send_Status_PIR_Sensor += "The PIR sensor can already work.";
        bot.sendMessage(CHAT_ID, send_Status_PIR_Sensor, "");
      }
      lastTime_countdown_Ran = millis();
    }
  }
  /* ---------------------------------------- */

  /* ---------------------------------------- Kondisi untuk membaca sensor PIR dan menjalankan perintahnya. */
  /*
   * "capture_Photo_with_PIR_state()" harus "ON" untuk mengambil foto dengan sensor PIR.
   * Anda dapat mengaturnya dari pesan/perintah Telegram.
   */
  if(capture_Photo_with_PIR_state() == ON) {
    if(PIR_State() == true && PIR_Sensor_is_stable == true) {
      Serial.println("------------");
      Serial.println("The PIR sensor detects objects and movements.");
      boolPIRState = true;
      sendPhotoTelegram();
      boolPIRState = false;
    }
  }
  /* ---------------------------------------- */
}
/* ________________________________________________________________________________ */
//<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<