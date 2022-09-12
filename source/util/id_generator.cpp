#include "id_generator.hpp"

uint32_t IdGenerator::next()
{
    while (_used.find(_next) != _used.end())
    {
        _next++;
    }
    uint32_t id = _next;
    _used.insert(id);
    _next++;
    return id;
}

void IdGenerator::reclaim(uint32_t id)
{
    _used.erase(id);
}
