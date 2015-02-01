/*******************************************************************************
The MIT License (MIT)

Copyright (c) 2014 Dmitry "Dima" Korolev <dmitry.korolev@gmail.com>

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

#include "dispatcher.h"

#include <string>
#include <tuple>

#include "../3party/gtest/gtest-main.h"

using std::string;
using std::tuple;

template <typename... TYPES>
struct TypeList;

struct Base {
  // Empty constructor required by clang++.
  Base() {}
  // Need to define at least one virtual method.
  virtual ~Base() = default;
};
struct Foo : Base {
  Foo() {}
};
struct Bar : Base {
  Bar() {}
};
struct Baz : Base {
  Baz() {}
};

struct OtherBase {
  // Empty constructor required by clang++.
  OtherBase() {}
  // Need to define at least one virtual method.
  virtual ~OtherBase() = default;
};

typedef TypeList<Foo, Bar, Baz> FooBarBazTypeList;

struct Processor {
  string s;
  void operator()(const Base&) { s = "const Base&"; }
  void operator()(const Foo&) { s = "const Foo&"; }
  void operator()(const Bar&) { s = "const Bar&"; }
  void operator()(const Baz&) { s = "const Baz&"; }
  void operator()(Base&) { s = "Base&"; }
  void operator()(Foo&) { s = "Foo&"; }
  void operator()(Bar&) { s = "Bar&"; }
  void operator()(Baz&) { s = "Baz&"; }
};

TEST(RuntimeDispatcher, StaticCalls) {
  Processor p;
  EXPECT_EQ("", p.s);
  p(Base());
  EXPECT_EQ("const Base&", p.s);
  p(Foo());
  EXPECT_EQ("const Foo&", p.s);
  p(Bar());
  EXPECT_EQ("const Bar&", p.s);
  p(Baz());
  EXPECT_EQ("const Baz&", p.s);
}

TEST(RuntimeDispatcher, ImmutableStaticCalls) {
  const Base base;
  const Foo foo;
  const Bar bar;
  const Baz baz;
  Processor p;
  EXPECT_EQ("", p.s);
  p(base);
  EXPECT_EQ("const Base&", p.s);
  p(foo);
  EXPECT_EQ("const Foo&", p.s);
  p(bar);
  EXPECT_EQ("const Bar&", p.s);
  p(baz);
  EXPECT_EQ("const Baz&", p.s);
}

TEST(RuntimeDispatcher, MutableStaticCalls) {
  Base base;
  Foo foo;
  Bar bar;
  Baz baz;
  Processor p;
  EXPECT_EQ("", p.s);
  p(base);
  EXPECT_EQ("Base&", p.s);
  p(foo);
  EXPECT_EQ("Foo&", p.s);
  p(bar);
  EXPECT_EQ("Bar&", p.s);
  p(baz);
  EXPECT_EQ("Baz&", p.s);
}

TEST(RuntimeDispatcher, ImmutableWithoutDispatching) {
  const Base base;
  const Foo foo;
  const Bar bar;
  const Baz baz;
  const Base& rbase = base;
  const Base& rfoo = foo;
  const Base& rbar = bar;
  const Base& rbaz = baz;
  Processor p;
  EXPECT_EQ("", p.s);
  p(rbase);
  EXPECT_EQ("const Base&", p.s);
  p(rfoo);
  EXPECT_EQ("const Base&", p.s);
  p(rbar);
  EXPECT_EQ("const Base&", p.s);
  p(rbaz);
  EXPECT_EQ("const Base&", p.s);
}

TEST(RuntimeDispatcher, MutableWithoutDispatching) {
  Base base;
  Foo foo;
  Bar bar;
  Baz baz;
  Base& rbase = base;
  Base& rfoo = foo;
  Base& rbar = bar;
  Base& rbaz = baz;
  Processor p;
  EXPECT_EQ("", p.s);
  p(rbase);
  EXPECT_EQ("Base&", p.s);
  p(rfoo);
  EXPECT_EQ("Base&", p.s);
  p(rbar);
  EXPECT_EQ("Base&", p.s);
  p(rbaz);
  EXPECT_EQ("Base&", p.s);
}

TEST(RuntimeDispatcher, ImmutableWithDispatching) {
  const Base base;
  const Foo foo;
  const Bar bar;
  const Baz baz;
  const OtherBase other;
  const Base& rbase = base;
  const Base& rfoo = foo;
  const Base& rbar = bar;
  const Base& rbaz = baz;
  const OtherBase& rother = other;
  Processor p;
  EXPECT_EQ("", p.s);
  bricks::rtti::RuntimeDispatcher<Base, Foo, Bar, Baz>::DispatchCall(rbase, p);
  EXPECT_EQ("const Base&", p.s);
  bricks::rtti::RuntimeDispatcher<Base, Foo, Bar, Baz>::DispatchCall(rfoo, p);
  EXPECT_EQ("const Foo&", p.s);
  bricks::rtti::RuntimeDispatcher<Base, Foo, Bar, Baz>::DispatchCall(rbar, p);
  EXPECT_EQ("const Bar&", p.s);
  bricks::rtti::RuntimeDispatcher<Base, Foo, Bar, Baz>::DispatchCall(rbaz, p);
  EXPECT_EQ("const Baz&", p.s);
  ASSERT_THROW((bricks::rtti::RuntimeDispatcher<Base, Foo, Bar, Baz>::DispatchCall(rother, p)),
               bricks::rtti::UnrecognizedPolymorphicType);
}

TEST(RuntimeDispatcher, MutableWithDispatching) {
  Base base;
  Foo foo;
  Bar bar;
  Baz baz;
  OtherBase other;
  Base& rbase = base;
  Base& rfoo = foo;
  Base& rbar = bar;
  Base& rbaz = baz;
  OtherBase& rother = other;
  Processor p;
  EXPECT_EQ("", p.s);
  bricks::rtti::RuntimeDispatcher<Base, Foo, Bar, Baz>::DispatchCall(rbase, p);
  EXPECT_EQ("Base&", p.s);
  bricks::rtti::RuntimeDispatcher<Base, Foo, Bar, Baz>::DispatchCall(rfoo, p);
  EXPECT_EQ("Foo&", p.s);
  bricks::rtti::RuntimeDispatcher<Base, Foo, Bar, Baz>::DispatchCall(rbar, p);
  EXPECT_EQ("Bar&", p.s);
  bricks::rtti::RuntimeDispatcher<Base, Foo, Bar, Baz>::DispatchCall(rbaz, p);
  EXPECT_EQ("Baz&", p.s);
  ASSERT_THROW((bricks::rtti::RuntimeDispatcher<Base, Foo, Bar, Baz>::DispatchCall(rother, p)),
               bricks::rtti::UnrecognizedPolymorphicType);
}

TEST(RuntimeDispatcher, ImmutableWithTupleTypeListDispatching) {
  const Base base;
  const Foo foo;
  const Bar bar;
  const Baz baz;
  const OtherBase other;
  const Base& rbase = base;
  const Base& rfoo = foo;
  const Base& rbar = bar;
  const Base& rbaz = baz;
  const OtherBase& rother = other;
  Processor p;
  EXPECT_EQ("", p.s);
  bricks::rtti::RuntimeTupleDispatcher<Base, tuple<Foo, Bar, Baz>>::DispatchCall(rbase, p);
  EXPECT_EQ("const Base&", p.s);
  bricks::rtti::RuntimeTupleDispatcher<Base, tuple<Foo, Bar, Baz>>::DispatchCall(rfoo, p);
  EXPECT_EQ("const Foo&", p.s);
  bricks::rtti::RuntimeTupleDispatcher<Base, tuple<Foo, Bar, Baz>>::DispatchCall(rbar, p);
  EXPECT_EQ("const Bar&", p.s);
  bricks::rtti::RuntimeTupleDispatcher<Base, tuple<Foo, Bar, Baz>>::DispatchCall(rbaz, p);
  EXPECT_EQ("const Baz&", p.s);
  ASSERT_THROW((bricks::rtti::RuntimeDispatcher<Base, Foo, Bar, Baz>::DispatchCall(rother, p)),
               bricks::rtti::UnrecognizedPolymorphicType);
}

TEST(RuntimeDispatcher, MutableWithTupleTypeListDispatching) {
  Base base;
  Foo foo;
  Bar bar;
  Baz baz;
  OtherBase other;
  Base& rbase = base;
  Base& rfoo = foo;
  Base& rbar = bar;
  Base& rbaz = baz;
  OtherBase& rother = other;
  Processor p;
  EXPECT_EQ("", p.s);
  bricks::rtti::RuntimeTupleDispatcher<Base, tuple<Foo, Bar, Baz>>::DispatchCall(rbase, p);
  EXPECT_EQ("Base&", p.s);
  bricks::rtti::RuntimeTupleDispatcher<Base, tuple<Foo, Bar, Baz>>::DispatchCall(rfoo, p);
  EXPECT_EQ("Foo&", p.s);
  bricks::rtti::RuntimeTupleDispatcher<Base, tuple<Foo, Bar, Baz>>::DispatchCall(rbar, p);
  EXPECT_EQ("Bar&", p.s);
  bricks::rtti::RuntimeTupleDispatcher<Base, tuple<Foo, Bar, Baz>>::DispatchCall(rbaz, p);
  EXPECT_EQ("Baz&", p.s);
  ASSERT_THROW((bricks::rtti::RuntimeDispatcher<Base, Foo, Bar, Baz>::DispatchCall(rother, p)),
               bricks::rtti::UnrecognizedPolymorphicType);
}
