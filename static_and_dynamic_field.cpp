#include"align_memory_field.hpp"
#include<iostream>

auto do_nothing = [](unsigned char* p) {};

struct X {
    int a;
    char str[20]{ "hello world" };
    int arr[10]{};
    double c;
};

template<class T, std::size_t N> constexpr void print_array(T (&arr) [N])
{
    for (const auto& i: arr) {
        std::cout << i;
    }
    std::cout << std::endl;
}

int main() {
    X x{};
    align_memory_field<8, 
        decltype(do_nothing), //do nothing because it is not a allocated memory blocked in heap
        int, char[20], int[10], double
    > static_field{ &x };

    auto& ch_arr_ref = static_field.read<1>();//get ref of X::str
    auto& int_arr_ref = static_field.read<2>();//get ref of X::arr

    for (auto i = 0; i < 10; ++i) {
        int_arr_ref[i] = i;
    }

    align_memory_field<8, int, char[20], int[10], double> dynamic_field{};

    copy_struct<X>(dynamic_field, x);

    [&] <std::size_t ...I>(std::index_sequence<I...>){
        (print_array(dynamic_field.template read<I>()), ...);
    }(std::index_sequence<1,2>{});
}
