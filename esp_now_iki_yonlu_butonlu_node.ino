//MUCİT PİLOT 2020
//ESP NOW fonksiyonu
//İki yönlü İletişim Örneği

//ESPNOW İSTEK YAPAN CİHAZ KODU
//Bir cihazdan diğerine istek mesajı gönderiyoruz. İsteği alan diğer cihaz sensöründen okudğu verileri
//isteği yapan cihaza gönderiyor.


//gerekli kütüphanler ve linkleri
#include <ESP8266WiFi.h>//esp kartta mevcut yüklemeye gerek yok
#include <espnow.h>//esp kartta mevcut yüklemeye gerek yok

#include <Adafruit_Sensor.h>//https://github.com/adafruit/Adafruit_Sensor
#include <Wire.h>//esp kartta mevcut yüklemeye gerek yok
#include <Adafruit_GFX.h>//https://github.com/adafruit/Adafruit-GFX-Library
#include <Adafruit_SSD1306.h>//https://github.com/adafruit/Adafruit_SSD1306

//OLED ekran ayarları
#define SCREEN_WIDTH 128 // OLED display genişlik piksel
#define SCREEN_HEIGHT 32 // OLED display yükseklik piksel
//kullanacağımız buton
#define buton D0

// kullanılacak değişkenler
float gelenTemp;
float gelenHum;
bool gelencevap;
bool istekiletildi=1;
const long bekleme = 5000; 
unsigned long oncekiMillis = 0;  

// I2C protokolü ile çalışan bir SSD1306 ekran nesnesi yaratıyoruz (SDA (D2), SCL (D1) pinlerine bağlanacak)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);//wire protokolü belirtir. -1 ise ekranda reset butonu olup olmadığını. -1 ise yok demek


// Alıncak ve gönderilecek veri yapısı için structure tanımlama
// Diğer kartlar ile aynı yapı olmalı
typedef struct struct_message {
    bool istekgonder;
    float temp;
    float hum;
    bool cevap;
} struct_message;

int butondurumu=0; //butonu takip için
    
//diğer kartın mac adresini girin 
uint8_t alici_macadresi[] = {0x98, 0xF4, 0xAB, 0xDC, 0xC6, 0x91};  


// İstek göndermek için yarattığımız veri yapısı
struct_message istekyolla;

// gelen verileri almak için yarattığımız veri yapısı
struct_message gelenveriler;

// Veriler gönderildiğinde çalıştıracağımız fonksiyon
void VerilerGonderildiginde(uint8_t *mac_addr, uint8_t sendStatus) {
  Serial.print("Son Verinin GÖnderilme Durumu: ");
  if (sendStatus == 0){
    Serial.println("İstek mesajı iletildi");
    istekiletildi=1;
  }
  else{
    Serial.println("İstek İletilemedi !!!");
    istekiletildi=0;//hata mesajını oled ekranda göstermek için kullandım
  }
}

// Veriler alındığında çalışacak fonksiyon
void VerilerAlindiginda(uint8_t * mac, uint8_t *incomingData, uint8_t len) {
  memcpy(&gelenveriler, incomingData, sizeof(gelenveriler));
  Serial.print("Alınan veri boyutu: ");
  Serial.println(len);
  //alınan verileri değişkenlerimize atıyoruz
  gelenTemp = gelenveriler.temp;
  gelenHum = gelenveriler.hum;
  gelencevap = gelenveriler.cevap;
     
  
}

void gelenverileriyazdir(){
  //Alınan verileri Serial Monitöre yazmak için
  Serial.println("GELEN DEGERLER:");
  Serial.print("Sıcaklık: ");
  Serial.print(gelenTemp);
  Serial.println(" ºC");
  Serial.print("Rutubet: ");
  Serial.print(gelenHum);
  Serial.println(" %");

}
 
void setup() {

  Serial.begin(115200);
  pinMode(buton, INPUT_PULLUP);//butonu tanımladık
 
  // Cihazı station olarak tanımlayıp bağlıysa ağlardan çıkıyoruz
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  // ESP-NOW'u başlatıyoruz
  if (esp_now_init() != 0) {
    Serial.println("ESP NOW Başaltılamadı!!!");
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
  
  if (digitalRead(buton)==HIGH) //eğer butona basılmışsa
  {
        delay(100);
        istekyolla.istekgonder = 1;//alıcıya istekgonder değişkenini 1 yapıp gönderiyoruz
        esp_now_send(alici_macadresi, (uint8_t *) &istekyolla, sizeof(istekyolla));
        Serial.println("İstek Gönderilme Denendi");
  
     butondurumu=1;//buton durumunu kontrol amaçlı takip ediyoruz.
     oncekiMillis=millis(); //cevap alındığında ekranda belli bir süre göndermek için önceki milisi güncelledik
     
  }
  
  //eğer gelen cevap paketi içinde gelencevap değişkeni 1 ise ve öncesinde butonla talep de etmişsek
  if (gelencevap==true && butondurumu==1){
 
    unsigned long simdikiMillis = millis();//sayacı başlattık
    //bekleme süresi kadar zaman geçmişse 
    if (simdikiMillis - oncekiMillis >= bekleme) { 
      gelencevap=false;//artık gelen cevap değişkenini tekrar sıfırla çünkü gösterdik işimiz bitti
      butondurumu=0;//artık buton değişkenini tekrar sıfırla çünkü gösterdik işimiz bitti
      oncekiMillis = simdikiMillis;//sayacı tekrar sıfırladık
    }

  //gelen verileri serial monitöre yazdır
  gelenverileriyazdir();
     
  // sıcaklık verisinin OLED ekrana yazdırılması
      display.clearDisplay();
      display.setTextSize(1);//metin boyutu en küçük
      display.setCursor(0,0);//cursor'u pozisyona konumlandırdık
      display.print("Sicaklik: ");
      display.setTextSize(1);
      display.setCursor(65,0);
      display.print(gelenTemp);
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
      display.print(gelenHum);
      display.print(" %"); 

      display.display(); 
    
  }
  else{ //veri gösterme ekranında değilsek istek yap ekranı gösterilecek
      display.clearDisplay();
      display.setTextSize(2);
      display.setCursor(0, 0);
      display.print("Istek Yap: ");
      display.display(); 
      //eğer isteğimizin iletilmesinde sorun varsa istek iletilemedi hata mesajı 2sn gösterilecek
      if (istekiletildi==0){
          istekiletildi=1;
          display.clearDisplay();
          display.setTextSize(1);//metin boyutu en küçük
          display.setCursor(0,0);//cursor'u pozisyona konumlandırdık
          display.print("istek iletilemedi !");
          display.display(); 
          delay(2000);
      }
  }
 
 
    

}
