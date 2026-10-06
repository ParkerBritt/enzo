#include "Graphics/GraphicsDevice.h"
#include "Graphics/DiligentObjects.h"
#include <iostream>
#include <stdexcept>

#include <CommandQueueVk.h>
#include <EngineFactoryVk.h>
#include <RenderDeviceVk.h>

namespace enzo::gfx {

namespace {

GraphicsDevice* processDevice = nullptr;

/// @brief Prints Diligent's warnings and errors, and drops its info messages.
void printDiligentMessage(
    Diligent::DEBUG_MESSAGE_SEVERITY severity,
    const Diligent::Char* message,
    const Diligent::Char*,
    const Diligent::Char*,
    int
)
{
    if (severity == Diligent::DEBUG_MESSAGE_SEVERITY_INFO) return;
    std::cerr << "graphics device: " << message << "\n";
}

} // namespace

GraphicsDevice::GraphicsDevice(const std::vector<std::string>& instanceExtensions)
{
    if (processDevice)
    {
        throw std::runtime_error("A graphics device already exists in this process");
    }

    std::vector<const char*> instanceExtensionNames;
    for (const std::string& extension : instanceExtensions)
    {
        instanceExtensionNames.push_back(extension.c_str());
    }

    Diligent::EngineVkCreateInfo createInfo;
    createInfo.GraphicsAPIVersion = {1, 3};
    createInfo.InstanceExtensionCount = static_cast<Diligent::Uint32>(instanceExtensionNames.size());
    createInfo.ppInstanceExtensionNames = instanceExtensionNames.data();

    diligent_ = std::make_unique<DiligentObjects>();
    Diligent::IEngineFactoryVk* factory = Diligent::GetEngineFactoryVk();
    factory->SetMessageCallback(printDiligentMessage);
    factory->CreateDeviceAndContextsVk(createInfo, &diligent_->device, &diligent_->context);
    if (!diligent_->device)
    {
        throw std::runtime_error("No Vulkan device could be created");
    }

    processDevice = this;
}

GraphicsDevice::~GraphicsDevice()
{
    diligent_->context->Flush();
    diligent_->device->IdleGPU();

    processDevice = nullptr;
}

GraphicsDevice& GraphicsDevice::get()
{
    if (!processDevice)
    {
        throw std::runtime_error("No graphics device exists in this process");
    }
    return *processDevice;
}

GraphicsDevice::DiligentObjects& GraphicsDevice::getDiligentObjects() const { return *diligent_; }

VulkanHandles GraphicsDevice::getVulkanHandles() const
{
    Diligent::RefCntAutoPtr<Diligent::IRenderDeviceVk> vulkanDevice{
        diligent_->device, Diligent::IID_RenderDeviceVk
    };

    // Takes the queue from the context, since every context shares the one queue Diligent creates.
    Diligent::ICommandQueue* queue = diligent_->context->LockCommandQueue();
    Diligent::RefCntAutoPtr<Diligent::ICommandQueueVk> vulkanQueue{
        queue, Diligent::IID_CommandQueueVk
    };
    VulkanHandles vulkanHandles;
    vulkanHandles.queue = vulkanQueue->GetVkQueue();
    vulkanHandles.queueFamilyIndex = vulkanQueue->GetQueueFamilyIndex();
    diligent_->context->UnlockCommandQueue();

    vulkanHandles.instance = vulkanDevice->GetVkInstance();
    vulkanHandles.physicalDevice = vulkanDevice->GetVkPhysicalDevice();
    vulkanHandles.device = vulkanDevice->GetVkDevice();
    vulkanHandles.apiVersion = vulkanDevice->GetVkVersion();
    return vulkanHandles;
}

} // namespace enzo::gfx
