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

template<class structT, class AMF>
inline void copy_struct(AMF& amf, structT&& s);

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

    align_memory_field(align_memory_field&& another) noexcept
    {
        base_ptr = another.base_ptr;
        another.base_ptr = nullptr;
    }

    template<typename structT> structT* cast() const
    {
        static_assert(alignof(structT) == pack_size);
#ifdef HAS_POSSIBILITY //use the possiblity library to check if the dest struct is compatible
        if constexpr (requires{typename possibilities::possibility<structT, possibilities::tl<Args...>>; }) {
            if constexpr (!std::is_same_v<possibilities::tl<Args...>, Dest_Member_Types>) {
                return std::nullptr;
            }
        }
#endif 
        return reinterpret_cast<structT*>(base_ptr);
    }

    template<class structT, class ...TArgs>
    friend inline void copy_struct<>(align_memory_field<alignof(structT), TArgs...>& amf, structT&& s);

    template<std::size_t I, class Source_t> requires std::is_convertible_v<Source_t, exp_select<I, exp_list<Args...>>> constexpr void initialize(Source_t const& value)
    {
        using Dest_offset_t = exp_select<I, All_align_ptrs>;

        unsigned char* place_ptr = base_ptr + Dest_offset_t::layer * pack_size + Dest_offset_t::value;

        new(place_ptr) exp_select<I, exp_list<Args...>>{value};
    }

    template<std::size_t I, class Source_t> requires std::is_convertible_v<Source_t, exp_select<I, exp_list<Args...>>>
    constexpr void write(Source_t const& value)
    {
        using Dest_offset_t = exp_select<I, All_align_ptrs>;

        read_from_align_offset<unsigned char, exp_select<I, exp_list<Args...>>>(
           base_ptr, Dest_offset_t::layer * pack_size + Dest_offset_t::value) = value;
    }

    template<std::size_t I> requires (!std::is_fundamental_v<exp_select<I, exp_list<Args...>>>)
    constexpr void write(exp_select<I, exp_list<Args...>>& value)
    {
        using Dest_offset_t = exp_select<I, All_align_ptrs>;

        unsigned char* place_ptr = base_ptr + Dest_offset_t::layer * pack_size + Dest_offset_t::value;

        std::copy(reinterpret_cast<unsigned char*>(&value), reinterpret_cast<unsigned char*>(&value) + sizeof(exp_select<I, exp_list<Args...>>), place_ptr);
    }

    template<std::size_t I> constexpr exp_select<I, exp_list<Args...>>& read()
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

struct make_align
{
    template<class this_align, class T> using apply = exp_if<
        (alignof(T) > this_align::value), Idx<alignof(T)>, this_align>;
};

template<class TL> using decl_align_t = meta_all_transfer<
    meta_object<Idx<0>, make_align>,
    meta_istream<TL>
>::to::type;

template<class...Args>
constexpr std::size_t decl_align = decl_align_t<exp_list<Args...>>::value;

template<class ...Args> 
using default_align_memory_field = align_memory_field<decl_align<Args...>, Args...>;

template<class structT, class ...Args>
inline void copy_struct(align_memory_field<alignof(structT), Args...>& amf, structT&& s)
{
    new(amf.base_ptr) std::remove_cvref_t<structT>{ s };
}
