#include "imgui_renderer.hpp"

#include "engine/engine.hpp"
#include <imgui.h>
#include <imgui_impl_vulkan.h>
#include <imgui_impl_glfw.h>

ImguiRenderer::ImguiRenderer(const std::shared_ptr<Engine>& engine, const vk::RenderPass& renderPass) : AResource(engine)
{
    // Create descriptor pool for ImGui to use
    vk::DescriptorPoolSize poolSizes[] =
    {
        { vk::DescriptorType::eSampler, 1000 },
        { vk::DescriptorType::eCombinedImageSampler, 1000 },
        { vk::DescriptorType::eSampledImage, 1000 },
        { vk::DescriptorType::eStorageImage, 1000 },
        { vk::DescriptorType::eUniformTexelBuffer, 1000 },
        { vk::DescriptorType::eStorageTexelBuffer, 1000 },
        { vk::DescriptorType::eUniformBuffer, 1000 },
        { vk::DescriptorType::eStorageBuffer, 1000 },
        { vk::DescriptorType::eUniformBufferDynamic, 1000 },
        { vk::DescriptorType::eStorageBufferDynamic, 1000 },
        { vk::DescriptorType::eInputAttachment, 1000 }
    };
    vk::DescriptorPoolCreateInfo poolInfo = {};
    poolInfo.flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet;
    poolInfo.maxSets = 1000;
    poolInfo.poolSizeCount = static_cast<uint32_t>(std::size(poolSizes));
    poolInfo.pPoolSizes = poolSizes;
    vk::DescriptorPool imguiPool =  engine->device.createDescriptorPool(poolInfo);

    ImGui::CreateContext();

    ImGui_ImplGlfw_InitForVulkan(engine->window, true);

    ImGui_ImplVulkan_InitInfo init_info = {};
	init_info.Instance = engine->instance;
	init_info.PhysicalDevice = engine->physicalDevice;
	init_info.Device = engine->device;
	init_info.Queue = engine->graphicsQueue;
	init_info.DescriptorPool = imguiPool;
	init_info.MinImageCount = 3;
	init_info.ImageCount = 3;
	init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;

	ImGui_ImplVulkan_Init(&init_info, renderPass);

	// Execture a command to upload font textures
	engine->upload_submit([&](const vk::CommandBuffer& cmd) {
		ImGui_ImplVulkan_CreateFontsTexture(cmd);
    });
	ImGui_ImplVulkan_DestroyFontUploadObjects();

    pushDeletor([=](const std::shared_ptr<Engine>& engine) {
        engine->device.destroyDescriptorPool(imguiPool);
        ImGui_ImplVulkan_Shutdown();
    });
}

void ImguiRenderer::beginFrame() const
{
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void ImguiRenderer::draw(const vk::CommandBuffer& cmd) const
{
    ImGui::Render();
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd);
}
