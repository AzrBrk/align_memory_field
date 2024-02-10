#pragma once
#include"meta_object_traits.hpp"
#include"offset_pointer.hpp"


using namespace offset_pointer;
using namespace meta_typelist;
using namespace list_common_object;

//crash when try to double delete the allocate memory
#define not_null (unsigned char*)(0x66)

//meta_object function for offset calculate
struct seek_and_advance
{
    template<typename this_align_off_ptr, typename T>
    using apply = typename this_align_off_ptr
        ::template seek<sizeof(T)>
        ::template advance<sizeof(T)>;
};

//loop the meta_object in a meta_stream
//with each loop the each align offset pointer advance to the end
//offset of each member type should be
template<std::size_t pack_size, class TL> using seek_loop = make_timer_loop<
    meta_all_transfer_o<
    meta_object<align_offset_ptr<pack_size, 0, 0>, seek_and_advance>,
    meta_istream<TL>
    >
>;

//collect the result from the meta looper
template<std::size_t pack_size, class TL>
using seek_collect = typename meta_to_array<meta_iota<max_index<TL> -1>>
::template to<exp_select_list>
::template apply<typename collector::collect<seek_loop<pack_size, TL>>>;

//the cache of each meta_stream store the next type of current states
//so we use these cache to seek the begin offset of each member type
template<class MSO> using seek_cache = typename MSO::to::type
::template seek<sizeof(typename MSO::cache)>;

//collect all the begin offset of each member type
//result is a series of meta align pointers
template<std::size_t pack_size, class TL>
using align_offset_pointer_collect = typename add_to_front<align_offset_ptr<pack_size, 0, 0>,
    typename exp_apply<seek_collect<pack_size, TL>, seek_cache>::type>::type;

//friend function to copy from a custom class
template<class structT, class AMF>
inline void copy_struct(AMF& amf, structT&& s);

template<class T> concept is_functor = requires{
    &T::operator();
};

//we don't save the function itself but just a type to generate the destructor
//so it must be a functor but not a function pointer
template<class T> constexpr bool is_functor_v = is_functor<T> && !std::is_function_v<T>;

//RAII for new
template<std::size_t pack_size, class Self_destruct_determin_t, class ...Args> 
class align_memory_field
{
private:
    //judge if the first type in given list is a functor
    static constexpr bool Is_self_destructible = is_functor_v<Self_destruct_determin_t>;

    //if first type is a functor, the first type should be ignore in offset calculation
    using Member_types = exp_if<Is_self_destructible,
        selectable_list<Args...>, //true
        selectable_list<Self_destruct_determin_t, Args...>//false
    >;

    //member types iterator
    template<std::size_t I> using Member_t = typename Member_types::template get<I>;

    //all pointer offsets, generated in compile time
    //we don't want to waste the runtime to calculate what's already known
    using All_align_ptrs = align_offset_pointer_collect<pack_size, Member_types>;

    //a meta pointer points to the end offset of this memory field
    using Last_off_t =
        typename exp_select<max_index<All_align_ptrs>, All_align_ptrs>
        ::template advance<sizeof(exp_select<max_index<Member_types>, Member_types>)>
        ::template seek<pack_size>;

    unsigned char* base_ptr{ nullptr };
    
public:
    
    //allocate constructor
    align_memory_field(std::size_t N = Last_off_t::layer * pack_size)
        :
        base_ptr(new unsigned char[N])
    {}

    //static constructor
    template<class En = std::enable_if_t<Is_self_destructible>>
    align_memory_field(unsigned char* p_memory_locate)
        :
        base_ptr(p_memory_locate)
    {}

    //disabled when not applying default delete
    template<class En = std::enable_if_t<!Is_self_destructible>>
    align_memory_field(const align_memory_field& another)
        :
        align_memory_field()
    {
        std::copy(another.base_ptr, another.base_ptr + Last_off_t::layer * pack_size, base_ptr);
    }

    //disabled when not applying default delete
    template<class En = std::enable_if_t<!Is_self_destructible>>
    align_memory_field(align_memory_field&& another) noexcept
    {
        base_ptr = another.base_ptr;
        //set to nullptr to prevent the dying object from releasing the memory
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

    //copy a struct to the field
    template<class structT, class ...TArgs>
    friend inline void copy_struct(align_memory_field<alignof(structT), TArgs...>& amf, structT&& s);

    //use placement new to construct a certain type if needed
    template<std::size_t I, class Source_t> requires std::is_convertible_v<Source_t, Member_t<I>> 
    constexpr void initialize(Source_t const& value)
    {
        using Dest_offset_t = exp_select<I, All_align_ptrs>;

        unsigned char* place_ptr = base_ptr + Dest_offset_t::layer * pack_size + Dest_offset_t::value;

        new(place_ptr) Member_t<I>{value};
    }

    //write const data
    template<std::size_t I, class Source_t> requires std::is_convertible_v<Source_t, Member_t<I>>
    constexpr void write(Source_t const& value)
    {
        using Dest_offset_t = exp_select<I, All_align_ptrs>;

        read_from_align_offset<unsigned char, Member_t<I>>(
           base_ptr, Dest_offset_t::layer * pack_size + Dest_offset_t::value) = value;
    }

    //write ref, this is much faster if you already have the reference
    template<std::size_t I> requires (!std::is_fundamental_v<Member_t<I>>)
    constexpr void write(Member_t<I> & value)
    {
        using Dest_offset_t = exp_select<I, All_align_ptrs>;

        unsigned char* place_ptr = base_ptr + Dest_offset_t::layer * pack_size + Dest_offset_t::value;

        std::copy(reinterpret_cast<unsigned char*>(&value), reinterpret_cast<unsigned char*>(&value) + sizeof(Member_t<I>), place_ptr);
    }

    template<std::size_t I> constexpr Member_t<I> & read()
    {
        using Dest_offset_t = exp_select<I, All_align_ptrs>;

        return read_from_align_offset<unsigned char, Member_t<I>>(
            base_ptr, Dest_offset_t::layer * pack_size + Dest_offset_t::value);
    }
    ~align_memory_field()
    {
        if constexpr (!Is_self_destructible) {
            if (base_ptr) {
                delete[] base_ptr;
            }
            base_ptr = not_null;
        }
        else {
            Self_destruct_determin_t{}(base_ptr);
        }
        
    }
};

//tool for alignment calculate
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

template<class structT, class ...Args>
inline void copy_struct(align_memory_field<alignof(structT), Args...>& amf, structT&& s)
{
    new(amf.base_ptr) std::remove_cvref_t<structT>{ s };
}
