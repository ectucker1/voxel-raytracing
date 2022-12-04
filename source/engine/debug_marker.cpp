#include "engine/debug_marker.hpp"

void DebugMarker::setObjectName(const vk::Device& device, uint64_t object, vk::ObjectType objectType, const std::string& name)
{
    vk::DebugUtilsObjectNameInfoEXT nameInfo = {};
    nameInfo.objectType = objectType;
    nameInfo.objectHandle = object;
    nameInfo.pObjectName = name.c_str();
    device.setDebugUtilsObjectNameEXT(nameInfo);
}
