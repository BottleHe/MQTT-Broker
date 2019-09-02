#ifndef __CLIENT_H__
#define __CLIENT_H__
#include <event.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <queue>
#include "MQTT.h"
#define RD_BUFSIZE (4096)
class Master;
class Client {
public:
    Client();
    Client(int fd, Master* master);
    ~Client();
    static void onMessage(evutil_socket_t fd, short what, void* arg);

    int fd() const;
    void fd(int fd);

    struct event* rdEvt() const;
    void rdEvt(struct event* event);

    Master* master() const;
    void master(Master* master);

    unsigned int linkedTimestamp() const;
    void linkedTimestamp(unsigned int ts);

    unsigned int connectTimestamp() const;

    unsigned int keepAlive() const;
    unsigned int lastMsgTimestamp() const;

    static const char* generateConnectAck();
private:
    int m_fd; // cliend fd
    Master* m_pMaster; // master pointer
    char* m_pBuf;
    struct event* m_pRdEvt;
    std::queue<MQTT*> m_mqttPackageQueue; 
    MQTT* m_pReal;
    bool m_willFlag; // 是否有遗嘱标志
    bool m_willRetain; // 遗嘱标志是否保留
    short m_willQos; // 遗嘱QOS, 不能为3, 取值 [0, 1, 2]
    bool m_cleanSession; // 清除会话标志
    unsigned int m_keepAlive; // keep alive 时间
    unsigned int m_lastMsgTimestamp; // 收到最后一个数据包的时间
    char m_pClientId[CLIENT_ID_SIZE + 1]; // ClientId 最大CLIENT_ID_SIZE 个, 默认 32 , +1 是为了简单存放最后一个 '\0'
    char* m_pWillTopic; // 遗嘱主题
    unsigned int m_willTopicLen; // 遗嘱主题长度
    char* m_pWillMsg; // 遗嘱消息
    unsigned int m_willMsgLen; // 遗嘱消息长度
    char* m_pUsername; // 用户名
    unsigned int m_usernameLen;
    char* m_pPassword; // 密码
    unsigned int m_passwordLen; // 密码长度

    unsigned int m_linkedTimestamp; // 创建连接的时间
    unsigned int m_connectTimestamp; // 登录时间, 若未登录, 先收到其它包, 断开, 连续登录包处理

    void runMsgControl();
    int controlConnect(MQTT* mqtt);
};
#endif // __CLIENT_H__