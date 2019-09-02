#ifndef __ERROR_H__
#define __ERROR_H__

#define SUCCESS (0)
#define NULL_PACKAGE (-1)  // 空数据包
#define ERROR_PACKAGE_LEN (-2) // 错误的包长
#define FULL_PACKAGE (-3)  // 满数据包
#define AMBIGUOUS_PACKAGE_LEN (-4) // 无法准确解析出包长
#define PACKAGE_LEN_EXCEED (-5) // 包长超出限制
#define NULL_BUFFER (-6)  // Buffer 为空
#define INVALID_DATA (-7)  // 无效的数据
#define LEN_VALUE_EXCEED (-8) // 尺寸超限制
#define INVALID_LEN_VAL (-9)  // 尺寸值异常
#define VAL_FORMAT_ERR (-10)  // 格式校验失败
#define MQTT_FIXED_ERR  (-11)  // MQTT 头部固定值异常
#define MQTT_PROTO_LV_ERR (-12) // MQTT 协议级别出错
#define MQTT_CONN_FLAG_ERR (-13) // MQTT 连接标志出错
#define MQTT_USER_PWD_NOT_MATCH (-14) // 用户名和密码标志不匹配
#define MQTT_WILL_FLAG_QOS_NOT_MATCH (-15) // 遗嘱标志和 遗嘱保留/QOS 标志值不匹配
#define MQTT_INVALID_WILL_QOS (-16) // 无效的遗嘱QOS
#define MQTT_USER_PWD_NOT_NULL (-17) // 用户名和密码不能为空
#define MQTT_USER_PWD_INVALID (-18)  // 用户名和密码校验失败

inline const char* getErrStr(int code) {
    switch (code) {
        case SUCCESS:
            return "调用成功";
        case NULL_PACKAGE:
            return "空数据包";
        case ERROR_PACKAGE_LEN:
            return "错误的包长";
        case FULL_PACKAGE:
            return "满数据包";
        case AMBIGUOUS_PACKAGE_LEN:
            return "无法准确解析出包长";
        case PACKAGE_LEN_EXCEED:
            return "包长超出限制";
        case NULL_BUFFER:
            return "Buffer 为空";
        case INVALID_DATA:
            return "无效的数据";
        case LEN_VALUE_EXCEED:
            return "尺寸超限制";
        case INVALID_LEN_VAL:
            return "尺寸值异常";
        case VAL_FORMAT_ERR:
            return "格式校验失败";
        case MQTT_FIXED_ERR:
            return "MQTT 头部固定值异常";
        case MQTT_PROTO_LV_ERR:
            return "MQTT 协议级别出错";
        case MQTT_CONN_FLAG_ERR:
            return "MQTT 连接标志出错";
        case MQTT_USER_PWD_NOT_MATCH:
            return "用户名和密码标志不匹配";
        case MQTT_WILL_FLAG_QOS_NOT_MATCH:
            return "遗嘱标志和 遗嘱保留/QOS 标志值不匹配";
        case MQTT_INVALID_WILL_QOS:
            return "无效的遗嘱QOS";
        case MQTT_USER_PWD_NOT_NULL:
            return "用户名和密码不能为空";
        case MQTT_USER_PWD_INVALID:
            return "用户名和密码校验失败";
        default:
            return "未知错误";
    }
}

#endif // __ERROR_H__