#pragma once

#include <condition_variable>
#include <memory>

namespace lt {
class Awaitable {
   public:
    virtual ~Awaitable() = default;
    virtual void attachToCondition(std::condition_variable* i_variable) = 0;
    virtual void detachFromCondition() = 0;
    virtual bool ready() const = 0;
};
using AwaitablePtr = std::shared_ptr<Awaitable>;
}  // namespace lt