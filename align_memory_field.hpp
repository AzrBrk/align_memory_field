#pragma once
#include"meta_object_traits.hpp"
#include"offset_pointer.hpp"


using namespace offset_pointer;
using namespace meta_typelist;
using namespace list_common_object;

//crash when try to double delete the allocate memory
#define not_null (unsigned char*)(0x66)


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
    static constexpr std::size_t Max_types_index = Is_self_destructible ?
        sizeof...(Args) - 1 :
        sizeof...(Args);

    //if first type is a functor, the first type should be ignore in offset calculation
    using Member_offset_vector = exp_if<Is_self_destructible,
        offset_vector<pack_size, Args...>, //true
        offset_vector<pack_size, Self_destruct_determin_t, Args...>//false
    >;

    static constexpr std::size_t End_of_field_offset = align_offset_advance_t<
        typename Member_offset_vector :: template iterator<Max_types_index>::get,
        typename Member_offset_vector :: template value_type<Max_types_index>
    >::template seek<pack_size>::offset_v;
        
        

    unsigned char* base_ptr{ nullptr };
    
public:
    
    //allocate constructor
    align_memory_field(std::size_t N = End_of_field_offset)
        :
        base_ptr(new unsigned char[N])
    {}

    //static constructor
    template<class Obj_t, class En = std::enable_if_t<Is_self_destructible>>
    align_memory_field(Obj_t* p_memory_locate)
        :
        base_ptr(reinterpret_cast<unsigned char*>(p_memory_locate))
    {}

    align_memory_field(const align_memory_field& another)
        :
        align_memory_field()
    {
        std::copy(another.base_ptr, another.base_ptr + End_of_field_offset, base_ptr);
    }

    align_memory_field(align_memory_field&& another) noexcept
    {
        if constexpr (!Is_self_destructible) {
            base_ptr = another.base_ptr;
            //set to nullptr to prevent the dying object from releasing the memory
            another.base_ptr = nullptr;
        }
        else
        {
            std::copy(another.base_ptr, another.base_ptr + End_of_field_offset, base_ptr);
        }
    }

    template<typename structT> structT* cast() const
    {
        static_assert(alignof(structT) == pack_size);

        return reinterpret_cast<structT*>(base_ptr);
    }

    //copy a struct to the field
    template<class structT, class ...TArgs>
    friend inline void copy_struct(align_memory_field<alignof(structT), TArgs...>& amf, structT&& s);

    //use placement new to construct a certain type if needed
    template<std::size_t I, class Source_t> requires std::is_convertible_v<
        Source_t, 
        typename Member_offset_vector::template value_type<I>
    > 
    constexpr void initialize(Source_t const& value)
    {
        unsigned char* place_ptr = base_ptr + Member_offset_vector::template offset_v<I>;

        new(place_ptr) typename Member_offset_vector::template value_type<I>{value};
    }

    //write const data
    template<std::size_t I, class Source_t> 
        requires std::is_convertible_v<Source_t, typename Member_offset_vector::template value_type<I>>
    constexpr void write(Source_t const& value)
    {
        read_from_align_offset<unsigned char, typename Member_offset_vector::template value_type<I>>(
           base_ptr, Member_offset_vector::template offset_v<I>) = value;
    }

    //write ref, this is much faster if you already have the reference
    template<std::size_t I> requires (!std::is_fundamental_v<typename Member_offset_vector::template value_type<I>>)
    constexpr void write(typename Member_offset_vector::template value_type<I>& value)
    {
        unsigned char* place_ptr = base_ptr + Member_offset_vector::template offset_v<I>;

        std::copy(
            reinterpret_cast<unsigned char*>(&value), 
            reinterpret_cast<unsigned char*>(&value) + 
            sizeof(typename Member_offset_vector::template value_type<I>), 
            place_ptr
        );
    }

    template<std::size_t I> 
    constexpr typename Member_offset_vector::template value_type<I>& read()
    {
       
        return read_from_align_offset<
            unsigned char, 
            typename Member_offset_vector::template value_type<I>>(
                    base_ptr, Member_offset_vector::template offset_v<I>
            );
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
