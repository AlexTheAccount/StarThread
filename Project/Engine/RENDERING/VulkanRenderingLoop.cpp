#include "RenderingComponents.h"

namespace RENDERING::RENDERER_HELPERS
{
    void RenderingLoop()
    {
        RendererComponent* rendererComponent = GetGlobalRenderer();
        if (!rendererComponent)
        {
            printf("No RendererComponent found in GlobalRegistry\n");
            return;
        }

        while (!glfwWindowShouldClose(rendererComponent->window))
        {
            glfwPollEvents();
            DrawFrame();
        }
        if (rendererComponent->device != VK_NULL_HANDLE) vkDeviceWaitIdle(rendererComponent->device);
    }
}