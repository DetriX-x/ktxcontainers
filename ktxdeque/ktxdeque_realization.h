#pragma once

#include "ktxdeque.h"


namespace ktx {



// public

template<typename T, typename Allocator>
deque<T, Allocator>::deque(const std::initializer_list<value_type> items, Allocator alloc) : ai_{0}, sz_{std::size(items)} {
    auto blocks_count = sz_*2 / BlockSize + (sz_*2 % BlockSize ? 1 : 0);
    auto count_of_free_cells = blocks_count * BlockSize;
    ai_ = (count_of_free_cells - sz_) / 2;


    outer_.resize(blocks_count);
    allocateBlocks(outer_, 0, blocks_count);

    auto i = begin();
    auto items_it = items.begin();
    try {
        for (; i != end(); ++i, ++items_it) {
            alloc_traits::construct(alloc_, &*i, std::move_if_noexcept(*items_it));
        }
    } catch (...) {
        for (auto j = begin(); j != i; ++j) {
            alloc_traits::destroy(alloc_, &*j);
        }
        deallocateBlocks(outer_);
        throw;
    }
}

template<typename T, typename Allocator>
deque<T, Allocator>::deque(size_type n, const T& val, Allocator a) : ai_{0}, sz_{n} {
    auto blocks_count = n*2 / BlockSize + (n*2 % BlockSize ? 1 : 0);
    auto count_of_free_cells = blocks_count * BlockSize;
    ai_ = (count_of_free_cells - n) / 2;

    outer_.resize(blocks_count);
    allocateBlocks(outer_, 0, blocks_count);

    auto i = begin();
    try {
        for (; i != end(); ++i) {
            alloc_traits::construct(alloc_, &*i, val);
        }
    } catch (...) {
        for (auto j = begin(); j != i; ++j) {
            alloc_traits::destroy(alloc_, &*j);
        }
        deallocateBlocks(outer_);
        throw;
    }
}

template <typename T, typename Allocator>
template <std::forward_iterator Iter>
deque<T, Allocator>::deque(Iter fst, Iter lst): ai_{0}, sz_{0} {
    if constexpr (std::is_base_of_v<std::random_access_iterator_tag,
            typename std::iterator_traits<Iter>::iterator_category>) {
        sz_ = std::distance(fst, lst);
        auto blocks_count = sz_*2 / BlockSize + (sz_*2 % BlockSize ? 1 : 0);
        auto count_of_free_cells = blocks_count * BlockSize;
        ai_ = (count_of_free_cells - sz_) / 2;


        outer_.resize(blocks_count);
        allocateBlocks(outer_, 0, blocks_count);

        auto i = begin();
        try {
            for (; i != end(); ++i, ++fst) {
                alloc_traits::construct(alloc_, &*i, std::move_if_noexcept(*fst));
            }
        } catch (...) {
            for (auto j = begin(); j != i; ++j) {
                alloc_traits::destroy(alloc_, &*j);
            }
            deallocateBlocks(outer_);
            throw;
        }
    } else {
        std::copy(fst, lst, std::back_inserter(*this));
    }
}

template<typename T, typename Allocator>
deque<T, Allocator>::deque(const deque<T, Allocator>& other)
    : ai_{other.ai_}
    , sz_{other.sz_}
    , outer_{}
    , alloc_{std::allocator_traits<Allocator>::select_on_container_copy_construction(other.alloc_)} {
    outer_.resize(other.outer_.size());
    allocateBlocks(outer_, 0, other.outer_.size());
    auto i = begin();
    auto other_i = other.begin();
    try {
        for (; i != end(); ++i, ++other_i) {
            alloc_traits::construct(alloc_, &*i, std::move_if_noexcept(*other_i));
        }
    } catch (...) {
        for (auto j = begin(); j != i; ++j) {
            alloc_traits::destroy(alloc_, &*j);
        }
        deallocateBlocks(outer_);
        throw;
    }
}

template <typename T, typename Allocator>
template <typename... Args>
void deque<T, Allocator>::emplace_back(Args&&... args) {
    if (outer_.empty()) {
        outer_.push_back(nullptr);
        allocateBlocks(outer_, 0, 1);
        ai_ = BlockSize / 2 - 1;
    }
    auto [n, freeBlocksFromBot, freeBlocksFromTop, occupiedBlocks] = getCapacityState();

    if (ai_ + sz_ < n) {
        alloc_traits::construct(alloc_, &*end(), std::forward<Args>(args)...);
        ++sz_;
        return;
    }
    const auto newBlockCount = occupiedBlocks * expansion + outer_.size();
    vector<pointer, rebinded> newOuter(newBlockCount);

    size_t i = 0;
    for (; i < outer_.size(); ++i) {
        newOuter[i] = outer_[i];
    }
    allocateBlocks(newOuter, i, newBlockCount);

    try {
        alloc_traits::construct(alloc_, &newOuter[i][0], std::forward<Args>(args)...);
    } catch (...) {
        deallocateBlocks(newOuter, i);
    }

    swap(outer_, newOuter);
    ++sz_;
}

template <typename T, typename Allocator>
template <typename... Args>
void deque<T, Allocator>::emplace_front(Args&&... args) {
    if (outer_.empty()) {
        outer_.push_back(nullptr);
        allocateBlocks(outer_, 0, 1);
        ai_ = BlockSize / 2 - 1;
    }
    auto [n, freeBlocksFromBot, freeBlocksFromTop, occupiedBlocks] = getCapacityState();

    if (ai_ != 0) {
        alloc_traits::construct(alloc_, &*(begin() - 1), std::forward<Args>(args)...);
        --ai_;
        ++sz_;
        return;
    }
    const auto newBlockCount = occupiedBlocks * expansion + outer_.size();
    vector<pointer, rebinded> newOuter(newBlockCount);

    size_t offset = newBlockCount - outer_.size();
    allocateBlocks(newOuter, 0, offset);
    for (size_t i = 0; i < outer_.size(); ++i) {
        newOuter[i + offset] = outer_[i];
    }

    try {
        alloc_traits::construct(alloc_, &newOuter[offset - 1][BlockSize - 1], std::forward<Args>(args)...);
    } catch (...) {
        deallocateBlocks(newOuter, offset);
    }

    swap(outer_, newOuter);
    ai_ += offset * BlockSize;
    --ai_;
    ++sz_;
}

template<typename T, typename Allocator>
void deque<T, Allocator>::push_back(value_type value) { 
    emplace_back(std::move(value));
}

template<typename T, typename Allocator>
void deque<T, Allocator>::push_front(value_type value) { 
    emplace_front(std::move(value));
}

template<typename T, typename Allocator>
void deque<T, Allocator>::clear() {
    for (auto& i: *this) {
        alloc_traits::destroy(alloc_, &i);
    }
}

template<typename T, typename Allocator>
deque<T, Allocator>::~deque() {
    clear();
    deallocateBlocks(outer_);
}

template<typename T, typename Allocator>
template <typename Self>
constexpr auto deque<T, Allocator>::at(this Self&& self,
                                        size_type index) -> 
std::conditional_t<
    std::is_const_v<std::remove_reference_t<Self>>,
    const_reference,
    reference> {
        if (index >= self.sz_) {
            throw std::out_of_range{"Index is out of range of deque"};
        }
        return std::forward<Self>(self)[index];
}

template<typename T, typename Allocator>
void deque<T, Allocator>::resize(size_type count, const value_type& val) {
    if (sz_ == count) {
        return;
    }

    if (sz_ < count) {
        auto diff = count - sz_;
        while (diff) {
            push_back(val);
            --diff;
        }
    } else {
        auto diff = sz_ - count;
        while (diff) {
            pop_back();
            --diff;
        }
    }
}

template<typename T, typename Allocator>
void deque<T, Allocator>::resize(size_type count) {
    resize(count, T());
}

template<typename T, typename Allocator>
void deque<T, Allocator>::shrink_to_fit() {
    if (empty()) {
        deallocateBlocks(outer_, 0);
    }
    auto [n, freeBlocksFromBot, freeBlocksFromTop, occupiedBlocks] = getCapacityState();

    if (occupiedBlocks == outer_.size()) {
        return;
    }

    for (size_t i = 0; i < freeBlocksFromTop; ++i) {
        alloc_traits::deallocate(alloc_, outer_[i], BlockSize);
        outer_[i] = nullptr;
    }
    for (size_t i = 0; i < freeBlocksFromBot; ++i) {
        alloc_traits::deallocate(alloc_, outer_[i + freeBlocksFromTop + occupiedBlocks], BlockSize);
        outer_[i + freeBlocksFromTop + occupiedBlocks] = nullptr;
    }

}

template <typename T, typename Allocator>
void deque<T, Allocator>::pop_back() {
    erase(cend() - 1);
}

template <typename T, typename Allocator>
void deque<T, Allocator>::pop_front() {
    erase(cbegin());
}

template<typename T, typename Allocator>
template<typename... Args>
deque<T, Allocator>::iterator deque<T, Allocator>::emplace(const_iterator pos, Args&&... args) {
    auto distance_to_begin = std::distance(cbegin(), pos);
    auto distance_to_end = std::distance(pos, cend());
    iterator p = begin() + std::distance(cbegin(), pos);

    if (distance_to_end < distance_to_begin) {
        emplace_back(std::forward<Args>(args)...);
        for (auto it = end() - 1; it != pos; --it) {
            std::swap(*it, *(it - 1));
        }
    } else {
        emplace_front(std::forward<Args>(args)...);
        for (auto it = begin(); it != pos - 1; ++it) {
            std::swap(*it, *(it + 1));
        }
    }

    return p;
}

template<typename T, typename Allocator>
deque<T, Allocator>::iterator deque<T, Allocator>::insert(const_iterator pos, value_type value) {
    return emplace(pos, std::move(value));
}

template<typename T, typename Allocator>
deque<T, Allocator>::iterator deque<T, Allocator>::erase(const_iterator pos) {
    auto distance_to_begin = std::distance(cbegin(), pos);
    auto distance_to_end = std::distance(pos, cend());
    iterator p = begin() + std::distance(cbegin(), pos);

    if (distance_to_end <= distance_to_begin) {
        for (iterator it = p; it != end() - 1; ++it) {
            *it = std::move(*(it + 1));
        }
        alloc_traits::destroy(alloc_, std::addressof(*(cend() - 1)));
    } else {
        for (iterator it = p; it != begin(); --it) {
            *it = std::move(*(it - 1));
        }
        alloc_traits::destroy(alloc_, std::addressof(*begin()));
        ++ai_;
    }
    --sz_;

    return p;
}

// private

template <typename T, typename Allocator>
template <std::input_iterator InputIt,
         std::forward_iterator NoThrowForwardIt,
         std::predicate<T&&> UnaryPred>
auto deque<T, Allocator>::uninitialized_move_if(
       InputIt first,
       InputIt last,
       NoThrowForwardIt d_first,
       UnaryPred P) -> NoThrowForwardIt {
    auto current = d_first;
    try {
        for(; first != last; ++first, ++current) {
            if (P(*first)) {
                ++current;
            }
            alloc_traits::construct(alloc_,
                            std::addressof(*current),
                            std::move_if_noexcept(*first));
        }
        return current;
    } catch (...) {
        for (; d_first != current; ++d_first) {
            alloc_traits::destroy(alloc_, d_first);
        }
        throw;
    }
}

template <typename T, typename Allocator>
template <std::input_iterator InputIt,
         std::forward_iterator NoThrowForwardIt>
auto deque<T, Allocator>::uninitialized_move(
        InputIt first,
        InputIt last,
        NoThrowForwardIt d_first) -> NoThrowForwardIt {
    auto current = d_first;
    try {
        for(; first != last; ++first, ++current) {
            alloc_traits::construct(alloc_,
                            std::addressof(*current),
                            std::move_if_noexcept(*first));
        }
        return current;
    } catch (...) {
        for (; d_first != current; ++d_first) {
            alloc_traits::destroy(alloc_, d_first);
        }
        throw;
    }
}

// friend

template<typename T, typename Allocator>
void swap(deque<T, Allocator>& to, deque<T, Allocator>& from) {
    std::swap(from.outer_, to.outer_);
    std::swap(from.ai_, to.ai_);
    std::swap(from.sz_, to.sz_);
}

// global

template<typename T, typename Allocator>
std::ostream& operator<<(std::ostream& os,
        const deque<T, Allocator>& vec) {
    for (auto it = std::begin(vec); it != std::end(vec); ++it) {
        os << *it;
        if (it != std::end(vec) - 1) {
            os << ' ';
        }
    }

    return os;
}



};


