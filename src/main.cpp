//Nama Program    : TIMBANGAN V1.0
//Nama Pembuat    : Ali Akbar
//Tanggal Di Buat : 23/07/2023


#include <Arduino.h>


void sendMessage(String outgoing, byte Node_MesinPetik, byte otherNode);
void onReceive(int packetSize);
void LEDCOLOR(String color);


//Inisialisasi library SPI
#include <SPI.h>
//Inisialisasi library LoRa
#include <LoRa.h>
//Inisialisasi pin yang digunakan LoRa => ESP32
#define SS   18     
#define RST  14   
#define DIO0 26   
#define SCK  5
#define MISO 19
#define MOSI 27
//Insialisasi variabel untuk kirim dan terima pesan pada komunikasi LoRa
String RSI;         //Variabel untuk menampung nilai RSI
String outgoing;            //Pesan keluar
byte msgCount = 0;          //Menghitung jumlah pesan keluar
byte Node_Repeater1 = 0xFA;     //Addres Node Gateway
byte Node_Timbangan = 0xBA;    //Addres Node Telemetri GPS
String pesan;               //Variabel untuk menampung pesan yang akan dikirim ke Gateway


//Inisialisasi library axp20x (Power Management)
#include <axp20x.h>
AXP20X_Class axp;
//Inisialisasi variabel untuk menyimpan addres axp192
const uint8_t slave_address = AXP192_SLAVE_ADDRESS;
String VBAT;

//Inisialisasi library GPS Neo 6
#include <TinyGPS++.h>
TinyGPSPlus gps;
//Menggunakan pin serial 1 pada esp32
HardwareSerial GPS(1);
//Inisialisasi variabel untuk menampung nilai koordinat dari GPS
String Latitude, Longitude;


//DHT22
#include "DHT.h" 
#define DHTPIN 13
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);
//Inisialisasi Variabel Sensor
float suhu;
int kelembaban;


//LoadCell
#include <HX711_ADC.h>
#include <EEPROM.h>
const int HX711_dout = 25; //mcu > HX711 dout pin
const int HX711_sck = 14; //mcu > HX711 sck pin
HX711_ADC LoadCell(HX711_dout, HX711_sck);

int i;
float nilaikalibrasi = 519.83;
const int alamatkalibrasi_eeprom = 0;
const int intervalwaktu = 500;
unsigned long stabilizingtime = 2000;
unsigned long t = 0;
static boolean newDataReady = 0;
boolean _tare = true;


//Library Time
#include <time.h>
unsigned long waktusebelumnya1 = 0;
unsigned long waktusekarang1 = 0;

unsigned long waktusebelumnya2 = 0;
unsigned long waktusekarang2 = 0;


#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(0x27, 16, 2);



void setup()
{
  // Debug console
  Serial.begin(9600);
  Serial.println();
  Serial.println("Menyiapkan Timbangan...");

  //Mulai menjalankan komunikasi I2C
  Wire.begin(21,22);

  //Mulai menjalankan library DHT22
  dht.begin();

  //Memulai komunikasi SPI
  SPI.begin(SCK, MISO, MOSI, SS);
  //Setting pin LoRa
  LoRa.setPins(SS, RST, DIO0);
  if (!LoRa.begin(915E6)) {            
    Serial.println("Gagal menjalankan LoRa. Periksan wiring rangkaian.");
  }


  //Menyimpan nilai kalibrasi pada eeprom
  EEPROM.get(alamatkalibrasi_eeprom, nilaikalibrasi);
  LoadCell.begin();

  //Mulai menjalankan library Load Cell sekaligus memulai kalibrasi timbangan
  LoadCell.start(stabilizingtime, _tare);
  if (LoadCell.getTareTimeoutFlag()) {
    Serial.println("Waktu pembacaan habis, cek kabel ESP32>HX711 pastikan sudah tepat");
    while (1);
  }
  else {
    LoadCell.setCalFactor(nilaikalibrasi);
    Serial.println("Persiapan selesai");
  }

  //Mulai menjalankan GPS
  GPS.begin(9600, SERIAL_8N1, 34, 12);


  //Mulai menjalankan library AXP (Power Management)
  int ret = axp.begin(Wire, slave_address);
  if (ret) {
    Serial.println("Ooops, AXP202/AXP192 power chip detected ... Check your wiring!");
  }
  axp.adc1Enable(AXP202_VBUS_VOL_ADC1 |
                 AXP202_VBUS_CUR_ADC1 |
                 AXP202_BATT_CUR_ADC1 |
                 AXP202_BATT_VOL_ADC1,
                 true);

  lcd.init();
  lcd.backlight();
  

  //Menampilkan Pesan
  lcd.clear();
  lcd.setCursor(0, 1);
  lcd.print("MAKERINDO");
  lcd.setCursor(0, 2);
  lcd.print("IoT Timbangan");
 
}

void loop()
{
 
  //Membaca Nilai Suhu dan Kelembaban
  suhu = dht.readTemperature();
  kelembaban = dht.readHumidity();

  //Membaca nilai Timbangan
  if (LoadCell.update()) newDataReady = true;
  if (newDataReady){
      i = LoadCell.getData() + 10;
      if(i<12){i=0;}
    }

  
 

  //Menggunakan fungsi millis()
  waktusebelumnya1 = millis();

  if (waktusebelumnya1 - waktusekarang1 > 1000){
      waktusekarang1 = waktusebelumnya1;

    
    //Membaca nilai Tegangan Baterai
    if (axp.isBatteryConnect()) {
        VBAT = axp.getBattVoltage();
      } 
      
    else {
        VBAT = "NAN";
      }
    
    
    if (gps.location.isValid()) {
      Latitude = String(gps.location.lat(), 9);
      Longitude = String(gps.location.lng(), 9);
    } 
    
    else {
      Latitude = "NAN";
      Longitude = "NAN";
    }


      pesan = String() + "TIMBANGANPPTK0823" + "," + Latitude + "," + Longitude + "," + suhu + "," + kelembaban + "," + i + "," + VBAT + ",*";
      

  }

  //Menggunakan fungsi millis()
  waktusebelumnya2 = millis();

  if (waktusebelumnya2 - waktusekarang2 > 2000){
      waktusekarang2 = waktusebelumnya2;

    //Menampilkan data pembacaan suhu dan kelembaban
      Serial.println();
      Serial.println(String() + "Temperatur    = " + String(suhu) + " C");
      Serial.println(String() + "Kelembaban    = " + String(kelembaban) + " %");
      Serial.println(String() + "Berat Badan   = " + String(i) + " Gram");
      Serial.print("Latitude      = ");
      Serial.println(Latitude);
      Serial.print("Longitude     = ");
      Serial.println(Longitude);
      Serial.print(String() + "Batterai     = " + String(VBAT) + " mV");
      Serial.println();

      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(" IoT Timbangan");
      
      lcd.setCursor(0, 1);
      lcd.print("Berat = ");
      lcd.setCursor(8, 1);
      lcd.print(i);
      lcd.setCursor(13, 1);
      lcd.print("Gr");

  }

  else {
    while (GPS.available()) {
      gps.encode(GPS.read());
    }
  }

  onReceive(LoRa.parsePacket());      
  
}

void sendMessage(String outgoing, byte Node_Timbangan, byte otherNode) {
  LoRa.beginPacket();                   // start packet
  LoRa.write(otherNode);              // add destination address
  LoRa.write(Node_Timbangan);             // add sender address
  LoRa.write(msgCount);                 // add message ID
  LoRa.write(outgoing.length());        // add payload length
  LoRa.print(outgoing);                 // add payload
  LoRa.endPacket();                     // finish packet and send it
  msgCount++;                           // increment message ID
}
 
void onReceive(int packetSize) {
  if (packetSize == 0) return;          // if there's no packet, return
 
  // read packet header bytes:
  int recipient = LoRa.read();          // recipient address
  byte sender = LoRa.read();            // sender address
  byte incomingMsgId = LoRa.read();     // incoming msg ID
  byte incomingLength = LoRa.read();    // incoming msg length
 
  String incoming = "";
 
  while (LoRa.available()) {
    incoming += (char)LoRa.read();
  }
 
  if (incomingLength != incoming.length()) {   // check length for error
   // Serial.println("error: message length does not match length");
    return;                             // skip rest of function
  }
 
  // if the recipient isn't this device or broadcast,
  if (recipient != Node_Timbangan && recipient != Node_Repeater1) {
    //Serial.println("This message is not for me.");
    return;                             // skip rest of function
  } 

  if (incoming == "TIMBANGAN1"){ 
    sendMessage(pesan,Node_Timbangan,Node_Repeater1);
    Serial.println(pesan);
    RSI = String(LoRa.packetRssi());
    delay(100);
  }


}