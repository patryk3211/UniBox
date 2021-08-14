#pragma once

#include <mutex>
#include <condition_variable>

namespace unibox {
    class Semaphore {
        std::mutex mtx;
        std::condition_variable cv;
        int count;
    public:
        Semaphore() {
            count = 0;
        }

        void notify() {
            std::unique_lock<std::mutex> lock(mtx);
            count++;
            cv.notify_one();
        }

        void wait() {
            std::unique_lock<std::mutex> lock(mtx);

            while(count == 0){
                cv.wait(lock);
            }
            count--;
        }
    };
}