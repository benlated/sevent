#include "sevent/net/EventLoop.h"

#include "sevent/base/CurrentThread.h"
#include "sevent/base/Logger.h"
#include "sevent/net/Channel.h"
#include "sevent/net/Poller.h"
#include "sevent/net/TimerManager.h"
#include "sevent/net/WakeupChannel.h"
#include <assert.h>
#include <vector>

using namespace std;
using namespace sevent;
using namespace sevent::net;

thread_local EventLoop *EventLoop::threadEvLoop = nullptr;

EventLoop::EventLoop(const std::string &name)
    : isTasking(false), isQuit(false), threadId(CurrentThread::gettid()),
      timeout(pollTimeout), poller(Poller::newPoller()), 
      wakeupChannel(new WakeupChannel(this)), 
      timerManager(new TimerManager(this, wakeupChannel)), loopName(name) {
    if (!threadEvLoop)
        threadEvLoop = this;
    else
        LOG_FATAL << "EventLoop - " << threadEvLoop->loopName
                  << " already exists in this thread " << threadId
                  << ", create" << loopName <<" failed";
}

EventLoop::~EventLoop() { 
    LOG_TRACE << "EventLoop::~EventLoop() - " << loopName << " of thread " << threadId
            << " destructs in thread " << CurrentThread::gettid();
    threadEvLoop = nullptr; 
}

void EventLoop::loop() { 
    assertInOwnerThread();
    LOG_INFO << "EventLoop::loop() - start looping, " << loopName << " = " << this;
    while (!isQuit) {
        #ifdef _WIN32
        timeout = timerManager->getNextTimeout();
        #endif
        poller->poll(timeout);
        #ifdef _WIN32
        timerManager->handleExpired();
        #endif
        vector<Channel *> &activeChannels = poller->getActiveChannels();
        for (Channel *channel : activeChannels) {
            channel->handleEvent();
        }
        doPendingTasks();
    }
    LOG_INFO << "EventLoop::loop() - stop looping, " << loopName << " = " << this;
}

void EventLoop::quit() { 
    isQuit = true; 
    if (!isInOwnerThread())
        wakeup();
}
void EventLoop::updateChannel(Channel *channel) {
    assertInOwnerThread();
    poller->updateChannel(channel);
}
void EventLoop::removeChannel(Channel *channel) {
    assert(channel->getOwnerLoop() == this);
    assertInOwnerThread();
    poller->removeChannel(channel);
}

void EventLoop::runInLoop(function<void()> cb) {
    if (isInOwnerThread())
        cb();
    else
        queueInLoop(std::move(cb));
}

void EventLoop::queueInLoop(function<void()> cb) {
    {
        lock_guard<mutex> lg(mtx);
        pendingTasks.push_back(std::move(cb));
    }
    // ownerLoop????????????????????????(isTasking),??????????????????,??????wakeup;???????????????????????????,?????????wakeup
    // isTasking???????????????????????????,????????????
    if (!isInOwnerThread() || isTasking)
        wakeup();
}
void EventLoop::wakeup() { wakeupChannel->wakeup(); }
void EventLoop::doPendingTasks() {
    // ??????swap on write,??????pendingTasks:
    // 1.???????????????????????????,??????queueInLoop????????????
    // 2.?????????????????????
    vector<function<void()>> tmpTasks;
    isTasking = true;
    {
        lock_guard<mutex> lg(mtx);
        tmpTasks.swap(pendingTasks);
    }
    for (function<void()> &cb : tmpTasks) {
        cb();
    }
    isTasking = false;
}
bool EventLoop::isInOwnerThread() const {
    return threadId == CurrentThread::gettid();
}
void EventLoop::assertInOwnerThread() const {
    if (!isInOwnerThread()) {
        LOG_FATAL << "EventLoop::assertInOwnThread() - "<< loopName <<":" << this
                  << " was belonged to threadId: " << threadId
                  << ", current threadId: " << CurrentThread::gettid();
    }
}
void EventLoop::assertInOwnerThread(const std::string &msg) const {
    if (!isInOwnerThread()) {
        LOG_FATAL << "EventLoop::assertInOwnThread() - "<< loopName <<":" << this
                  << " was belonged to threadId: " << threadId
                  << ", current threadId: " << CurrentThread::gettid() << ", " << msg;
    }    
}
// Timestamp::now, microseconds since the Epoch, 1970-01-01 00:00:00 +0000 (UTC)
TimerId EventLoop::addTimer(Timestamp time, function<void()> cb, int64_t interval) {
    
    return timerManager->addTimer(std::move(cb), time, interval);
}
// ???millisecond????????????, ??????interval??????????????????(0????????????)
TimerId EventLoop::addTimer(int64_t millisecond, std::function<void()> cb, int64_t interval) {
    Timestamp time(Timestamp::addTime(Timestamp::now(), millisecond));
    return timerManager->addTimer(std::move(cb), time, interval);
}
void EventLoop::cancelTimer(TimerId timerId) {
    timerManager->cancel(std::move(timerId));
}

Timestamp EventLoop::getPollTime() const { return poller->getPollTime(); }