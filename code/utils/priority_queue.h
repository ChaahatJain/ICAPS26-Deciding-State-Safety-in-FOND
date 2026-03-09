//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2022) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_PRIORITY_QUEUE_H
#define PLAJA_PRIORITY_QUEUE_H

/**
 * The classes in this file adapt from Fast Downward (https://github.com/aibasel/downward),
 * specifically https://github.com/aibasel/downward/blob/5f9d2489542f06fc45cfbd1633e95564b3ebe349/src/search/algorithms/priority_queues.h
 * and https://github.com/aibasel/downward/blob/5f9d2489542f06fc45cfbd1633e95564b3ebe349/src/search/open_lists/best_first_open_list.cc
 * (January 2022).
 */

#include <list>
#include <map>
#include <queue>
#include <vector>
#include "rng.h"


namespace PRIORITY_QUEUE {

    // Ordering:

    template<typename Priority_t>
    class Compare {
    protected:
        Compare() = default;
    public:
        virtual ~Compare() = default;
        virtual bool operator()(const Priority_t& lhs, const Priority_t& rhs) const = 0;
    };

    template<typename Priority_t, bool ASC = true>
    class CompareNumeric final: public Compare<Priority_t> {
    public:
        CompareNumeric() = default;
        ~CompareNumeric() final = default;

        bool operator()(const Priority_t& lhs, const Priority_t& rhs) const final { if constexpr(ASC) { return lhs < rhs; } else { return lhs > rhs; } }

        inline static bool is_valid(const Priority_t& priority) { return priority <= std::numeric_limits<Priority_t>::max(); }
    };

    template<typename Priority1st_t, typename Priority2nd_t, bool FIRST_SECOND = true, bool ASC_1ST = true, bool ASC_2ND = true>
    class CompareNumericPair final: public Compare<std::pair<Priority1st_t, Priority2nd_t>> {
    public:
        using Priority_t = std::pair<Priority1st_t, Priority2nd_t>;
        CompareNumericPair() = default;
        ~CompareNumericPair() final = default;

        bool operator()(const Priority_t& lhs, const Priority_t& rhs) const final {
            if constexpr (FIRST_SECOND) {
                if (lhs.first == rhs.first) {
                    if constexpr (ASC_2ND) { return lhs.second < rhs.second; } else { return lhs.second > rhs.second; }
                }
                if constexpr(ASC_1ST) { return lhs.first < rhs.first; } else { return lhs.first > rhs.first; }
            } else {
                if (lhs.second == rhs.second) {
                    if constexpr (ASC_1ST) { return lhs.first < rhs.first; } else { return lhs.first > rhs.first; }
                }
                if constexpr(ASC_2ND) { return lhs.second < rhs.second; } else { return lhs.second > rhs.second; }
            }
        }

        inline static bool is_valid(const Priority_t& priority) { return priority.first <= std::numeric_limits<Priority1st_t>::max() and priority.second <= std::numeric_limits<Priority2nd_t>::max(); }
    };

    template<typename Priority_t, typename Value_t, typename Compare_t = PRIORITY_QUEUE::CompareNumeric<Priority_t>>
    class CompareEntry {
    private:
        Compare_t comparator;
    public:
        using Entry = std::pair<Priority_t, Value_t>;
        CompareEntry() = default;
        ~CompareEntry() = default;
        bool operator()(const Entry& lhs, const Entry& rhs) const { return comparator.operator()(lhs.first, rhs.first); }
        static bool is_valid(const Priority_t& priority) { return Compare_t::is_valid(priority); }
    };

/**********************************************************************************************************************/

    // Tie-breaking bucket for same priority:

    template<typename Value_t>
    class Bucket {
    protected:
        Bucket() = default;
    public:
        virtual ~Bucket() = default;

        virtual void push(Value_t&& value) = 0;
        [[nodiscard]] virtual const Value_t& top() const = 0;
        [[nodiscard]] virtual Value_t pop() = 0;

        [[nodiscard]] virtual std::size_t size() const = 0;
        [[nodiscard]] virtual bool empty() const = 0;
        virtual void clear() = 0;

        [[nodiscard]] virtual std::list<Value_t> to_list() const = 0;
    };

    template<typename Value_t>
    class BucketList: public Bucket<Value_t> {
    protected:
        std::list<Value_t> stack; // under LIFO one can use vector

        BucketList() = default;
    public:
        ~BucketList() override = default;

       inline void push(Value_t&& value) final { stack.push_back( std::move(value) ); }

        [[nodiscard]] inline std::size_t size() const final { return stack.size(); }
        [[nodiscard]] inline bool empty() const final { return stack.empty(); }
        inline void clear() final { stack.clear(); }

        [[nodiscard]] std::list<Value_t> to_list() const final { return stack; }
    };

    template<typename Value_t>
    class BucketFIFO final: public BucketList<Value_t> {
    public:
        BucketFIFO() = default;
        ~BucketFIFO() final = default;
        [[nodiscard]] const Value_t& top() const override { PLAJA_ASSERT(not this->empty()) return this->stack.front(); };
        [[nodiscard]] Value_t pop() override { PLAJA_ASSERT(not this->empty()) auto elem = std::move(this->stack.front()); this->stack.pop_front(); return elem; };
    };


    template<typename Value_t>
    class BucketLIFO final: public BucketList<Value_t> {
    public:
        BucketLIFO() = default;
        ~BucketLIFO() final = default;
        [[nodiscard]] const Value_t& top() const override { PLAJA_ASSERT(not this->empty()) return this->stack.back(); };
        [[nodiscard]] Value_t pop() override { PLAJA_ASSERT(not this->empty()) auto elem = std::move(this->stack.back()); this->stack.pop_back(); return elem; };
    };

    template<typename Value_t>
    class BucketRandom final: public BucketList<Value_t> {
    public:
        BucketRandom() = default;
        ~BucketRandom() final = default;

        [[nodiscard]] const Value_t& top() const override {
            PLAJA_ASSERT(not this->stack.empty())
            int index = PLAJA_GLOBAL::rng->index(this->size());
            auto it = this->stack.begin();
            while (index > 0) { --index; ++it; }
            PLAJA_ASSERT(it != this->stack.end());
            return *it;
        };

        [[nodiscard]] Value_t pop() override {
            PLAJA_ASSERT(not this->stack.empty())
            int index = PLAJA_GLOBAL::rng->index(this->size());
            auto it = this->stack.begin();
            while (index > 0) { --index; ++it; }
            PLAJA_ASSERT(it != this->stack.end());
            auto elem = std::move(*it);
            this->stack.erase(it);
            return elem;
        };
    };

}

/**********************************************************************************************************************/


template<typename Priority_t, typename Value_t>
class PriorityQueue {
public:
    using Entry = std::pair<Priority_t, Value_t>;

    PriorityQueue() = default;
    virtual ~PriorityQueue() = default;

    virtual void push(const Priority_t& priority, Value_t&& value) = 0;
    virtual Entry pop_with_priority() = 0;
    inline Value_t pop() { return pop_with_priority().second; }

    [[nodiscard]] virtual std::size_t size() const = 0;
    [[nodiscard]] virtual bool empty() const = 0;
    virtual void clear() = 0;

    [[nodiscard]] virtual const Value_t& top() const = 0;
    [[nodiscard]] virtual std::list<Value_t> to_list() const = 0;
};

/**********************************************************************************************************************/

template<typename Priority_t, typename Value_t, typename Compare_t = PRIORITY_QUEUE::CompareNumeric<Priority_t, true>, typename Bucket_t = PRIORITY_QUEUE::BucketFIFO<Value_t>>
class MapQueue final: public PriorityQueue<Priority_t, Value_t> {
    using Entry = typename PriorityQueue<Priority_t, Value_t>::Entry;

#if 0
    struct Compare {
        bool operator()(Priority_type priority_left, Priority_type priority_right) const {
            if constexpr(ASC) { return priority_left < priority_right; }
            else { return priority_left > priority_right; }
        }
    };
#endif

    std::map<Priority_t, Bucket_t, Compare_t> buckets;
    std::size_t num_entries;

public:
    MapQueue(): num_entries(0) {}
    ~MapQueue() final = default;

    void push(const Priority_t& priority, Value_t&& value) final {
        PLAJA_ASSERT(Compare_t::is_valid(priority))
        buckets[priority].push(std::move(value));
        ++num_entries;
    }

    Entry pop_with_priority() final {
        PLAJA_ASSERT(num_entries > 0)
        auto it = buckets.begin();
        PLAJA_ASSERT(it != buckets.end())
        auto& bucket = it->second;
        Entry rlt(it->first, bucket.pop());
        if (bucket.empty()) { buckets.erase(it); }
        --num_entries;
        return rlt;
    }

    [[nodiscard]] std::size_t size() const final { return num_entries; }
    [[nodiscard]] bool empty() const final { return num_entries == 0; }
    void clear() final { buckets.clear(); num_entries = 0; }


    [[nodiscard]] const Value_t& top() const final {
        PLAJA_ASSERT(num_entries > 0)
        auto it = buckets.cbegin();
        PLAJA_ASSERT(it != buckets.cend())
        return it->second.top();
    }

    [[nodiscard]] std::list<Value_t> to_list() const final {
        std::list<Value_t> list_queue;
        for (const auto& bucket: buckets) { list_queue.splice(list_queue.end(), bucket.second.to_list()); }
        return list_queue;
    };

};

/**********************************************************************************************************************/

template<typename Priority_t, typename Value_t, typename Compare_t = PRIORITY_QUEUE::CompareNumeric<Priority_t, false>>
class HeapQueue final: public PriorityQueue<Priority_t, Value_t> {
    using Entry = typename PriorityQueue<Priority_t, Value_t>::Entry;
    using CompareEntry = PRIORITY_QUEUE::CompareEntry<Priority_t, Value_t, Compare_t>;

private:

    // !!! if constexpr(ASC) { return lhs.first > rhs.first; } else { return lhs.first < rhs.first; }
    // From https://en.cppreference.com/w/cpp/container/priority_queue [March 2023]:
    // Note that the Compare parameter is defined such that it returns true if its first argument comes before its second argument in a weak ordering.
    // But because the priority queue outputs largest elements first, the elements that "come before" are actually output last.
    // That is, the front of the queue contains the "last" element according to the weak ordering imposed by Compare.

    class Heap: public std::priority_queue<Entry, std::vector<Entry>, CompareEntry> { friend class HeapQueue; };

    Heap heap;

public:
    HeapQueue() = default;
    ~HeapQueue() final = default;

    void push(const Priority_t& priority, Value_t&& value) final {
        PLAJA_ASSERT(Compare_t::is_valid(priority))
        heap.push(std::make_pair(priority, std::move(value)));
    }

    Entry pop_with_priority() final {
        PLAJA_ASSERT(not heap.empty())
        auto result = std::move(heap.top());
        heap.pop();
        return result;
    }

    [[nodiscard]] std::size_t size() const final { return heap.size(); }
    [[nodiscard]] bool empty() const final { return heap.empty(); }
    void clear() final { heap.c.clear(); }

    [[nodiscard]] const Value_t& top() const final { return heap.top().second; }
    [[nodiscard]] std::list<Value_t> to_list() const final {
        std::list<Value_t> list_queue;
        for (const auto& entry: heap.c) { list_queue.push_back(entry.second); }
        return list_queue;
    };

};

/**********************************************************************************************************************/

template<typename Priority_type, typename Value_type, bool ASC, typename Bucket_t = PRIORITY_QUEUE::BucketFIFO<Value_type>>
class BucketQueue final: public PriorityQueue<Priority_type, Value_type> {
    using Entry = typename PriorityQueue<Priority_type, Value_type>::Entry;

    std::vector<Bucket_t> buckets;
    std::size_t num_entries;
    Priority_type current_priority;

    inline void update_current_priority() {
        auto num_buckets = buckets.size();
        if constexpr (ASC) {
            while (current_priority < num_buckets and buckets[current_priority].empty()) { ++current_priority; }
        } else {
            while (current_priority > 0 and buckets[current_priority].empty()) { --current_priority; }
        }
    }

public:
    BucketQueue(): num_entries(0), current_priority(0) {}
    ~BucketQueue() final = default;

    void push(const Priority_type& priority, Value_type&& value) final {
        PLAJA_ASSERT(0 <= priority and priority <= std::numeric_limits<Priority_type>::max())
        ++num_entries;
        auto num_buckets = buckets.size();
        if (priority >= num_buckets) { buckets.resize(priority + 1); }
        buckets[priority].push(std::move(value));
        // update priority:
        if constexpr (ASC) { if (priority < current_priority) { current_priority = priority; } }
        else { if (priority > current_priority) { current_priority = priority; } }
        //
        update_current_priority();
    }

    Entry pop_with_priority() final {
        PLAJA_ASSERT(num_entries > 0)
        auto& current_bucket = buckets[current_priority];
        auto top_element = current_bucket.pop();
        // update queue:
        --num_entries;
        update_current_priority();
        //
        return std::make_pair(current_priority, std::move(top_element));
    }

    [[nodiscard]] std::size_t size() const final { return num_entries; }
    [[nodiscard]] bool empty() const final { return num_entries == 0; }

    void clear() final {
        for (auto& bucket: buckets) {
            auto bucket_size = bucket.size();
            PLAJA_ASSERT(bucket_size <= num_entries)
            num_entries -= bucket_size;
            bucket.clear();
        }
        current_priority = 0;
        PLAJA_ASSERT(num_entries == 0)
    }

    [[nodiscard]] const Value_type& top() const final {
        PLAJA_ASSERT(num_entries > 0)
        const auto& current_bucket = buckets[current_priority];
        return current_bucket.top();
    }

    [[nodiscard]] std::list<Value_type> to_list() const final {
        std::list<Value_type> list_queue;
        for (const auto& bucket: buckets) { list_queue.splice(list_queue.end(), bucket.to_list()); }
        return list_queue;
    };

};

/**********************************************************************************************************************/

// no priority classes:

template<typename Value_t, typename Priority_t> class FIFOQueue;
template<typename Value_t, typename Priority_t> class LIFOQueue;

template<typename Value_t, bool FIFO=true, typename Priority_t = int>
class NoPriorityQueue: public PriorityQueue<Priority_t, Value_t> {
    using Entry = typename PriorityQueue<Priority_t, Value_t>::Entry;

private:
    std::list<Value_t> queue;

protected:
    NoPriorityQueue() = default;
public:
    ~NoPriorityQueue() = default;

    inline void push(Value_t&& value) { queue.push_back(std::move(value)); }
    void push(const Priority_t& /*priority*/, Value_t&& value) final { push(std::move(value)); }

    Entry pop_with_priority() final {
        Value_t value;
        if constexpr(FIFO) {
            value = std::move(queue.front());
            queue.pop_front();
        } else { // LIFO
            value = std::move(queue.back());
            queue.pop_back();
        }
        return std::make_pair(0, std::move(value));
    }

    [[nodiscard]] std::size_t size() const final { return queue.size(); }
    [[nodiscard]] bool empty() const final { return queue.empty(); }
    void clear() final { queue.clear(); }

    [[nodiscard]] const Value_t& top() const final { if constexpr(FIFO) { return queue.front(); } else { return queue.back(); } }
    [[nodiscard]] std::list<Value_t> to_list() const final { return queue; };

};

template<typename Value_type, typename Priority_t = int>
class FIFOQueue final: public NoPriorityQueue<Value_type, true, Priority_t> {};

template<typename Value_type, typename Priority_t = int>
class LIFOQueue final: public NoPriorityQueue<Value_type, false, Priority_t> {};

#endif // PLAJA_PRIORITY_QUEUE_H
