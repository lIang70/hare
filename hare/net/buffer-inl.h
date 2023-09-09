#ifndef _HARE_NET_BUFFER_INL_H_
#define _HARE_NET_BUFFER_INL_H_

#include "base/fwd-inl.h"
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
        class Cache : public util::Buffer<char> {
            std::size_t misalign_ { 0 };

        public:
            using Base = util::Buffer<char>;

            HARE_INLINE
            explicit Cache(std::size_t _max_size)
                : Base(new Base::ValueType[_max_size], 0, _max_size)
            { }

            HARE_INLINE
            Cache(Base::ValueType* _data, std::size_t _max_size)
                : Base(_data, _max_size, _max_size)
            { }

            HARE_INLINE
            ~Cache() override
            { delete[] Begin(); }

            // write to data_ directly
            HARE_INLINE auto Writeable() -> Base::ValueType* { return Begin() + size(); }
            HARE_INLINE auto Writeable() const -> const Base::ValueType* { return Begin() + size(); }
            HARE_INLINE auto WriteableSize() const -> std::size_t { return capacity() - size(); }

            HARE_INLINE auto Readable() -> Base::ValueType* { return Data() + misalign_; }
            HARE_INLINE auto ReadableSize() const -> std::size_t { return size() - misalign_; }
            HARE_INLINE auto Full() const -> bool { return size() == capacity(); }
            HARE_INLINE auto Empty() const -> bool { return ReadableSize() == 0; }
            HARE_INLINE void Clear() { Base::Clear(); misalign_ = 0; }

            HARE_INLINE void Drain(std::size_t _size) { misalign_ += _size; }
            HARE_INLINE void Add(std::size_t _size) { size_ += _size; }

            HARE_INLINE void Bzero() { hare::detail::FillN(Data(), capacity(), 0); }

            auto Realign(std::size_t _size) -> bool;

        private:
            HARE_INLINE void Grow(std::size_t _capacity) override { IgnoreUnused(_capacity); }

            friend class net::Buffer;
        };

        using Ucache = UPtr<detail::Cache>;

        /** @code
         *  +-------++-------++-------++-------++-------++-------+
         *  | empty || full  || full  || write || empty || empty |
         *  +-------++-------++-------++-------++-------++-------+
         *              |                 |
         *          read_iter         write_iter
         *  @endcode
         **/
        struct CacheList {
            struct Node {
                Ucache cache {};
                Node* next {};
                Node* prev {};

                HARE_INLINE auto operator->() -> Ucache&
                { return cache; }
            };

            Node* head {};
            Node* read {};
            Node* write {};
            std::size_t node_size_ { 0 };

            HARE_INLINE
            CacheList()
                : head(new Node)
                , read(head)
                , write(head)
                , node_size_(1)
            {
                head->next = head;
                head->prev = head;
            }

            HARE_INLINE
            ~CacheList()
            {
                Reset();
                delete head;
            }

            HARE_INLINE auto Begin() const -> Node* { return read; }
            HARE_INLINE auto End() const -> Node* { return write; }
            HARE_INLINE auto Size() const -> std::size_t { return node_size_; }
            HARE_INLINE auto Empty() const -> bool { return read == write && (!write->cache || (*write)->Empty());}
            HARE_INLINE auto Full() const -> bool { return write->cache && (*write)->Full() && write->next == read;}

            HARE_INLINE
            void Swap(CacheList& _other)
            {
                std::swap(head, _other.head);
                std::swap(read, _other.read);
                std::swap(write, _other.write);
                std::swap(node_size_, _other.node_size_);
            }

            void CheckSize(std::size_t _size);

            void Append(CacheList& _other);

            auto FastExpand(std::size_t _size) -> std::int32_t;

            void Add(std::size_t _size);

            void Drain(std::size_t _size);

            void Reset();

#ifdef HARE_DEBUG
            void PrintStatus(const std::string& _status) const;
#endif

        private:
            auto GetNextWrite() -> Node*
            {
                if (!write->cache || (*write)->Empty()) {
                    if (write->cache) {
                        (*write)->Clear();
                    }
                    return End();
                } else if (write->next == read) {
                    auto* tmp = new Node;
                    tmp->next = write->next;
                    tmp->prev = write;
                    write->next->prev = tmp;
                    write->next = tmp;
                    ++node_size_;
                }
                write = write->next;
                HARE_ASSERT(write != read);
                return End();
            }
            
        };
        
    } // namespace detail

    struct BufferIteratorImpl : public hare::detail::Impl {
        const detail::CacheList* list;
        detail::CacheList::Node* iter {};
        std::size_t curr_index {};

        HARE_INLINE
        explicit BufferIteratorImpl(detail::CacheList* _list)
            : list(_list)
            , iter(_list->Begin())
            , curr_index(hare::detail::ToUnsigned((*_list->Begin())->Readable() - (*_list->Begin())->Data()))
        { }

        HARE_INLINE
        BufferIteratorImpl(const detail::CacheList* _list, detail::CacheList::Node* _iter)
            : list(_list)
            , iter(_iter)
            , curr_index((*_iter)->size())
        { }

        ~BufferIteratorImpl() override = default;
    };

} // namespace net
} // namespace hare

#endif // _HARE_NET_BUFFER_INL_H_