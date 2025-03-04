#include <WiFi.h>
#include <esp_now.h>

#define ECHO_PIN 13
#define TRIG_PIN 12
#define MAX_DISTANCE 50

#define uS_TO_S_FACTOR 1000000
#define TIME_TO_SLEEP 6 //51%50+5

//MAC receiver
uint8_t broadcastAddress[] = {0x8C, 0xAA, 0xB5, 0x84, 0xFB, 0x90};

esp_now_peer_info_t peerInfo;

//A simple function used to print the timestamp
String timestamp() {
  //Get current milliseconds since program started
  unsigned long currentMillis = millis();  

  //Calculate seconds, minutes, hours, and days
  unsigned long seconds = currentMillis / 1000;
  unsigned long minutes = seconds / 60;
  unsigned long hours = minutes / 60;
  unsigned long days = hours / 24;

  //Calculate individual components
  unsigned int hour = hours % 24;
  unsigned int minute = minutes % 60;
  unsigned int second = seconds % 60;
  unsigned int millisecond = currentMillis % 1000;

  //Format the timestamp into a string
  char buffer[30];
  snprintf(buffer, sizeof(buffer), "day:%u, time:%02u:%02u:%02u.%03u", days, hour, minute, second, millisecond);

  return String(buffer);
}

//Sending callback
void onDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("Message Sent: ");
  Serial.print(status == ESP_NOW_SEND_SUCCESS ? "OK" : "ERROR");
  Serial.println(", completed at [" + timestamp() + "]");
}

//Receiving callback
void onDataRecv(const uint8_t * mac, const uint8_t *data, int len) {
  Serial.print("( [SINK NODE] Receiving Message -> Parking Status: ");
  char receivedString[len];
  memcpy(receivedString, data, len);
  Serial.println(String(receivedString) + " )");
}

//Computing the distance in CM
float readDistanceCM() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  int duration = pulseIn(ECHO_PIN, HIGH);
  return duration / 58.00;
}

//A simple function that returns the parking status based on the distance detected
String isOccupied(){
  float distance = readDistanceCM();
  Serial.print("Distance detected: " + (String)distance + "cm");
  Serial.println(", completed at [" + timestamp() + "]");
  if(distance < MAX_DISTANCE){
    return "OCCUPIED";
  }
  return "FREE";
}

void setup() {
  Serial.begin(115200);
  Serial.println("-----------------------------------------------------------------------------------");
  Serial.print("BOOT period -> ");
  Serial.println("Completed at [" + timestamp() + "]");

  Serial.println("ACTIVE period:");
  Serial.print("- SENSOR IDLE period -> ");
  WiFi.mode(WIFI_STA);
  
  esp_now_init();
  esp_now_register_send_cb(onDataSent);
  esp_now_register_recv_cb(onDataRecv);

  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;  
  peerInfo.encrypt = false;
  esp_now_add_peer(&peerInfo);

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  Serial.println("Completed at [" + timestamp() + "]");

  Serial.print("- SENSOR ACTIVE period -> ");
  String message = isOccupied();

  Serial.print("- MESSAGE TX period -> ");
  WiFi.setTxPower(WIFI_POWER_2dBm);
  esp_now_send(broadcastAddress, (uint8_t*)message.c_str(), message.length() + 1);
  
  delay(20); //This delay was added to allow the send and received message to be printed correctly

  Serial.print("Wi-Fi OFF period -> ");
  WiFi.mode(WIFI_OFF);

  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);

  Serial.println("Completed at [" + timestamp() + "]");
  Serial.println("Going to sleep now (" + String((float)TIME_TO_SLEEP) + "s)");
  Serial.println("-----------------------------------------------------------------------------------");
  
  //--------------
  //THIS DELAY IS ONLY USED TO SIMULATE DEEP SLEEP TIME ON WOKWI, ON REAL HARDWARE IT MUST BE REMOVED
  delay(TIME_TO_SLEEP*1000);
  //--------------

  Serial.flush();
  esp_deep_sleep_start();
}

void loop() {
  delay(10);
}