#include "RenderingComponents.h"

void RENDERING::SetDebugName(VkDevice device, VkObjectType objectType, uint64_t objectHandle, const char* name)
{
    if (!device || !name) return;
    auto function = (PFN_vkSetDebugUtilsObjectNameEXT)vkGetDeviceProcAddr(device, "vkSetDebugUtilsObjectNameEXT");
    if (!function) return;

    VkDebugUtilsObjectNameInfoEXT nameInfo{};
    nameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
    nameInfo.objectType = objectType;
    nameInfo.objectHandle = objectHandle;
    nameInfo.pObjectName = name;
    function(device, &nameInfo);
}
