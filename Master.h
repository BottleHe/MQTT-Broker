#ifndef __MASTER_H__
#define __MASTER_H__
#include <event.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/types.h>
#include <inttypes.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <map>
#include <signal.h>
#include "Client.h"
class Master {
public:
    Master();
    ~Master();
    void Startup();
    void releaseAndExit(const struct timeval* tv, int exitCode = 0);
    void breakLoop(int exitCode = 0);
    void closeAndReleaseClient(Client**);

    static void acceptCallBack(evutil_socket_t fd, short what, void* arg);
    static void signalCallBack(evutil_socket_t, short, void*);
    static void timerCallBack(evutil_socket_t, short, void*);

    std::map<int, Client*> m_clientMap;
private:
    struct event_base* m_pEvtBase;
    int m_sFd;
    int m_exitCode;
};
#endif // __MASTER_H__