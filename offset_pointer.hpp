#pragma once
#include<type_traits>
namespace offset_pointer {
    template<typename structT, typename T>
    T& read_from_align_offset(structT* base_ptr, const std::size_t offset)
    {
        return *(reinterpret_cast<T*>(reinterpret_cast<unsigned char*>(base_ptr) + offset));
    }

    //to fix the std::conditional_t with SFINAE
    template<bool con, class T> struct invoke_type_if_impl
    {
        using type = T;
    };

    template<class T> struct invoke_type_if_impl<true, T>
    {
        using type = typename T::type;
    };

    template<bool con, class T> using invoke_type_if = typename invoke_type_if_impl<con, T>::type;

    //stimulate members layout in a structure
    template<std::size_t pack_size, std::size_t layer_offset, std::size_t offset> struct align_offset_ptr
    {
        static constexpr std::size_t layer = layer_offset;//alignment layer as base
        static constexpr std::size_t value = offset;//offset from the current layer
        template<std::size_t lay> struct advance_layer
        {
            using type = align_offset_ptr<pack_size, layer + lay, 0>;
        };
        template<bool can_be_placed, std::size_t seek_offset> struct seek_impl
        {
            using type = align_offset_ptr<pack_size, layer, value>;
        };
        template<std::size_t seek_offset> struct seek_impl<false, seek_offset>
        {
            using type = typename std::conditional_t<
                (value + seek_offset > pack_size),
                typename advance_layer<1>::type,
                align_offset_ptr<pack_size, layer_offset, value + 1>
                >::template seek<seek_offset>;
        };
        template<std::size_t seek_offset> using seek = typename seek_impl<value% seek_offset == 0, seek_offset>::type;

        template<std::size_t advance_offset> struct advance_impl
        {
            using type = std::conditional_t<(value + advance_offset < pack_size),
                align_offset_ptr<pack_size, layer, value + advance_offset>,
                std::conditional_t<
                static_cast<bool>(advance_offset% pack_size),
                typename advance_layer<advance_offset / pack_size + 1>::type,
                typename advance_layer<advance_offset / pack_size>::type>
            >;
        };
        template<std::size_t advance_offset> using advance = typename advance_impl<advance_offset>::type;
       
        static constexpr std::size_t offset_v = layer * pack_size + value;
    };

 
    //align offset iterator impl
    template<class align_offset_pointer, class T>
    struct align_offset_advance_basic
    {
        using type = typename align_offset_pointer
            ::template seek<sizeof(T)>
            ::template advance<sizeof(T)>;
    };

    template<class align_offset_pointer, class T>
    using align_offset_advance_basic_t =
        typename align_offset_advance_basic<align_offset_pointer, T>::type;

    //array in structure is considered as N members
    template<class align_offset_pointer, class array_t> struct align_offset_advance_array_impl {};
    template<class align_offset_pointer, class basic_type, std::size_t N>
    struct align_offset_advance_array_impl<align_offset_pointer, basic_type[N]>
    {
        using type = typename align_offset_advance_array_impl<
            align_offset_advance_basic_t<align_offset_pointer, basic_type>,
            basic_type[N - 1]>::type;
    };
    template<class align_offset_pointer, class basic_type>
    struct align_offset_advance_array_impl<align_offset_pointer, basic_type[1]>
    {
        using type = align_offset_advance_basic_t<align_offset_pointer, basic_type>;
    };

    template<class align_offset_pointer, class array_t>
    using align_offset_advance_array_t = typename align_offset_advance_array_impl<
        align_offset_pointer, array_t
    >::type;

    template<class align_offset_pointer, class T> 
    using align_offset_advance_t = std::conditional_t<
        std::is_array_v<T>,
        invoke_type_if<std::is_array_v<T>, align_offset_advance_array_impl<align_offset_pointer, T>>,
        invoke_type_if<!std::is_array_v<T>, align_offset_advance_basic<align_offset_pointer, T>>
    >;

    
    template<class align_offset_pointer, class T> struct align_offset_seek_impl
    {
        using type = std::conditional_t<std::is_array_v<T>,
            typename align_offset_pointer::template seek<sizeof(std::remove_extent_t<T>)>,
            typename align_offset_pointer::template seek<sizeof(T)>
        >;
    };

    template<class align_offset_pointer, class T>
    using align_offset_seek_t = typename align_offset_seek_impl<align_offset_pointer, T>::type;

    template<class align_offset_pointer, class offset_map, std::size_t N>
    struct align_offset_iterator
    {
        using type = align_offset_pointer;
    };

    template<class align_offset_pointer, std::size_t N,
        template<class ...> class offset_map, class first, class ...rest>
    struct align_offset_iterator<align_offset_pointer, offset_map<first, rest...>, N>
    {
        using recursive_type = align_offset_iterator<
            align_offset_advance_t<align_offset_pointer, first>,
            offset_map<rest...>, N - 1>;
        using type = typename recursive_type::type;
        using get = typename recursive_type::get;
        using value_type = typename recursive_type::value_type;
    };
    template<class align_offset_pointer, template<class...> class offset_map, class first, class ...rest>
    struct align_offset_iterator<align_offset_pointer, offset_map<first, rest...>, 0>
    {
        //kept pointer
        using type = align_offset_pointer;
        //common interface
        using get = align_offset_seek_t<type, first>;
        //common interface for Ty info
        using value_type = first;
    };

    template<class ...> struct offset_pattern {};
    template<std::size_t pack_size, class ...typs> 
    struct offset_vector
    {
        template<std::size_t I>
        using iterator = align_offset_iterator<
            align_offset_ptr<pack_size, 0, 0>,
            offset_pattern<typs...>, I>;

        template<std::size_t I> 
        static constexpr std::size_t offset_v = iterator<I>::get::offset_v;

        template<std::size_t I> 
        using value_type = typename iterator<I>::value_type;
    };
}