// Copyright 2013-2025 Daniel Parker
// Distributed under the Boost license, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// See https://github.com/danielaparker/jsoncons for latest version

#ifndef JSONCONS_CONVERSION_HPP
#define JSONCONS_CONVERSION_HPP

#include <memory>
#include <string>
#include <system_error> // std::error_code

#include <jsoncons/config/compiler_support.hpp>
#include <jsoncons/conv_error.hpp>
#include <jsoncons/utility/write_number.hpp> // from_integer
#include <jsoncons/json_type.hpp>
#include <jsoncons/semantic_tag.hpp>
#include <jsoncons/utility/byte_string.hpp>
#include <jsoncons/utility/more_type_traits.hpp>
#include <jsoncons/utility/unicode_traits.hpp>

namespace jsoncons {

template <typename InputIt,typename Container>
typename std::enable_if<std::is_same<typename std::iterator_traits<InputIt>::value_type,uint8_t>::value
    && ext_traits::is_string<Container>::value,size_t>::type
bytes_to_string(InputIt first, InputIt last, semantic_tag tag, Container& str)
{
    switch (tag)
    {
        case semantic_tag::base64:
            return bytes_to_base64(first, last, str);
        case semantic_tag::base16:
            return bytes_to_base16(first, last, str);
        default:
            return bytes_to_base64url(first, last, str);
    }
}

template <typename From,typename Into,typename Enable = void>
class value_converter
{
};

template <typename Into>
class value_converter_base
{
public:
    using allocator_type = typename std::conditional<ext_traits::has_allocator_type<Into>::value,typename Into::allocator_type, std::allocator<char>>::type;
private:
    allocator_type alloc_;

public:
    value_converter_base(const allocator_type& alloc = allocator_type())
        : alloc_(alloc)
    {
    }

    allocator_type get_allocator() const noexcept
    {
        return alloc_;
    }
};

// From any byte sequence, Into string
/*template <typename From,typename Into>
class value_converter<From, Into, 
    typename std::enable_if<ext_traits::is_byte_sequence<From>::value && !ext_traits::is_string_or_string_view<From>::value &&
        ext_traits::is_string<Into>::value>::type> : value_converter_base<Into>
{
public:
    using allocator_type = typename value_converter_base<Into>::allocator_type;

    template <typename CharT = typename Into::value_type>
    typename std::enable_if<ext_traits::is_narrow_character<CharT>::value,Into>::type
    convert(const From& value, semantic_tag tag, std::error_code&)
    {
        Into s(this->get_allocator());
        switch (tag)
        {
            case semantic_tag::base64:
                bytes_to_base64(value.begin(), value.end(), s);
                break;
            case semantic_tag::base16:
                bytes_to_base16(value.begin(), value.end(), s);
                break;
            default:
                bytes_to_base64url(value.begin(), value.end(), s);
                break;
        }
        return s;
    }
    template <typename CharT = typename Into::value_type>
    typename std::enable_if<ext_traits::is_wide_character<CharT>::value,Into>::type
    convert(const From& value, semantic_tag tag, std::error_code& ec)
    {
        std::string s;
        switch (tag)
        {
            case semantic_tag::base64:
                bytes_to_base64(value.begin(), value.end(), s);
                break;
            case semantic_tag::base16:
                bytes_to_base16(value.begin(), value.end(), s);
                break;
            default:
                bytes_to_base64url(value.begin(), value.end(), s);
                break;
        }

        Into ws(this->get_allocator());
        auto retval = unicode_traits::convert(s.data(), s.size(), ws);
        if (retval.ec != unicode_traits::conv_errc())
        {
            ec = conv_errc::not_wide_char;
        }

        return ws;
    }
};*/

// From byte string, Into byte string
template <typename From,typename Into>
class value_converter<From, Into, 
    typename std::enable_if<ext_traits::is_byte_sequence<From>::value && 
        !ext_traits::is_string_or_string_view<From>::value &&
        !ext_traits::is_string_or_string_view<Into>::value && 
        ext_traits::is_back_insertable_byte_container<Into>::value>::type> : value_converter_base<Into>
{
public:
    using allocator_type = typename value_converter_base<Into>::allocator_type;

    Into convert(const From& value, semantic_tag, std::error_code&)
    {
        Into s(value.begin(),value.end(),this->get_allocator());
        return s;
    }
};

// From string or string_view, Into string, same character type
template <typename From,typename Into>
class value_converter<From, Into, 
    typename std::enable_if<ext_traits::is_string_or_string_view<From>::value &&
        ext_traits::is_string<Into>::value && 
        std::is_same<typename From::value_type,typename Into::value_type>::value>::type> : value_converter_base<Into>
{
public:
    using allocator_type = typename value_converter_base<Into>::allocator_type;

    Into convert(const From& value, semantic_tag, std::error_code&)
    {
        return Into(value.begin(),value.end(),this->get_allocator());
    }
};

// From string or string_view, Into string, different character type
template <typename From,typename Into>
class value_converter<From, Into, 
    typename std::enable_if<ext_traits::is_string_or_string_view<From>::value &&
        ext_traits::is_string<Into>::value && 
        !std::is_same<typename From::value_type,typename Into::value_type>::value>::type> : value_converter_base<Into>
{
public:
    using allocator_type = typename value_converter_base<Into>::allocator_type;

    Into convert(const From& value, semantic_tag, std::error_code& ec)
    {
        Into ws(this->get_allocator());
        auto retval = unicode_traits::convert(value.data(), value.size(), ws);
        if (retval.ec != unicode_traits::conv_errc())
        {
            ec = conv_errc::not_wide_char;
        }

        return ws;
    }
};

// From string, Into byte_string
template <typename From,typename Into>
class value_converter<From, Into, 
    typename std::enable_if<ext_traits::is_char_sequence<From>::value &&
        !ext_traits::is_string_or_string_view<Into>::value && 
        ext_traits::is_back_insertable_byte_container<Into>::value>::type> : value_converter_base<Into>
{
public:
    using allocator_type = typename value_converter_base<Into>::allocator_type;

    template <typename CharT = typename From::value_type>
    typename std::enable_if<ext_traits::is_narrow_character<CharT>::value,Into>::type
    convert(const From& value, semantic_tag tag, std::error_code& ec)
    {
        Into bytes(this->get_allocator());
        switch (tag)
        {
            case semantic_tag::base16:
            {
                auto res = base16_to_bytes(value.begin(), value.end(), bytes);
                if (res.ec != conv_errc::success)
                {
                    ec = conv_errc::not_byte_string;
                }
                break;
            }
            case semantic_tag::base64:
            {
                base64_to_bytes(value.begin(), value.end(), bytes);
                break;
            }
            case semantic_tag::base64url:
            {
                base64url_to_bytes(value.begin(), value.end(), bytes);
                break;
            }
            default:
            {
                ec = conv_errc::not_byte_string;
                break;
            }
        }
        return bytes;
    }

    template <typename CharT = typename From::value_type>
    typename std::enable_if<ext_traits::is_wide_character<CharT>::value,Into>::type
    convert(const From& value, semantic_tag tag, std::error_code& ec)
    {
        Into bytes(this->get_allocator());

        std::string s(this->get_allocator());
        auto retval = unicode_traits::convert(value.data(), value.size(), s);
        if (retval.ec != unicode_traits::conv_errc())
        {
            ec = conv_errc::not_wide_char;
        }
        switch (tag)
        {
            case semantic_tag::base16:
            {
                auto res = base16_to_bytes(s.begin(), s.end(), bytes);
                if (res.ec != conv_errc::success)
                {
                    ec = conv_errc::not_byte_string;
                }
                break;
            }
            case semantic_tag::base64:
            {
                base64_to_bytes(s.begin(), s.end(), bytes);
                break;
            }
            case semantic_tag::base64url:
            {
                base64url_to_bytes(s.begin(), s.end(), bytes);
                break;
            }
            default:
            {
                ec = conv_errc::not_byte_string;
                break;
            }
        }
        return bytes;
    }
};

// From integer, Into string
template <typename From,typename Into>
class value_converter<From, Into, 
    typename std::enable_if<ext_traits::is_integer<From>::value &&
        ext_traits::is_string<Into>::value>::type> : value_converter_base<Into>
{
public:
    using allocator_type = typename value_converter_base<Into>::allocator_type;

    Into convert(From value, semantic_tag, std::error_code&)
    {
        Into s(this->get_allocator());
        jsoncons::utility::from_integer(value, s);
        return s;
    }
};

// From integer, Into string
template <typename From,typename Into>
class value_converter<From, Into, 
    typename std::enable_if<std::is_floating_point<From>::value &&
        ext_traits::is_string<Into>::value>::type> : value_converter_base<Into>
{
public:
    using allocator_type = typename value_converter_base<Into>::allocator_type;

    Into convert(From value, semantic_tag, std::error_code&)
    {
        Into s(this->get_allocator());
        jsoncons::utility::write_double f{float_chars_format::general,0};
        f(value, s);
        return s;
    }
};

// From half, Into string
template <typename Into>
class value_converter<half_arg_t, Into,
    typename std::enable_if<ext_traits::is_string<Into>::value>::type> : value_converter_base<Into>
{
public:
    using allocator_type = typename value_converter_base<Into>::allocator_type;

    Into convert(uint16_t value, semantic_tag, std::error_code&)
    {
        Into s(this->get_allocator());
        jsoncons::utility::write_double f{float_chars_format::general,0};
        double x = binary::decode_half(value);
        f(x, s);
        return s;
    }
};

// From bool, Into string
template <typename From,typename Into>
class value_converter<From, Into, 
    typename std::enable_if<ext_traits::is_bool<From>::value &&
        ext_traits::is_string<Into>::value>::type> : value_converter_base<Into>
{
public:
    using allocator_type = typename value_converter_base<Into>::allocator_type;
    using char_type = typename Into::value_type;

    JSONCONS_CPP14_CONSTEXPR 
    Into convert(From value, semantic_tag, std::error_code&)
    {
        constexpr const char_type* true_constant = JSONCONS_CSTRING_CONSTANT(char_type,"true"); 
        constexpr const char_type* false_constant = JSONCONS_CSTRING_CONSTANT(char_type,"false"); 

        return value ? Into(true_constant,4) : Into(false_constant,5);
    }
};

// From null, Into string
template <typename Into>
class value_converter<null_type, Into, void>  : value_converter_base<Into>
{
public:
    using allocator_type = typename value_converter_base<Into>::allocator_type;
    using char_type = typename Into::value_type;

    JSONCONS_CPP14_CONSTEXPR 
    Into convert(semantic_tag, std::error_code&)
    {
        constexpr const char_type* null_constant = JSONCONS_CSTRING_CONSTANT(char_type,"null"); 

        return Into(null_constant,4);
    }
};

} // namespace jsoncons

#endif // JSONCONS_CONVERSION_HPP

