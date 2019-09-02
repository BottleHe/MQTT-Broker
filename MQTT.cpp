#include "MQTT.h"
#include "logger.h"
#include "error.h"

MQTT::MQTT() : m_type(NONE), m_length(0), m_offset(0), m_full(false), m_empty(true), m_pBuffer(NULL), m_fixedHead('\0'), m_lengthOffset(0) {
    memset(m_pLengthBuf, '\0', 4);
}
MQTT::~MQTT() {
    if (m_pBuffer) {
        free(m_pBuffer);
        m_pBuffer = NULL;
    }
}
bool MQTT::isEmpty() const {
    return m_empty;
}
bool MQTT::isFull() const {
    return m_full;
}

int MQTT::appendBuffer(const char* buf, unsigned int length) {
    // printf("m_pLengthBuf: {0x%02x, 0x%02x, 0x%02x, 0x%02x\n", m_pLengthBuf[0], m_pLengthBuf[1], m_pLengthBuf[2], m_pLengthBuf[3]);
    LOG_DEBUG_EX("Buff长度: %u, 包长: %u, 当前偏移: %d\n", length, m_length, m_offset);
    if (0 >= length) { // 空包
        LOG_DEBUG_T("空包, 不做作何处理");
        return NULL_PACKAGE;
    }
    if (m_full) { // 满包
        LOG_DEBUG_T("满包, 直接返回0");
        return FULL_PACKAGE;
    }
    if (isEmpty()) { // 表示是一个空包
        m_fixedHead = buf[0]; // 保留最前面的一个字节, 以备后面使用
        m_type = static_cast<MQTTType>((m_fixedHead >> 4) & 0xF);
        // 这里直接做 fixHeader 有效性判断
        // TODO 
        LOG_DEBUG_EX("类型为: %s", getPackageType(m_type));
        m_empty = false;
        if (length < 2) return 1; // 只有一个字节
        // 如果第二个字节直接为 0 , 则表示无任何包体
        if (buf[1] == 0) { // 如果第二个字节为0, 表示无包体
            m_full = true; // 表示读完了
            return 2; // 表示读了两个字节 offset == 2
        }
        int offset = readHeader(buf + 1, length - 1);
        if (0 > offset) {
            return ERROR_PACKAGE_LEN; // 包长解析出错. 应断开连接
        }
        LOG_DEBUG_EX("读完包头后偏移为: %d, 包长为: %d\n", offset, m_length);
        offset++; // 这里加上头部最前面一个字节
        if (m_length > 0) { // length 大于0, 需要分配内存
            m_pBuffer = (char*)calloc(m_length, sizeof(char)); // 分配内存
        } else { // 未读出来length, 肯定是已经读完了 buf 里面的数据
            return offset; // 这时候, offset == length
        }
        if ((unsigned int)offset < length) { // 表示还有后续数据, 
            // 可copy的内存空间为 buf + offset, length - offset, 
            // 实际copy的内存空间为
            int size = m_length > length - offset ? length - offset : m_length;
            memcpy(m_pBuffer + m_offset, buf + offset, size);
            m_offset += size; // 读出size个字节
            offset += size; // 实际使用了 offset 个字节
            if ((unsigned int)size == m_length) { // 如果已读完
                m_full = true;
            }
            return offset;
        } else { // 若它们想等, 表示无后续数据可读了. offset 不可能大于 length
            return offset;
        }
    } else {
        // 包非空
        // 判断 m_length 是否有值, 没有值, 表示头还没读完
        LOG_DEBUG_EX("非空包 m_length = %u, m_lengthOffset = %d, length = %u\n", m_length, m_lengthOffset, length);
        if (m_length <= 0) {
            int offset = readHeader(buf, length); // 头部偏移了多少个字节
            if (m_length > 0) { // length 大于0, 需要分配内存
                m_pBuffer = (char*)calloc(m_length, sizeof(char)); // 分配内存
            } else { // 未读出来length, 肯定是已经读完了 buf 里面的数据, 但是包还未读到长度
                return offset; // 这时候, offset == length
            }
            if ((unsigned int)offset < length) { // 表示还有后续数据, 
                // 可copy的内存空间为 buf + offset, length - offset, 
                // 实际copy的内存空间为
                int size = m_length > length - offset ? length - offset : m_length; // 实际要读的字节数
                memcpy(m_pBuffer + m_offset, buf + offset, size);
                m_offset += size; // 读出size个字节
                offset += size; // 实际使用了 offset 个字节
                if ((unsigned int)size == m_length) { // 如果已读完
                    m_full = true;
                }
                return offset;
            } else { // 若它们想等, 表示无后续数据可读了. offset 不可能大于 length
                return offset;
            }
        } else {
            int offset = 0;
            if ((unsigned int)offset < length) { // 表示 有可读数据
                // 可copy的内存空间为 buf + offset, length - offset, 
                // 实际copy的内存空间为
                int size = m_length - m_offset > length - offset ? length - offset : m_length - m_offset; // 实际要读的字节数
                memcpy(m_pBuffer + m_offset, buf + offset, size);
                m_offset += size; // 读出size个字节
                offset += size; // 实际使用了 offset 个字节
                LOG_DEBUG_EX("size: %d, offset: %d, length: %u, m_length: %u, m_offset: %d", size, offset, length, m_length, m_offset);
                if (m_offset /* 总共读取字节数 */ == m_length) { // 如果已读完
                    m_full = true;
                }
                return offset;
            } else { // 若它们想等, 表示无后续数据可读了. offset 不可能大于 length
                return offset;
            }
        }
    }
}

int MQTT::readHeader(const char* buf, unsigned int length) {
    if (m_lengthOffset >= 4) return 0;
    int preCopy = m_lengthOffset; // 记下来上次copy过的字节数
    int multiplier = 1;
    int cpl = (int)length >= (4 - m_lengthOffset) ? (4 - m_lengthOffset) : (int)length; // need copy length 
    if (cpl <= 0) { // 无可copy的内存
        return NULL_BUFFER;
    }
    LOG_DEBUG_EX("头部内容拷贝, 拷贝了 %d 字节, bufLength = %u, m_lengthOffset = %d", cpl, length, m_lengthOffset);
    memcpy(m_pLengthBuf + m_lengthOffset, buf, cpl);
    m_lengthOffset += cpl;
    int i = 0;
    m_length = 0;
    do {
        m_length += (m_pLengthBuf[i++] & 127) * multiplier;
        if (i > m_lengthOffset && (m_pLengthBuf[i - 1] & 128) != 0) { // 但是长度还未读完(不够读)
            LOG_DEBUG_EX("头部偏移 m_lengthOffset = %d, 循环次数 i = %d", m_lengthOffset, i);
            m_length = 0; // 将length 初始回0
            return i - preCopy; // 返回实际有效的长度值
        }
        multiplier *= 128;
        if (multiplier > 128 * 128 * 128)
            break;
    } while ((m_pLengthBuf[i - 1] & 128) != 0);
    if (i >= 4 && m_length == 0) {
        LOG_ERROR_EX("无法准确读取到包长, 读取字节数: %d, 长度值为: %u", i, m_length);
        return AMBIGUOUS_PACKAGE_LEN;
    }
    if (m_length > 268435456) {
        LOG_DEBUG_EX("头部字节数: %d, 计算后包长: %u\n", i, m_length);
        return PACKAGE_LEN_EXCEED;
    }
    return i - preCopy;
}

void MQTT::showPackageInfo() const {
    LOG_WRITE_T("");
    LOG_WRITE(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>");
    LOG_WRITE("MQTT数据包详情: ");
    LOG_WRITE_EX("空包: %s", m_empty ? "是" : "否");
    LOG_WRITE_EX("包完整性: %s", m_full ? "完整" : "不完整");
    LOG_WRITE_EX("包类型: %s", getPackageType(m_type));
    LOG_WRITE_EX("剩余长度: %d", m_length);
    LOG_WRITE_EX("控制报文: 0x%2x, [ %d %d %d %d %d %d %d %d ]", m_fixedHead & 0xff, (m_fixedHead >> 7) & 0x1, (m_fixedHead >> 6) & 0x1, (m_fixedHead >> 5) & 0x1, 
    (m_fixedHead >> 4) & 0x1, (m_fixedHead >> 3) & 0x1, (m_fixedHead >> 2) & 0x1, (m_fixedHead >> 1) & 0x1, m_fixedHead & 0x1);
    LOG_WRITE_EX("长度内容: 0x%02x 0x%02x 0x%02x 0x%02x", m_pLengthBuf[0] & 0xff, m_pLengthBuf[1] & 0xff, m_pLengthBuf[2] & 0xff, m_pLengthBuf[3] & 0xff);
    if (m_length > 0) {
        int l = m_length > 20 ? 20 : m_length;
        LOG_WRITE_EX("前%d字节内容: \n    ", l);
        printf("    ");
        for (int i = 0; i < l; ++i) {
            printf("0x%02x ", m_pBuffer[i] & 0xff);
            if (i == 9) {
                printf("\n    ");
            }
        }
        printf("\n");
    }
    LOG_WRITE("<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<\n");
}
const char* MQTT::getPackageType(MQTTType type) const {
    switch (type) {
        case NONE:
            return"NONE (0)"; 
        case CONNECT:
            return"CONNECT (1)"; 
        case CONNACK:
            return"CONNACK (2)"; 
        case PUBLISH:
            return"PUBLISH (3)"; 
        case PUBACK:
            return"PUBACK (4)"; 
        case PUBREC:
            return"PUBREC (5)"; 
        case PUBREL:
            return"PUBREL (6)"; 
        case PUBCOMP:
            return"PUBCOMP (7)"; 
        case SUBSCRIBE:
            return"SUBSCRIBE (8)"; 
        case SUBACK:
            return"SUBACK (9)"; 
        case UNSUBSCRIBE:
            return"UNSUBSCRIBE (10)"; 
        case UNSUBACK:
            return"UNSUBACK (11)"; 
        case PINGREQ:
            return"PINGREQ (12)"; 
        case PINGRESP:
            return"PINGRESP (13)"; 
        case DISCONNECT:
            return"DISCONNECT (14)"; 
        case RETAIN:
            return"RETAIN (15)";
        default:
            return "UNKNOWN";
        break;
    }
}

MQTTType MQTT::type() const {
    return m_type;
}

unsigned int MQTT::length() const {
    return m_length;
}

const char* MQTT::payload() const {
    return m_pBuffer;
}

int MQTT::checkClientId(const char* clientId, unsigned int clientLen) {
    if (clientLen > CLIENT_ID_SIZE) {
        return LEN_VALUE_EXCEED; // 尺寸超限制
    }
    if (0 >= clientLen) {
        return INVALID_LEN_VAL; // 尺寸为0 或负数
    }
    for (int i = 0; i < clientLen; ++i) {
        if (!((*(clientId + i) >= 0x30 && *(clientId + i) <= 0x39 ) 
        || (*(clientId + i) >= 0x41 && *(clientId + i) <= 0x5a)
        || (*(clientId + i) >= 0x61 && *(clientId + i) <= 0x7a)
        || *(clientId + i) == 0x5f || *(clientId + i) == 0x2d)) { 
            return VAL_FORMAT_ERR; // ClientID 不符合[A-Za-z0-9\_\-], 无效
        }
    }
}
int MQTT::checkTopic(const char* topic, unsigned int topicLen) {
    if (topicLen > CLIENT_ID_SIZE) {
        return LEN_VALUE_EXCEED; // 尺寸超限制
    }
    if (0 >= topicLen) {
        return INVALID_LEN_VAL; // 尺寸为0 或负数
    }
    for (int i = 0; i < topicLen; ++i) {
        if (!((*(topic + i) >= 0x30 && *(topic + i) <= 0x39 ) 
        || (*(topic + i) >= 0x41 && *(topic + i) <= 0x5a)
        || (*(topic + i) >= 0x61 && *(topic + i) <= 0x7a)
        || *(topic + i) == 0x5f || *(topic + i) == 0x2d || *(topic + i) == 0x2f)) { 
            return VAL_FORMAT_ERR; // topic 不符合[A-Za-z0-9\_\-\/], 无效
        }
    }
}