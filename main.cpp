#include<chrono>
#include<vector>
#include<numeric>
#include<algorithm>
#include<iostream>
#include"align_memory_field.hpp"
#include<benchmark/benchmark.h>




static void foo_tuple(benchmark::State& state)
{
    std::tuple<int, double, char> tp{};
    for (auto _ : state) {
        std::get<0>(tp) = 10;
        std::get<1>(tp) = 2.33;
        std::get<2>(tp) = 'l';
        std::get<0>(tp); std::get<1>(tp); std::get<2>(tp);
    }

}

void foo_align_offset_f(benchmark::State& state)
{

    align_memory_field<8, int, double, char> m_field{};
    for (auto _ : state) {
        m_field.template write<0>(10);
        m_field.template write<1>(2.33);
        m_field.template write<2>('l');
        m_field.read<0>(); m_field.read<1>(); m_field.read<2>();
    }


}
BENCHMARK(foo_align_offset_f);
BENCHMARK(foo_tuple);
BENCHMARK(foo_align_offset_f);
BENCHMARK(foo_tuple);
BENCHMARK(foo_align_offset_f);
BENCHMARK(foo_tuple);
BENCHMARK(foo_align_offset_f);
BENCHMARK(foo_tuple);
BENCHMARK(foo_align_offset_f);
BENCHMARK(foo_tuple);
BENCHMARK(foo_align_offset_f);
BENCHMARK(foo_tuple);

BENCHMARK_MAIN();
