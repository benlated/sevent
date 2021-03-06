#ifndef SEVENT_BASE_COUNTDOWNLATCH_H
#define SEVENT_BASE_COUNTDOWNLATCH_H

#include "sevent/base/noncopyable.h"
#include <mutex>
#include <condition_variable>

namespace sevent{

class CountDownLatch : noncopyable {
public:
    explicit CountDownLatch(int count);
    void wait();
    void countDown();
    int getCount();

private:
    std::mutex mtx;
    std::condition_variable cond;
    int count;
};

} // namespace sevent

#endif