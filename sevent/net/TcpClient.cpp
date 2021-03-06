#include "sevent/net/TcpClient.h"

#include "sevent/base/Logger.h"
#include "sevent/net/Connector.h"
#include "sevent/net/EventLoop.h"
#include "sevent/net/InetAddress.h"

using namespace std;
using namespace sevent;
using namespace sevent::net;
using std::placeholders::_1;

TcpClient::TcpClient(EventLoop *loop, const InetAddress &addr)
    : started(false), ownerLoop(loop), connector(createConnector(addr)) {}
TcpClient::~TcpClient() {
    LOG_TRACE << "TcpClient::~TcpClient()";
    connector->stop();
}
Connector *TcpClient::createConnector(const InetAddress &addr) {
    return new Connector(ownerLoop, addr);
}

void TcpClient::setTcpHandler(TcpHandler *handler) {
    connector->setTcpHandler(handler);
}

void TcpClient::connect() {
    if (!started.exchange(true)) {
        LOG_TRACE << "TcpClient::connect() - connecting to "
                  << connector->getServerAddr().toStringIpPort();
        ownerLoop->runInLoop(std::bind(&Connector::connect, connector));
    }
}
// 关闭连接, 包括已经连接或正在连接
void TcpClient::shutdown() {
    started = false;
    connector->shutdown();
}

void TcpClient::forceClose() {
    started = false;
    connector->forceClose();
}

const InetAddress &TcpClient::getServerAddr() const {
    return connector->getServerAddr();
}

void TcpClient::setRetryCount(int count) { connector->setRetryCount(count); }
void TcpClient::setTimeout(int64_t millisecond, std::function<void()> cb) {
    connector->setTimeout(millisecond, std::move(cb));
}