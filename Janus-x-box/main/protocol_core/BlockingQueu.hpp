//
// Created by panshiquan on 2026/3/20.
//

#ifndef MYPROJECT_BLOCKINGQUEUE_H
#define MYPROJECT_BLOCKINGQUEUE_H

#include <queue>
#include <mutex>
#include <condition_variable>
#include <optional>

template<typename T>
class BlockingQueue {
public:
    void push(const T &value) {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            queue_.push(value); // 拷贝
        }
        not_empty_.notify_one();
    }

    T pop() {
        std::unique_lock<std::mutex> lock(mutex_);
        not_empty_.wait(lock, [this]() { return !queue_.empty(); });

        T value = queue_.front(); // 拷贝
        queue_.pop();
        return value; // 返回拷贝
    }

    std::optional<T> try_pop() {
        std::lock_guard<std::mutex> lock(mutex_);
        if (queue_.empty()) {
            return std::nullopt;
        }

        T value = queue_.front(); // 拷贝
        queue_.pop();
        return value;
    }

private:
    std::queue<T> queue_;
    std::mutex mutex_;
    std::condition_variable not_empty_;
};

#endif //MYPROJECT_BLOCKINGQUEUE_H
