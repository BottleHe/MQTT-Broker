#include "define.h"
#include "Client.h"
#include <errno.h>
#include "Master.h"
#include <sys/socket.h>
#include "logger.h"
#include "error.h"
Client::Client() : m_fd(0), m_pMaster(NULL), m_pRdEvt(NULL), m_willFlag(false), m_willRetain(false), m_willQos(0),
    m_cleanSession(false), m_keepAlive(0), m_lastMsgTimestamp(0), m_pWillTopic(NULL), m_willTopicLen(0), 
    m_pWillMsg(NULL), m_willMsgLen(0), m_pUsername(NULL), m_usernameLen(0), m_pPassword(NULL), m_passwordLen(0),
    m_connectTimestamp(0) {
    m_pBuf = (char*)malloc(RD_BUFSIZE);
    memset(m_pClientId, 0, CLIENT_ID_SIZE + 1);
}
Client::Client(int fd, Master* master) : m_fd(fd), m_pMaster(master), m_pReal(NULL), m_pRdEvt(NULL), 
    m_willFlag(false), m_willRetain(false), m_willQos(0), m_cleanSession(false), m_keepAlive(0), m_lastMsgTimestamp(0),
    m_pWillTopic(NULL), m_willTopicLen(0), 
    m_pWillMsg(NULL), m_willMsgLen(0), m_pUsername(NULL), m_usernameLen(0), m_pPassword(NULL), m_passwordLen(0),
    m_connectTimestamp(0) {
    m_pBuf = (char*)malloc(RD_BUFSIZE);
    memset(m_pClientId, 0, CLIENT_ID_SIZE + 1);
}

Client::~Client() {
    LOG_DEBUG("[Destructor]: Client::~Client();");
    if (m_pPassword) {
        free(m_pPassword);
        m_pPassword = NULL;
    }
    if (m_pUsername) {
        free(m_pUsername);
        m_pUsername = NULL;
    }
    if (m_pWillMsg) {
        free(m_pWillMsg);
        m_pWillMsg = NULL;
    }
    if (m_pWillTopic) {
        free(m_pWillTopic);
        m_pWillTopic = NULL;
    }
    if (m_pReal) {
        delete m_pReal;
        m_pReal = NULL;
    }
    if (m_pBuf) {
        free(m_pBuf);
        m_pBuf = NULL;
    }
    if (m_pRdEvt) {
        event_del(m_pRdEvt);
        event_free(m_pRdEvt);
        m_pRdEvt = NULL;
    }
    while(!m_mqttPackageQueue.empty()) {
        MQTT* _mqtt = m_mqttPackageQueue.front();
        delete _mqtt;
        m_mqttPackageQueue.pop();
    }
}

void Client::onMessage(evutil_socket_t fd, short what, void* arg) {
    LOG_DEBUG_EX("Got an event on socket read %d, %s%s%s%s", (int) fd, 
        (what & EV_TIMEOUT) ? " | timeout" : "",
        (what & EV_READ)    ? " | read" : "",
        (what & EV_WRITE)   ? " | write" : "",
        (what & EV_SIGNAL)  ? " | signal" : "");

    Client* _this = (Client*)arg;
    extern int errno;
    // 先定义一个 MQTT 包组, 每读完一个包, 往包组里丢一个MQTT 然后统一处理
    if (!_this->m_pReal || _this->m_pReal->isFull()) {
        _this->m_pReal = new MQTT();
    }
    for (;;) {
        ssize_t s = recv(fd, _this->m_pBuf, RD_BUFSIZE, 0);
        if (-1 == s) {
            if (EAGAIN == errno) { // 数据被读完
                break;
            }
            LOG_ERROR_EX("Recv error(%d): %s\n", errno, strerror(errno));
            break;
        } else if (0 == s) { // client close
            // TODO release this
            LOG_DEBUG_EX("FD: %d is close, release it\n", _this->m_fd);
            _this->m_pMaster->closeAndReleaseClient(&_this);
            return;
        }
        LOG_DEBUG_EX("接收到%ld字节数据.\n", s);
        // 按MQTT 协议解包
        int offset = 0;
        do {
            int readSize = _this->m_pReal->appendBuffer(_this->m_pBuf + offset, s - offset);
            if (readSize > 0) {
                offset += readSize;
            }
            if (NULL_PACKAGE == readSize) {
                LOG_ERROR("包解析出错(return -1)");
                _this->m_pMaster->closeAndReleaseClient(&_this);
                return;
            }
            if (ERROR_PACKAGE_LEN == readSize) {
                LOG_ERROR("包长超限制(return -2)");
                _this->m_pMaster->closeAndReleaseClient(&_this);
                return;
            }
            LOG_DEBUG_EX("包长检查, offset = %d, isFull: %d", readSize, _this->m_pReal->isFull());
            if (FULL_PACKAGE == readSize || _this->m_pReal->isFull()) {
                LOG_DEBUG_EX("数据包已读满, 执行操作, offset = %d, isFull: %d", offset, _this->m_pReal->isFull());
                _this->m_mqttPackageQueue.push(_this->m_pReal);
                _this->m_pReal = new MQTT();
            }
        } while (s - offset > 0);
    }

    LOG_DEBUG("数据读取完毕, 执行操作\n");
    _this->runMsgControl(); // 是否可交到task进程完成, IO 操作需要交到task进程完成
}

void Client::runMsgControl() {
    int ret = 0;
    while (!this->m_mqttPackageQueue.empty()) {
        MQTT* _mqtt = this->m_mqttPackageQueue.front();
        _mqtt->showPackageInfo();
        ret = 0;
        // 解析数据包, 做回复等处理
        switch (_mqtt->type()) {
            case CONNECT:
                ret = this->controlConnect(_mqtt);
                LOG_DEBUG_EX("解析连接包, 返回值为: %d", ret);
                if (0 == ret) { // 表示解析校验成功, 这边应该发送 CONNACK 到客户端
                    LOG_DEBUG("CONNECT 包解析成功, 校验成功");
                } else { // 不为0 表示有错误
                    LOG_DEBUG_EX("CONNECT 包解析失败, 返回值: %d [%s]", ret, getErrStr(ret));
                }
                break;
            case CONNACK:
                break;
            case PUBLISH:
                break;
            case PUBACK:
                break;
            case PUBREC:
                break;
            case PUBREL:
                break;
            case PUBCOMP:
                break;
            case SUBSCRIBE:
                break;
            case SUBACK:
                break;
            case UNSUBSCRIBE:
                break;
            case UNSUBACK:
                break;
            case PINGREQ:
                break;
            case PINGRESP:
                break;
            case DISCONNECT:
                break;
            case RETAIN:
            default:
                // 无效包, 理论上前一层应被拦截
                Client* _this = this;
                m_pMaster->closeAndReleaseClient(&_this);
                return;
        }

        delete _mqtt; // TODO 这里后续会做个 池子 存放, 这里写回收的代码
        this->m_mqttPackageQueue.pop();
    }
}

void Client::fd(int fd) {
    m_fd = fd;
}

int Client::fd() const {
    return m_fd;
}

void Client::rdEvt(struct event* event) {
    m_pRdEvt = event;
}

struct event* Client::rdEvt() const {
    return m_pRdEvt;
}

int Client::controlConnect(MQTT* mqtt) {
    // 解析Connect 包
    const char* payload = mqtt->payload(); 
    if (NULL == payload) {
        return INVALID_DATA; // 无交的payload
    }
    if (0 != payload[0] || 4 != payload[1] 
    || 'M' != payload[2] || 'Q' != payload[3] || 'T' != payload[4] || 'T' != payload[5]) {
        return MQTT_FIXED_ERR; // MQTT 前六字节应是固定值, 非此值, 表示异常
    }
    // 协议级别
    if (0x4 != payload[6]) { // 协议级别判断, 如果不是4, 表示异常
        return MQTT_PROTO_LV_ERR; 
    }
    // 连接标志
    if ((payload[7] << 7) & 0xff) {
        LOG_DEBUG_EX("连接标志: %d [%d %d %d %d %d %d %d %d]", (payload[7] << 7) & 0xff, (payload[7] >> 7 )& 0x1, (payload[7] >> 6) & 0x1, 
        (payload[7] >> 5) & 0x1, (payload[7] >> 4) & 0x1, (payload[7] >> 3) & 0x1, (payload[7] >> 2) & 0x1, 
        (payload[7] >> 1) & 0x1, payload[7] & 0x1);
        return MQTT_CONN_FLAG_ERR; // 连接标志第0位不为0, 表示异常
    }
    bool usernameFlag = payload[7] & MQTT_USERNAME_FLAG ? true : false;
    bool passwordFlag = payload[7] & MQTT_PASSWORD_FLAG ? true : false;
    if (!usernameFlag && passwordFlag) {
        return MQTT_USER_PWD_NOT_MATCH; // 若没有用户名标志, 密码标志必需设置成0
    }

    m_willRetain = payload[7] & MQTT_WILL_RETAIL_FLAG ? true : false;
    m_willFlag = payload[7] & MQTT_FLAG_FLAG ? true : false;
    m_cleanSession = payload[7] & MQTT_CLEAN_SESSION_FLAG ? true : false;
    m_willQos = (payload[7] >> 3) & 0x3;
    if (!m_willFlag) {
        if (m_willRetain || 0 != m_willQos) { // 若没有 遗嘱标志, 则 遗嘱保留 和 遗嘱QOS 字段都必需设置成0
            return MQTT_WILL_FLAG_QOS_NOT_MATCH;
        }
    } else {
        if (m_willQos == 0x3) { // 遗嘱保留时, QOS 不能设置成3
            return MQTT_INVALID_WILL_QOS;
        }
    }
    unsigned int keepAlive = payload[8] * 0xff + payload[9]; // 大端字节序直接转数字
    // 下面解析有效载荷
    int offset = 10; // 从第10位开始
    unsigned _len = *(payload + (offset++)) * 0xff + *(payload + (offset++));
    LOG_DEBUG_EX("客户端ID(clientId) 长度为: %u", _len);
    // connect ID 格式校验
    int ret = MQTT::checkClientId(payload + offset, _len);
    if (0 > ret) {
        return ret;
    }
    
    memcpy(m_pClientId, payload + offset, _len);
    offset += _len;
    LOG_DEBUG("客户端ID(ClientId): ");
#ifdef __DEBUG_MODE
    LOG_DEBUG_P("    ");
    for (int i = 0; i < _len; ++i) {
        if (m_pClientId[i] == 0) break;
        LOG_DEBUG_EX_P("%c", m_pClientId[i]);
    }
    LOG_DEBUG_P("\n\n");
#endif
    if (m_willFlag) {
        _len = *(payload + (offset++)) * 0xff + *(payload + (offset++)); // 现在是willTopic的长度
        int ret = MQTT::checkTopic(payload + offset, _len);
        if (0 > ret) {
            return ret;
        }
        m_willTopicLen = _len;
        m_pWillTopic = (char*)calloc(m_willTopicLen, sizeof(char));
        memcpy(m_pWillTopic, payload + offset, _len);
        offset += _len;

        LOG_DEBUG("遗嘱主题 (willTopic) : ");
#ifdef __DEBUG_MODE
        LOG_DEBUG_P("    ");
        for (int i = 0; i < m_willTopicLen; ++i) {
            LOG_DEBUG_EX_P("%c", m_pWillTopic[i]);
            if ((i + 1) % 32 == 0) {
                LOG_DEBUG_P("\n    ");
            }
        }
        LOG_DEBUG_P("\n\n");
#endif
        LOG_DEBUG_EX("遗嘱消息起始位置为: %d", offset);
#ifdef __DEBUG_MODE
        LOG_DEBUG("后续字节依次为(只显示后续的16字节, 如果有):");
        LOG_DEBUG_P("    ");
        for (int i = offset; i < offset + 16; ++i) {
            if (i < mqtt->length()) {
                LOG_DEBUG_EX_P("0x%02x ", *(payload + i) & 0xff);
            }
        }
        LOG_DEBUG_P("\n\n");
#endif
        _len = *(payload + (offset++)) * 0xff + *(payload + (offset++)); // 现在是遗嘱消息的长度
        LOG_DEBUG_EX("收到遗嘱消息, 长度为: %u", _len);
        if (0 >= _len) { // 
            free(m_pWillTopic);
            m_pWillTopic = NULL;
            m_willTopicLen = 0;
            return INVALID_LEN_VAL;
        }
        if (MQTT_WILL_MSG_SIZE < _len) {
            free(m_pWillTopic);
            m_pWillTopic = NULL;
            m_willTopicLen = 0;
            return LEN_VALUE_EXCEED;
        }
        m_willMsgLen = _len;
        m_pWillMsg = (char*)calloc(m_willMsgLen, sizeof(char));
        memcpy(m_pWillMsg, payload + offset, _len);
        offset += _len;
        LOG_DEBUG("遗嘱消息 (willMessage) : ");
#ifdef __DEBUG_MODE
        LOG_DEBUG_P("    ");
        for (int i = 0; i < m_willMsgLen; ++i) {
            LOG_DEBUG_EX_P("0x%02x ", (m_pWillMsg[i] & 0xff));
            if ((i + 1) % 16 == 0) {
                LOG_DEBUG_P("\n    ");
            }
        }
        LOG_DEBUG_P("\n\n");
#endif
    }
    if (usernameFlag) {
        _len = *(payload + (offset++)) * 0xff + *(payload + (offset++)); // 现在是用户名的长度
        int ret = MQTT::checkTopic(payload + offset, _len); // 因用户名的校验规则和clientId 一样, 这里使用一样的调用
        if (0 > ret) {
            if (m_pWillTopic) {
                free(m_pWillTopic);
                m_pWillTopic = NULL;
                m_willTopicLen = 0;
            }
            if (m_pWillMsg) {
                free(m_pWillMsg);
                m_pWillMsg = NULL;
                m_willMsgLen = 0;
            }
            return ret;
        }
        m_usernameLen = _len;
        m_pUsername = (char*)calloc(m_usernameLen, sizeof(char));
        memcpy(m_pUsername, payload + offset, _len);
        offset += _len;

        LOG_DEBUG("用户名 (username) : ");
#ifdef __DEBUG_MODE
        LOG_DEBUG_P("    ");
        for (int i = 0; i < m_usernameLen; ++i) {
            LOG_DEBUG_EX_P("%c", (m_pUsername[i] & 0xff));
        }
        LOG_DEBUG_P("\n\n");
#endif
    }
    if (passwordFlag) {
        _len = *(payload + (offset++)) * 0xff + *(payload + (offset++));
        if (32 < _len) { // 密码长度不能大于32个字符
            if (m_pWillTopic) {
                free(m_pWillTopic);
                m_pWillTopic = NULL;
                m_willTopicLen = 0;
            }
            if (m_pWillMsg) {
                free(m_pWillMsg);
                m_pWillMsg = NULL;
                m_willMsgLen = 0;
            }
            if (m_pUsername) {
                free(m_pUsername);
                m_pUsername = NULL;
                m_usernameLen = 0;
            }
            return INVALID_LEN_VAL;
        }
        if (0 >= _len) { // 密码为空
            // 是否做验证处理
        } else {
            m_passwordLen = _len;
            m_pPassword = (char*)calloc(m_passwordLen, sizeof(char));
            memcpy(m_pPassword, payload + offset, _len);
            offset += _len;

            LOG_DEBUG("密码 (password) : ");
#ifdef __DEBUG_MODE
            LOG_DEBUG_P("    ");
            for (int i = 0; i < m_passwordLen; ++i) {
                LOG_DEBUG_EX_P("0x%02x ", (m_pPassword[i] & 0xff));
                if ((i + 1) % 16 == 0) {
                    LOG_DEBUG_P("\n    ");
                }
            }
            LOG_DEBUG_P("\n\n");
#endif
        }
    }

    // TODO 检验用户名和密码, 现在只针对一种情况做校验, 即所有设备统一密码, 后续做分各设备不同密码的解决方案
    // Send ack
    if (!usernameFlag || !passwordFlag || 0 >= m_usernameLen || 0 >= m_passwordLen) {
        return MQTT_USER_PWD_NOT_NULL;
    }

    if (m_usernameLen == 6 && 0 == memcmp(m_pUsername, "bottle", 6) && 
    m_passwordLen == 6 && 0 == memcmp(m_pPassword, "123123", 6)) {
        return SUCCESS;
    } else {
        return MQTT_USER_PWD_INVALID;
    }
}

unsigned int Client::linkedTimestamp() const {
    return m_linkedTimestamp;
}

void Client::linkedTimestamp(unsigned int ts) {
    m_linkedTimestamp = ts;
}

unsigned int Client::connectTimestamp() const {
    return m_connectTimestamp;
}

unsigned int Client::keepAlive() const {
    return m_keepAlive;
}

unsigned int Client::lastMsgTimestamp() const {
    return m_lastMsgTimestamp;
}