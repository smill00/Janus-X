//
// Created by void on 2026/3/22.
//

#ifndef CBLOCKQUEUE_H
#define CBLOCKQUEUE_H

#include <queue>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <memory>
#include <chrono>

template<typename T>
class BlockingQueue {
public:
    BlockingQueue() = default;

    // 禁止拷贝和赋值
    BlockingQueue(const BlockingQueue&) = delete;
    BlockingQueue& operator=(const BlockingQueue&) = delete;

    /**
     * 将元素放入队列尾部
     * @param value 要添加的元素
     */
    void push(const T& value) {
        std::unique_lock<std::mutex> lock(mutex_);
        queue_.push(value);
        lock.unlock();
        cond_.notify_one();
    }

    /**
     * 将元素放入队列尾部（移动语义版本）
     * @param value 要添加的元素
     */
    void push(T&& value) {
        std::unique_lock<std::mutex> lock(mutex_);
        queue_.push(std::move(value));
        lock.unlock();
        cond_.notify_one();
    }

    /**
     * 从队列头部取出元素（阻塞直到有元素可用）
     * @return 取出的元素
     */
    T pop() {
        std::unique_lock<std::mutex> lock(mutex_);
        cond_.wait(lock, [this]() { return !queue_.empty(); });
        T value = std::move(queue_.front());
        queue_.pop();
        return value;
    }

    /**
     * 尝试从队列头部取出元素（非阻塞）
     * @param value 接收取出的元素
     * @return 是否成功取出元素
     */
    bool try_pop(T& value) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (queue_.empty()) {
            return false;
        }
        value = std::move(queue_.front());
        queue_.pop();
        return true;
    }

    /**
     * 从队列头部取出元素（带超时）
     * @param value 接收取出的元素
     * @param timeout 超时时间（毫秒）
     * @return 是否成功取出元素
     */
    bool pop_with_timeout(T& value, int timeout_ms) {
        std::unique_lock<std::mutex> lock(mutex_);

        if (!cond_.wait_for(lock, std::chrono::milliseconds(timeout_ms),
                           [this]() { return !queue_.empty(); })) {
            return false;
        }

        value = std::move(queue_.front());
        queue_.pop();
        return true;
    }

    /**
     * 获取队列大小
     * @return 队列中元素的数量
     */
    size_t size() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.size();
    }

    /**
     * 检查队列是否为空
     * @return 队列是否为空
     */
    bool empty() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.empty();
    }

    /**
     * 清空队列
     */
    void clear() {
        std::lock_guard<std::mutex> lock(mutex_);
        while (!queue_.empty()) {
            queue_.pop();
        }
    }

private:
    mutable std::mutex mutex_;               // 互斥锁
    std::condition_variable cond_;          // 条件变量
    std::queue<T> queue_;                   // 底层队列
};


#endif //CBLOCKQUEUE_H
