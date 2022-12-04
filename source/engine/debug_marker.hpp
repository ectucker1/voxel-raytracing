#pragma once

#include <unordered_map>
#include <typeindex>
#include "vulkan/vulkan.hpp"

namespace DebugMarker
{
    void setObjectName(const vk::Device& device, uint64_t object, vk::ObjectType objectType, const std::string& name);

    template<typename T>
    void setObjectName(const vk::Device& device, T object, const std::string& name)
    {
        static std::unordered_map<std::type_index, vk::ObjectType> objectMap = {
            { std::type_index(typeid(VkInstance)), vk::ObjectType::eInstance },
            { std::type_index(typeid(VkPhysicalDevice)), vk::ObjectType::ePhysicalDevice },
            { std::type_index(typeid(VkDevice)), vk::ObjectType::eDevice },
            { std::type_index(typeid(VkRenderPass)), vk::ObjectType::eRenderPass },
            { std::type_index(typeid(VkFramebuffer)), vk::ObjectType::eFramebuffer },
            { std::type_index(typeid(VkImage)), vk::ObjectType::eImage },
            { std::type_index(typeid(VkImageView)), vk::ObjectType::eImageView },
            { std::type_index(typeid(VkSampler)), vk::ObjectType::eSampler },
            { std::type_index(typeid(VkBuffer)), vk::ObjectType::eBuffer },
            { std::type_index(typeid(VkDescriptorSet)), vk::ObjectType::eDescriptorSet }
        };

        setObjectName(device, reinterpret_cast<uint64_t>(object), objectMap[typeid(T)], name);
    }
}
