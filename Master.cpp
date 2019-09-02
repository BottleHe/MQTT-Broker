#include "define.h"
#include "Master.h"
#include <assert.h>
#include "bt_utils.h"
#include <iostream>
#include "logger.h"
// static function
void Master::signalCallBack(evutil_socket_t signal, short what, void* arg) {
    LOG_DEBUG_EX("Got an event on signal %d, %s%s%s%s", (int) signal, 
        (what & EV_TIMEOUT) ? " | timeout" : "",
        (what & EV_READ)    ? " | read" : "",
        (what & EV_WRITE)   ? " | write" : "",
        (what & EV_SIGNAL)  ? " | signal" : "");
    Master* _this = (Master*)arg;
    switch (signal) {
        case SIGINT: {
            _this->breakLoop(0);
            break;
        }
        case SIGQUIT: {
            struct timeval tv = {0, 0};
            _this->releaseAndExit(&tv);
            break;
        }
        default:
            break;
    }
}
void Master::timerCallBack(evutil_socket_t fd, short what, void* arg) {
    LOG_DEBUG_EX_T("Got an event on timerout %d, %s%s%s%s", (int) fd, 
        (what & EV_TIMEOUT) ? " | timeout" : "",
        (what & EV_READ)    ? " | read" : "",
        (what & EV_WRITE)   ? " | write" : "",
        (what & EV_SIGNAL)  ? " | signal" : "");
    Master* _this = (Master*)arg;
    if (_this->m_clientMap.size() > 0) {
        LOG_DEBUG("现有客户端如下: ");
        for (const std::pair<int, Client*> &pair : _this->m_clientMap) {
            // std::cout << "    " << pair.first << " => " << pair.second->fd() << std::endl;
            LOG_DEBUG_EX("%d", pair.second->fd());
            // 检查是否连接
            if (pair.second->connectTimestamp() <= 0) { // 如果未登录, 检查距离现在多长时间了
                time_t now = time(NULL);
                if (now - pair.second->linkedTimestamp() >= CLIENT_CONNECT_TIMEOUT) { // 如果连接上超过30秒未连接, 则认为是非法连接, 应直接断开
                    LOG_DEBUG_EX("连接FD(%d) 超时, 断开连接", pair.second->fd());
                    Client* _tmp = pair.second;
                    _this->closeAndReleaseClient(&_tmp); // 断开
                }
            } else {
                // TODO 检查超时时间
                if (0 >= pair.second->keepAlive()) {  // 如果没有keepAlive 直接过滤
                    continue; // 跳过
                }
                time_t now = time(NULL);
                // 检查超时时间
                if (now - pair.second->lastMsgTimestamp() > pair.second->keepAlive()) {
                    LOG_DEBUG_EX("连接FD(%d) 最后消息时间: %d, 超过 keepAlive: %d, 断开连接", pair.second->fd(), pair.second->lastMsgTimestamp(), pair.second->keepAlive());
                    Client* _tmp = pair.second;
                    _this->closeAndReleaseClient(&_tmp); // 断开
                }
            }
        }
    }
}

void Master::acceptCallBack(evutil_socket_t fd, short what, void* arg) {
    LOG_DEBUG_EX("Got an event on socket accept %d, %s%s%s%s", (int) fd, 
        (what & EV_TIMEOUT) ? " | timeout" : "",
        (what & EV_READ)    ? " | read" : "",
        (what & EV_WRITE)   ? " | write" : "",
        (what & EV_SIGNAL)  ? " | signal" : "");
    Master* _this = (Master*)arg;
    struct sockaddr addr;
    socklen_t len = 0;
    int i = 0;
    extern int errno;
    for (;;) { // 因为是非阻塞边缘模式, 需要将可读资源全部处理完
        // printf("Accept times: %d\n", i++);
        int cfd = accept(fd, &addr, &len);
        if (-1 == cfd) {
            if (EAGAIN == errno) { // 表示暂时无可读数据了
                break;
            }
            // fprintf(stderr, "Accept(%d): %s\n", errno, strerror(errno));
            LOG_ERROR_EX("Accept(%d): %s", errno, strerror(errno));
            return;
        }
        LOG_DEBUG_EX("Accept FD(%d)", cfd);
        setNonBlock(cfd);
        Client* pClient = new Client(cfd, _this);
        pClient->linkedTimestamp(time(NULL));
        _this->m_clientMap[cfd] = pClient;
        pClient->rdEvt(event_new(_this->m_pEvtBase, cfd, EV_READ | EV_PERSIST | EV_ET, Client::onMessage, pClient));
        event_add(pClient->rdEvt(), NULL);
        
        LOG_DEBUG_EX("事件指针为: 0x%016p", pClient->rdEvt());
    }
}

Master::Master() : m_pEvtBase(NULL) {
}

Master::~Master() {
    LOG_DEBUG("[Destructor]: Master::~Master();");
    if (!!m_pEvtBase) {
        event_base_free(m_pEvtBase);
        m_pEvtBase = NULL;
    }
    if (m_sFd > 0) {
        close(m_sFd);
        m_sFd = 0;
    }
    std::map<int, Client*>::iterator it;
    for (it = m_clientMap.begin(); it != m_clientMap.end(); ++it) {
        close(it->second->fd());
        delete it->second;
        m_clientMap.erase(it);
    }
}

void Master::Startup() {
    m_sFd = socket(AF_INET, SOCK_STREAM, 0);
    int ret = 0;
    assert(m_sFd > 0);
    setNonBlock(m_sFd);
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(7010);
    addr.sin_addr.s_addr = INADDR_ANY;
    ret = bind(m_sFd, (struct sockaddr*)&addr, sizeof(struct sockaddr));
    assert(ret != -1);
    ret = listen(m_sFd, 10); /* backlog */
    assert(ret != -1);

    m_pEvtBase = event_base_new();
    struct event* acceptEvt = event_new(m_pEvtBase, m_sFd, EV_READ | EV_PERSIST | EV_ET, Master::acceptCallBack, this);
    event_add(acceptEvt, NULL);

    struct event* sigIntEvt = event_new(m_pEvtBase, SIGINT, EV_SIGNAL | EV_PERSIST | EV_ET, Master::signalCallBack, this);
    event_add(sigIntEvt, NULL);
    struct event* sigQuitEvt = event_new(m_pEvtBase, SIGQUIT, EV_SIGNAL | EV_PERSIST | EV_ET, Master::signalCallBack, this);
    event_add(sigQuitEvt, NULL);

    struct timeval timeout = {5, 0}; // TODO check timeout
    struct event* timerEvt = event_new(m_pEvtBase, -1, EV_TIMEOUT | EV_PERSIST, Master::timerCallBack, this);
    event_add(timerEvt, &timeout);

    event_base_dispatch(m_pEvtBase);
}

void Master::releaseAndExit(const struct timeval* tv, int exitCode) {
    m_exitCode = exitCode;
    event_base_loopexit(m_pEvtBase, tv);
}

void Master::breakLoop(int exitCode) {
    m_exitCode = exitCode;
    event_base_loopbreak(m_pEvtBase);
}

void Master::closeAndReleaseClient(Client** client) {
    // 关闭socket
    close((*client)->fd());
    // release 资源
    m_clientMap.erase((*client)->fd());
    delete(*client);
    *client = NULL;
}