#include "ActiveObject.hpp"

#include <future>

#include "doctest.h"

class Counter : public lt::ActiveObject {
 public:
  Counter() : m_count(0) {}

  void add(int i_amount)
  {
    submitJob([i_amount, this]() { m_count += i_amount; });
  }

  std::future<int> value()
  {
    // Note: we must handle the promise via a pointer to caputre it correctly.
    auto promise = std::make_shared<std::promise<int>>();
    submitJob([promise, this]() { promise->set_value(m_count); });
    return promise->get_future();
  }

 protected:
  int m_count;
};

TEST_CASE("ActiveObjectBasic")
{
  Counter counter;
  REQUIRE(counter.isWorking());
  REQUIRE(counter.value().get() == 0);
  counter.stopWork();
  REQUIRE(counter.isWorking() == false);
  counter.startWork();
  REQUIRE(counter.isWorking());
  counter.add(10);
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  CHECK(counter.value().get() == 10);
}
