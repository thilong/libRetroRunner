//
// Created by aidoo on 2024/11/1.
//

#ifndef _THREADSAFE_QUEUE_HPP
#define _THREADSAFE_QUEUE_HPP

#include <mutex>
#include <queue>

template<typename T>
class threadsafe_queue {
private:
    mutable std::mutex mut;
    using queue_type = std::queue<T>;
    queue_type data_queue;
public:
    using value_type = typename queue_type::value_type;
    using container_type = typename queue_type::container_type;

    threadsafe_queue() = default;

    threadsafe_queue(const threadsafe_queue &) = delete;

    threadsafe_queue &operator=(const threadsafe_queue &) = delete;

    template<typename _InputIterator>
    threadsafe_queue(_InputIterator first, _InputIterator last) {
        for (auto itor = first; itor != last; ++itor) {
            data_queue.push(*itor);
        }
    }

    explicit threadsafe_queue(const container_type &c) : data_queue(c) {}

    threadsafe_queue(std::initializer_list<value_type> list) : threadsafe_queue(list.begin(),
                                                                                list.end()) {
    }


    void push(const value_type &new_value) {
        std::lock_guard<std::mutex> lk(mut);
        data_queue.push(std::move(new_value));
    }

    bool try_pop(value_type &value) {
        std::lock_guard<std::mutex> lk(mut);
        if (data_queue.empty())
            return false;
        value = std::move(data_queue.front());
        data_queue.pop();
        return true;
    }

    auto empty() const -> decltype(data_queue.empty()) {
        std::lock_guard<std::mutex> lk(mut);
        return data_queue.empty();
    }

    auto size() const -> decltype(data_queue.size()) {
        std::lock_guard<std::mutex> lk(mut);
        return data_queue.size();
    }
};

#endif
