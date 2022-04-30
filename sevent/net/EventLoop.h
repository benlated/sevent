#ifndef SEVENT_NET_EVENTLOOP_H
#define SEVENT_NET_EVENTLOOP_H

#include "../base/noncopyable.h"
#include "../base/Timestamp.h"
#include "TimerId.h"

#include <atomic>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace sevent {
namespace net {
class Channel;
class Poller;
class TimerManager;
class WakeupChannel;

class EventLoop : noncopyable {
public:
    EventLoop(const std::string &name = "bossLoop");
    ~EventLoop();

    void loop();
    void quit();
    // 必须确保在EventLoop所在ownerThread调用
    void updateChannel(Channel *channel);
    void removeChannel(Channel *channel);

    bool isInOwnerThread() const;
    void assertInOwnerThread() const;
    void assertInOwnerThread(const std::string &msg) const;

    // 若在ownerThread中,则直接执行回调;
    // 否则queueInLoop:加入ownerThread的任务队列(线程安全,下一次任务循环中执行);
    void runInLoop(std::function<void()> cb);
    // 加入任务队列:若在非ownerThread调用或ownerThread正在执行任务循环,则还要wakeup该ownerLoop(下一次任务循环)
    void queueInLoop(std::function<void()> cb);

    // Timestamp::now, microseconds since the Epoch, 1970-01-01 00:00:00 +0000 (UTC)
    // 在time时间运行, 间隔interval秒再次运行(0运行一次)
    TimerId addTimer(Timestamp time, std::function<void()> cb, double interval = 0.0);
    // 在second秒后运行(0立即运行), 间隔interval秒再次运行(0运行一次) (0.1秒->microsecond)
    TimerId addTimer(double second, std::function<void()> cb, double interval = 0.0);
    void cancelTimer(TimerId timerId);

    const std::string getLoopName() { return loopName; }

private:
    void wakeup();
    void doPendingTasks();

private:
    bool isTasking;
    std::atomic<bool> isQuit;
    const pid_t threadId;
    std::unique_ptr<Poller> poller;
    std::unique_ptr<TimerManager> timerManager;
    std::unique_ptr<WakeupChannel> wakeupChannel;
    thread_local static EventLoop *threadEvLoop;

    const int pollTimeout = 10000; // 10秒(10000 millsecond)
    const std::string loopName;

    std::mutex mtx;
    std::vector<std::function<void()>> pendingTasks; // 任务队列
};

} // namespace net
} // namespace sevent

#endif