#ifndef __DEFINE_H__
#define __DEFINE_H__

#define LOG_TIME_FORMAT "%T"
#define __DEBUG_MODE

#define CLIENT_CONNECT_TIMEOUT 15 // 连接超时, 如果超过 CLIENT_CONNECT_TIMEOUT 时间没有发送CONNECT请求, 则应断开

#define CLIENT_ID_SIZE 32  // MQTT client ID 最大长度 32字符
#define MQTT_TOPIC_SIZE 255 // MQTT topic 最大长度不能为 255 字符
#define MQTT_WILL_MSG_SIZE 65535 // MQTT 遗嘱消息最大长度不能大于65535 字符

#define MQTT_USERNAME_FLAG 0x80
#define MQTT_PASSWORD_FLAG 0x40
#define MQTT_WILL_RETAIL_FLAG 0x20
#define MQTT_FLAG_FLAG 0x04
#define MQTT_CLEAN_SESSION_FLAG 0x02

#endif