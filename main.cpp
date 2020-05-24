
// MIT License
//
// Copyright (c) 2020 degski
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdlib>

#include <algorithm>
#include <array>
#include <atomic>
#include <initializer_list>
#include <iostream>
#include <jthread>
#include <limits>
#include <memory>
#include <mutex>
#include <new>
#include <random>
#include <span>
#include <stdexcept>
#include <string>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

#include <hedley.h>
#include <plf/plf_nanotimer.h>

template <typename T>
struct aligned_quad {
  alignas(32) T a, b, c, d;

  aligned_quad(T&& a_, T&& b_, T&& c_, T&& d_)
      : a{std::forward<T>(a_)},
        b{std::forward<T>(b_)},
        c{std::forward<T>(c_)},
        d{std::forward<T>(d_)} {}
};

std::vector<aligned_quad<double>> fix_test_vector(std::size_t length_) {
  length_ /= 4;
  std::vector<aligned_quad<double>> v;
  for (std::size_t i = 0; i < length_; ++i)
    v.emplace_back(
        std::numeric_limits<double>::max(), std::numeric_limits<double>::max(),
        std::numeric_limits<double>::max(), std::numeric_limits<double>::max());
  return v;
}

std::vector<aligned_quad<double>> ran_test_vector(std::size_t length_,
                                                  unsigned int seed_) {
  length_ /= 4;
  std::uniform_real_distribution<double> d(-1.0, 1.0);
  std::minstd_rand g(seed_);
  int c = 1'024;
  while (--c)
    auto _ = d(g);
  std::vector<aligned_quad<double>> v;
  for (std::size_t i = 0; i < length_; ++i)
    v.emplace_back(d(g), d(g), d(g), d(g));
  return v;
}

[[nodiscard]] HEDLEY_ALWAYS_INLINE bool equal_m64(double const& a_,
                                                  double const& b_) noexcept {
  __int64 a;
  memcpy(&a, &a_, sizeof(__int64));
  __int64 b;
  memcpy(&b, &b_, sizeof(__int64));
  return a == b;
}

[[nodiscard]] HEDLEY_ALWAYS_INLINE bool equal_m128(double const& a_,
                                                   double const& b_) noexcept {
  return not _mm_movemask_pd(_mm_cmpneq_pd(_mm_load_pd(&a_), _mm_load_pd(&b_)));
}

[[nodiscard]] HEDLEY_ALWAYS_INLINE bool equal_m256(double const& a_,
                                                   double const& b_) noexcept {
  return not _mm256_movemask_pd(
      _mm256_cmp_pd(_mm256_load_pd(&a_), _mm256_load_pd(&b_), _CMP_NEQ_UQ));
}

[[nodiscard]] HEDLEY_ALWAYS_INLINE bool equal_m256_m128(
    double const& a_,
    double const& b_) noexcept {
  return equal_m128(a_, b_) and equal_m128(*(&a_ + 2), *(&b_ + 2));
}

int main() {
  constexpr std::size_t N = 10'000'000;

  auto fa = fix_test_vector(N);
  auto fb = fa;
  auto ra = ran_test_vector(N, 42);
  auto rb = ra;

  double* fap = (double*)fa.data();
  double* fbp = (double*)fb.data();
  double* rap = (double*)ra.data();
  double* rbp = (double*)rb.data();

  {
    bool result = true;

    std::uint64_t duration;
    plf::nanotimer timer;
    timer.start();

    for (std::size_t i = 0; i < N; i += result) {
      bool r = rap[i] == rbp[i];
  //    if (not r)
   //     std::cout << i << ' ';
      result = result and r;
    }

    duration = static_cast<std::uint64_t>(timer.get_elapsed_us()) / 100 * 100;
    std::cout << std::dec << duration << " ms " << result << '\n';
  }
  {
    bool result = true;

    std::uint64_t duration;
    plf::nanotimer timer;
    timer.start();

    for (std::size_t i = 0; i < N; i += 2) {
      bool r = equal_m128(fap[i], fbp[i]);
  //    if (not r)
   //     std::cout << i << ' ';
      result = result and r;
    }

    duration = static_cast<std::uint64_t>(timer.get_elapsed_us()) / 100 * 100;
    std::cout << std::dec << duration << " ms " << result << '\n';
  }
  {
    bool result = true;

    std::uint64_t duration;
    plf::nanotimer timer;
    timer.start();

    for (std::size_t i = 0; i < N; i += 4) {
      bool r = equal_m256(fap[i], fbp[i]);
    //  if (not r)
    //    std::cout << i << ' ';
      result = result and r;
    }

    duration = static_cast<std::uint64_t>(timer.get_elapsed_us()) / 100 * 100;
    std::cout << std::dec << duration << " ms " << result << '\n';
  }
  {
    bool result = true;

    std::uint64_t duration;
    plf::nanotimer timer;
    timer.start();

    for (std::size_t i = 0; i < N; i += 1) {
      bool r = equal_m64(fap[i], fbp[i]);
 //     if (not r)
  //      std::cout << i << ' ';
      result = result and r;
    }

    duration = static_cast<std::uint64_t>(timer.get_elapsed_us()) / 100 * 100;
    std::cout << std::dec << duration << " ms " << result << '\n';
  }
  {
      bool result = true;

      std::uint64_t duration;
      plf::nanotimer timer;
      timer.start ();

      for (std::size_t i = 0; i < N; i += result) {
          bool r = rap[i] == rbp[i];
          //    if (not r)
           //     std::cout << i << ' ';
          result = result and r;
      }

      duration = static_cast<std::uint64_t>(timer.get_elapsed_us ()) / 100 * 100;
      std::cout << std::dec << duration << " ms " << result << '\n';
  }
  return EXIT_SUCCESS;
}
