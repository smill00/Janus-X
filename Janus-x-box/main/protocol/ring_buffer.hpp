#ifndef RING_BUFFER_HPP
#define RING_BUFFER_HPP

#include <vector>
#include <mutex>
#include <condition_variable>

template<typename T>
class RingBuffer {
public:
    // 构造函数
    explicit RingBuffer(size_t capacity) 
        : buffer_(capacity)
        , capacity_(capacity)
        , read_idx_(0)
        , write_idx_(0)
        , count_(0) {
    }
    
    // 阻塞push，当缓冲区满时等待
    void push(const T& item) {
        std::unique_lock<std::mutex> lock(mutex_);
        
        // 等待缓冲区有空位
        not_full_.wait(lock, [this]() { 
            return count_ < capacity_; 
        });
        
        // 存入数据
        buffer_[write_idx_] = item;
        write_idx_ = (write_idx_ + 1) % capacity_;
        ++count_;
        
        // 通知可能等待的pull操作
        not_empty_.notify_one();
    }
    
    // 阻塞pull，当缓冲区空时等待
    T pull() {
        std::unique_lock<std::mutex> lock(mutex_);
        
        // 等待缓冲区有数据
        not_empty_.wait(lock, [this]() { 
            return count_ > 0; 
        });
        
        // 取出数据
        T item = std::move(buffer_[read_idx_]);
        read_idx_ = (read_idx_ + 1) % capacity_;
        --count_;
        
        // 通知可能等待的push操作
        not_full_.notify_one();
        
        return item;
    }
    
    // 重置缓冲区
    void reset() {
        std::unique_lock<std::mutex> lock(mutex_);
        
        read_idx_ = 0;
        write_idx_ = 0;
        count_ = 0;
        
        // 清空缓冲区内容
        std::fill(buffer_.begin(), buffer_.end(), T());
        
        // 唤醒所有等待的线程
        not_full_.notify_all();
        not_empty_.notify_all();
    }
    
    // 可选：获取当前缓冲区中元素数量
    size_t size() const {
        std::unique_lock<std::mutex> lock(mutex_);
        return count_;
    }
    
    // 可选：检查缓冲区是否为空
    bool empty() const {
        std::unique_lock<std::mutex> lock(mutex_);
        return count_ == 0;
    }
    
    // 可选：检查缓冲区是否已满
    bool full() const {
        std::unique_lock<std::mutex> lock(mutex_);
        return count_ == capacity_;
    }
    
    // 可选：获取缓冲区容量
    size_t capacity() const {
        return capacity_;
    }

private:
    std::vector<T> buffer_;              // 存储容器
    const size_t capacity_;             // 缓冲区容量
    size_t read_idx_;                    // 读位置
    size_t write_idx_;                   // 写位置
    size_t count_;                       // 当前元素数量
    
    mutable std::mutex mutex_;           // 互斥锁
    std::condition_variable not_full_;   // 缓冲区不满条件
    std::condition_variable not_empty_;  // 缓冲区不空条件
};

#endif // RING_BUFFER_HPP