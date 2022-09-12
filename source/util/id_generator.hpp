#pragma once

#include <cstdint>
#include <unordered_set>

// A generator for unique numerical IDs.
class IdGenerator
{
private:
    uint32_t _next;
    std::unordered_set<uint32_t> _used;

public:
    // Returns the next ID to be generated.
    uint32_t next();
    // Reclaims an ID so it can be reused for later resources.
    void reclaim(uint32_t id);
};
