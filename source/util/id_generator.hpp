#pragma once

#include <cstdint>
#include <unordered_set>

class IdGenerator
{
private:
    uint32_t _next;
    std::unordered_set<uint32_t> _used;

public:
    uint32_t next();
    void reclaim(uint32_t id);
};
