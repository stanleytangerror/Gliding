#pragma once

#ifdef VulkanBackend_Export
#define GD_VULKANBACKEND_API __declspec(dllexport)
#else
#define GD_VULKANBACKEND_API
#endif