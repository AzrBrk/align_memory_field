#include<iostream>
#include"meta_object_traits.hpp"
#include"offset_pointer.hpp"
#include"align_memory_field.hpp"
#include<string>

using namespace offset_pointer;
using namespace meta_typelist;
using namespace list_common_object;

#define print_in_new_line(x) std::cout << '\n' << x << '\n';\

namespace my_detail {
    struct append_type_f
    {
        template<class this_align_t, class T> struct impl
        {
            using type = this_align_t;
        };
        template<class ...typs, class T> struct impl<static_align_memory_field<typs...>, T>
        {
            using type = static_align_memory_field<typs..., T>;
        };

        template<class T> struct impl<meta_empty, T>
        {
            using type = static_align_memory_field<T>;
        };

        template<class this_align_t, class T> using apply = typename impl<this_align_t, T>::type;

    };

    using align_type_append_o = meta_object<meta_empty, append_type_f>;
}
//warning: the following codes use STMP 
//a trick that is not suitable for real work using

//STMP implementation, using the keyword friend to inject the complier
//the friend keyword can be used in different classes, while
//keeping the friend function as one among those classes, 
//but not different functions
template<std::size_t I> struct reader
{
    friend auto stmp_flag(reader<I>);
};

//Note: the argument tag: when a lambda is created, it has a different type from the others
//even if they're written in a same form, it is used to prevent the compiler
//from saving the result of a certain operation

template<std::size_t I, class Tylist, auto tag = [] {} > struct setter
{
    friend auto stmp_flag(reader<I>) { return Tylist{}; }
    std::size_t value = I;
};

template<class T> constexpr const char* type_str()
{
    return typeid(T).name();
}

template struct setter<0, my_detail::align_type_append_o>;

template<std::size_t I, auto tag = [] {} >
constexpr bool has_I = requires(reader<I> rdr) { stmp_flag(rdr); };

template<std::size_t I, auto tag = [] {} > requires has_I<I>
using stmp_nth_Tylist = typename decltype(stmp_flag(reader<I>{}))::type;

template<class T, std::size_t I = 0, auto tag = [] {}, bool condition = has_I<I >>
    constexpr std::size_t stmp_append()
{
    if constexpr (!condition) {
        using current_list_o = decltype(stmp_flag(reader<I - 1>{}));
        setter<I, meta_invoke<current_list_o, T>> s{};
        reader<I> r{};
        return I;
    }
    else {
        return stmp_append<T, I + 1>();
    }
}
template<bool condition, std::size_t I = 0, auto tag = [] {} >//true
struct stmp_latest_list {
    using type = typename stmp_latest_list<has_I<I + 1>, I + 1>::type;
};
template<std::size_t I, auto tag >
struct stmp_latest_list<false, I, tag>
{
    using type = stmp_nth_Tylist<I - 1>;
};

template<std::size_t I = 0, auto tag = [] {} >
using stmp_latest_t = typename stmp_latest_list<has_I<I>, I>::type;


int main()
{
    unsigned char base_ptr[200];
    constexpr auto a = stmp_append<int>();
    constexpr auto b = stmp_append<double>();
    constexpr auto c = stmp_append<char>();

    stmp_latest_t<>{base_ptr}.write<0>(10);
    stmp_latest_t<>{base_ptr}.write<1>(12.33);
    stmp_latest_t<>{base_ptr}.write<2>('k');

    std::cout << "current_align_field_type:" << type_str<stmp_latest_t<>>() << std::endl;

    constexpr auto d = stmp_append<const char*>();
    std::cout << "current_align_field_type:" << type_str<stmp_latest_t<>>() << std::endl;

    stmp_latest_t<>{base_ptr}.write<3>("hello world");

    template_func_execute_launcher(meta_iota<3>{}, 
        [&base_ptr]<std::size_t ix>(Idx<ix>) {
        std::cout << stmp_latest_t<>{base_ptr}.read<ix>() << std::endl;
    });
    
    constexpr auto e = stmp_append<std::string>();
    
    stmp_latest_t<>{base_ptr}.initialize<4>("hello world");
    

    template_func_execute_launcher(meta_iota<4>{}, 
        [&base_ptr]<std::size_t ix>(Idx<ix>) {
        std::cout << stmp_latest_t<>{base_ptr}.read<ix>() << std::endl;
    });
}
