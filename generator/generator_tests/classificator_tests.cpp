#include "../../testing/testing.hpp"

#include "../../indexer/classificator.hpp"
#include "../../indexer/classificator_loader.hpp"

#include "../../base/logging.hpp"


namespace
{
  class DoCheckConsistency
  {
    Classificator const & m_c;
  public:
    DoCheckConsistency() : m_c(classif()) {}
    void operator() (ClassifObject const * p, uint32_t type)
    {
      if (p->IsDrawableAny() && !m_c.IsTypeValid(type))
        TEST(false, ("Inconsistency type", type, m_c.GetFullObjectName(type)));
    }
  };
}

UNIT_TEST(Classificator_CheckConsistency)
{
  classificator::Load();

  DoCheckConsistency doCheck;
  classif().ForEachTree(doCheck);
}
