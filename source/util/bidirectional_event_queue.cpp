#include "bidirectional_event_queue.hpp"

void BidirectionalEventQueue::push(const std::function<void()>& before, const std::function<void()>& after)
{
    _beforeListeners.push_front(before);
    _afterListeners.push_back(after);
}

void BidirectionalEventQueue::fireListeners()
{
    // Notify all the old resize listeners
    auto oldBeforeListeners = _beforeListeners;
    auto oldAfterListeners = _afterListeners;

    // Clear listeners (they'll probably be adding back to this when called)
    _beforeListeners.clear();
    _afterListeners.clear();

    // Fire before and after
    for (const auto& listener : oldBeforeListeners)
    {
        listener();
    }
    for (const auto& listener : oldAfterListeners)
    {
        listener();
    }
}
