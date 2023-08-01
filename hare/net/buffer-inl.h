#ifndef _HARE_NET_BUFFER_INL_H_
#define _HARE_NET_BUFFER_INL_H_

#include <hare/base/util/buffer.h>
#include <hare/net/buffer.h>

#define MAX_TO_REALIGN 2048U
#define MIN_TO_ALLOC 4096U
#define MAX_TO_COPY_IN_EXPAND 4096U
#define MAX_SIZE (0xffffffff)
#define MAX_CHINA_SIZE (16U)

namespace hare {
namespace net {
    namespace detail {
        /**
         * @brief The block of buffer.
         * @code
         * +----------------+-------------+------------------+
         * | misalign bytes |  off bytes  |  residual bytes  |
         * |                |  (CONTENT)  |                  |
         * +----------------+-------------+------------------+
         * |                |             |                  |
         * @endcode
         **/
        class cache : public util::buffer<char> {
            std::size_t misalign_ { 0 };

        public:
            using base = util::buffer<char>;

            HARE_INLINE
            explicit cache(std::size_t _max_size)
                : base(new base::value_type[_max_size], 0, _max_size)
            { }

            HARE_INLINE
            cache(base::value_type* _data, std::size_t _max_size)
                : base(_data, _max_size, _max_size)
            { }

            HARE_INLINE
            ~cache() override
            { delete[] begin(); }

            // write to data_ directly
            HARE_INLINE auto writeable() -> base::value_type* { return begin() + size(); }
            HARE_INLINE auto writeable() const -> const base::value_type* { return begin() + size(); }
            HARE_INLINE auto writeable_size() const -> std::size_t { return capacity() - size(); }

            HARE_INLINE auto readable() -> base::value_type* { return data() + misalign_; }
            HARE_INLINE auto readable_size() const -> std::size_t { return size() - misalign_; }
            HARE_INLINE auto full() const -> bool { return size() == capacity(); }
            HARE_INLINE auto empty() const -> bool { return readable_size() == 0; }
            HARE_INLINE void clear() { base::clear(); misalign_ = 0; }

            HARE_INLINE void drain(std::size_t _size) { misalign_ += _size; }
            HARE_INLINE void add(std::size_t _size) { size_ += _size; }

            HARE_INLINE void bzero() { hare::detail::fill_n(data(), capacity(), 0); }

            auto realign(std::size_t _size) -> bool;

        private:
            HARE_INLINE void grow(std::size_t _capacity) override { ignore_unused(_capacity); }

            friend class net::buffer;
        };

        using ucache = uptr<detail::cache>;

        /** @code
         *  +-------++-------++-------++-------++-------++-------+
         *  | empty || full  || full  || write || empty || empty |
         *  +-------++-------++-------++-------++-------++-------+
         *              |                 |
         *          read_iter         write_iter
         *  @endcode
         **/
        using cache_list = struct cache_list {
            struct node {
                ucache cache {};
                node* next {};
                node* prev {};

                HARE_INLINE auto operator->() -> ucache&
                { return cache; }
            };

            node* head {};
            node* read {};
            node* write {};
            std::size_t node_size_ { 0 };

            HARE_INLINE
            cache_list()
                : head(new node)
                , read(head)
                , write(head)
                , node_size_(1)
            {
                head->next = head;
                head->prev = head;
            }

            HARE_INLINE
            ~cache_list()
            {
                reset();
                delete head;
            }

            HARE_INLINE auto begin() const -> node* { return read; }
            HARE_INLINE auto end() const -> node* { return write; }
            HARE_INLINE auto size() const -> std::size_t { return node_size_; }
            HARE_INLINE auto empty() const -> bool { return read == write && (!write->cache || (*write)->empty());}
            HARE_INLINE auto full() const -> bool { return write->cache && (*write)->full() && write->next == read;}

            HARE_INLINE
            void swap(cache_list& _other)
            {
                std::swap(head, _other.head);
                std::swap(read, _other.read);
                std::swap(write, _other.write);
                std::swap(node_size_, _other.node_size_);
            }

            void check_size(std::size_t _size);

            void append(cache_list& _other);

            auto fast_expand(std::size_t _size) -> std::int32_t;

            void add(std::size_t _size);

            void drain(std::size_t _size);

            void reset();

#ifdef HARE_DEBUG
            void print_status(const std::string& _status) const;
#endif

        private:
            HARE_INLINE
            auto get_next_write() -> node*
            {
                if (!write->cache || (*write)->empty()) {
                    if (write->cache) {
                        (*write)->clear();
                    }
                    return end();
                } else if (write->next == read) {
                    auto* tmp = new node;
                    tmp->next = write->next;
                    tmp->prev = write;
                    write->next->prev = tmp;
                    write->next = tmp;
                    ++node_size_;
                }
                write = write->next;
                assert(write != read);
                return end();
            }
            
        };
        
    } // namespace detail

    struct buffer_iterator_impl : public hare::detail::impl {
        const detail::cache_list* list_;
        detail::cache_list::node* iter_ {};
        std::size_t curr_index_ {};

        HARE_INLINE
        explicit buffer_iterator_impl(detail::cache_list* _list)
            : list_(_list)
            , iter_(_list->begin())
            , curr_index_(hare::detail::to_unsigned((*_list->begin())->readable() - (*_list->begin())->data()))
        { }

        HARE_INLINE
        buffer_iterator_impl(const detail::cache_list* _list, detail::cache_list::node* _iter)
            : list_(_list)
            , iter_(_iter)
            , curr_index_((*_iter)->size())
        { }

        ~buffer_iterator_impl() override = default;
    };

} // namespace net
} // namespace hare

#endif // _HARE_NET_BUFFER_INL_H_