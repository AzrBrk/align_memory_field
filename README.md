# align_memory_field

`align_memory_field` 是一个异构容器，允许在内存中创建一段由指定对齐限制所构成的数据块，并提供类似于 `std::tuple` 的编译期常数访问。该容器的所有指针偏移都基于编译时运算。

## 注意事项

需要C++20， 测试通过:msvc latest, clang17.0, gcc13.2

## 使用

所有的文件都是头文件，只需包含头文件即可。

### 声明

```cpp
template<std::size_t pack_size, class ...Args> class align_memory_field
```

- `pack_size`: 指定的对齐长度，合理的对齐长度将加快内存存取。
- `Args...`: 需要创建的类型。

### 示例

```cpp
align_memory_field<8, int, double, char> m_field{};

// 写入
m_field.template write<0>(10);
m_field.template write<1>(2.33);
m_field.template write<2>('l');

// 读取
m_field.read<0>(); m_field.read<1>(); m_field.read<2>();

// 转换
struct X { int a; double b; char c; };

X* ptr = m_field.template cast<X>();
```

需要注意的是：

- 不能声明引用类型。
- 不会自动初始化内存。
- 在出现异常时，可能存在内存泄漏的风险。
- 暂未支持聚合体中拥有数组，但可以有指针，数组是可实现的，但尚未实现。

## 优势

`align_memory_field` 的性能优势主要体现在以下几个方面：

- **对齐优化：** 使用指定的对齐长度，有助于提高内存存取速度，特别是对于大量数据的处理。

- **编译期常数访问：** 通过编译时运算的方式，提供了类似于 `std::tuple` 的访问接口，同时避免了运行时性能开销。

- **灵活性与效率的平衡：** 拥有和 std::tuple几乎一样的效率（可以运行main.cpp中的基准测试):

```cpp  
2024-02-10T07:15:15+08:00
Running G:\cpp\lib\Meta_Object\x64\Release\MO.exe
Run on (12 X 3493 MHz CPU s)
CPU Caches:
  L1 Data 32 KiB (x6)
  L1 Instruction 32 KiB (x6)
  L2 Unified 512 KiB (x6)
  L3 Unified 32768 KiB (x1)
--------------------------------------------------------------
Benchmark                    Time             CPU   Iterations
--------------------------------------------------------------
foo_align_offset_f4       3.91 ns         2.13 ns    263529412
foo_align_offset_f8       3.66 ns         2.67 ns    298666667
foo_tuple                 3.69 ns         2.57 ns    280000000
foo_align_offset_f4       3.90 ns         3.01 ns    280000000
foo_align_offset_f8       3.66 ns         2.73 ns    280000000
foo_tuple                 3.75 ns         2.52 ns    235789474
foo_align_offset_f4       3.89 ns         2.79 ns    224000000
foo_align_offset_f8       3.42 ns         2.61 ns    263529412
foo_tuple                 3.64 ns         2.52 ns    235789474
foo_align_offset_f4       3.87 ns         2.93 ns    224000000
foo_align_offset_f8       3.42 ns         2.13 ns    344615385
foo_tuple                 3.63 ns         2.78 ns    235789474
foo_align_offset_f4       3.88 ns         2.92 ns    235789474
foo_align_offset_f8       3.41 ns         2.85 ns    280000000
foo_tuple                 3.65 ns         2.62 ns    280000000
```

这里可以看出来8在int string int的内存组合下，8位内存对齐是最优的
解释了为什么要内存对齐的原理

  

- **特点** 可以与拥有同样类型结构的结构体之间相互转化，并且无转化开销，转化使用`reinterpret_cast`。

## 示例说明

以下是一些关键操作的示例说明：

### 写入数据

```cpp
m_field.template write<0>(10);
m_field.template write<1>(2.33);
m_field.template write<2>('l');
```

### 读取数据

```cpp
int val1 = m_field.read<0>();
double val2 = m_field.read<1>();
char val3 = m_field.read<2>();
```

### 自定义类型转换

```cpp
struct X { int a; double b; char c; };
X* ptr = m_field.template cast<X>();

decltype(m_field) m_field2{};
copy_struct(m_field2, *ptr);

m_field2.write<0>(10);
//...
```

注意：

- 当你在你的代码中使用了possibilities这个元编程库时，align_memory_field将验证指定的结构体是否拥有相同的
类型成员。但不是必须的，使用了之后不会带来性能损失，但是会带来一定的代码限制。
- 当你将m_field的内存转换为一个结构体之后，请保证m_field的生存，因为m_field在析构时将销毁它拥有的内存

PS：possibilities是一个在编译期推导聚合类中的成员的工具。

### 支持placement new

假如你类型列表中的类型需要初始化，则在赋值之前应当初始化它，使用如下代码：

```cpp
 align_memory_field<8, std::string, int> m_field{};

 m_field.initialize<0>();

 m_field.write<0>("hello");
 m_field.write<1>(443);

 struct hello { std::string str{}; int No; };

 hello* Hello = m_field.cast<hello>();

 std::print("m_field:{}, No.{}", Hello->str, Hello->No);
```

### 辅助模板

align_memory_field允许你使用任意的对齐长度， 但当你不知道你应该采取怎样的对齐长度时，
你可以通过头文件提供的别名模板decl_align来获取默认的对齐长度。

```cpp
#include"align_memory_field.hpp"

int main()
{
    constexpr std::size_t align_s = decl_align<int, const char*, double>;
    align_memory_field<align_s, int, const char*, double> m_field;
}
```

### 内存管理

align_memory_field在无异常的情况下可以自动管理申请的内存

```cpp
#include"align_memory_field.hpp"
#include<iostream>
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>

template<class ...Args> 
    requires std::conjunction_v<std::is_fundamental<std::remove_cvref_t<Args>>...>
auto make_field(Args const& ...args)
{
    constexpr std::size_t align_s = decl_align<Args...>;

    align_memory_field<align_s, std::decay_t<Args>...> field{};

    [&] <std::size_t ...I, class ...LArgs>(std::index_sequence<I...>, LArgs const& ...largs)
    {
        (field.write<I>(largs), ...);
    }(std::make_index_sequence<sizeof...(Args)>{}, args...);
    
    return field;
}

void detect_leak()
{
    auto mf = std::move(make_field(1, 2.33, 'k', 1.f)); //移动构造，推荐使用，由于使用动态内存，直接交换指针即可，消亡的对象不会释放指针，只有当mf析构时才会释放内存；
    std::cout << mf.read<2>();

    auto mf2 = mf;//复制构造，但无需手动释放内存，但是需要深拷贝, align_memory_field会直接调用std::copy，不会进行赋值操作，这意味着你的复制构造函数，重载赋值运算符将不会生效
}

int main()
{
    detect_leak();
    _CrtDumpMemoryLeaks();
}
```

