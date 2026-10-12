#include "testing/testing.hpp"

#include "drape/vulkan/vulkan_layers.hpp"

#include <cstring>
#include <utility>
#include <vector>

namespace vulkan_layers_tests
{
std::vector<char const *> g_instanceExtensions;
std::vector<char const *> g_deviceExtensions;
bool g_feature;
unsigned g_queries;

VkResult Enumerate(std::vector<char const *> const & names, uint32_t * count, VkExtensionProperties * properties)
{
  if (properties)
    for (size_t i = 0; i < names.size(); ++i)
      std::strcpy(properties[i].extensionName, names[i]);
  *count = static_cast<uint32_t>(names.size());
  return VK_SUCCESS;
}

VKAPI_ATTR VkResult VKAPI_CALL InstanceExtensions(char const *, uint32_t * count, VkExtensionProperties * properties)
{
  return Enumerate(g_instanceExtensions, count, properties);
}

VKAPI_ATTR VkResult VKAPI_CALL DeviceExtensions(VkPhysicalDevice, char const *, uint32_t * count,
                                                VkExtensionProperties * properties)
{
  return Enumerate(g_deviceExtensions, count, properties);
}

VKAPI_ATTR void VKAPI_CALL Features(VkPhysicalDevice, VkPhysicalDeviceFeatures2 * features)
{
  ++g_queries;
  auto * maintenance = static_cast<VkPhysicalDeviceSwapchainMaintenance1FeaturesKHR *>(features->pNext);
  TEST_EQUAL(maintenance->sType, VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SWAPCHAIN_MAINTENANCE_1_FEATURES_KHR, ());
  maintenance->swapchainMaintenance1 = g_feature;
}

VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL InstanceProc(VkInstance, char const * name)
{
  TEST_EQUAL(std::string(name), "vkGetPhysicalDeviceFeatures2KHR", ());
  return reinterpret_cast<PFN_vkVoidFunction>(&Features);
}

class EntryPoints
{
public:
  EntryPoints()
    : m_instance(std::exchange(vkEnumerateInstanceExtensionProperties, &InstanceExtensions))
    , m_device(std::exchange(vkEnumerateDeviceExtensionProperties, &DeviceExtensions))
    , m_proc(std::exchange(vkGetInstanceProcAddr, &InstanceProc))
  {
    g_queries = 0;
  }
  ~EntryPoints()
  {
    vkEnumerateInstanceExtensionProperties = m_instance;
    vkEnumerateDeviceExtensionProperties = m_device;
    vkGetInstanceProcAddr = m_proc;
  }

private:
  PFN_vkEnumerateInstanceExtensionProperties m_instance;
  PFN_vkEnumerateDeviceExtensionProperties m_device;
  PFN_vkGetInstanceProcAddr m_proc;
};

void Prepare(bool khr)
{
  g_instanceExtensions = {
      VK_KHR_SURFACE_EXTENSION_NAME, VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME,
      VK_KHR_GET_SURFACE_CAPABILITIES_2_EXTENSION_NAME,
      khr ? VK_KHR_SURFACE_MAINTENANCE_1_EXTENSION_NAME : VK_EXT_SURFACE_MAINTENANCE_1_EXTENSION_NAME};
  g_deviceExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME, khr ? VK_KHR_SWAPCHAIN_MAINTENANCE_1_EXTENSION_NAME
                                                             : VK_EXT_SWAPCHAIN_MAINTENANCE_1_EXTENSION_NAME};
  g_feature = true;
}

bool ContainsDeviceExtension(dp::vulkan::Layers const & layers, char const * name)
{
  for (uint32_t i = 0; i < layers.GetDeviceExtensionsCount(); ++i)
    if (std::strcmp(layers.GetDeviceExtensions()[i], name) == 0)
      return true;
  return false;
}

UNIT_TEST(VulkanLayers_MaintenanceRequiresFeatureAndDependencies)
{
  EntryPoints entryPoints;
  for (bool khr : {true, false})
  {
    Prepare(khr);
    dp::vulkan::Layers supported(false);
    TEST(supported.Initialize(VK_NULL_HANDLE, VK_NULL_HANDLE), ());
    TEST(supported.IsSwapchainMaintenanceEnabled(), ());
    TEST(ContainsDeviceExtension(supported, g_deviceExtensions.back()), ());

    g_feature = false;
    dp::vulkan::Layers disabled(false);
    TEST(disabled.Initialize(VK_NULL_HANDLE, VK_NULL_HANDLE), ());
    TEST(!disabled.IsSwapchainMaintenanceEnabled(), ());
    TEST(!ContainsDeviceExtension(disabled, g_deviceExtensions.back()), ());

    Prepare(khr);
    g_instanceExtensions.erase(g_instanceExtensions.begin() + 2);
    g_queries = 0;
    dp::vulkan::Layers missingDependency(false);
    TEST(missingDependency.Initialize(VK_NULL_HANDLE, VK_NULL_HANDLE), ());
    TEST(!missingDependency.IsSwapchainMaintenanceEnabled(), ());
    TEST_EQUAL(g_queries, 0, ());
  }
}
}  // namespace vulkan_layers_tests
