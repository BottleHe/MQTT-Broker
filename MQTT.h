#ifndef __MQTT_H__
#define __MQTT_H__
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "define.h"


enum MQTT_Type {
    NONE = 0, // DEFAULT NONE 
    CONNECT = 1, // 连接请求
    CONNACK = 2, // 连接确认
    PUBLISH = 3, // 发布消息
    PUBACK = 4, // 发布确认
    PUBREC = 5, // 发布收到 (QoS2)
    PUBREL = 6, // 发布释放 (QoS2)
    PUBCOMP = 7, // 发布完成 (QoS2)
    SUBSCRIBE = 8, // 客户端向代理发起订阅请求
    SUBACK = 9, // 订阅确认
    UNSUBSCRIBE = 10, // 取消订阅
    UNSUBACK = 11, // 取消订阅确认
    PINGREQ = 12, // PING 请求
    PINGRESP = 13, // PING 响应
    DISCONNECT = 14, // 断开连接
    RETAIN = 15 // 保留
};

typedef enum MQTT_Type MQTTType;
class MQTT {
public:
    MQTT();
    ~MQTT();
    bool isEmpty() const;
    bool isFull() const;
    int appendBuffer(const char* buf, unsigned int length);
    int readHeader(const char* buf, unsigned int length);

    MQTTType type() const; // MQTT 类型
    unsigned int length() const; // MQTT 剩余长度
    const char* payload() const; // MQTT 剩余内容
    void showPackageInfo() const;
    const char* getPackageType(MQTTType type) const;

    static int checkClientId(const char* clientId, unsigned int clientLen);
    static int checkTopic(const char* topic, unsigned int topicLen);
private:
    MQTTType m_type;
    // 实际长度
    // unsigned int m_size; // 这个留到优化时用到
    unsigned int m_length;
    int m_offset; // 读的时候为偏移字段, 解析时为长度最大值, 超过这个值为非法字段
    bool m_full;
    bool m_empty;
    char* m_pBuffer;
    char m_fixedHead; // 最前面的固定一字节
    char m_pLengthBuf[4]; // 长度暂存文件
    int m_lengthOffset; // 长度偏移
};
#endif // __MQTT_H__