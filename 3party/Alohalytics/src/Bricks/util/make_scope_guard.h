#ifndef BRICKS_UTIL_MAKE_SCOPED_GUARD_H
#define BRICKS_UTIL_MAKE_SCOPED_GUARD_H

/*******************************************************************************
The MIT License (MIT)

Copyright (c) 2014 Alexander Zolotarev <me@alex.bio> from Minsk, Belarus

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*******************************************************************************/

#include <memory>
#include <utility>

namespace bricks {

template <typename POINTER, typename DELETER>
std::unique_ptr<POINTER, DELETER> MakePointerScopeGuard(POINTER* x, DELETER t) {
  return std::unique_ptr<POINTER, DELETER>(x, t);
}

template <typename F>
class ScopeGuard final {
  F f_;
  ScopeGuard(const ScopeGuard&) = delete;
  void operator=(const ScopeGuard&) = delete;

 public:
  explicit ScopeGuard(F f) : f_(f) {
  }
  ScopeGuard(ScopeGuard&& other) : f_(std::forward<F>(other.f_)) {
  }
  ~ScopeGuard() {
    f_();
  }
};

template <typename F>
ScopeGuard<F> MakeScopeGuard(F f) {
  return ScopeGuard<F>(f);
}

}  // namespace bricks

#endif  // BRICKS_UTIL_MAKE_SCOPED_GUARD_H
