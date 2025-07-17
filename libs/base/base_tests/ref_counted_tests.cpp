#include "testing/testing.hpp"

#include "base/ref_counted.hpp"

using namespace base;

namespace
{
struct Resource : public RefCounted
{
  explicit Resource(bool & destroyed) : m_destroyed(destroyed) { m_destroyed = false; }

  ~Resource() override { m_destroyed = true; }

  bool & m_destroyed;
};

UNIT_TEST(RefCounted_Smoke)
{
  {
    RefCountPtr<Resource> p;
  }

  {
    bool destroyed;
    {
      RefCountPtr<Resource> p(new Resource(destroyed));
      TEST_EQUAL(1, p->NumRefs(), ());
      TEST(!destroyed, ());
    }
    TEST(destroyed, ());
  }

  {
    bool destroyed;
    {
      RefCountPtr<Resource> a(new Resource(destroyed));
      TEST_EQUAL(1, a->NumRefs(), ());
      TEST(!destroyed, ());

      RefCountPtr<Resource> b(a);
      TEST(a.Get() == b.Get(), ());
      TEST_EQUAL(2, a->NumRefs(), ());
      TEST(!destroyed, ());

      {
        RefCountPtr<Resource> c;
        TEST(c.Get() == nullptr, ());

        c = b;
        TEST(a.Get() == b.Get(), ());
        TEST(b.Get() == c.Get(), ());
        TEST_EQUAL(3, a->NumRefs(), ());
        TEST(!destroyed, ());
      }

      TEST(a.Get() == b.Get(), ());
      TEST_EQUAL(2, a->NumRefs(), ());
      TEST(!destroyed, ());

      RefCountPtr<Resource> d(std::move(b));
      TEST(b.Get() == nullptr, ());
      TEST(a.Get() == d.Get(), ());
      TEST_EQUAL(2, a->NumRefs(), ());
      TEST(!destroyed, ());

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wself-assign-overloaded"
#endif  // #ifdef __clang__
      a = a;
#ifdef __clang__
#pragma clang diagnostic pop
#endif  // #ifdef __clang__

      TEST_EQUAL(a.Get(), d.Get(), ());
      TEST_EQUAL(2, a->NumRefs(), ());
      TEST(!destroyed, ());
    }
    TEST(destroyed, ());
  }
}
}  // namespace
