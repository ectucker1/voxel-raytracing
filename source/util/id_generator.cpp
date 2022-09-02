#include "id_generator.hpp"

uint32_t IdGenerator::next()
{
    while (_used.find(_next) != _used.end())
    {
        _next++;
    }
    _used.insert(_next);
    _next++;
    return _next;
}

void IdGenerator::reclaim(uint32_t id)
{
    _used.erase(id);
}
