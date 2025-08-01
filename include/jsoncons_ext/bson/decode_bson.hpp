// Copyright 2013-2025 Daniel Parker
// Distributed under the Boost license, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// See https://github.com/danielaparker/jsoncons for latest version

#ifndef JSONCONS_EXT_BSON_DECODE_BSON_HPP
#define JSONCONS_EXT_BSON_DECODE_BSON_HPP

#include <istream> // std::basic_istream
#include <type_traits> // std::enable_if

#include <jsoncons/allocator_set.hpp>
#include <jsoncons/basic_json.hpp>
#include <jsoncons/config/compiler_support.hpp>
#include <jsoncons/conv_error.hpp>
#include <jsoncons/json_decoder.hpp>
#include <jsoncons/json_visitor.hpp>
#include <jsoncons/read_result.hpp>
#include <jsoncons/reflect/decode_traits.hpp>
#include <jsoncons/source.hpp>
#include <jsoncons/utility/more_type_traits.hpp>

#include <jsoncons_ext/bson/bson_cursor.hpp>
#include <jsoncons_ext/bson/bson_options.hpp>
#include <jsoncons_ext/bson/bson_reader.hpp>

namespace jsoncons { 
namespace bson {

template <typename T,typename BytesLike>
typename std::enable_if<ext_traits::is_basic_json<T>::value &&
                        ext_traits::is_byte_sequence<BytesLike>::value,read_result<T>>::type 
try_decode_bson(const BytesLike& v, 
    const bson_decode_options& options = bson_decode_options())
{
    using value_type = T;
    using result_type = read_result<value_type>;

    std::error_code ec;   
    jsoncons::json_decoder<T> decoder;
    auto adaptor = make_json_visitor_adaptor<json_visitor>(decoder);
    basic_bson_reader<jsoncons::bytes_source> reader(v, adaptor, options);
    reader.read(ec);
    if (JSONCONS_UNLIKELY(ec))
    {
        return result_type{jsoncons::unexpect, ec, reader.line(), reader.column()};
    }
    if (JSONCONS_UNLIKELY(!decoder.is_valid()))
    {
        return result_type{jsoncons::unexpect, conv_errc::conversion_failed, reader.line(), reader.column()};
    }
    return result_type{decoder.get_result()};
}

template <typename T,typename BytesLike>
typename std::enable_if<!ext_traits::is_basic_json<T>::value &&
                        ext_traits::is_byte_sequence<BytesLike>::value,read_result<T>>::type 
try_decode_bson(const BytesLike& v, 
    const bson_decode_options& options = bson_decode_options())
{
    using value_type = T;
    using result_type = read_result<value_type>;

    std::error_code ec;
    basic_bson_cursor<bytes_source> cursor(v, options, ec);
    if (JSONCONS_UNLIKELY(ec))
    {
        return result_type{jsoncons::unexpect, ec, cursor.line(), cursor.column()};
    }

    return reflect::decode_traits<T>::try_decode(make_alloc_set(), cursor);
}

template <typename T>
typename std::enable_if<ext_traits::is_basic_json<T>::value,read_result<T>>::type 
try_decode_bson(std::istream& is, 
    const bson_decode_options& options = bson_decode_options())
{
    using value_type = T;
    using result_type = read_result<value_type>;

    std::error_code ec;   
    jsoncons::json_decoder<T> decoder;
    auto adaptor = make_json_visitor_adaptor<json_visitor>(decoder);
    bson_stream_reader reader(is, adaptor, options);
    reader.read(ec);
    if (JSONCONS_UNLIKELY(ec))
    {
        return result_type{jsoncons::unexpect, ec, reader.line(), reader.column()};
    }
    if (JSONCONS_UNLIKELY(!decoder.is_valid()))
    {
        return result_type{jsoncons::unexpect, conv_errc::conversion_failed, reader.line(), reader.column()};
    }
    return result_type{decoder.get_result()};
}

template <typename T>
typename std::enable_if<!ext_traits::is_basic_json<T>::value,read_result<T>>::type 
try_decode_bson(std::istream& is, 
    const bson_decode_options& options = bson_decode_options())
{
    using value_type = T;
    using result_type = read_result<value_type>;

    std::error_code ec;
    basic_bson_cursor<binary_stream_source> cursor(is, options, ec);
    if (JSONCONS_UNLIKELY(ec))
    {
        return result_type{jsoncons::unexpect, ec, cursor.line(), cursor.column()};
    }

    return reflect::decode_traits<T>::try_decode(make_alloc_set(), cursor);
}

template <typename T,typename InputIt>
typename std::enable_if<ext_traits::is_basic_json<T>::value,read_result<T>>::type 
try_decode_bson(InputIt first, InputIt last,
    const bson_decode_options& options = bson_decode_options())
{
    using value_type = T;
    using result_type = read_result<value_type>;

    std::error_code ec;   
    jsoncons::json_decoder<T> decoder;
    auto adaptor = make_json_visitor_adaptor<json_visitor>(decoder);
    basic_bson_reader<binary_iterator_source<InputIt>> reader(binary_iterator_source<InputIt>(first, last), adaptor, options);
    reader.read(ec);
    if (JSONCONS_UNLIKELY(ec))
    {
        return result_type{jsoncons::unexpect, ec, reader.line(), reader.column()};
    }
    if (JSONCONS_UNLIKELY(!decoder.is_valid()))
    {
        return result_type{jsoncons::unexpect, conv_errc::conversion_failed, reader.line(), reader.column()};
    }
    return result_type{decoder.get_result()};
}

template <typename T,typename InputIt>
typename std::enable_if<!ext_traits::is_basic_json<T>::value,read_result<T>>::type 
try_decode_bson(InputIt first, InputIt last,
    const bson_decode_options& options = bson_decode_options())
{
    using value_type = T;
    using result_type = read_result<value_type>;

    std::error_code ec;
    basic_bson_cursor<binary_iterator_source<InputIt>> cursor(binary_iterator_source<InputIt>(first, last), options, ec);
    if (JSONCONS_UNLIKELY(ec))
    {
        return result_type{jsoncons::unexpect, ec, cursor.line(), cursor.column()};
    }

    return reflect::decode_traits<T>::try_decode(make_alloc_set(), cursor);
}

// With leading allocator_set parameter

template <typename T,typename BytesLike,typename Alloc,typename TempAlloc >
typename std::enable_if<ext_traits::is_basic_json<T>::value &&
                        ext_traits::is_byte_sequence<BytesLike>::value,read_result<T>>::type 
try_decode_bson(const allocator_set<Alloc,TempAlloc>& aset,
    const BytesLike& v, 
    const bson_decode_options& options = bson_decode_options())
{
    using value_type = T;
    using result_type = read_result<value_type>;

    std::error_code ec;   
    json_decoder<T,TempAlloc> decoder(aset.get_allocator(), aset.get_temp_allocator());
    auto adaptor = make_json_visitor_adaptor<json_visitor>(decoder);
    basic_bson_reader<jsoncons::bytes_source,TempAlloc> reader(v, adaptor, options, aset.get_temp_allocator());
    reader.read(ec);
    if (JSONCONS_UNLIKELY(ec))
    {
        return result_type{jsoncons::unexpect, ec, reader.line(), reader.column()};
    }
    if (JSONCONS_UNLIKELY(!decoder.is_valid()))
    {
        return result_type{jsoncons::unexpect, conv_errc::conversion_failed, reader.line(), reader.column()};
    }
    return result_type{decoder.get_result()};
}

template <typename T,typename BytesLike,typename Alloc,typename TempAlloc >
typename std::enable_if<!ext_traits::is_basic_json<T>::value &&
                        ext_traits::is_byte_sequence<BytesLike>::value,read_result<T>>::type 
try_decode_bson(const allocator_set<Alloc,TempAlloc>& aset,
    const BytesLike& v, 
    const bson_decode_options& options = bson_decode_options())
{
    using value_type = T;
    using result_type = read_result<value_type>;

    std::error_code ec;
    basic_bson_cursor<bytes_source,TempAlloc> cursor(std::allocator_arg, aset.get_temp_allocator(), v, options, ec);
    if (JSONCONS_UNLIKELY(ec))
    {
        return result_type{jsoncons::unexpect, ec, cursor.line(), cursor.column()};
    }

    return reflect::decode_traits<T>::try_decode(aset, cursor);
}

template <typename T,typename Alloc,typename TempAlloc >
typename std::enable_if<ext_traits::is_basic_json<T>::value,read_result<T>>::type 
try_decode_bson(const allocator_set<Alloc,TempAlloc>& aset,
    std::istream& is, 
    const bson_decode_options& options = bson_decode_options())
{
    using value_type = T;
    using result_type = read_result<value_type>;

    std::error_code ec;   
    json_decoder<T,TempAlloc> decoder(aset.get_allocator(), aset.get_temp_allocator());
    auto adaptor = make_json_visitor_adaptor<json_visitor>(decoder);
    basic_bson_reader<jsoncons::binary_stream_source,TempAlloc> reader(is, adaptor, options, aset.get_temp_allocator());
    reader.read(ec);
    if (JSONCONS_UNLIKELY(ec))
    {
        return result_type{jsoncons::unexpect, ec, reader.line(), reader.column()};
    }
    if (JSONCONS_UNLIKELY(!decoder.is_valid()))
    {
        return result_type{jsoncons::unexpect, conv_errc::conversion_failed, reader.line(), reader.column()};
    }
    return result_type{decoder.get_result()};
}

template <typename T,typename Alloc,typename TempAlloc >
typename std::enable_if<!ext_traits::is_basic_json<T>::value,read_result<T>>::type 
try_decode_bson(const allocator_set<Alloc,TempAlloc>& aset,
    std::istream& is, 
    const bson_decode_options& options = bson_decode_options())
{
    using value_type = T;
    using result_type = read_result<value_type>;

    std::error_code ec;
    basic_bson_cursor<binary_stream_source,TempAlloc> cursor(std::allocator_arg, aset.get_temp_allocator(), is, options, ec);
    if (JSONCONS_UNLIKELY(ec))
    {
        return result_type{jsoncons::unexpect, ec, cursor.line(), cursor.column()};
    }

    return reflect::decode_traits<T>::try_decode(aset, cursor);
}

template <typename T, typename... Args>
T decode_bson(Args&& ... args)
{
    auto result = try_decode_bson<T>(std::forward<Args>(args)...); 
    if (!result)
    {
        JSONCONS_THROW(ser_error(result.error().code(), result.error().line(), result.error().column()));
    }
    return std::move(*result);
}
  
} // namespace bson
} // namespace jsoncons

#endif // JSONCONS_EXT_BSON_DECODE_BSON_HPP
