package window

import "core:fmt"
import "core:slice"
import "vendor:glfw"
import vk "vendor:vulkan"

SHADER :: #load("out.spv")

MAX_FRAMES_IN_FLIGHT :: 1

window: glfw.WindowHandle
surface: vk.SurfaceKHR
physical_device: vk.PhysicalDevice
device: vk.Device
swapchain: vk.SwapchainKHR
swapchain_extent: vk.Extent2D
swapchain_image_format: vk.Format
swapchain_images: []vk.Image
swapchain_image_views: []vk.ImageView
command_buffer: vk.CommandBuffer
pipeline: vk.Pipeline

main :: proc() {
	glfw.Init()
	glfw.WindowHint(glfw.CLIENT_API, glfw.NO_API)

	window = glfw.CreateWindow(800, 600, "Vulkan window in Odin", nil, nil)

	// Load the initial Vulkan functions needed for creating the instance
	vk.load_proc_addresses_global(rawptr(glfw.GetInstanceProcAddress))

	// CREATE INSTANCE
	extensions := glfw.GetRequiredInstanceExtensions()
	create_info := vk.InstanceCreateInfo {
		sType                   = .INSTANCE_CREATE_INFO,
		pApplicationInfo        = &vk.ApplicationInfo{apiVersion = vk.API_VERSION_1_3},
		enabledExtensionCount   = u32(len(extensions)),
		ppEnabledExtensionNames = raw_data(extensions),
	}
	instance: vk.Instance
	vk.CreateInstance(&create_info, nil, &instance)

	// Load the rest of the Vulkan functions based on the instance
	vk.load_proc_addresses_instance(instance)

	// CREATE SURFACE
	glfw.CreateWindowSurface(instance, window, nil, &surface)

	// PICK PHYSICAL DEVICE
	physical_device_count: u32
	vk.EnumeratePhysicalDevices(instance, &physical_device_count, nil)
	physical_devices := make([]vk.PhysicalDevice, physical_device_count)
	vk.EnumeratePhysicalDevices(instance, &physical_device_count, raw_data(physical_devices))
	queue_family_index: u32 = 0
	physical_device = physical_devices[queue_family_index]

	// CREATE LOGICAL DEVICE
	enabled_vk_13_features := vk.PhysicalDeviceVulkan13Features {
		sType            = .PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
		synchronization2 = true,
		dynamicRendering = true,
	}
	enabled_vk_11_features := vk.PhysicalDeviceVulkan11Features {
		sType                = .PHYSICAL_DEVICE_VULKAN_1_1_FEATURES,
		pNext                = &enabled_vk_13_features,
		shaderDrawParameters = true,
	}
	device_create_info := vk.DeviceCreateInfo {
		sType                   = .DEVICE_CREATE_INFO,
		pNext                   = &enabled_vk_11_features,
		queueCreateInfoCount    = 1,
		pQueueCreateInfos       = raw_data(
			[]vk.DeviceQueueCreateInfo {
				{
					sType = .DEVICE_QUEUE_CREATE_INFO,
					queueFamilyIndex = queue_family_index,
					queueCount = 1,
					pQueuePriorities = raw_data([]f32{1}),
				},
			},
		),
		enabledExtensionCount   = 1,
		ppEnabledExtensionNames = raw_data([]cstring{vk.KHR_SWAPCHAIN_EXTENSION_NAME}),
	}
	vk.CreateDevice(physical_device, &device_create_info, nil, &device)

	// CREATE SWAPCHAIN
	create_swapchain()

	// CREATE COMMAND BUFFERS
	command_pool_create_info := vk.CommandPoolCreateInfo {
		sType            = .COMMAND_POOL_CREATE_INFO,
		flags            = {.RESET_COMMAND_BUFFER},
		queueFamilyIndex = queue_family_index,
	}
	command_pool: vk.CommandPool
	vk.CreateCommandPool(device, &command_pool_create_info, nil, &command_pool)
	command_buffer_allocate_info := vk.CommandBufferAllocateInfo {
		sType              = .COMMAND_BUFFER_ALLOCATE_INFO,
		commandPool        = command_pool,
		level              = .PRIMARY,
		commandBufferCount = MAX_FRAMES_IN_FLIGHT,
	}
	command_buffers: [MAX_FRAMES_IN_FLIGHT]vk.CommandBuffer
	vk.AllocateCommandBuffers(device, &command_buffer_allocate_info, &command_buffers[0])
	command_buffer = command_buffers[0]

	// GET DEVICE QUEUE
	queue: vk.Queue
	vk.GetDeviceQueue(device, queue_family_index, 0, &queue)

	// CREATE SYNC OBJECTS
	semaphore_ci := vk.SemaphoreCreateInfo {
		sType = .SEMAPHORE_CREATE_INFO,
	}
	image_available_semaphore: vk.Semaphore
	vk.CreateSemaphore(device, &semaphore_ci, nil, &image_available_semaphore)
	render_finished_semaphore: vk.Semaphore
	vk.CreateSemaphore(device, &semaphore_ci, nil, &render_finished_semaphore)
	fence_ci := vk.FenceCreateInfo {
		sType = .FENCE_CREATE_INFO,
		flags = {.SIGNALED},
	}
	fence: vk.Fence
	vk.CreateFence(device, &fence_ci, nil, &fence)

	// CREATE PIPELINE
	create_pipeline()

	for !glfw.WindowShouldClose(window) {
		glfw.PollEvents()

		vk.WaitForFences(device, 1, &fence, true, max(u64))
		vk.ResetFences(device, 1, &fence)

		image_index: u32
		vk.AcquireNextImageKHR(
			device,
			swapchain,
			max(u64),
			image_available_semaphore,
			0,
			&image_index,
		)

		record_commands(image_index)

		vk.QueueWaitIdle(queue)

		submit_info := vk.SubmitInfo {
			sType                = .SUBMIT_INFO,
			waitSemaphoreCount   = 1,
			pWaitSemaphores      = &image_available_semaphore,
			pWaitDstStageMask    = &vk.PipelineStageFlags{.COLOR_ATTACHMENT_OUTPUT},
			commandBufferCount   = 1,
			pCommandBuffers      = &command_buffer,
			signalSemaphoreCount = 1,
			pSignalSemaphores    = &render_finished_semaphore,
		}
		vk.QueueSubmit(queue, 1, &submit_info, fence)

		present_info := vk.PresentInfoKHR {
			sType              = .PRESENT_INFO_KHR,
			waitSemaphoreCount = 1,
			pWaitSemaphores    = &render_finished_semaphore,
			swapchainCount     = 1,
			pSwapchains        = &swapchain,
			pImageIndices      = &image_index,
		}
		vk.QueuePresentKHR(queue, &present_info)
	}
}

create_swapchain :: proc() {
	surface_caps: vk.SurfaceCapabilitiesKHR
	vk.GetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device, surface, &surface_caps)
	swapchain_extent = choose_surface_extent(surface_caps)

	format_count: u32
	vk.GetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &format_count, nil)
	formats := make([]vk.SurfaceFormatKHR, format_count)
	vk.GetPhysicalDeviceSurfaceFormatsKHR(
		physical_device,
		surface,
		&format_count,
		raw_data(formats),
	)
	format := formats[0]
	swapchain_image_format = format.format

	swapchain_create_info := vk.SwapchainCreateInfoKHR {
		sType            = .SWAPCHAIN_CREATE_INFO_KHR,
		surface          = surface,
		minImageCount    = surface_caps.minImageCount + 1,
		imageFormat      = swapchain_image_format,
		imageColorSpace  = format.colorSpace,
		imageExtent      = swapchain_extent,
		imageArrayLayers = 1,
		imageUsage       = {.COLOR_ATTACHMENT},
		imageSharingMode = .EXCLUSIVE,
		preTransform     = surface_caps.currentTransform,
		compositeAlpha   = {.OPAQUE},
		presentMode      = .FIFO,
		clipped          = true,
	}
	vk.CreateSwapchainKHR(device, &swapchain_create_info, nil, &swapchain)

	swapchain_image_count: u32
	vk.GetSwapchainImagesKHR(device, swapchain, &swapchain_image_count, nil)
	swapchain_images = make([]vk.Image, swapchain_image_count)
	vk.GetSwapchainImagesKHR(device, swapchain, &swapchain_image_count, raw_data(swapchain_images))

	swapchain_image_views = make([]vk.ImageView, swapchain_image_count)
	for image, i in swapchain_images {
		image_view_create_info := vk.ImageViewCreateInfo {
			sType = .IMAGE_VIEW_CREATE_INFO,
			image = swapchain_images[i],
			viewType = .D2,
			format = format.format,
			subresourceRange = {aspectMask = {.COLOR}, levelCount = 1, layerCount = 1},
		}
		vk.CreateImageView(device, &image_view_create_info, nil, &swapchain_image_views[i])
	}

}

choose_surface_extent :: proc(caps: vk.SurfaceCapabilitiesKHR) -> vk.Extent2D {
	if (caps.currentExtent.width != max(u32)) {
		return caps.currentExtent
	}

	width, height := glfw.GetFramebufferSize(window)
	return (vk.Extent2D {
				width = clamp(u32(width), caps.minImageExtent.width, caps.maxImageExtent.width),
				height = clamp(
					u32(height),
					caps.minImageExtent.height,
					caps.maxImageExtent.height,
				),
			})
}

create_pipeline :: proc() {
	rendering_create_info := vk.PipelineRenderingCreateInfo {
		sType                   = .PIPELINE_RENDERING_CREATE_INFO,
		colorAttachmentCount    = 1,
		pColorAttachmentFormats = &swapchain_image_format,
	}

	shader_module := create_shader_module(SHADER)

	layout: vk.PipelineLayout
	vk.CreatePipelineLayout(
		device,
		&vk.PipelineLayoutCreateInfo{sType = .PIPELINE_LAYOUT_CREATE_INFO},
		nil,
		&layout,
	)

	pipeline_create_info := vk.GraphicsPipelineCreateInfo {
		sType               = .GRAPHICS_PIPELINE_CREATE_INFO,
		pNext               = &rendering_create_info,
		stageCount          = 2,
		pStages             = raw_data(
			[]vk.PipelineShaderStageCreateInfo {
				{
					sType = .PIPELINE_SHADER_STAGE_CREATE_INFO,
					stage = {.VERTEX},
					module = shader_module,
					pName = "vertMain",
				},
				{
					sType = .PIPELINE_SHADER_STAGE_CREATE_INFO,
					stage = {.FRAGMENT},
					module = shader_module,
					pName = "fragMain",
				},
			},
		),
		pVertexInputState   = &vk.PipelineVertexInputStateCreateInfo {
			sType = .PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
		},
		pInputAssemblyState = &vk.PipelineInputAssemblyStateCreateInfo {
			sType = .PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
			topology = .TRIANGLE_LIST,
		},
		pViewportState      = &vk.PipelineViewportStateCreateInfo {
			sType = .PIPELINE_VIEWPORT_STATE_CREATE_INFO,
			viewportCount = 1,
			scissorCount = 1,
		},
		pRasterizationState = &vk.PipelineRasterizationStateCreateInfo {
			sType = .PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
			polygonMode = .FILL,
			lineWidth = 1.0,
		},
		pMultisampleState   = &vk.PipelineMultisampleStateCreateInfo {
			sType = .PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
			rasterizationSamples = {._1},
		},
		pColorBlendState    = &vk.PipelineColorBlendStateCreateInfo {
			sType = .PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
			attachmentCount = 1,
			pAttachments = &vk.PipelineColorBlendAttachmentState {
				colorWriteMask = {.R, .G, .B, .A},
			},
		},
		pDynamicState       = &vk.PipelineDynamicStateCreateInfo {
			sType = .PIPELINE_DYNAMIC_STATE_CREATE_INFO,
			dynamicStateCount = 2,
			pDynamicStates = raw_data([]vk.DynamicState{.VIEWPORT, .SCISSOR}),
		},
		layout              = layout,
	}
	vk.CreateGraphicsPipelines(device, 0, 1, &pipeline_create_info, nil, &pipeline)
}

create_shader_module :: proc(code: []byte) -> (module: vk.ShaderModule) {
	as_u32 := slice.reinterpret([]u32, code)

	create_info := vk.ShaderModuleCreateInfo {
		sType    = .SHADER_MODULE_CREATE_INFO,
		codeSize = len(code),
		pCode    = raw_data(as_u32),
	}
	vk.CreateShaderModule(device, &create_info, nil, &module)
	return
}

record_commands :: proc(image_index: u32) {
	command_buffer_begin_info := vk.CommandBufferBeginInfo {
		sType = .COMMAND_BUFFER_BEGIN_INFO,
	}
	vk.BeginCommandBuffer(command_buffer, &command_buffer_begin_info)

	vk.CmdBindPipeline(command_buffer, .GRAPHICS, pipeline)
	vk.CmdSetViewport(
		command_buffer,
		0,
		1,
		raw_data(
			[]vk.Viewport {
				{
					y = f32(swapchain_extent.height),
					width = f32(swapchain_extent.width),
					height = -f32(swapchain_extent.height),
					maxDepth = 1.0,
				},
			},
		),
	)
	vk.CmdSetScissor(
		command_buffer,
		0,
		1,
		raw_data([]vk.Rect2D{{offset = {0, 0}, extent = swapchain_extent}}),
	)

	image_memory_barrier := vk.ImageMemoryBarrier2 {
		sType = .IMAGE_MEMORY_BARRIER_2,
		srcStageMask = {.COLOR_ATTACHMENT_OUTPUT},
		dstStageMask = {.COLOR_ATTACHMENT_OUTPUT},
		dstAccessMask = {.COLOR_ATTACHMENT_WRITE},
		oldLayout = .UNDEFINED,
		newLayout = .ATTACHMENT_OPTIMAL_KHR,
		srcQueueFamilyIndex = vk.QUEUE_FAMILY_IGNORED,
		dstQueueFamilyIndex = vk.QUEUE_FAMILY_IGNORED,
		image = swapchain_images[image_index],
		subresourceRange = {aspectMask = {.COLOR}, levelCount = 1, layerCount = 1},
	}
	dependency_info := vk.DependencyInfo {
		sType                   = .DEPENDENCY_INFO,
		imageMemoryBarrierCount = 1,
		pImageMemoryBarriers    = &image_memory_barrier,
	}
	vk.CmdPipelineBarrier2(command_buffer, &dependency_info)

	clear_value := vk.ClearValue {
		color = {float32 = {0.0, 0.0, 0.0, 1.0}},
	}
	color_attachment_info := vk.RenderingAttachmentInfo {
		sType       = .RENDERING_ATTACHMENT_INFO,
		imageView   = swapchain_image_views[image_index],
		imageLayout = .ATTACHMENT_OPTIMAL_KHR,
		loadOp      = .CLEAR,
		clearValue  = clear_value,
	}
	rendering_info := vk.RenderingInfo {
		sType = .RENDERING_INFO,
		renderArea = {extent = swapchain_extent},
		layerCount = 1,
		colorAttachmentCount = 1,
		pColorAttachments = &color_attachment_info,
	}
	vk.CmdBeginRendering(command_buffer, &rendering_info)
	vk.CmdDraw(command_buffer, 3, 1, 0, 0)
	vk.CmdEndRendering(command_buffer)

	image_memory_barrier.oldLayout = .ATTACHMENT_OPTIMAL_KHR
	image_memory_barrier.newLayout = .PRESENT_SRC_KHR
	vk.CmdPipelineBarrier2(command_buffer, &dependency_info)

	vk.EndCommandBuffer(command_buffer)
}
