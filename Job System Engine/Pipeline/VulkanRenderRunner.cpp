//#include "VulkanRenderRunner.h"
//#include "FrameData.h"
//#include "../Assert.h"
//#include "../Input.h"
//#include <iostream>
//#include <cstdio>
//#include <GLFW/glfw3.h>
//
//// Debug callback
//static VKAPI_ATTR VkBool32 VKAPI_CALL vulkanCallbackFunction(
//	VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
//	VkDebugUtilsMessageTypeFlagsEXT messageType,
//	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
//	void* pUserData)
//{
//	if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
//		std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;
//	}
//	return VK_FALSE;
//}
//
//void VulkanRenderRunner::Init()
//{
//	CreateInstance();
//	SetupDebugMessenger();
//}
//
//void VulkanRenderRunner::CreateInstance()
//{
//	// Print available extensions
//	uint32_t extensionCount = 0;
//	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
//	std::vector<VkExtensionProperties> extensions(extensionCount);
//	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());
//	std::cout << "VULKAN: " << extensionCount << " extensions supported\n";
//	for (const auto& ext : extensions)
//	{
//		std::cout << "    " << ext.extensionName << std::endl;
//	}
//
//	// Check for validation layers if using them (Debug builds only)
//	if (m_enableValidationLayers && !CheckValidationLayerSupport())
//	{
//		ASSERT(false);
//	}
//
//	// Vulkan app info
//	VkApplicationInfo appInfo{};
//	{
//		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
//		appInfo.pApplicationName = "Test";
//		appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
//		appInfo.pEngineName = "None";
//		appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
//		appInfo.apiVersion = VK_API_VERSION_1_0;
//	}
//
//	// Vulkan instance create info
//	VkInstanceCreateInfo createInfo{};
//	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
//	createInfo.pApplicationInfo = &appInfo;
//	// Validate GLFW extensions
//	std::vector<const char*> requiredExtensions = GetRequiredExtensions();
//	createInfo.enabledExtensionCount = static_cast<uint32_t>(requiredExtensions.size());
//	createInfo.ppEnabledExtensionNames = requiredExtensions.data();
//	// Enable validation layers (Debug only)
//	VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
//	if (m_enableValidationLayers)
//	{
//		createInfo.enabledLayerCount = static_cast<uint32_t>(m_validationLayers.size());
//		createInfo.ppEnabledLayerNames = m_validationLayers.data();
//		debugCreateInfo = CreateDebugMessengerCreateInfo();
//		createInfo.pNext = &debugCreateInfo;
//	}
//	else
//	{
//		createInfo.enabledLayerCount = 0;
//		createInfo.pNext = nullptr;
//	}
//
//	if (vkCreateInstance(&createInfo, nullptr, &m_instance) != VK_SUCCESS)
//	{
//		ASSERT(false);
//	}
//}
//
//std::vector<const char*> VulkanRenderRunner::GetRequiredExtensions()
//{
//	uint32_t glfwExtensionCount = 0;
//	const char** glfwExtensions;
//	glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
//
//	std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
//
//	if (m_enableValidationLayers)
//	{
//		extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
//	}
//
//	return extensions;
//}
//
//bool VulkanRenderRunner::CheckValidationLayerSupport()
//{
//	uint32_t layerCount;
//	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
//
//	std::vector<VkLayerProperties> availableLayers(layerCount);
//	vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());
//
//	for (const char* layerName : m_validationLayers)
//	{
//		bool layerFound = false;
//
//		for (const auto& layerProperties : availableLayers)
//		{
//			if (strcmp(layerName, layerProperties.layerName) == 0)
//			{
//				layerFound = true;
//				break;
//			}
//		}
//
//		if (!layerFound)
//		{
//			return false;
//		}
//	}
//
//	return true;
//}
//
//VkDebugUtilsMessengerCreateInfoEXT VulkanRenderRunner::CreateDebugMessengerCreateInfo()
//{
//	VkDebugUtilsMessengerCreateInfoEXT createInfo{};
//	createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
//	createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
//	createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
//	createInfo.pfnUserCallback = vulkanCallbackFunction;
//	createInfo.pUserData = nullptr; // Optional
//	return createInfo;
//}
//
//void VulkanRenderRunner::SetupDebugMessenger()
//{
//	if (m_enableValidationLayers)
//	{
//		VkDebugUtilsMessengerCreateInfoEXT createInfo = CreateDebugMessengerCreateInfo();
//		// Get extension function for debug messenger
//		auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(m_instance, "vkCreateDebugUtilsMessengerEXT");
//		if (func != nullptr)
//		{
//			func(m_instance, &createInfo, nullptr, &m_debugMessenger);
//		}
//		else
//		{
//			ASSERTM(false, "Unable to create Vulkan debug messenger");
//		}
//	}
//}
//
//VulkanRenderRunner::~VulkanRenderRunner()
//{
//	// Destroy debug messenger
//	if (m_enableValidationLayers)
//	{
//		auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(m_instance, "vkDestroyDebugUtilsMessengerEXT");
//		if (func != nullptr)
//		{
//			func(m_instance, m_debugMessenger, nullptr);
//		}
//	}
//
//	vkDestroyInstance(m_instance, nullptr);
//}
//
//void VulkanRenderRunner::RunJobInner(JobCounterPtr& jobCounter)
//{
//	// Debug: Make sure frames are running in the correct order
//	ASSERT(m_frameData->m_frameNumber == m_framesCompleted);
//
//	m_frameData->m_stage = FrameStage::GPU_EXECUTION;
//	if (m_frameData->m_frameNumber % 100 == 0)
//	{
//		//std::printf("Completed frame %I64d \n", m_frameData->m_frameNumber);
//	}
//
//	m_framesCompleted++;
//}
