#include "testing/testing.hpp"

#include "indexer/classificator.hpp"
#include "indexer/classificator_loader.hpp"
#include "indexer/ftraits.hpp"

UNIT_TEST(Smoking_GetType)
{
  classificator::Load();
  Classificator const & c = classif();

  using ftraits::Smoking;
  using ftraits::SmokingAvailability;

  feature::TypesHolder holder;
  {
    holder.Assign(c.GetTypeByPath({"smoking", "no"}));
    TEST_EQUAL(*Smoking::GetValue(holder), SmokingAvailability::No, ());
  }
  {
    holder.Assign(c.GetTypeByPath({"smoking", "yes"}));
    TEST_EQUAL(*Smoking::GetValue(holder), SmokingAvailability::Yes, ());
  }
  {
    holder.Assign(c.GetTypeByPath({"smoking", "separated"}));
    TEST_EQUAL(*Smoking::GetValue(holder), SmokingAvailability::Separated, ());
  }
  {
    holder.Assign(c.GetTypeByPath({"smoking", "isolated"}));
    TEST_EQUAL(*Smoking::GetValue(holder), SmokingAvailability::Isolated, ());
  }
  {
    holder.Assign(c.GetTypeByPath({"smoking", "outside"}));
    TEST_EQUAL(*Smoking::GetValue(holder), SmokingAvailability::Outside, ());
  }
  {
    holder.Assign(c.GetTypeByPath({"amenity", "dentist"}));
    TEST(!Smoking::GetValue(holder), ());
  }
  {
    holder.Assign(c.GetTypeByPath({"amenity", "dentist"}));
    holder.Add(c.GetTypeByPath({"smoking", "yes"}));
    TEST_EQUAL(*Smoking::GetValue(holder), SmokingAvailability::Yes, ());
  }
}
