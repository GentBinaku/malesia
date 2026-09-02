#include <malesia/render.hpp>

#include <array>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <functional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include <vulkan/vulkan.h>

#include <VkBootstrap.h>
#include <vk_mem_alloc.h>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>

namespace malesia::render {

namespace {

constexpr uint32_t kFramesInFlight = 2;

auto check(VkResult result, const char *what) -> void {
  if (result != VK_SUCCESS) {
    throw std::runtime_error(std::string(what) + " failed (VkResult " +
                             std::to_string(static_cast<int>(result)) + ")");
  }
}

} // namespace

// ===========================================================================
// Renderer::Impl — all Vulkan state lives here so the public header stays
// clean.
// ===========================================================================
struct Renderer::Impl {

  //TODO unique_ptr
  window::Window *window = nullptr;

  vkb::Instance instance;
  VkSurfaceKHR surface = VK_NULL_HANDLE;
  vkb::PhysicalDevice physicalDevice;
  vkb::Device device;

  VkQueue graphicsQueue = VK_NULL_HANDLE;
  VkQueue presentQueue = VK_NULL_HANDLE;
  uint32_t graphicsQueueFamily = 0;

  VmaAllocator allocator = VK_NULL_HANDLE;

  vkb::Swapchain swapchain;
  std::vector<VkImage> images;
  std::vector<VkImageView> imageViews;

  VkCommandPool commandPool = VK_NULL_HANDLE;
  std::array<VkCommandBuffer, kFramesInFlight> commandBuffers{};
  std::array<VkSemaphore, kFramesInFlight> imageAvailable{};
  std::array<VkFence, kFramesInFlight> inFlight{};
  // Signalled by the frame's submit, waited on by present. One per swapchain
  // image: a present can still be reading image i while we start recording the
  // next frame, so this semaphore must not be shared across images.
  std::vector<VkSemaphore> renderFinished;

  uint32_t currentFrame = 0;

  // Dear ImGui debug overlay. Owned here; the UI itself is supplied by the app
  // through Renderer::setDebugUi and invoked once per frame if set. The ImGui
  // Vulkan backend creates and owns its own descriptor pool (DescriptorPoolSize
  // below), so there is nothing extra to destroy here.
  VkFormat swapchainColorFormat = VK_FORMAT_UNDEFINED;
  bool imguiInitialized = false;
  Renderer::DebugUiFn debugUi;

  explicit Impl(window::Window &win);
  ~Impl();

  Impl(const Impl &) = delete;
  Impl &operator=(const Impl &) = delete;

  auto createSwapchain(bool recreate) -> void;
  auto destroySwapchain() -> void;
  auto recreateSwapchain() -> void;
  auto recordFrame(VkCommandBuffer cmd, uint32_t imageIndex) -> void;
  auto render(const Scene &scene, glm::mat4 const &viewProj) -> void;
  auto initImGui() -> void;
  auto shutdownImGui() -> void;
};

Renderer::Impl::Impl(window::Window &win) : window(&win) {
  // --- Instance -----------------------------------------------------------
  vkb::InstanceBuilder instanceBuilder;
  instanceBuilder.set_app_name("Malesia")
      .set_engine_name("malesia")
      .require_api_version(1, 3, 0)
      .request_validation_layers(true)
      .use_default_debug_messenger();
  for (const char *ext : window::Window::requiredInstanceExtensions()) {
    instanceBuilder.enable_extension(ext);
  }
  auto instanceResult = instanceBuilder.build();
  if (!instanceResult) {
    throw std::runtime_error("vkb instance: " +
                             instanceResult.error().message());
  }
  instance = instanceResult.value();

  // --- Surface ----------------------------------------------------------
  surface = window->createSurface(instance.instance);

  // --- Physical device (Vulkan 1.3 core features we rely on) -----------
  VkPhysicalDeviceVulkan13Features features13{};
  features13.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
  features13.dynamicRendering = VK_TRUE;
  features13.synchronization2 = VK_TRUE;

  vkb::PhysicalDeviceSelector selector(instance);
  auto physicalResult = selector.set_surface(surface)
                            .set_minimum_version(1, 3)
                            .set_required_features_13(features13)
                            .select();
  if (!physicalResult) {
    throw std::runtime_error("vkb physical device: " +
                             physicalResult.error().message());
  }
  physicalDevice = physicalResult.value();

  // --- Logical device + queues ---------------------------------------
  vkb::DeviceBuilder deviceBuilder(physicalDevice);
  auto deviceResult = deviceBuilder.build();
  if (!deviceResult) {
    throw std::runtime_error("vkb device: " + deviceResult.error().message());
  }
  device = deviceResult.value();

  graphicsQueue = device.get_queue(vkb::QueueType::graphics).value();
  presentQueue = device.get_queue(vkb::QueueType::present).value();
  graphicsQueueFamily =
      device.get_queue_index(vkb::QueueType::graphics).value();

  // --- VMA allocator (used from the mesh-upload step onward) ---------
  VmaAllocatorCreateInfo allocatorInfo{};
  allocatorInfo.physicalDevice = physicalDevice.physical_device;
  allocatorInfo.device = device.device;
  allocatorInfo.instance = instance.instance;
  allocatorInfo.vulkanApiVersion = VK_API_VERSION_1_3;
  check(vmaCreateAllocator(&allocatorInfo, &allocator), "vmaCreateAllocator");

  // --- Command pool + per-frame command buffers --------------------
  VkCommandPoolCreateInfo poolInfo{};
  poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
  poolInfo.queueFamilyIndex = graphicsQueueFamily;
  check(vkCreateCommandPool(device.device, &poolInfo, nullptr, &commandPool),
        "vkCreateCommandPool");

  VkCommandBufferAllocateInfo cmdAllocInfo{};
  cmdAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  cmdAllocInfo.commandPool = commandPool;
  cmdAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  cmdAllocInfo.commandBufferCount = kFramesInFlight;
  check(vkAllocateCommandBuffers(device.device, &cmdAllocInfo,
                                 commandBuffers.data()),
        "vkAllocateCommandBuffers");

  // --- Per-frame sync objects -------------------------------------
  VkSemaphoreCreateInfo semaphoreInfo{};
  semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
  VkFenceCreateInfo fenceInfo{};
  fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
  for (uint32_t i = 0; i < kFramesInFlight; ++i) {
    check(vkCreateSemaphore(device.device, &semaphoreInfo, nullptr,
                            &imageAvailable[i]),
          "vkCreateSemaphore(imageAvailable)");
    check(vkCreateFence(device.device, &fenceInfo, nullptr, &inFlight[i]),
          "vkCreateFence");
  }

  createSwapchain(/*recreate=*/false);
  initImGui();
}

Renderer::Impl::~Impl() {
  if (device.device != VK_NULL_HANDLE) {
    vkDeviceWaitIdle(device.device);
  }

  shutdownImGui();
  destroySwapchain();

  for (uint32_t i = 0; i < kFramesInFlight; ++i) {
    if (imageAvailable[i] != VK_NULL_HANDLE) {
      vkDestroySemaphore(device.device, imageAvailable[i], nullptr);
    }
    if (inFlight[i] != VK_NULL_HANDLE) {
      vkDestroyFence(device.device, inFlight[i], nullptr);
    }
  }
  if (commandPool != VK_NULL_HANDLE) {
    vkDestroyCommandPool(device.device, commandPool, nullptr);
  }
  if (allocator != VK_NULL_HANDLE) {
    vmaDestroyAllocator(allocator);
  }
  if (device.device != VK_NULL_HANDLE) {
    vkb::destroy_device(device);
  }
  if (surface != VK_NULL_HANDLE) {
    vkb::destroy_surface(instance, surface);
  }
  vkb::destroy_instance(instance);
}

auto Renderer::Impl::createSwapchain(bool recreate) -> void {
  auto [width, height] = window->framebufferSize();

  vkb::SwapchainBuilder builder(device);
  builder
      .set_desired_format(
          VkSurfaceFormatKHR{.format = VK_FORMAT_B8G8R8A8_SRGB,
                             .colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR})
      .set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR)
      .set_desired_extent(width, height)
      .add_image_usage_flags(VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);

  vkb::Swapchain old = swapchain;
  if (recreate && old.swapchain != VK_NULL_HANDLE) {
    builder.set_old_swapchain(old);
  }

  auto swapchainResult = builder.build();
  if (!swapchainResult) {
    throw std::runtime_error("vkb swapchain: " +
                             swapchainResult.error().message());
  }

  if (recreate) {
    old.destroy_image_views(imageViews);
    vkb::destroy_swapchain(old);
  }

  swapchain = swapchainResult.value();
  swapchainColorFormat = swapchain.image_format;
  images = swapchain.get_images().value();
  imageViews = swapchain.get_image_views().value();

  for (VkSemaphore semaphore : renderFinished) {
    vkDestroySemaphore(device.device, semaphore, nullptr);
  }
  renderFinished.assign(images.size(), VK_NULL_HANDLE);
  VkSemaphoreCreateInfo semaphoreInfo{};
  semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
  for (VkSemaphore &semaphore : renderFinished) {
    check(vkCreateSemaphore(device.device, &semaphoreInfo, nullptr, &semaphore),
          "vkCreateSemaphore(renderFinished)");
  }
}

auto Renderer::Impl::destroySwapchain() -> void {
  for (VkSemaphore semaphore : renderFinished) {
    if (semaphore != VK_NULL_HANDLE) {
      vkDestroySemaphore(device.device, semaphore, nullptr);
    }
  }
  renderFinished.clear();

  swapchain.destroy_image_views(imageViews);
  imageViews.clear();
  images.clear();
  vkb::destroy_swapchain(swapchain);
  swapchain = {};
}

auto Renderer::Impl::recreateSwapchain() -> void {
  window->waitWhileMinimized();
  vkDeviceWaitIdle(device.device);
  createSwapchain(/*recreate=*/true);
}

auto Renderer::Impl::initImGui() -> void {
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
  ImGui::StyleColorsDark();

  // install_callbacks = true: ImGui chains onto the window's existing GLFW
  // callbacks rather than replacing them, so Window's key handler still fires.
  ImGui_ImplGlfw_InitForVulkan(window->handle(), true);

  ImGui_ImplVulkan_InitInfo initInfo{};
  initInfo.ApiVersion = VK_API_VERSION_1_3;
  initInfo.Instance = instance.instance;
  initInfo.PhysicalDevice = physicalDevice.physical_device;
  initInfo.Device = device.device;
  initInfo.QueueFamily = graphicsQueueFamily;
  initInfo.Queue = graphicsQueue;
  // Non-zero DescriptorPoolSize + null DescriptorPool: the backend creates and
  // owns an internal pool sized for the font atlas plus this many extra
  // ImGui::Image textures.
  initInfo.DescriptorPoolSize = 16;
  initInfo.MinImageCount = 2;
  initInfo.ImageCount = static_cast<uint32_t>(images.size());
  initInfo.UseDynamicRendering = true;
  // Since imgui 1.92 the pipeline/MSAA/dynamic-rendering formats live in a
  // nested PipelineInfoMain rather than directly on InitInfo.
  initInfo.PipelineInfoMain.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
  initInfo.PipelineInfoMain.PipelineRenderingCreateInfo.sType =
      VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
  initInfo.PipelineInfoMain.PipelineRenderingCreateInfo.colorAttachmentCount = 1;
  initInfo.PipelineInfoMain.PipelineRenderingCreateInfo.pColorAttachmentFormats =
      &swapchainColorFormat;
  // If the vcpkg imgui vulkan backend is ever built with
  // IMGUI_IMPL_VULKAN_NO_PROTOTYPES (volk), add ImGui_ImplVulkan_LoadFunctions
  // here before Init.
  ImGui_ImplVulkan_Init(&initInfo);

  imguiInitialized = true;
}

auto Renderer::Impl::shutdownImGui() -> void {
  if (!imguiInitialized) return;
  ImGui_ImplVulkan_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();
  imguiInitialized = false;
}

auto Renderer::Impl::recordFrame(VkCommandBuffer cmd, uint32_t imageIndex)
    -> void {
  VkCommandBufferBeginInfo beginInfo{};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  check(vkBeginCommandBuffer(cmd, &beginInfo), "vkBeginCommandBuffer");

  VkImageSubresourceRange fullColor{};
  fullColor.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  fullColor.levelCount = 1;
  fullColor.layerCount = 1;

  // UNDEFINED -> COLOR_ATTACHMENT_OPTIMAL
  VkImageMemoryBarrier2 toColor{};
  toColor.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
  toColor.srcStageMask = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT;
  toColor.srcAccessMask = 0;
  toColor.dstStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
  toColor.dstAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
  toColor.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  toColor.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
  toColor.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  toColor.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  toColor.image = images[imageIndex];
  toColor.subresourceRange = fullColor;

  VkDependencyInfo toColorDep{};
  toColorDep.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
  toColorDep.imageMemoryBarrierCount = 1;
  toColorDep.pImageMemoryBarriers = &toColor;
  vkCmdPipelineBarrier2(cmd, &toColorDep);

  VkClearValue clear{};
  clear.color = {{0.2f, 0.3f, 0.3f, 1.0f}};

  VkRenderingAttachmentInfo colorAttachment{};
  colorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
  colorAttachment.imageView = imageViews[imageIndex];
  colorAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
  colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  colorAttachment.clearValue = clear;

  VkRenderingInfo renderingInfo{};
  renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
  renderingInfo.renderArea.offset = {0, 0};
  renderingInfo.renderArea.extent = swapchain.extent;
  renderingInfo.layerCount = 1;
  renderingInfo.colorAttachmentCount = 1;
  renderingInfo.pColorAttachments = &colorAttachment;

  vkCmdBeginRendering(cmd, &renderingInfo);
  // TODO(vulkan/mesh-step): bind pipeline for item.program, set dynamic
  // viewport+scissor from swapchain.extent, push the MVP + color, and
  // vkCmdDraw the sorted scene queue here.

  // Debug overlay, drawn on top of the scene into the same attachment.
  if (debugUi) {
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    debugUi();
    ImGui::Render();
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd);
  }

  vkCmdEndRendering(cmd);

  // COLOR_ATTACHMENT_OPTIMAL -> PRESENT_SRC
  VkImageMemoryBarrier2 toPresent{};
  toPresent.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
  toPresent.srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
  toPresent.srcAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
  toPresent.dstStageMask = VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT;
  toPresent.dstAccessMask = 0;
  toPresent.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
  toPresent.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
  toPresent.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  toPresent.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  toPresent.image = images[imageIndex];
  toPresent.subresourceRange = fullColor;

  VkDependencyInfo toPresentDep{};
  toPresentDep.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
  toPresentDep.imageMemoryBarrierCount = 1;
  toPresentDep.pImageMemoryBarriers = &toPresent;
  vkCmdPipelineBarrier2(cmd, &toPresentDep);

  check(vkEndCommandBuffer(cmd), "vkEndCommandBuffer");
}

// ===========================================================================
// Renderer — thin forwarding shell over Impl.
// ===========================================================================
Renderer::Renderer(window::Window &window) : _impl(std::in_place, window) {}

Renderer::~Renderer() = default;
Renderer::Renderer(Renderer &&) noexcept = default;
Renderer &Renderer::operator=(Renderer &&) noexcept = default;

auto Renderer::uploadMesh(std::span<const float> /*positions*/,
                          Primitive /*primitive*/, Usage /*usage*/)
    -> MeshHandle {
  // TODO(vulkan/mesh-step): create a device-local VkBuffer via VMA, stage the
  // positions into it, remember (buffer, allocation, vertexCount, topology),
  // and hand back a 1-based handle.
  return static_cast<MeshHandle>(1);
}

auto Renderer::createProgram(std::string const & /*vertexSource*/,
                             std::string const & /*fragmentSource*/)
    -> std::expected<ProgramHandle, std::string> {
  // TODO(vulkan/pipeline-step): compile GLSL -> SPIR-V with shaderc, create
  // VkShaderModules, build a VkPipeline (dynamic viewport/scissor, topology
  // from the mesh, push-constant range for MVP + color), store it, return a
  // 1-based handle.
  return static_cast<ProgramHandle>(1);
}

auto Renderer::Impl::render(const Scene & /*scene*/,
                            glm::mat4 const & /*viewProj*/) -> void {
  const uint32_t frame = currentFrame;

  check(vkWaitForFences(device.device, 1, &inFlight[frame], VK_TRUE,
                        UINT64_MAX),
        "vkWaitForFences");

  uint32_t imageIndex = 0;
  VkResult acquire = vkAcquireNextImageKHR(
      device.device, swapchain.swapchain, UINT64_MAX, imageAvailable[frame],
      VK_NULL_HANDLE, &imageIndex);
  if (acquire == VK_ERROR_OUT_OF_DATE_KHR) {
    recreateSwapchain();
    return;
  }
  if (acquire != VK_SUCCESS && acquire != VK_SUBOPTIMAL_KHR) {
    check(acquire, "vkAcquireNextImageKHR");
  }

  check(vkResetFences(device.device, 1, &inFlight[frame]), "vkResetFences");

  VkCommandBuffer cmd = commandBuffers[frame];
  check(vkResetCommandBuffer(cmd, 0), "vkResetCommandBuffer");
  recordFrame(cmd, imageIndex);

  VkSemaphoreSubmitInfo waitInfo{};
  waitInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
  waitInfo.semaphore = imageAvailable[frame];
  waitInfo.stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;

  VkSemaphoreSubmitInfo signalInfo{};
  signalInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
  signalInfo.semaphore = renderFinished[imageIndex];
  signalInfo.stageMask = VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT;

  VkCommandBufferSubmitInfo cmdInfo{};
  cmdInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
  cmdInfo.commandBuffer = cmd;

  VkSubmitInfo2 submit{};
  submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
  submit.waitSemaphoreInfoCount = 1;
  submit.pWaitSemaphoreInfos = &waitInfo;
  submit.commandBufferInfoCount = 1;
  submit.pCommandBufferInfos = &cmdInfo;
  submit.signalSemaphoreInfoCount = 1;
  submit.pSignalSemaphoreInfos = &signalInfo;
  check(vkQueueSubmit2(graphicsQueue, 1, &submit, inFlight[frame]),
        "vkQueueSubmit2");

  VkPresentInfoKHR present{};
  present.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
  present.waitSemaphoreCount = 1;
  present.pWaitSemaphores = &renderFinished[imageIndex];
  present.swapchainCount = 1;
  present.pSwapchains = &swapchain.swapchain;
  present.pImageIndices = &imageIndex;

  VkResult presented = vkQueuePresentKHR(presentQueue, &present);
  if (presented == VK_ERROR_OUT_OF_DATE_KHR || presented == VK_SUBOPTIMAL_KHR ||
      window->framebufferResized()) {
    window->clearFramebufferResized();
    recreateSwapchain();
  } else if (presented != VK_SUCCESS) {
    check(presented, "vkQueuePresentKHR");
  }

  currentFrame = (frame + 1) % kFramesInFlight;
}

auto Renderer::render(const Scene &scene, glm::mat4 const &viewProj) -> void {
  _impl->render(scene, viewProj);
}

auto Renderer::setDebugUi(DebugUiFn fn) -> void {
  _impl->debugUi = std::move(fn);
}

auto loadShaderSource(std::filesystem::path const &path)
    -> std::expected<std::string, std::string> {
  std::ifstream file(path);
  if (!file) {
    return std::unexpected("failed to open shader file: " + path.string());
  }

  std::ostringstream contents;
  contents << file.rdbuf();
  return std::move(contents).str();
}

} // namespace malesia::render
