# align_memory_field

C++ 元偏移指针

## 注意

需要C++20， 测试通过:msvc latest, clang17.0, gcc13.2

# align_memory_field

`align_memory_field` 是一个异构容器，允许在内存中创建一段由指定对齐限制所构成的数据块，并提供类似于 `std::tuple` 的编译期常数访问。该容器的所有指针偏移都基于编译时运算。

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

## 优势

`align_memory_field` 的性能优势主要体现在以下几个方面：

- **对齐优化：** 使用指定的对齐长度，有助于提高内存存取速度，特别是对于大量数据的处理。

- **编译期常数访问：** 通过编译时运算的方式，提供了类似于 `std::tuple` 的访问接口，同时避免了运行时性能开销。

- **灵活性与效率的平衡：** 相对于一些通用容器，`align_memory_field` 可以在特定场景下取得更好的性能表现，但需要注意在其他方面可能会有些许限制。

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

### 转换为自定义类型

```cpp
struct X { int a; double b; char c; };
X* ptr = m_field.template cast<X>();
```

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

## 性能比较

通过与 `std::tuple` 的基准测试，`align_memory_field` 在特定场景下表现更为优异，但在实际应用中，建议根据具体需求和场景选择最适合的数据结构。
