#include <iterator>


template <typename T, typename Allocator = std::allocator<T>>
class list {
private:
    template <bool>
    class base_iterator;
public:
    using value_type = T;
    using allocator_type = Allocator;
    using reference = T&;
    using const_reference = const T&;
    using pointer = typename std::allocator_traits<Allocator>::pointer;
    using const_pointer = typename 
        std::allocator_traits<Allocator>::const_pointer;
    using difference_type = std::ptrdiff_t;
    using size_type = std::size_t;
    using iterator = base_iterator<false>;
    using const_iterator = base_iterator<true>;
    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

private:
    struct BaseNode {
        BaseNode* next;
        BaseNode* prev;
    };

    struct Node : BaseNode {
        value_type value;
        Node(const value_type& value) : value(value) {}
    };

    BaseNode fakeNode;
    size_type sz;
    using rebinded = typename std::allocator_traits<Allocator>::template rebind<Node>::other;
    rebinded alloc;
    using AllocTraits = std::allocator_traits<rebinded>;

public:
    list() : fakeNode{&fakeNode, &fakeNode}, sz(0), alloc() {
    }

    explicit list(const Allocator& alloc) :
        fakeNode{&fakeNode, &fakeNode}, sz(0), alloc(alloc) {}

    explicit list(size_type count, 
                  const T& value = T(),
                  const Allocator& alloc = Allocator()) : 
        fakeNode{&fakeNode, &fakeNode},
        alloc(alloc)
    {
        size_type i = 0;
        Node* newNode;
        try {
            for(; i != count; ++i) {
                newNode = AllocTraits::allocate(this->alloc, 1);
                AllocTraits::construct(this->alloc, newNode, value);
                bindNewNodeToTop(newNode);
            }
        } catch(...) {
            Node* current = static_cast<Node*>(fakeNode.next);
            while(i) {
                Node* next = static_cast<Node*>(current->next);
                AllocTraits::destroy(this->alloc, current);
                AllocTraits::deallocate(this->alloc, current, 1);
                current = next;
                --i;
            }
            AllocTraits::deallocate(this->alloc, newNode, 1);
            throw;
        }

        sz = count;
    }

    list(const list& other) :
        fakeNode{&fakeNode, &fakeNode},
        sz(other.sz),
        alloc(AllocTraits::select_on_container_copy_construction(
                    other.alloc)) {
        auto it = other.cbegin(); 
        Node* newNode;
        try {
            for (; it != other.cend(); ++it) {
                newNode = AllocTraits::allocate(alloc, 1);
                AllocTraits::construct(alloc, newNode, *it);
                bindNewNodeToEnd(newNode);
            }
        } catch(...) {
            for (auto start = this->begin(); it != start; ++start) {
                AllocTraits::destroy(alloc, it.ptr);
                AllocTraits::deallocate(alloc, 
                        static_cast<Node*>(it.ptr), 1);
            }
            throw;
        }
    }

    list& operator=(list other) & noexcept {
        swap(*this, other);
        return *this;
    }

    list(list&& other) noexcept: list() {
        swap(*this, other);
    }

    friend void swap(list& lhs, list& rhs) {
        if (lhs.empty()) {
            lhs.fakeNode.next = rhs.fakeNode.next;
            lhs.fakeNode.prev = rhs.fakeNode.prev;
            lhs.fakeNode.next->prev = &lhs.fakeNode;
            lhs.fakeNode.prev->next = &lhs.fakeNode;
            rhs.fakeNode.next = &rhs.fakeNode;
            rhs.fakeNode.prev = &rhs.fakeNode;
        }

        if (rhs.empty()) {
            rhs.fakeNode.next = lhs.fakeNode.next;
            rhs.fakeNode.prev = lhs.fakeNode.prev;
            rhs.fakeNode.next->prev = &rhs.fakeNode;
            rhs.fakeNode.prev->next = &rhs.fakeNode;
            lhs.fakeNode.next = &lhs.fakeNode;
            lhs.fakeNode.prev = &lhs.fakeNode;
        }

        if (lhs.size() && rhs.size()) {
            auto lhs_first = lhs.fakeNode.next;
            auto lhs_last = lhs.fakeNode.prev;
            auto rhs_first = rhs.fakeNode.next;
            auto rhs_last = rhs.fakeNode.prev;

            lhs.fakeNode.next = rhs_first;
            lhs.fakeNode.prev = rhs_last;

            rhs.fakeNode.next = lhs_first;
            rhs.fakeNode.prev = lhs_last;

            lhs_first->prev = &rhs.fakeNode;
            lhs_last->next = &rhs.fakeNode;

            rhs_first->prev = &lhs.fakeNode;
            rhs_last->next = &lhs.fakeNode;
        }

        std::swap(lhs.sz, rhs.sz);
        std::swap(lhs.alloc, rhs.alloc);
    }

    ~list() {
        clear();
    }

    void clear() {
        Node* current = static_cast<Node*>(fakeNode.next);
        while(sz) {
            Node* next = static_cast<Node*>(current->next);
            AllocTraits::destroy(alloc, current);
            AllocTraits::deallocate(alloc, current, 1);
            current = next;
            --sz;
        }
    }

    size_type size() const {
        return sz;
    }

    bool empty() const {
        return !sz;
    }

    reference front() {
        return static_cast<Node*>(fakeNode.next)->value;
    }

    const_reference front() const {
        return static_cast<Node*>(fakeNode.next)->value;
    }

    reference back() {
        return static_cast<Node*>(fakeNode.prev)->value;
    }

    const_reference back() const {
        return static_cast<Node*>(fakeNode.prev)->value;
    }

    void push_front(const T& value) {
        Node* newNode = AllocTraits::allocate(alloc, 1);
        AllocTraits::construct(alloc, newNode, value);

        bindNewNodeToTop(newNode);
        ++sz;
    }

    void pop_front() {
        Node* node = static_cast<Node*>(fakeNode.next);
        fakeNode.next = node->next;
        node->next->prev = &fakeNode;

        AllocTraits::destroy(alloc, node);
        AllocTraits::deallocate(alloc, node, 1);
        --sz;
    }

    void push_back(const T& value) {
        Node* newNode = AllocTraits::allocate(alloc, 1);
        AllocTraits::construct(alloc, newNode, value);

        bindNewNodeToEnd(newNode);
        ++sz;
    }

    void pop_back() {
        Node* node = static_cast<Node*>(fakeNode.prev);
        fakeNode.prev = node->prev;
        node->prev->next = &fakeNode;

        AllocTraits::destroy(alloc, node);
        AllocTraits::deallocate(alloc, node, 1);
        --sz;
    }

    void insert(const_iterator pos, const T& value) {
        Node* newNode = AllocTraits::allocate(alloc, 1);
        AllocTraits::construct(alloc, newNode, value);

        Node* before = pos.ptr;
        Node* after = static_cast<Node*>(before->prev);

        newNode->next = before;
        newNode->prev = after;

        after->next = newNode;
        before->prev = newNode;
        ++sz;
    }

    void erase(const_iterator pos) {
        Node* node = pos.ptr;
        node->prev->next = node->next;
        node->next->prev = node->prev;

        AllocTraits::destroy(alloc, node);
        AllocTraits::deallocate(alloc, node, 1);
        --sz;
    }

    iterator begin() {
        return fakeNode.next;
    }

    iterator end() {
        return &fakeNode;
    }

    const_iterator begin() const {
        return fakeNode.next;
    }

    const_iterator end() const {
        return &fakeNode;
    }

    const_iterator cbegin() const {
        return const_cast<BaseNode*>(fakeNode.next);
    }

    const_iterator cend() const {
        return const_cast<BaseNode*>(&fakeNode);
    }

private:
    void bindNewNodeToTop(Node* node) {
        BaseNode* head = fakeNode.next;
        fakeNode.next = node;
        node->prev = &fakeNode;
        head->prev = node;
        node->next = head;
    }

    void bindNewNodeToEnd(Node* node) {
        BaseNode* tail = fakeNode.prev;
        fakeNode.prev = node;
        tail->next = node;
        node->next = &fakeNode;
        node->prev = tail;
    }

    template <bool isConst>
    class base_iterator {
    public:
        using iterator_category = std::random_access_iterator_tag;
        using value_type = typename list<T>::value_type;
        using difference_type = ptrdiff_t;
        using pointer = std::conditional_t<isConst, const T*, T*>;
        using reference = std::conditional_t<isConst, const T&, T&>;

        base_iterator(const base_iterator&) = default;
        base_iterator& operator=(const base_iterator&) = default;
        base_iterator(base_iterator&&) = default;
        base_iterator& operator=(base_iterator&&) = default;
        ~base_iterator() = default;

        base_iterator& operator++() {
            ptr = static_cast<Node*>(ptr->next);
            return *this;
        }

        base_iterator operator++(int) {
            base_iterator it = *this;
            ++(*this);
            return it;
        }

        base_iterator& operator--() {
            ptr = static_cast<Node*>(ptr->prev);
            return *this;
        }

        base_iterator operator--(int) {
            base_iterator it = *this;
            --(*this);
            return it;
        }

        reference operator*() const {
            return static_cast<Node*>(ptr)->value;
        }

        pointer operator->() const {
            return static_cast<Node*>(ptr);
        }

        bool operator==(base_iterator other) const {
            return ptr == other.ptr;
        }

        bool operator!=(base_iterator other) const {
            return ptr != other.ptr;
        }

        operator base_iterator<true>() const {
            return {ptr};
        }

        BaseNode* ptr;
        base_iterator(BaseNode* ptr) : ptr(ptr) {}
    };
};
