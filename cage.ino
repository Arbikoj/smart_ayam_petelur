#include <NTPClient.h>
#include <WiFi.h>
#include <esp_adc_cal.h>
#include <FirebaseESP32.h>
#include <DHT.h>
#include "time.h"


//config NTP
// UTC +7 => 7*60*60 = 25200
const long utcOffsetInSeconds = 25200;

char daysOfTheWeek[7][12] = {"Minggu", "Senin", "Selasa", "Rabu", "Kamis", "Jumat", "Sabtu"};
//Month names
String months[12]={"January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December"};
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", utcOffsetInSeconds);

//config NTP date
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 21600;
const int   daylightOffset_sec = 3600;

//config WiFi
const char * ssid = "Pixel"; //nama wifi
const char * password = ""; //password wifi   

//config adc sensor
#define sensor_suhu 35
#define sensor_humi 25

//config firebase
#define FIREBASE_HOST "smartcage-6afcb-default-rtdb.asia-southeast1.firebasedatabase.app/"
#define FIREBASE_AUTH "UMrx6gGbe9ZGYnFwHlRfqCu5B2d0nrc9KjqtFTvC"

//config ultrasonic 
#define echoPin 12
#define trigPin 14
long duration; 
int distance; 


FirebaseData firebaseData;
FirebaseJson jsonSuhu, jsonHumi, jsonRain, jsonEgg;  

//suhu
int LM35_Raw_Sensor1 = 0;
float suhuC = 0.0;
float Voltage = 0.0;

//humidity
#define DHTTYPE DHT11
DHT dht(sensor_humi,DHTTYPE);

//hujan
#define sensorPower 2
#define sensorPin 15
#define adc_rain 18
//fungsi sensor suhu
uint32_t readADC_Cal(int ADC_Raw)
{
  esp_adc_cal_characteristics_t adc_chars;
  esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_11, ADC_WIDTH_BIT_12, 1100, &adc_chars);
  return(esp_adc_cal_raw_to_voltage(ADC_Raw, &adc_chars));
}

//fungsi sensor hujan 
int readSensor() {
  digitalWrite(sensorPower, HIGH);  // Turn the sensor ON
  delay(10);              // Allow power to settle
  int val = digitalRead(sensorPin); // Read the sensor output
  digitalWrite(sensorPower, LOW);   // Turn the sensor OFF
  return val;             // Return the value
}

//fungsi time lokal
void printLocalTime()
{
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time");
    return;
  }
  Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
}

void setup() {

  //init and get the time
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  printLocalTime();
  
  //sensorhujan
  pinMode(sensorPower, OUTPUT);
  digitalWrite(sensorPower, LOW);

  //ultrasonic
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  
  Serial.begin(115200); //init mode
  dht.begin();

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }

    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());

    //koneksi ke firebase
    Firebase.begin(FIREBASE_HOST,FIREBASE_AUTH);
    Firebase.reconnectWiFi(true); 
    
    //Set database read timeout to 1 minute (max 15 minutes)
    Firebase.setReadTimeout(firebaseData, 1000 * 60);
     
     //tiny, small, medium, large and unlimited.
    //Size and its write timeout e.g. tiny (1s), small (10s), medium (30s) and large (60s).
    Firebase.setwriteSizeLimit(firebaseData, "tiny");

}
void loop() {

  printLocalTime();
  timeClient.update();
  Serial.println(timeClient.getFormattedTime());
  /*------SENSOR ULTRASONIC------*/
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  duration = pulseIn(echoPin, HIGH);
  distance = duration * 0.034 / 2;

  /*------SENSOR SUHU------*/
  LM35_Raw_Sensor1 = analogRead(sensor_suhu);  
  Voltage = readADC_Cal(LM35_Raw_Sensor1);
  suhuC = Voltage / 10;
  
  /*------SENSOR HUMIDITY------*/
  float h = dht.readHumidity();

  //sensor curah hujan
  int val = readSensor();
  int rainA = analogRead(adc_rain);

  
  // Print The Readings
  Serial.print("Suhu = ");
  Serial.print(suhuC);
  Serial.print(" °C , ");
  Serial.print("humi = ");
  Serial.print(h);
  Serial.print(" % , ");
if(val == 0) 
  {
    Serial.print("Hujan, "); 
    delay(10); 
  }
else
  {
    Serial.print("Cerah, ");
    delay(10); 
  }
  
  //kirim data ke firebase
  jsonSuhu.set("/suhu", suhuC);
  Firebase.updateNode(firebaseData,"/sensor",jsonSuhu);

  jsonHumi.set("/humi", h);
  Firebase.updateNode(firebaseData,"/sensor",jsonHumi);

  jsonRain.set("/rain", val);
  Firebase.updateNode(firebaseData,"/sensor",jsonRain);


    int bb, counter = 0, currentEgg;
  if(distance<=20){
      ++bb;
      Serial.print(" kecil ");
    }
  
    unsigned long epochTime = timeClient.getEpochTime();
    struct tm *ptm = gmtime ((time_t *)&epochTime);
    String formattedTime = timeClient.getFormattedTime();
      int monthDay = ptm->tm_mday;
      int currentMonth = ptm->tm_mon+1;
      String currentMonthName = months[currentMonth-1];
      int currentYear = ptm->tm_year+1900;
      String currentDate = String(currentYear) + "-" + String(currentMonth) + "-" + String(monthDay);
      jsonEgg.set(String(currentYear) + "/"+ String(currentMonth) +"/"+ String(monthDay), bb);
      Firebase.updateNode(firebaseData,"/sensor/Egg",jsonEgg);
  Serial.print(" ");
  Serial.print(distance);
  Serial.print(" ");
  Serial.print(bb);
  Serial.println("");
  delay(500);
}
