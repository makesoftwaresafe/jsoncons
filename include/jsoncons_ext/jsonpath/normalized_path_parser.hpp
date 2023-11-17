// Copyright 2013-2023 Daniel Parker
// Distributed under the Boost license, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// See https://github.com/danielaparker/jsoncons for latest version

#ifndef JSONCONS_JSONPATH_NORMALIZED_PATH_PARSER_HPP
#define JSONCONS_JSONPATH_NORMALIZED_PATH_PARSER_HPP

#include <string>
#include <vector>
#include <memory>
#include <type_traits> // std::is_const
#include <limits> // std::numeric_limits
#include <utility> // std::move
#include <regex>
#include <algorithm> // std::reverse
#include <jsoncons/json.hpp>
#include <jsoncons_ext/jsonpath/jsonpath_error.hpp>
#include <jsoncons_ext/jsonpath/jsonpath_utilities.hpp>

namespace jsoncons { 
namespace jsonpath {

    template <class CharT,class Allocator>
    class basic_path_element 
    {
    public:
        using char_type = CharT;
        using allocator_type = Allocator;
        using char_allocator_type = typename std::allocator_traits<allocator_type>:: template rebind_alloc<CharT>;
        using string_type = std::basic_string<char_type,std::char_traits<char_type>,char_allocator_type>;
    private:
        bool has_name_;
        string_type name_;
        std::size_t index_;

    public:
        basic_path_element(const string_type& name)
            : has_name_(true), name_(name), index_(0)
        {
        }

        basic_path_element(string_type&& name)
            : has_name_(true), name_(std::move(name)), index_(0)
        {
        }

        basic_path_element(std::size_t index)
            : has_name_(false), index_(index)
        {
        }

        basic_path_element(const basic_path_element& other) = default;

        basic_path_element& operator=(const basic_path_element& other) = default;

        bool has_name() const
        {
            return has_name_;
        }

        bool has_index() const
        {
            return !has_name_;
        }

        const string_type& name() const
        {
            return name_;
        }

        std::size_t index() const 
        {
            return index_;
        }

        int compare(const basic_path_element& other) const
        {
            int diff = 0;
            if (has_name_ != other.has_name_)
            {
                diff = static_cast<int>(has_name_) - static_cast<int>(other.has_name_);
            }
            else
            {
                if (has_name_)
                {
                    diff = name_.compare(other.name_);
                }
                else
                {
                    diff = index_ < other.index_ ? -1 : index_ > other.index_ ? 1 : 0;
                }
            }
            return diff;
        }
    };


namespace detail {
     
    enum class normalized_path_state 
    {
        start,
        relative_location,
        single_quoted_string,
        bracket_specifier,
        digit,
        expect_rbracket,
        quoted_string_escape_char
    };

    template<class CharT, class Allocator>
    class normalized_path_parser
    {
    public:
        using allocator_type = Allocator;
        using char_type = CharT;
        using string_type = std::basic_string<CharT>;
        using string_view_type = jsoncons::basic_string_view<CharT>;
        using path_element_type = basic_path_element<CharT,Allocator>;
        using path_element_allocator_type = typename std::allocator_traits<allocator_type>:: template rebind_alloc<path_element_type>;
        using path_type = std::vector<path_element_type>;

    private:

        allocator_type alloc_;
        std::size_t line_;
        std::size_t column_;
        const char_type* end_input_;
        const char_type* p_;

    public:
        normalized_path_parser(const allocator_type& alloc = allocator_type())
            : alloc_(alloc), line_(1), column_(1),
              end_input_(nullptr),
              p_(nullptr)
        {
        }

        normalized_path_parser(std::size_t line, std::size_t column, 
            const allocator_type& alloc = allocator_type())
            : alloc_(alloc), line_(line), column_(column),
              end_input_(nullptr),
              p_(nullptr)
        {
        }

        std::size_t line() const
        {
            return line_;
        }

        std::size_t column() const
        {
            return column_;
        }

        path_type parse(const string_view_type& path)
        {
            std::error_code ec;
            auto result = parse(path, ec);
            if (ec)
            {
                JSONCONS_THROW(jsonpath_error(ec, line_, column_));
            }
            return result;
        }

        path_type parse(const string_view_type& path, std::error_code& ec)
        {
            std::vector<path_element_type> elements;

            string_type buffer(alloc_);

            end_input_ = path.data() + path.length();
            p_ = path.data();

            normalized_path_state state = normalized_path_state::start;
            while (p_ < end_input_)
            {
                switch (state)
                {
                    case normalized_path_state::start: 
                    {
                        switch (*p_)
                        {
                            case ' ':case '\t':case '\r':case '\n':
                                advance_past_space_character();
                                break;
                            case '$':
                            case '@':
                            {
                                state = normalized_path_state::relative_location;
                                ++p_;
                                ++column_;
                                break;
                            }
                            default:
                            {
                                ec = jsonpath_errc::expected_root_or_current_node;
                                return path_type{};
                            }
                        }
                        break;
                    }
                    case normalized_path_state::relative_location: 
                        switch (*p_)
                        {
                            case ' ':case '\t':case '\r':case '\n':
                                advance_past_space_character();
                                break;
                            case '[':
                                state = normalized_path_state::bracket_specifier;
                                ++p_;
                                ++column_;
                                break;
                            default:
                                ec = jsonpath_errc::expected_lbracket;
                                return path_type();
                        };
                        break;
                    case normalized_path_state::bracket_specifier:
                        switch (*p_)
                        {
                            case ' ':case '\t':case '\r':case '\n':
                                advance_past_space_character();
                                break;
                            case '\'':
                                state = normalized_path_state::single_quoted_string;
                                ++p_;
                                ++column_;
                                break;
                            case '0':case '1':case '2':case '3':case '4':case '5':case '6':case '7':case '8':case '9':
                                state = normalized_path_state::digit;
                                break;
                            default:
                                ec = jsonpath_errc::expected_single_quote_or_digit;
                                return path_type();
                        }
                        break;
                    case normalized_path_state::single_quoted_string:
                        switch (*p_)
                        {
                            case '\'':
                                elements.emplace_back(buffer);
                                buffer.clear();
                                state = normalized_path_state::expect_rbracket;
                                ++p_;
                                ++column_;
                                break;
                            case '\\':
                                state = normalized_path_state::quoted_string_escape_char;
                                ++p_;
                                ++column_;
                                break;
                            default:
                                buffer.push_back(*p_);
                                ++p_;
                                ++column_;
                                break;
                        };
                        break;
                    case normalized_path_state::expect_rbracket:
                        switch (*p_)
                        {
                            case ' ':case '\t':case '\r':case '\n':
                                advance_past_space_character();
                                break;
                            case ']':
                                state = normalized_path_state::relative_location;
                                ++p_;
                                ++column_;
                                break;
                            default:
                                ec = jsonpath_errc::expected_rbracket;
                                return path_type(alloc_);
                        }
                        break;

                    case normalized_path_state::digit:
                        switch(*p_)
                        {
                            case '0':case '1':case '2':case '3':case '4':case '5':case '6':case '7':case '8':case '9':
                                buffer.push_back(*p_);
                                ++p_;
                                ++column_;
                                break;
                            default:
                                std::size_t n{0};
                                auto r = jsoncons::detail::to_integer(buffer.data(), buffer.size(), n);
                                if (!r)
                                {
                                    ec = jsonpath_errc::invalid_number;
                                    return path_type(alloc_);
                                }
                                elements.emplace_back(n);
                                buffer.clear();
                                state = normalized_path_state::expect_rbracket;
                                break;
                        }
                        break;
                    case normalized_path_state::quoted_string_escape_char:
                        switch (*p_)
                        {
                            case '\"':
                                buffer.push_back('\"');
                                ++p_;
                                ++column_;
                                state = normalized_path_state::single_quoted_string;
                                break;
                            case '\'':
                                buffer.push_back('\'');
                                ++p_;
                                ++column_;
                                state = normalized_path_state::single_quoted_string;
                                break;
                            case '\\': 
                                buffer.push_back('\\');
                                ++p_;
                                ++column_;
                                state = normalized_path_state::single_quoted_string;
                                break;
                            case '/':
                                buffer.push_back('/');
                                ++p_;
                                ++column_;
                                state = normalized_path_state::single_quoted_string;
                                break;
                            case 'b':
                                buffer.push_back('\b');
                                ++p_;
                                ++column_;
                                state = normalized_path_state::single_quoted_string;
                                break;
                            case 'f':
                                buffer.push_back('\f');
                                ++p_;
                                ++column_;
                                state = normalized_path_state::single_quoted_string;
                                break;
                            case 'n':
                                buffer.push_back('\n');
                                ++p_;
                                ++column_;
                                state = normalized_path_state::single_quoted_string;
                                break;
                            case 'r':
                                buffer.push_back('\r');
                                ++p_;
                                ++column_;
                                state = normalized_path_state::single_quoted_string;
                                break;
                            case 't':
                                buffer.push_back('\t');
                                ++p_;
                                ++column_;
                                state = normalized_path_state::single_quoted_string;
                                break;
                            case 'u':
                                ++p_;
                                ++column_;
                                state = normalized_path_state::single_quoted_string;
                                break;
                            default:
                                ec = jsonpath_errc::illegal_escaped_character;
                                return path_type(alloc_);
                        }
                        break;
                    default:
                        ++p_;
                        ++column_;
                        break;
                }
            }

            if (state != normalized_path_state::relative_location)
            {
                ec = jsonpath_errc::unexpected_eof;
                return path_type();
            }
            return path_type(std::move(elements));
        }

        void advance_past_space_character()
        {
            switch (*p_)
            {
                case ' ':case '\t':
                    ++p_;
                    ++column_;
                    break;
                case '\r':
                    if (p_+1 < end_input_ && *(p_+1) == '\n')
                        ++p_;
                    ++line_;
                    column_ = 1;
                    ++p_;
                    break;
                case '\n':
                    ++line_;
                    column_ = 1;
                    ++p_;
                    break;
                default:
                    break;
            }
        }
    };

    } // namespace detail

    using path_element = basic_path_element<char,std::allocator<char>>;
    using wpath_element = basic_path_element<wchar_t,std::allocator<char>>;

} // namespace jsonpath
} // namespace jsoncons

#endif
