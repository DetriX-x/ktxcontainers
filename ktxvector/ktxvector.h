#pragma once


#include <initializer_list>
#include <memory>
#include <iterator>
#include <iostream>
#include <concepts>

namespace ktx {


constexpr size_t expansion {2};

template <typename T, typename Allocator = std::allocator<T>>
class vector {
private:
    template <bool isConst>
    class base_iterator;

public:
    using value_type = T;
    using size_type = std::size_t;
    using allocator_type = Allocator;
    using difference_type = std::ptrdiff_t;
    using reference = value_type&;
    using const_reference = const value_type&;
    using pointer = typename std::allocator_traits<Allocator>::pointer;
    using const_pointer = typename 
        std::allocator_traits<Allocator>::const_pointer;
    using iterator = base_iterator<false>;
    using const_iterator = base_iterator<true>;
    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;
    using alloc_traits = std::allocator_traits<allocator_type>;
    

private:
    size_type sz_;
    size_type cap_;
    pointer data_;
    [[no_unique_address]] allocator_type alloc_;


public:
    // constructors and assign
    vector() : sz_{0}, cap_{0}, data_{nullptr}, alloc_{} {}

    vector(std::initializer_list<value_type> list, 
            Allocator alloc = Allocator());

    explicit vector(Allocator a) 
        : sz_{0}, cap_(0), data_{nullptr}, alloc_{a} {}

    explicit vector(size_type n, const T& val = T(), Allocator a = Allocator());

    template <std::forward_iterator Iter>
    vector(Iter fst, Iter lst);

    vector(const vector& other);

    vector(vector&& other) noexcept : vector{} { swap(*this, other); }

    vector& operator=(vector other) {
        swap(*this, other);
        return *this;
    }

    // dtor
    ~vector();

    // modifiers

    template<typename... Args>
    void emplace(const_iterator, Args&&... args);

    void insert(const_iterator pos, value_type value);

    iterator erase(const_iterator pos);

    void clear();

    void push_back(value_type value);

    template <typename... Args>
    void emplace_back(Args&&... args); 

    void reserve(size_type newcap);

    void pop_back();

    void shrink_to_fit();

    void resize(size_type count);

    void resize(size_type count, const value_type& value);

    template <typename U, typename A>
    friend void swap(vector<U, A>& to, vector<U, A>& from);

    // comparison

    // accessors
    template <typename Self>
    constexpr auto operator[](this Self&& self, size_type index) -> 
    std::conditional_t<
        std::is_const_v<std::remove_reference_t<Self>>,
        const_reference,
        reference> {
        return std::forward<Self>(self).data_[index];
    }

    size_type size() const { return sz_; }

    size_type capacity() const { return cap_; }

    [[nodiscard]] bool empty() { return !sz_; }

    template <typename Self>
    constexpr auto data(this Self&& self) -> 
    std::conditional_t<
        std::is_const_v<std::remove_reference_t<Self>>,
        const_pointer,
        pointer> {
        return std::forward<Self>(self).data_;
    }

    template <typename Self>
    constexpr auto back(this Self&& self) -> 
    std::conditional_t<
        std::is_const_v<std::remove_reference_t<Self>>,
        const_pointer,
        pointer> {
        if (0 == self.sz_) {
            return std::forward<Self>(self).data_[0];
        } else {
            return std::forward<Self>(self).data_[self.sz_ - 1];
        }  
    }

    template <typename Self>
    constexpr auto front(this Self&& self) -> 
    std::conditional_t<
        std::is_const_v<std::remove_reference_t<Self>>,
        const_pointer,
        pointer> {
        return std::forward<Self>(self).data_[0];
    }

    template <typename Self>
    constexpr auto at(this Self&& self, size_type index) -> 
    std::conditional_t<
        std::is_const_v<std::remove_reference_t<Self>>,
        const_reference,
        reference>;

    // iterator

    iterator begin() {
        return {data_};
    }

    iterator end() {
        return {data_ + sz_};
    }

    const_iterator begin() const {
        return {data_};
    }

    const_iterator end() const {
        return {data_ + sz_};
    }

    const_iterator cbegin() const {
        return {data_};
    }

    const_iterator cend() const {
        return {data_ + sz_};
    }

private:
    template <std::input_iterator InputIt,
             std::forward_iterator NoThrowForwardIt>
    auto uninitialized_move(
            InputIt first,
            InputIt last,
            NoThrowForwardIt d_first) -> NoThrowForwardIt;

    template <std::input_iterator InputIt,
             std::forward_iterator NoThrowForwardIt,
             std::predicate<value_type&&> UnaryPred>
    auto uninitialized_move_if(
            InputIt first,
            InputIt last,
            NoThrowForwardIt d_first,
            UnaryPred P = [](){return false;}) -> NoThrowForwardIt;
    
    void destroy_all();
    
    template <std::input_iterator InputIt>
    pointer create_from(Allocator alloc,
            size_type count,
            InputIt first,
            InputIt last);

    template<bool isConst>
    class base_iterator {
    public:
        friend class vector<T, Allocator>;
        using iterator_category = std::contiguous_iterator_tag;
        using value_type = vector<T>::value_type;
        using difference_type = ptrdiff_t;
        using pointer = std::conditional_t<isConst, const T*, T*>;
        using reference = std::conditional_t<isConst, const T&, T&>;

        base_iterator(): ptr_{} {}
        base_iterator(const base_iterator&) = default;
        base_iterator& operator=(const base_iterator&) = default;
        base_iterator(base_iterator&&) = default;
        base_iterator& operator=(base_iterator&&) = default;
        ~base_iterator() = default;

        friend difference_type operator-(base_iterator a,
                base_iterator b) {
            return a.ptr_ - b.ptr_;
        }

        friend base_iterator operator+(base_iterator it,
                                               difference_type index) {
           it.ptr_ += index;
           return {it};
        }

        friend base_iterator operator-(base_iterator it,
                                               difference_type index) {
           it.ptr_ -= index;
           return {it};
        }

        friend base_iterator operator+(difference_type index,
                                               base_iterator it) {
           it.ptr_ += index;
           return {it};
        }

        friend base_iterator operator-(difference_type index,
                                               base_iterator it) {
           it.ptr_ -= index;
           return {it};
        }

        iterator& operator+=(difference_type index) {
            ptr_ += index;
        }

        iterator& operator-=(difference_type index) {
            ptr_ -= index;
        }

        base_iterator& operator++() {
            ++ptr_;
            return *this;
        }

        base_iterator operator++(int) {
            base_iterator it = *this;
            ++ptr_;
            return it;
        }

        base_iterator& operator--() {
            --ptr_;
            return *this;
        }

        base_iterator operator--(int) {
            base_iterator it = *this;
            --ptr_;
            return it;
        }

        reference operator*() const {
            return *ptr_;
        }

        pointer operator->() const {
            return ptr_;
        }

        auto operator<=>(const base_iterator& it) const = default;

        operator base_iterator<true>() const {
            return ptr_;
        }

    private:
        pointer ptr_;
        base_iterator(T* ptr) noexcept
            : ptr_{ptr} {}
    };
};

template<typename T, typename Allocator>
std::ostream& operator<<(std::ostream& os, const vector<T, Allocator>& vec);


}

#include "ktxvector_realization.h" // IWYU pragma: keep
