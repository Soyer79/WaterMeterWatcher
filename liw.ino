#include <SuplaDevice.h>//////https://github.com/SUPLA
#include <WiFiClientSecure.h>
#include <supla/network/esp_wifi.h>
#include <supla/storage/littlefs_config.h>
#include <supla/network/esp_web_server.h>
#include <supla/network/html/device_info.h>
#include <supla/network/html/protocol_parameters.h>
#include <supla/network/html/wifi_parameters.h>
#include <supla/network/html/custom_parameter.h>
#include <supla/network/html/custom_text_parameter.h>
#include <supla/device/supla_ca_cert.h>
#include <supla/device/notifications.h>
#include <PubSubClient.h>
#include <supla/clock/clock.h> //int clock1.getYear(); getMonth(); getDay(); getDayOfWeek();1 - Sunday, 2 - Monday getHour(); getMin(); getSec();
#include "HWCDC.h"

HWCDC USBSerial;
Supla::ESPWifi wifi;
Supla::Clock clock1;
Supla::LittleFsConfig configSupla(4000);
Supla::EspWebServer suplaServer;
Supla::Html::DeviceInfo htmlDeviceInfo(&SuplaDevice);
Supla::Html::WifiParameters htmlWifi;
Supla::Html::ProtocolParameters htmlProto;

WiFiClientSecure client1;
PubSubClient client(client1);

const char PARAM1[] = "MQTT server";
const char PARAM2[] = "MQTT user";
const char PARAM3[] = "MQTT password";
const char PARAM4[] = "device ID";
const char PARAM5[] = "channel ID";
const char PARAM6[] = "night_h_start";
const char PARAM7[] = "night_h_end";
const char PARAM8[] = "deviceName in message ";
const char PARAM9[] = "message";
const char PARAM10[] = "level alarm";
const char PARAM11[] = "level alarm night";

int counter = 0;
int tempCounter;
int tempCounterNight;
boolean ResetTempCounterNight= 0;

char mqtt_server[200] ="a";
char mqtt_user[200] ="a";
char mqtt_pass[200] ="a";
int32_t dv_id=1;
int32_t chann_id=1;
int32_t night_h_start=1;
int32_t night_h_stop=1;
char dev_name_message[200] ="a";
char message[200]="a";
int32_t level_alarm = 40;
int32_t level_alarm_night = 10;
String top1 = "xxx";
String tim;

unsigned long prev_min_millis;
unsigned long prev_hour_millis;
unsigned long lastReconnectAttempt = 0;

void setup() {
  USBSerial.begin(9600);

  Supla::Storage::Init();
  SuplaDevice.addClock(new Supla::Clock);
  auto clock1 = SuplaDevice.getClock();
  new Supla::Html::CustomTextParameter(PARAM1, "MQTT server", 50);
  new Supla::Html::CustomTextParameter(PARAM2, "MQTT user", 50);
  new Supla::Html::CustomTextParameter(PARAM3, "MQTT password", 50);
  new Supla::Html::CustomParameter(PARAM4, "device ID");
  new Supla::Html::CustomParameter(PARAM5, "channel ID");
  new Supla::Html::CustomParameter(PARAM6, "night_h_start");
  new Supla::Html::CustomParameter(PARAM7, "night_h_end");
  new Supla::Html::CustomTextParameter(PARAM8, "devName message", 25);
  new Supla::Html::CustomTextParameter(PARAM9, "push message", 25);
  new Supla::Html::CustomParameter(PARAM10, "level alarm");
  new Supla::Html::CustomParameter(PARAM11, "level alarm night");
  Supla::Notification::RegisterNotification(-1);
  SuplaDevice.setSuplaCACert(suplaCACert);
  SuplaDevice.setSupla3rdPartyCACert(supla3rdCACert);
  SuplaDevice.begin();
  paramSave();
  prev_min_millis=millis();
  prev_hour_millis=millis();
  lastReconnectAttempt = 0;

  delay(2000);
  top1 = "supla/"+String(mqtt_user)+"/devices/"+String(dv_id)+"/channels/"+String(chann_id)+"/state/calculated_value";
  
  USBSerial.print("dv.............:");
  USBSerial.println(dv_id);
  USBSerial.print("channel.............:");
  USBSerial.println(chann_id);
  USBSerial.print("user.............:");
  USBSerial.println(mqtt_user);
  USBSerial.print("serwer.............:");
  USBSerial.println(mqtt_server);
  USBSerial.print("password:.............");
  USBSerial.println(mqtt_pass);
  USBSerial.print("message:.............");
  USBSerial.println(message);
  USBSerial.print("devName message:.............");
  USBSerial.println(dev_name_message);
  USBSerial.print("level alarm:.............");
  USBSerial.println(level_alarm);
  USBSerial.print("level alarm night:.............");
  USBSerial.println(level_alarm_night);
  USBSerial.println(top1);
  mqttConfig();
}

void loop() {
  SuplaDevice.iterate();

  if (!client.connected()) {
    long now = millis();
    if (now - lastReconnectAttempt > 5000) {
      lastReconnectAttempt = now;
      if (reconnect()) {
        lastReconnectAttempt = 0;
      }
    }
  } else {
    client.loop();
  }
  waterControl();
}

boolean reconnect() {
  if (WiFi.status() == WL_CONNECTED) {
    String clientId = "ESP32Client-";
    clientId += String(random(0xffff), HEX);
    if (client.connect(clientId.c_str(), mqtt_user, mqtt_pass)) {
      USBSerial.println("!!connected to MQTT!!");
      client.subscribe(top1.c_str());
    } else {
      USBSerial.print("failed, rc=");
      USBSerial.print(client.state());
    }
  }
  return client.connected();
}
void mqttConfig() {
  USBSerial.println("MQTT config...");
  client1.setInsecure();
  client.setServer(mqtt_server, 8883);
  client.setCallback(callback);
}

void callback(char* topic, byte* payload, unsigned int length) {
  USBSerial.print("Message arrived [");
  USBSerial.print(topic);
  USBSerial.print("] ");
  for (int i = 0; i < length; i++) {
    USBSerial.print((char)payload[i]);
  }
  USBSerial.println();
  if (strcmp(topic, top1.c_str()) == 0) {
    payload[length] = '\0';
    counter = atof((char*)payload);
    USBSerial.print("COUNTER:......................");
    USBSerial.println(counter);

  }
}

void paramSave(){
  if (Supla::Storage::ConfigInstance()->getString(PARAM1, mqtt_server, 200)) {
    SUPLA_LOG_DEBUG(" **** Param[%s]: %s", PARAM1, mqtt_server);
  } else {
    SUPLA_LOG_DEBUG(" **** Param[%s] is not set", PARAM1);
  }
  
  if (Supla::Storage::ConfigInstance()->getString(PARAM2, mqtt_user, 200)) {
    SUPLA_LOG_DEBUG(" **** Param[%s]: %s", PARAM2, mqtt_user);
  } else {
    SUPLA_LOG_DEBUG(" **** Param[%s] is not set", PARAM2);
  }
  if (Supla::Storage::ConfigInstance()->getString(PARAM3, mqtt_pass, 200)) {
    SUPLA_LOG_DEBUG(" **** Param[%s]: %s", PARAM3, mqtt_pass);
  } else {
    SUPLA_LOG_DEBUG(" **** Param[%s] is not set", PARAM3);
  }
  if (Supla::Storage::ConfigInstance()->getInt32(PARAM4, &dv_id)) {
    SUPLA_LOG_DEBUG(" **** Param[%s]: %d", PARAM4, dv_id);
  } else {
    SUPLA_LOG_DEBUG(" **** Param[%s] is not set", PARAM4);
  }
  if (Supla::Storage::ConfigInstance()->getInt32(PARAM5, &chann_id)) {
    SUPLA_LOG_DEBUG(" **** Param[%s]: %d", PARAM5, chann_id);
  } else {
    SUPLA_LOG_DEBUG(" **** Param[%s] is not set", PARAM5);
  }
  if (Supla::Storage::ConfigInstance()->getInt32(PARAM6, &night_h_start)) {
    SUPLA_LOG_DEBUG(" **** Param[%s]: %d", PARAM6, night_h_start);
  } else {
    SUPLA_LOG_DEBUG(" **** Param[%s] is not set", PARAM6);
  }
  if (Supla::Storage::ConfigInstance()->getInt32(PARAM7, &night_h_stop)) {
    SUPLA_LOG_DEBUG(" **** Param[%s]: %d", PARAM7, night_h_stop);
  } else {
    SUPLA_LOG_DEBUG(" **** Param[%s] is not set", PARAM7);
  }
  if (Supla::Storage::ConfigInstance()->getString(PARAM8, dev_name_message, 200)) {
    SUPLA_LOG_DEBUG(" **** Param[%s]: %s", PARAM8, dev_name_message);
  } else {
    SUPLA_LOG_DEBUG(" **** Param[%s] is not set", PARAM8);
  }
  if (Supla::Storage::ConfigInstance()->getString(PARAM9, message, 200)) {
    SUPLA_LOG_DEBUG(" **** Param[%s]: %s", PARAM9, message);
  } else {
    SUPLA_LOG_DEBUG(" **** Param[%s] is not set", PARAM9);
  }
  if (Supla::Storage::ConfigInstance()->getInt32(PARAM10, &level_alarm)) {
    SUPLA_LOG_DEBUG(" **** Param[%s]: %d", PARAM10, level_alarm);
  } else {
    SUPLA_LOG_DEBUG(" **** Param[%s] is not set", PARAM10);
  }
  if (Supla::Storage::ConfigInstance()->getInt32(PARAM11, &level_alarm_night)) {
    SUPLA_LOG_DEBUG(" **** Param[%s]: %d", PARAM11, level_alarm_night);
  } else {
    SUPLA_LOG_DEBUG(" **** Param[%s] is not set", PARAM11);
  }
}

void waterControl() {
  if (millis() - prev_min_millis > 60000) {
    if (night && (ResetTempCounterNight = false)) {
      tempCounterNight = counter;
      ResetTempCounterNight = true;
    }
    else if (night && (ResetTempCounterNight = true)) {
      tempCounterNight = counter;
      ResetTempCounterNight = false;
    }
    if (counter > (tempCounter + level_alarm)) {
      Supla::Notification::Send(-1, dev_name_message, message);
    }
    tempCounter = counter;
    prev_min_millis=millis();
  }
  if ((prev_hour_millis - millis() > 3600000) && night) {
    if (counter > (tempCounterNight + level_alarm_night)) {
      Supla::Notification::Send(-1, dev_name_message, message);
    }
    tempCounterNight = counter;
    prev_hour_millis=millis();
  }
}
void isNight(){
  if((millis()-prev_minute_millis)> 30000){
     if(clock1.getHour() >= night_h_start) {
      if(!night){
        tempCounterNight = counter;
        night = true;
      }
    }
    else if(clock1.getHour() < night_h_stop){
      if(!night){
       tempCounterNight = counter;
       night = true;
    }
    else{
      night = false;
    }
    }
  prev_minute_millis=millis(); 
  }
}
