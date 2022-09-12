#pragma once

#include <deque>
#include <functional>

// An event queue which calls all listeners for a before action,
// and then calls an after action in the reverse order.
// This is useful for destroying and then recreating a chain of resources.
class BidirectionalEventQueue
{
private:
    std::deque<std::function<void()>> _beforeListeners;
    std::deque<std::function<void()>> _afterListeners;
public:
    // Pushes a new listener onto the queue.
    // The before will be called before all current, the after will be called after all current.
    void push(const std::function<void()>& before, const std::function<void()>& after);
    // Fires the current listeners and clears the queue.
    void fireListeners();
};
