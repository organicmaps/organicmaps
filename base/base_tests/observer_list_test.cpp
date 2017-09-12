#include "testing/testing.hpp"

#include "base/observer_list.hpp"

#include <string>

namespace
{
struct Observer
{
  Observer() : status(0) {}

  void OnOperationCompleted(std::string const & message, int status)
  {
    this->message = message;
    this->status = status;
  }

  std::string message;
  int status;
};
}  // namespace

UNIT_TEST(ObserverList_Basic)
{
  Observer observer0;
  Observer observer1;
  Observer observer2;

  base::ObserverListSafe<Observer> observers;

  // Register all observers in a list. Also check that it's not
  // possible to add the same observer twice.
  TEST(observers.Add(observer0), ());
  TEST(!observers.Add(observer0), ());

  TEST(observers.Add(observer1), ());
  TEST(!observers.Add(observer1), ());

  TEST(observers.Add(observer2), ());
  TEST(!observers.Add(observer2), ());

  TEST(!observers.Add(observer1), ());
  TEST(!observers.Add(observer2), ());

  std::string const message = "HTTP OK";
  observers.ForEach(&Observer::OnOperationCompleted, message, 204);

  // Check observers internal state.
  TEST_EQUAL(message, observer0.message, ());
  TEST_EQUAL(204, observer0.status, ());

  TEST_EQUAL(message, observer1.message, ());
  TEST_EQUAL(204, observer1.status, ());

  TEST_EQUAL(message, observer2.message, ());
  TEST_EQUAL(204, observer2.status, ());

  // Remove all observers from a list.
  TEST(observers.Remove(observer0), ());
  TEST(!observers.Remove(observer0), ());

  TEST(observers.Remove(observer1), ());
  TEST(!observers.Remove(observer1), ());

  TEST(observers.Remove(observer2), ());
  TEST(!observers.Remove(observer2), ());
}
