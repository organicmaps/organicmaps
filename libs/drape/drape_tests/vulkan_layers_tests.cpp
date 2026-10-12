#include "testing/testing.hpp"

#include "drape/vulkan/vulkan_layers.hpp"

#include "base/scope_guard.hpp"

#include <cstring>
#include <iterator>
#include <string_view>

namespace vulkan_layers_tests
{
VKAPI_ATTR VkResult VKAPI_CALL EnumerateExtensions(char const *, uint32_t * count, VkExtensionProperties * properties)
{
  char const * const names[] = {"VK_KHR_surface", "VK_KHR_android_surface", "VK_EXT_debug_report",
                                "VK_EXT_validation_features"};
  if (properties)
    for (size_t i = 0; i < std::size(names); ++i)
      std::strcpy(properties[i].extensionName, names[i]);
  *count = std::size(names);
  return VK_SUCCESS;
}

VKAPI_ATTR VkResult VKAPI_CALL EnumerateLayers(uint32_t * count, VkLayerProperties *)
{
  *count = 0;
  return VK_SUCCESS;
}

bool HasExtension(dp::vulkan::Layers const & layers, std::string_view name)
{
  for (uint32_t i = 0; i < layers.GetInstanceExtensionsCount(); ++i)
    if (layers.GetInstanceExtensions()[i] == name)
      return true;
  return false;
}

UNIT_TEST(VulkanLayers_RequiredExtensions)
{
  auto const savedExtensions = vkEnumerateInstanceExtensionProperties;
  auto const savedLayers = vkEnumerateInstanceLayerProperties;
  SCOPE_GUARD(restore, [&]()
  {
    vkEnumerateInstanceExtensionProperties = savedExtensions;
    vkEnumerateInstanceLayerProperties = savedLayers;
  });
  vkEnumerateInstanceExtensionProperties = &EnumerateExtensions;
  vkEnumerateInstanceLayerProperties = &EnumerateLayers;

  // Equal names at different addresses must be deduplicated. Unsupported
  // requirements must reach vkCreateInstance instead of being silently filtered.
  char surface[] = "VK_KHR_surface";
  char xcb[] = "VK_KHR_xcb_surface";
  char duplicate[] = "VK_KHR_xcb_surface";
  char const * const required[] = {surface, xcb, duplicate};
  dp::vulkan::Layers layers(false, required);
  TEST_EQUAL(layers.GetInstanceExtensionsCount(), 3, ());
  TEST(HasExtension(layers, "VK_KHR_surface"), ());
  TEST(HasExtension(layers, "VK_KHR_xcb_surface"), ());
  TEST(HasExtension(layers, "VK_KHR_android_surface"), ());
  TEST(!HasExtension(layers, "VK_EXT_debug_report"), ());
  TEST(!layers.IsValidationFeaturesEnabled(), ());

  dp::vulkan::Layers defaultLayers(false);
  TEST_EQUAL(defaultLayers.GetInstanceExtensionsCount(), 2, ());

  dp::vulkan::Layers diagnosticLayers(true, required);
  TEST_EQUAL(diagnosticLayers.GetInstanceExtensionsCount(), 5, ());
  TEST(HasExtension(diagnosticLayers, "VK_EXT_debug_report"), ());
  TEST(diagnosticLayers.IsValidationFeaturesEnabled(), ());

  char const * const requiredDiagnostic[] = {"VK_EXT_debug_report"};
  dp::vulkan::Layers explicitDiagnostic(false, requiredDiagnostic);
  TEST(HasExtension(explicitDiagnostic, "VK_EXT_debug_report"), ());
}
}  // namespace vulkan_layers_tests
