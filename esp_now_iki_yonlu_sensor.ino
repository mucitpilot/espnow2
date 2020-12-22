//MUCİT PİLOT 2020
//ESP NOW fonksiyonu
//İki yönlü İletişim Örneği

//ESPNOW GELEN İSTEĞİ ALIP CEVAP VEREN CİHAZ KODU
//Bir cihazdan diğerine istek mesajı gönderiyoruz. İsteği alan diğer cihaz sensöründen okudğu verileri
//isteği yapan cihaza gönderiyor.


//gerekli kütüphanler ve linkleri

#include <ESP8266WiFi.h>//esp kartta mevcut yüklemeye gerek yok
#include <espnow.h>//esp kartta mevcut yüklemeye gerek yok

#include <Adafruit_Sensor.h>//https://github.com/adafruit/Adafruit_Sensor
#include <DHT.h>//https://github.com/adafruit/DHT-sensor-library
#include <Wire.h>//esp kartta mevcut yüklemeye gerek yok
#include <Adafruit_GFX.h>//https://github.com/adafruit/Adafruit-GFX-Library
#include <Adafruit_SSD1306.h>//https://github.com/adafruit/Adafruit_SSD1306

//OLED ekran ayarları
#define SCREEN_WIDTH 128 // OLED display genişlik piksel
#define SCREEN_HEIGHT 32 // OLED display yükseklik piksel

// I2C protokolü ile çalışan bir SSD1306 ekran nesnesi yaratıyoruz (SDA (D2), SCL (D1) pinlerine bağlanacak)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);//wire protokolü belirtir. -1 ise ekranda reset butonu olup olmadığını. -1 ise yok demek


//diğer kartın mac adresini girin 
uint8_t alici_macadresi[] = {0x98, 0xF4, 0xAB, 0xDA, 0xF0, 0x41}; 

// DHT sensör D5 pinine bağlı
#define DHTPIN D5    

// DHT tipini seçin
#define DHTTYPE    DHT11     // DHT 11
//#define DHTTYPE    DHT22     // DHT 22 (AM2302)
//#define DHTTYPE    DHT21     // DHT 21 (AM2301)

DHT dht(DHTPIN, DHTTYPE); //bir dht nesnesi oluşturuyoruz

// gönderilecek sıcaklık ve nem bilgisini saklayacağımız iki değişken tanımladık
float temperature;
float humidity;

//gerekli diğer değişkenler
bool alinanistekmesaji=false;

// Alıncak ve gönderilecek veri yapısı için structure tanımlama
// Diğer kartlar ile aynı yapı olmalı
typedef struct struct_message {
    bool istekgonder;
    float temp;
    float hum;
    bool cevap;
} struct_message;

// veri göndermek için yarattığımız veri yapısı
struct_message DHTverileri;

// gelen verileri almak için yarattığımız veri yapısı
struct_message gelenveriler;

// Veriler gönderildiğinde çalıştıracağımız fonksiyon
void VerilerGonderildiginde(uint8_t *mac_addr, uint8_t sendStatus) {
  Serial.print("Son verinin gonderilme durumu: ");
  if (sendStatus == 0){
    Serial.println("Veri Gonderme Başarılı");
  }
  else{
    Serial.println("Veriler Gönderilmedi!!!");
  }
}

// Veriler alındığında çalışacak fonksiyon
void VerilerAlindiginda(uint8_t * mac, uint8_t *incomingData, uint8_t len) {
  memcpy(&gelenveriler, incomingData, sizeof(gelenveriler));
  Serial.print("Alınan veri boyutu: ");
  Serial.println(len);
  alinanistekmesaji = gelenveriler.istekgonder;//diğer karttan istek gelmiş mi bakıyoruz

}

void sensorOku(){
  // Sıcaklığı oku
  temperature = dht.readTemperature();
  //veri okunamadıysa
  if (isnan(temperature)){
    Serial.println("DHT sensör sıcaklık verisi okunamadı!!!");
    temperature = 0.0;
  }
  Serial.print("Okunan Sıcaklık:");
  Serial.println(temperature);

  Serial.println(humidity);
  //veri okunamadıysa
  if (isnan(humidity)){
    Serial.println("DHT sensör nem verisi okunamadı!!!");
    humidity = 0.0;
  }
    
  Serial.print("Okunan Nem:");
  humidity = dht.readHumidity();
}

void gelenverileriyazdir(){
  // diğer karttan istek gelip gelmediği bilgisini serial ekrana yazdır
 
  Serial.println(" Gelen mesaj: ");
  Serial.println(alinanistekmesaji);

}
 
void setup() {
  
  Serial.begin(115200);

  // DHT sensörü başlat
  dht.begin();
 
  // Cihazı station olarak tanımlayıp bağlıysa ağlardan çıkıyoruz
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  // ESP-NOW'u başlatıyoruz
  if (esp_now_init() != 0) {
    Serial.println("ESP-NOW Başlatılamadı !!!");
    return;
  }

  // Bu kartın rolünü tanımlıyoruz. Çift yönlü iletişimde kartlar COMBO olmalı
  esp_now_set_self_role(ESP_NOW_ROLE_COMBO);

  // Yukarıda tanımladığımız ve veri gönderildiğinde çalışacak fonksiyonu atıyoruz
  esp_now_register_send_cb(VerilerGonderildiginde);
  
  // Bağlanılacak diğer karta bağlantıyı tanımlıyoruz
  esp_now_add_peer(alici_macadresi, ESP_NOW_ROLE_COMBO, 1, NULL, 0);
  
  // Yukarıda tanımladığımız ve veri alındığında çalışacak fonksiyonu atıyoruz
  esp_now_register_recv_cb(VerilerAlindiginda);

  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { //Eğer ekran başlatılamazsa hata mesajı verecek
    Serial.println(F("SSD1306 ekran hatası"));
    for(;;);//sonsuza kadar döner buradan çıkmaz
  }
  delay(2000);
  display.clearDisplay();//ekranı temizliyoruz.
  display.setTextColor(WHITE);//ekran yazı rengini beyaza ayarladık
}
 
void loop() {
  //DHT sensörden verileri okuyoruz.
  sensorOku();
  delay(100);
  //diğer karttan gelen verileri serial ekrana yazdır
  gelenverileriyazdir();
      
  //Okunan DHT verilerini OLED ekrana yazdır
      // sıcaklık verisinin ekrana yazdırılması
      display.clearDisplay();
      display.setTextSize(1);//metin boyutu en küçük
      display.setCursor(0,0);//cursor'u pozisyona konumlandırdık
      display.print("Sicaklik: ");
      display.setTextSize(1);
      display.setCursor(65,0);
      display.print(temperature);
      display.print(" ");
      display.setTextSize(1);
      display.cp437(true);//yazı karakterleri dışındaki sembolleri yazdırmak için cp437 (DOS) tablosunu kullanıyoruz
      display.write(167);//derece işaretinin karşılığı
      display.setTextSize(1);
      display.print("C");
  
       // rutubeti yazdırıyoruz
      display.setTextSize(1);
      display.setCursor(0, 20);
      display.print("Rutubet: ");
      display.setTextSize(1);
      display.setCursor(65, 20);
      display.print(humidity);
      display.print(" %"); 

      display.display(); 

  //eğer bir istek mesajı alınmış ise    
  if (alinanistekmesaji) {
   
    //gönderilmek üzere verileri ayarla
    DHTverileri.temp = temperature;
    DHTverileri.hum = humidity;
    DHTverileri.cevap=1;
    // hazırlanan verileri alıcıya gönder
    esp_now_send(alici_macadresi, (uint8_t *) &DHTverileri, sizeof(DHTverileri));
    //OLED ekrana 2sn süre ile veriler gönderildi yazdır 
      display.clearDisplay();
      display.setTextSize(1);//metin boyutu en küçük
      display.setCursor(0,0);//cursor'u pozisyona konumlandırdık
      display.print("Veriler Gonderildi.");
      display.display(); 
      delay(2000);
   //tekrar kullanmak üzere alinanistekmesaji değişkenini sıfırla   
   alinanistekmesaji=false;
    
  }
}
