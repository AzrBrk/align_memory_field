#pragma once
#include"meta_object_traits.hpp"
#include"offset_pointer.hpp"

using namespace offset_pointer;
using namespace meta_typelist;
using namespace list_common_object;
struct seek_and_advance
{
    template<typename this_align_off_ptr, typename T>
    using apply = typename this_align_off_ptr
        ::template seek<sizeof(T)>
        ::template advance<sizeof(T)>;
};

template<std::size_t pack_size, class TL> using seek_loop = make_timer_loop<
    meta_all_transfer_o<
    meta_object<align_offset_ptr<pack_size, 0, 0>, seek_and_advance>,
    meta_istream<TL>
    >
>;

template<std::size_t pack_size, class TL>
using seek_collect = typename meta_to_array<meta_iota<max_index<TL> -1>>
::template to<exp_select_list>
::template apply<typename collector::collect<seek_loop<pack_size, TL>>>;


template<class MSO> using seek_cache = typename MSO::to::type
::template seek<sizeof(typename MSO::cache)>;

template<std::size_t pack_size, class TL>
using align_offset_pointer_collect = typename add_to_front<align_offset_ptr<pack_size, 0, 0>,
    typename exp_apply<seek_collect<pack_size, TL>, seek_cache>::type>::type;

//RAII for new
template<std::size_t pack_size, class ...Args> class align_memory_field
{
private:

    unsigned char* base_ptr{ nullptr };
    using All_align_ptrs = align_offset_pointer_collect<pack_size, exp_list<Args...>>;
    using Last_off_t =
        typename exp_select<max_index<All_align_ptrs>, All_align_ptrs>
        ::template advance<sizeof(exp_select<max_index<exp_list<Args...>>, exp_list<Args...>>)>
        ::template seek<pack_size>;

public:

    align_memory_field(std::size_t N = Last_off_t::layer * pack_size)
        :
        base_ptr(new unsigned char[N])
    {}

    align_memory_field(const align_memory_field& another)
        :
        align_memory_field()
    {
        std::copy(another.base_ptr, another.base_ptr + Last_off_t::layer * pack_size, base_ptr);
    }

    align_memory_field(align_memory_field&& another)
    {
        base_ptr = another.base_ptr;
        //必须赋值为nullptr，否则当被移动的对象消亡，析构函数就会释放该内存
        another.base_ptr = nullptr;
    }

    template<typename structT> structT* cast() const
    {
        return reinterpret_cast<structT*>(base_ptr);
    }

    template<std::size_t I> constexpr void initialize()
    {
        using Dest_offset_t = exp_select<I, All_align_ptrs>;

        unsigned char* place_ptr = base_ptr + Dest_offset_t::layer * pack_size + Dest_offset_t::value;

        new(place_ptr) exp_select<I, exp_list<Args...>>;
    }

    template<std::size_t I> constexpr void write(exp_select<I, exp_list<Args...>> const& value)
    {
        using Dest_offset_t = exp_select<I, All_align_ptrs>;

        read_from_align_offset<unsigned char, exp_select<I, exp_list<Args...>>>(
            base_ptr, Dest_offset_t::layer * pack_size + Dest_offset_t::value) = value;
    }
    template<std::size_t I> constexpr exp_select<I, exp_list<Args...>> read()
    {
        using Dest_offset_t = exp_select<I, All_align_ptrs>;

        return read_from_align_offset<unsigned char, exp_select<I, exp_list<Args...>>>(
            base_ptr, Dest_offset_t::layer * pack_size + Dest_offset_t::value);
    }
    ~align_memory_field()
    {
        if (base_ptr) {
            delete[] base_ptr;
        }
        base_ptr = nullptr;
    }
};
