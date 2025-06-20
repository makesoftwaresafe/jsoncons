### jsoncons::decode_json

Decodes a JSON data format to a C++ data structure. `decode_json` will 
work for all C++ classes that have [json_type_traits](https://github.com/danielaparker/jsoncons/blob/master/doc/ref/corelib/json_type_traits.md) defined.

```cpp
#include <jsoncons/decode_json.hpp>

template <typename T,typename CharsLike>
T decode_json(const CharsLike& s,
    const basic_json_decode_options<CharsLike::value_type>& options 
        = basic_json_decode_options<CharsLike::value_type>());                                  (1)

template <typename T,typename CharT>
T decode_json(std::basic_istream<CharT>& is,
    const basic_json_decode_options<CharT>& options = basic_json_decode_options<CharT>());      (2)

template <typename T,typename CharsLike,typename Allocator,typename TempAllocator>
T decode_json(const allocator_set<Allocator,TempAllocator>& alloc_set,
    const CharsLike& s,
    const basic_json_decode_options<CharsLike::value_type>& options 
        = basic_json_decode_options<CharsLike::value_type>());                                  (3) (since 0.171.0)

template <typename T,typename CharT,typename Allocator,typename TempAllocator>
T decode_json(const allocator_set<Allocator,TempAllocator>& alloc_set,
    std::basic_istream<CharT>& is,
    const basic_json_decode_options<CharT>& options = basic_json_decode_options<CharT>());      (4) (since 0.171.0)

template <typename T,typename Iterator>
T decode_json(Iterator first, Iterator last,
    const basic_json_decode_options<CharT>& options = basic_json_decode_options<CharT>());      (5)
```

(1) Reads JSON from a contiguous character sequence provided by `s` into a type T, using the specified (or defaulted) [options](basic_json_options.md). 
Type 'T' must be an instantiation of [basic_json](../basic_json.md) 
or support [json_type_traits](../json_type_traits.md).

(2) Reads JSON from an input stream into a type T, using the specified (or defaulted) [options](basic_json_options.md). 
Type 'T' must be an instantiation of [basic_json](../basic_json.md) 
or support [json_type_traits](../json_type_traits.md).

(3)-(4) are identical to (1)-(2) except an [allocator_set](allocator_set.md) is passed as an additional argument and
provides allocators for result data and temporary allocations.

(5) Reads JSON from the range [first,last) into a type T, using the specified (or defaulted) [options](basic_json_options.md). 
Type 'T' must be an instantiation of [basic_json](../basic_json.md) 
or support [json_type_traits](../json_type_traits.md).

#### Exceptions

Any overload may throw `std::bad_alloc` if memory allocation fails.

### Examples

#### Map with string-tuple pairs

```cpp
#include <iostream>
#include <map>
#include <tuple>
#include <jsoncons/json.hpp>

int main()
{
    using employee_collection = std::map<std::string,std::tuple<std::string,std::string,double>>;

    std::string s = R"(
    {
        "Jane Doe": ["Commission","Sales",20000.0],
        "John Smith": ["Hourly","Software Engineer",10000.0]
    }
    )";

    employee_collection employees = jsoncons::decode_json<employee_collection>(s);

    for (const auto& pair : employees)
    {
        std::cout << pair.first << ": " << std::get<1>(pair.second) << '\n';
    }
}
```
Output:
```
Jane Doe: Sales
John Smith: Software Engineer
```

### See also

[allocator_set](allocator_set.md)

[encode_json](encode_json.md)

