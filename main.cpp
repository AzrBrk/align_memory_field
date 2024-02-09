#include<chrono>
#include<vector>
#include<numeric>
#include<algorithm>
#include<iostream>
#include<string>
#include"align_memory_field.hpp"
#include<benchmark/benchmark.h>



void foo_align_offset_f4(benchmark::State& state)
{
    //stimulate if alignsize is 4
    align_memory_field<4, int, std::string, int> m_field{};
    m_field.initialize<1>("");
    for (auto _ : state) {
        m_field.template write<0>(10);
        m_field.template write<1>("very long hello worldhello worldhello world");
        m_field.template write<2>(99);
        m_field.read<0>(); m_field.read<1>(); m_field.read<2>();
    }


}
void foo_align_offset_f8(benchmark::State& state)
{

    align_memory_field<8, int, std::string, int> m_field{};
    m_field.initialize<1>("");
    for (auto _ : state) {
        m_field.template write<0>(10);
        m_field.template write<1>("very long hello worldhello worldhello world");
        m_field.template write<2>(99);
        m_field.read<0>(); m_field.read<1>(); m_field.read<2>();
    }


}
static void foo_tuple(benchmark::State& state)
{
    std::tuple<int, std::string, int> tp{};
    for (auto _ : state) {
        std::get<0>(tp) = 10;
        std::get<1>(tp) = "very long hello worldhello worldhello world";
        std::get<2>(tp) = 99;
        std::get<0>(tp); std::get<1>(tp); std::get<2>(tp);
    }

}
BENCHMARK(foo_align_offset_f4);
BENCHMARK(foo_align_offset_f8);
BENCHMARK(foo_tuple);

BENCHMARK(foo_align_offset_f4);
BENCHMARK(foo_align_offset_f8);
BENCHMARK(foo_tuple);

BENCHMARK(foo_align_offset_f4);
BENCHMARK(foo_align_offset_f8);
BENCHMARK(foo_tuple);

BENCHMARK(foo_align_offset_f4);
BENCHMARK(foo_align_offset_f8);
BENCHMARK(foo_tuple);

BENCHMARK(foo_align_offset_f4);
BENCHMARK(foo_align_offset_f8);
BENCHMARK(foo_tuple);

BENCHMARK_MAIN();
