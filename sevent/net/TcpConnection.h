#ifndef SEVENT_NET_TCPCONNECTION_H
#define SEVENT_NET_TCPCONNECTION_H

#include "sevent/base/Timestamp.h"
#include "sevent/net/Buffer.h"
#include "sevent/net/Channel.h"
#include "sevent/net/InetAddress.h"
#include <stdint.h>
#include <any>
#include <memory>
#include <string>
#include <unordered_map>
namespace sevent {
namespace net {

class EventLoop;
class TcpHandler;
class TcpClient;
class Connector;
class TcpServer;
class TcpConnectionHolder;

// TcpConnectionPtr会被哪些持有?
// 1.TcpServer::connections
// 2.TcpConnection部分函数执行queueInLoop时,EventLoop任务队列会持有(shared_from_this())
// 3.TcpHandler执行函数时(shared_from_this())
// 4.用户自己持有
class TcpConnection : private Channel,
                      public std::enable_shared_from_this<TcpConnection> {
public:
    using ptr = std::shared_ptr<TcpConnection>;
    TcpConnection(EventLoop *loop, socket_t sockfd, int64_t connId,
                  const InetAddress &localAddr, const InetAddress &peerAddr);
    ~TcpConnection();
    // thread safe
    void send(const void *data, size_t len);
    void send(const std::string &data); 
    void send(const std::string &&data); // std::move(data)
    void send(Buffer *buf);
    void send(Buffer &buf); 
    void send(Buffer &&buf); // Buffer::swap
    void sendOutputBuf(); // 发送outputBuf的数据
    // void send(FILE *fp); // TODO sendfile
    void shutdown(); // 设置状态disconnecting, 若outputBuf存在数据, 则发送完毕后, 才shutdown(WR)
    void forceClose(); // 调用close, 有可能会丢失数据
    void forceClose(int64_t delayMs); // 延迟ms后, forceClose
    void enableRead();
    void disableRead();
    void enableWrite(); // 可以通过enableWrite, 发送outBuf里面的数据
    void disableWrite();
    bool isReading() const { return isRead; }
    bool isConnected() const { return state & connected; }

    // FIXME: context/setxx 方法是非线程安全
    std::unordered_map<std::string, std::any> &getContext() { return context; }
    std::any &getContext(const std::string &key) { return context[key]; }
    void setContext(const std::string &key, std::any &value) { context[key] = value;}
    void setContext(const std::string &key, std::any &&value) { context[key] = std::move(value);}
    void removeContext(const std::string &key) { context.erase(key); } 
    void setTcpNoDelay(bool on);
    int setSockOpt(int level, int optname, const void *optval, socklen_t optlen); 
    int getsockopt(int level, int optname, void *optval, socklen_t *optlen);
    void setHighWaterMark(size_t bytes) { hightWaterMark = bytes; }
    socket_t getsockfd() { return fd; }
    int64_t getId() { return id; }
    // 获取poll/epoll_wait等返回时的时刻
    Timestamp getPollTime();
    EventLoop *getLoop() { return getOwnerLoop(); }
    const InetAddress &getLocalAddr() const { return localAddr; }
    const InetAddress &getPeerAddr() const { return peerAddr; }

    Buffer *getInputBuf() { return &inputBuf; }
    Buffer *getOutputBuf() { return &outputBuf; }
    // 非线程安全, 应该在创建TcpConnectin时调用或在ownerThread中调用
    void setTcpHandler(TcpHandler *handler) { tcpHandler = handler; }

private:
    enum State { connecting, connected, disconnecting, disconnected };
    void setTcpState(State s) { state = s; }
    void handleRead() override;
    void handleWrite() override;
    void handleClose() override;
    void handleError() override;

    void sendInLoop(const void *data, size_t len);
    void sendInLoopStr(const std::string &data);
    void sendInLoopBuf(Buffer &buf);
    void sendOutputBufInLoop();
    void shutdownInLoop();
    void forceCloseInLoop();
    void enableReadInLoop();
    void disableReadInLoop();
    void enableWriteInLoop();
    void disableWriteInLoop();
    void dealErr(bool *isErr);
    void tie() {}
    // for TcpServer
    friend class TcpServer;
    // friend class TcpClient;
    friend class Connector;
    // for Acceptor::handleRead -> TcpServer::onConnection -> onConnection
    void onConnection();
    void setTcpHolder(TcpConnectionHolder *holder) { tcpHolder = holder; }
    void removeItself();

private:
    bool isRead;
    bool isWrite;
    int64_t id;
    State state;
    TcpConnectionHolder *tcpHolder;
    TcpHandler *tcpHandler;
    const InetAddress localAddr;
    const InetAddress peerAddr;
    Buffer inputBuf;
    Buffer outputBuf;
    size_t hightWaterMark; // 默认: 64 * 1024 * 1024
    std::unordered_map<std::string, std::any> context; // std::any C++17
};

} // namespace net
} // namespace sevent

#endif