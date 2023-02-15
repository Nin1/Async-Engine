//#pragma once
//#define GLFW_INCLUDE_VULKAN
//#include "FrameStageRunner.h"
//#include "../ShaderProgram.h"
//#include <GLFW/glfw3.h>
//
//
//class VulkanRenderRunner : public FrameStageRunner
//{
//public:
//	VulkanRenderRunner() : FrameStageRunner("GPU Execution") { }
//	~VulkanRenderRunner();
//
//	virtual void Init() override;
//
//protected:
//	virtual void RunJobInner(JobCounterPtr& jobCounter) override;
//
//	/** Vulkan initialisation */
//	void CreateInstance();
//	std::vector<const char*> GetRequiredExtensions();
//	bool CheckValidationLayerSupport();
//	void SetupDebugMessenger();
//	VkDebugUtilsMessengerCreateInfoEXT CreateDebugMessengerCreateInfo();
//
//private:
//	int m_framesCompleted = 0;
//	VkInstance m_instance;
//
//	/** Vulkan debug callback handler */
//	VkDebugUtilsMessengerEXT m_debugMessenger;
//
//	const std::vector<const char*> m_validationLayers = {
//		"VK_LAYER_KHRONOS_validation"
//	};
//#ifdef NDEBUG
//	const bool m_enableValidationLayers = false;
//#else
//	const bool m_enableValidationLayers = true;
//#endif
//};
//