//  Copyright (c) 2019 Thomas Heller
//  Copyright (c) 2017 Agustin Berge
//  Copyright (c) 2017 Google
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef HPX_EXCEPTION_INFO_HPP
#define HPX_EXCEPTION_INFO_HPP

#include <hpx/config.hpp>
#include <hpx/error_code.hpp>

#include <cstddef>
#include <exception>
#include <memory>
#include <type_traits>
#include <utility>

#if defined(HPX_WINDOWS)
#  include <excpt.h>
#  undef exception_info
#endif

namespace hpx
{
    namespace detail {
        class exception_info_node_base
        {
        public:
            virtual ~exception_info_node_base() = default;

            std::shared_ptr<exception_info_node_base> next;
        };

        template <typename Tag, typename T>
        struct exception_info_node_data
        {
            template <typename U>
            exception_info_node_data(U&& t)
              : data(std::forward<U>(t))
            {
            }

            T data;
        };
    }

    ///////////////////////////////////////////////////////////////////////////
    template <typename Tag, typename Type>
    struct error_info
    {
        using tag = Tag;
        using type = Type;

        explicit error_info(Type const& value)
          : _value(value)
        {}

        explicit error_info(Type&& value)
          : _value(std::move(value))
        {}

        static Type const* lookup(
            const detail::exception_info_node_base* node_base) noexcept
        {
            auto node = dynamic_cast<
                const detail::exception_info_node_data<Tag, Type>*>(node_base);
            if (node)
                return &node->data;

            if (node_base->next)
            {
                return lookup(node_base->next.get());
            }
            return nullptr;
        }

        Type _value;
    };

#define HPX_DEFINE_ERROR_INFO(NAME, TYPE)                                     \
    struct NAME : ::hpx::error_info<NAME, TYPE>                               \
    {                                                                         \
        explicit NAME(TYPE const& value)                                      \
          : error_info(value)                                                 \
        {}                                                                    \
                                                                              \
        explicit NAME(TYPE&& value)                                           \
          : error_info(::std::forward<TYPE>(value))                           \
        {}                                                                    \
    }                                                                         \
/**/

    ///////////////////////////////////////////////////////////////////////////
    namespace detail
    {
        template <typename... Ts>
        class exception_info_node
          : public exception_info_node_base
          , public exception_info_node_data<typename Ts::tag,
                typename Ts::type>...
        {
        public:
            explicit exception_info_node(Ts&&... tagged_values)
              : exception_info_node_data<typename Ts::tag, typename Ts::type>(
                    std::move(tagged_values._value))...
            {
            }
            explicit exception_info_node(Ts const&... tagged_values)
              : exception_info_node_data<typename Ts::tag, typename Ts::type>(
                    tagged_values._value)...
            {
            }
        };
    }

    ///////////////////////////////////////////////////////////////////////////
    class exception_info
    {
        using node_ptr = std::unique_ptr<detail::exception_info_node_base>;

    public:
        exception_info() noexcept
          : _data(nullptr)
        {}

        exception_info(exception_info&& other) noexcept = default;

        exception_info& operator=(exception_info&& other) noexcept = default;

        virtual ~exception_info() = default;

        template <typename ...ErrorInfo>
        exception_info& set(ErrorInfo&&... tagged_values)
        {
            using node_type = detail::exception_info_node<
                ErrorInfo...>;

            node_ptr node(
                new node_type(std::forward<ErrorInfo>(tagged_values)...));
            node->next = std::move(_data);
            _data = std::move(node);
            return *this;
        }

        template <typename Tag>
        typename Tag::type const* get() const noexcept
        {
            auto const* data = _data.get();

            return data ? Tag::lookup(data) : nullptr;
        }

    private:
        node_ptr _data;
    };

    ///////////////////////////////////////////////////////////////////////////
    namespace detail
    {
        template <typename E>
        struct exception_with_info
          : public E
          , public exception_info
        {
            explicit exception_with_info(E const& e, exception_info xi)
              : E(e)
              , exception_info(std::move(xi))
            {}

            explicit exception_with_info(E&& e, exception_info xi)
              : E(std::move(e))
              , exception_info(std::move(xi))
            {}
        };
    }

    template <typename E> HPX_NORETURN
    void throw_with_info(E&& e, exception_info&& xi = exception_info())
    {
        using ED = typename std::decay<E>::type;
        static_assert(
#if defined(HPX_HAVE_CXX14_STD_IS_FINAL)
            std::is_class<ED>::value && !std::is_final<ED>::value,
#else
            std::is_class<ED>::value,
#endif
            "E shall be a valid base class");
        static_assert(
            !std::is_base_of<exception_info, ED>::value,
            "E shall not derive from exception_info");

        throw detail::exception_with_info<ED>(std::forward<E>(e), std::move(xi));
    }

    ///////////////////////////////////////////////////////////////////////////
    template <typename E, typename F>
    auto invoke_with_exception_info(E const& e, F&& f)
      -> decltype(std::forward<F>(f)(std::declval<exception_info const*>()))
    {
        return std::forward<F>(f)(
            dynamic_cast<exception_info const*>(std::addressof(e)));
    }

    template <typename F>
    auto invoke_with_exception_info(std::exception_ptr const& p, F&& f)
      -> decltype(std::forward<F>(f)(std::declval<exception_info const*>()))
    {
        try
        {
            if (p) std::rethrow_exception(p);
        } catch (exception_info const& xi) {
            return std::forward<F>(f)(&xi);
        } catch (...) {
        }
        return std::forward<F>(f)(nullptr);
    }

    template <typename F>
    auto invoke_with_exception_info(hpx::error_code const& ec, F&& f)
      -> decltype(std::forward<F>(f)(std::declval<exception_info const*>()))
    {
        return invoke_with_exception_info(
            detail::access_exception(ec), std::forward<F>(f));
    }
}

#endif /*HPX_EXCEPTION_INFO_HPP*/
