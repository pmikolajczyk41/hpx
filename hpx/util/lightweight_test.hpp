////////////////////////////////////////////////////////////////////////////////
//  Copyright (c) 2011 Bryce Lelbach
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
////////////////////////////////////////////////////////////////////////////////

#if !defined(HPX_F646702C_6556_48FA_BF9D_3E7959983122)
#define HPX_F646702C_6556_48FA_BF9D_3E7959983122

#include <hpx/config.hpp>
#include <hpx/util/assert.hpp>
#include <hpx/util/detail/pp/stringize.hpp>

#include <boost/current_function.hpp>
#include <boost/io/ios_state.hpp>

// Use smart_ptr's spinlock header because this header is used by the CMake
// config tests, and therefore we can't include other hpx headers in this file.
#include <boost/smart_ptr/detail/spinlock.hpp>

#include <cstddef>
#include <cstdint>
#include <sstream>
#include <iostream>
#include <mutex>

namespace hpx { namespace util
{

enum counter_type
{
    counter_sanity,
    counter_test
};

namespace detail
{

struct fixture
{
  public:
    typedef boost::detail::spinlock mutex_type;

  private:
    std::ostream& stream_;
    std::size_t sanity_failures_;
    std::size_t test_failures_;
#if defined(HPX_HAVE_CXX11_NSDMI)
    mutex_type mutex_ = BOOST_DETAIL_SPINLOCK_INIT;
#else
    mutex_type mutex_;
#endif

  public:
    fixture(std::ostream& stream):
      stream_(stream), sanity_failures_(0), test_failures_(0)
#if !defined(HPX_HAVE_CXX11_NSDMI)
    , mutex_(BOOST_DETAIL_SPINLOCK_INIT)
#endif
    {
    }

    void increment(counter_type c);

    std::size_t get(counter_type c) const;

    template <typename T>
    bool check_(char const* file, int line, char const* function,
               counter_type c, T const& t, char const* msg)
    {
        if (!t)
        {
            std::lock_guard<mutex_type> l(mutex_);
            boost::io::ios_flags_saver ifs(stream_);
            stream_
                << file << "(" << line << "): "
                << msg << " failed in function '"
                << function << "'" << std::endl;
            increment(c);
            return false;
        }
        return true;
    }

    template <typename T, typename U>
    bool check_equal(char const* file, int line, char const* function,
                  counter_type c, T const& t, U const& u, char const* msg)
    {
        if (!(t == u))
        {
            std::lock_guard<mutex_type> l(mutex_);
            boost::io::ios_flags_saver ifs(stream_);
            stream_
                << file << "(" << line << "): " << msg
                << " failed in function '" << function << "': "
                << "'" << t << "' != '" << u << "'" << std::endl;
            increment(c);
            return false;
        }
        return true;
    }

    template <typename T, typename U>
    bool check_not_equal(char const* file, int line, char const* function,
                         counter_type c, T const& t, U const& u,
                         char const* msg)
    {
        if (!(t != u))
        {
            std::lock_guard<mutex_type> l(mutex_);
            boost::io::ios_flags_saver ifs(stream_);
            stream_
                << file << "(" << line << "): " << msg
                << " failed in function '" << function << "': "
                << "'" << t << "' != '" << u << "'" << std::endl;
            increment(c);
            return false;
        }
        return true;
    }

    template <typename T, typename U>
    bool check_less(char const* file, int line, char const* function,
                    counter_type c, T const& t, U const& u, char const* msg)
    {
        if (!(t < u))
        {
            std::lock_guard<mutex_type> l(mutex_);
            boost::io::ios_flags_saver ifs(stream_);
            stream_
                << file << "(" << line << "): " << msg
                << " failed in function '" << function << "': "
                << "'" << t << "' >= '" << u << "'" << std::endl;
            increment(c);
            return false;
        }
        return true;
    }

    template <typename T, typename U>
    bool check_less_equal(char const* file, int line, char const* function,
                          counter_type c, T const& t, U const& u,
                          char const* msg)
    {
        if (!(t <= u))
        {
            std::lock_guard<mutex_type> l(mutex_);
            boost::io::ios_flags_saver ifs(stream_);
            stream_
                << file << "(" << line << "): " << msg
                << " failed in function '" << function << "': "
                << "'" << t << "' > '" << u << "'" << std::endl;
            increment(c);
            return false;
        }
        return true;
    }

    template <typename T, typename U, typename V>
    bool check_range(char const* file, int line, char const* function,
                          counter_type c, T const& t, U const& u, V const& v,
                          char const* msg)
    {
        if (!(t >= u && t <= v))
        {
            std::lock_guard<mutex_type> l(mutex_);
            boost::io::ios_flags_saver ifs(stream_);
            if (!(t >= u))
            {
                stream_
                    << file << "(" << line << "): " << msg
                    << " failed in function '" << function << "': "
                    << "'" << t << "' < '" << u << "'" << std::endl;
            } else {
                stream_
                    << file << "(" << line << "): " << msg
                    << " failed in function '" << function << "': "
                    << "'" << t << "' > '" << v << "'" << std::endl;
            }
            increment(c);
            return false;
        }
        return true;
    }
};

extern fixture global_fixture;

} // hpx::util::detail

inline int report_errors(std::ostream& stream = std::cerr)
{
    std::size_t sanity = detail::global_fixture.get(counter_sanity),
                test   = detail::global_fixture.get(counter_test);
    if (sanity == 0 && test == 0)
        return 0;

    else
    {
        boost::io::ios_flags_saver ifs(stream);
        stream << sanity << " sanity check" //-V128
               << ((sanity == 1) ? " and " : "s and ")
               << test << " test"
               << ((test == 1) ? " failed." : "s failed.")
               << std::endl;
        return 1;
    }
}

inline void print_cdash_timing(const char *name, double time)
{
    // use stringstream followed by single cout for better multithreaded output
    std::stringstream temp;
    temp << "<DartMeasurement name=\"" << name << "\" "
         << "type=\"numeric/double\">" << time << "</DartMeasurement>";
    std::cout << temp.str() << std::endl;
}

inline void print_cdash_timing(const char *name, std::uint64_t time)
{
    print_cdash_timing(name, time / 1e9);
}

}} // hpx::util

#define HPX_TEST(expr)                                                        \
    ::hpx::util::detail::global_fixture.check_                                \
        (__FILE__, __LINE__, BOOST_CURRENT_FUNCTION,                          \
         ::hpx::util::counter_test,                                           \
         expr, "test '" HPX_PP_STRINGIZE(expr) "'")                           \
    /***/

#define HPX_TEST_MSG(expr, msg)                                               \
    ::hpx::util::detail::global_fixture.check_                                \
        (__FILE__, __LINE__, BOOST_CURRENT_FUNCTION,                          \
         ::hpx::util::counter_test,                                           \
         expr, msg)                                                           \
    /***/

#define HPX_TEST_EQ(expr1, expr2)                                             \
    ::hpx::util::detail::global_fixture.check_equal                           \
        (__FILE__, __LINE__, BOOST_CURRENT_FUNCTION,                          \
         ::hpx::util::counter_test,                                           \
         expr1, expr2, "test '" HPX_PP_STRINGIZE(expr1) " == "                \
                                HPX_PP_STRINGIZE(expr2) "'")                  \
    /***/

#define HPX_TEST_NEQ(expr1, expr2)                                            \
    ::hpx::util::detail::global_fixture.check_not_equal                       \
        (__FILE__, __LINE__, BOOST_CURRENT_FUNCTION,                          \
         ::hpx::util::counter_test,                                           \
         expr1, expr2, "test '" HPX_PP_STRINGIZE(expr1) " != "                \
                                HPX_PP_STRINGIZE(expr2) "'")                  \
    /***/

#define HPX_TEST_LT(expr1, expr2)                                             \
    ::hpx::util::detail::global_fixture.check_less                            \
        (__FILE__, __LINE__, BOOST_CURRENT_FUNCTION,                          \
         ::hpx::util::counter_test,                                           \
         expr1, expr2, "test '" HPX_PP_STRINGIZE(expr1) " < "                 \
                                HPX_PP_STRINGIZE(expr2) "'")                  \
    /***/

#define HPX_TEST_LTE(expr1, expr2)                                            \
    ::hpx::util::detail::global_fixture.check_less_equal                      \
        (__FILE__, __LINE__, BOOST_CURRENT_FUNCTION,                          \
         ::hpx::util::counter_test,                                           \
         expr1, expr2, "test '" HPX_PP_STRINGIZE(expr1) " <= "                \
                                HPX_PP_STRINGIZE(expr2) "'")                  \
    /***/

#define HPX_TEST_RANGE(expr1, expr2, expr3)                                   \
    ::hpx::util::detail::global_fixture.check_range                           \
        (__FILE__, __LINE__, BOOST_CURRENT_FUNCTION,                          \
         ::hpx::util::counter_test,                                           \
         expr1, expr2, expr3, "test '" HPX_PP_STRINGIZE(expr2) " <= "         \
                                HPX_PP_STRINGIZE(expr1) " <= "                \
                                HPX_PP_STRINGIZE(expr3) "'")                  \
    /***/

#define HPX_TEST_EQ_MSG(expr1, expr2, msg)                                    \
    ::hpx::util::detail::global_fixture.check_equal                           \
        (__FILE__, __LINE__, BOOST_CURRENT_FUNCTION,                          \
         ::hpx::util::counter_test,                                           \
         expr1, expr2, msg)                                                   \
    /***/

#define HPX_TEST_NEQ_MSG(expr1, expr2, msg)                                   \
    ::hpx::util::detail::global_fixture.check_not_equal                       \
        (__FILE__, __LINE__, BOOST_CURRENT_FUNCTION,                          \
         ::hpx::util::counter_test,                                           \
         expr1, expr2, msg)                                                   \
    /***/

#define HPX_SANITY(expr)                                                      \
    ::hpx::util::detail::global_fixture.check_                                \
        (__FILE__, __LINE__, BOOST_CURRENT_FUNCTION,                          \
         ::hpx::util::counter_sanity,                                         \
         expr, "sanity check '" HPX_PP_STRINGIZE(expr) "'")                   \
    /***/

#define HPX_SANITY_MSG(expr, msg)                                             \
    ::hpx::util::detail::global_fixture.check_                                \
        (__FILE__, __LINE__, BOOST_CURRENT_FUNCTION,                          \
         ::hpx::util::counter_sanity,                                         \
         expr, msg)                                                           \
    /***/

#define HPX_SANITY_EQ(expr1, expr2)                                           \
    ::hpx::util::detail::global_fixture.check_equal                           \
        (__FILE__, __LINE__, BOOST_CURRENT_FUNCTION,                          \
         ::hpx::util::counter_sanity,                                         \
         expr1, expr2, "sanity check '" HPX_PP_STRINGIZE(expr1) " == "        \
                                        HPX_PP_STRINGIZE(expr2) "'")          \
    /***/

#define HPX_SANITY_NEQ(expr1, expr2)                                          \
    ::hpx::util::detail::global_fixture.check_not_equal                       \
        (__FILE__, __LINE__, BOOST_CURRENT_FUNCTION,                          \
         ::hpx::util::counter_sanity,                                         \
         expr1, expr2, "sanity check '" HPX_PP_STRINGIZE(expr1) " != "        \
                                        HPX_PP_STRINGIZE(expr2) "'")          \
    /***/

#define HPX_SANITY_LT(expr1, expr2)                                           \
    ::hpx::util::detail::global_fixture.check_less                            \
        (__FILE__, __LINE__, BOOST_CURRENT_FUNCTION,                          \
         ::hpx::util::counter_sanity,                                         \
         expr1, expr2, "sanity check '" HPX_PP_STRINGIZE(expr1) " < "         \
                                        HPX_PP_STRINGIZE(expr2) "'")          \
    /***/

#define HPX_SANITY_LTE(expr1, expr2)                                          \
    ::hpx::util::detail::global_fixture.check_less_equal                      \
        (__FILE__, __LINE__, BOOST_CURRENT_FUNCTION,                          \
         ::hpx::util::counter_sanity,                                         \
         expr1, expr2, "sanity check '" HPX_PP_STRINGIZE(expr1) " <= "        \
                                        HPX_PP_STRINGIZE(expr2) "'")          \
    /***/

#define HPX_SANITY_RANGE(expr1, expr2, expr3)                                 \
    ::hpx::util::detail::global_fixture.check_range                           \
        (__FILE__, __LINE__, BOOST_CURRENT_FUNCTION,                          \
         ::hpx::util::counter_sanity,                                         \
         expr1, expr2, expr3, "sanity check '" HPX_PP_STRINGIZE(expr2) " <= " \
                                               HPX_PP_STRINGIZE(expr1) " <= " \
                                               HPX_PP_STRINGIZE(expr3) "'")   \
    /***/

#define HPX_SANITY_EQ_MSG(expr1, expr2, msg)                                  \
    ::hpx::util::detail::global_fixture.check_equal                           \
        (__FILE__, __LINE__, BOOST_CURRENT_FUNCTION,                          \
         ::hpx::util::counter_sanity,                                         \
         expr1, expr2)                                                        \
    /***/

#endif // HPX_F646702C_6556_48FA_BF9D_3E7959983122

