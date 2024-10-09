// TODO: Part 1b
#include "Models/FSLogo.h"
#include "shaderc/shaderc.h" // needed for compiling shaders at runtime
#ifdef _WIN32 // must use MT platform DLL libraries on windows
	#pragma comment(lib, "shaderc_combined.lib") 
#endif

void PrintLabeledDebugString(const char* label, const char* toPrint)
{
	std::cout << label << toPrint << std::endl;
#if defined WIN32 //OutputDebugStringA is a windows-only function 
	OutputDebugStringA(label);
	OutputDebugStringA(toPrint);
#endif
}

class Renderer
{
	// TODO: Part 2b
	struct shaderVars
	{
		GW::MATH::GMATRIXF viewMatrix, perspectiveMatrix;
		GW::MATH::GVECTORF lightColour, lightDir;
	};
	// TODO: Part 3a
	struct instanceData
	{
		GW::MATH::GMATRIXF worldMatrix;
		OBJ_ATTRIBUTES material;
	};

	// proxy handles
	GW::SYSTEM::GWindow win;
	GW::GRAPHICS::GVulkanSurface vlk;
	GW::CORE::GEventReceiver shutdown;
	
	// what we need at a minimum to draw a triangle
	VkDevice device = nullptr;
	VkPhysicalDevice physicalDevice = nullptr;
	VkRenderPass renderPass;
	VkBuffer vertexHandle = nullptr;
	VkDeviceMemory vertexData = nullptr;
	// TODO: Part 1g
	VkBuffer indexHandle = nullptr;
	VkDeviceMemory indexData = nullptr;
	// TODO: Part 2c
	VkShaderModule vertexShader = nullptr;
	VkShaderModule fragmentShader = nullptr;
	// pipeline settings for drawing (also required)
	VkPipeline pipeline = nullptr;
	VkPipelineLayout pipelineLayout = nullptr;
	// TODO: Part 2d
	std::vector<VkBuffer> uniformBufferHandle;
	std::vector<VkDeviceMemory> uniformBufferData;
	VkDescriptorSetLayout descriptorSetLayout = nullptr;
	VkDescriptorPool descriptorPool = nullptr;
	std::vector<VkDescriptorSet> descriptorSets = {};

	// TODO: Part 3d
	std::vector<VkBuffer> storageBufferHandle;
	std::vector<VkDeviceMemory> storageBufferData;

	unsigned int windowWidth, windowHeight;

	// TODO: Part 2a
	GW::MATH::GMatrix interfaceProxy;
	GW::MATH::GMATRIXF viewMatrix = GW::MATH::GIdentityMatrixF;
	GW::MATH::GMATRIXF leftHandedPerspectiveMatrix = GW::MATH::GIdentityMatrixF;
	GW::MATH::GVECTORF lightColour = { 0.9f, 0.9f, 1.0f, 1.0f };
	GW::MATH::GVECTORF lightDir = { (-1.0f/sqrt(6.0f)), (-1.0f / sqrt(6.0f)), (2.0f / sqrt(6.0f)) }; //original values were { -1.0f, -1.0f, 2.0f }

	// TODO: Part 2b // TODO: Part 4d
	shaderVars shader = {};
	
	// TODO: Part 3a
	std::vector<instanceData> instances;

public:

	Renderer(GW::SYSTEM::GWindow _win, GW::GRAPHICS::GVulkanSurface _vlk)
	{
		win = _win;
		vlk = _vlk;
		UpdateWindowDimensions();
		GetHandlesFromSurface();

		// TODO: Part 2a
		interfaceProxy.Create();
		// TODO: Part 2b // TODO: Part 4d
		shader.lightColour = lightColour;
		shader.lightDir = lightDir;

		// TODO: Part 2a
		initializeViewMatrix();
		initializePerspectiveMatrix();

		// TODO: part 3a

		for (int i = 0; i < FSLogo_meshcount; i++)
		{
			instanceData temp{};
			temp.worldMatrix = GW::MATH::GIdentityMatrixF;
			temp.material = FSLogo_materials[i].attrib;
			instances.push_back(temp);
		}

		createDescriptorLayout();
		InitializeGraphics();
		BindShutdownCallback();
	}

	void initializeViewMatrix()
	{
		GW::MATH::GVECTORF cameraPosition = { 0.75f, 0.25f, -1.5f };
		GW::MATH::GVECTORF targetPosition = { 0.15f, 0.75f, 0.0f };
		GW::MATH::GVECTORF upVector = { 0.0f, 1.0f, 0.0f };
		interfaceProxy.LookAtLHF(cameraPosition, targetPosition, upVector, viewMatrix);
		shader.viewMatrix = viewMatrix;
	}

	void initializePerspectiveMatrix()
	{
		float aspectRatio = 0.0f;
		vlk.GetAspectRatio(aspectRatio);
		interfaceProxy.ProjectionDirectXLHF(G_DEGREE_TO_RADIAN_F(65.0f), aspectRatio, 0.1, 100, leftHandedPerspectiveMatrix);
		shader.perspectiveMatrix = leftHandedPerspectiveMatrix;
	}

	void createDescriptorLayout()
	{
		VkDescriptorSetLayoutBinding uniformBinding = {};
		uniformBinding.binding = 0;
		uniformBinding.descriptorCount = 1;
		uniformBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		uniformBinding.pImmutableSamplers = nullptr;
		uniformBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

		VkDescriptorSetLayoutBinding storageBinding = {};
		storageBinding.binding = 1;
		storageBinding.descriptorCount = 1;
		storageBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		storageBinding.pImmutableSamplers = nullptr;
		storageBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

		std::array<VkDescriptorSetLayoutBinding, 2> bindings = { uniformBinding, storageBinding };

		VkDescriptorSetLayoutCreateInfo layoutInfo = {};
		layoutInfo.bindingCount = 2;
		layoutInfo.flags = 0;
		layoutInfo.pBindings = bindings.data();
		layoutInfo.pNext = nullptr;
		layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;

		vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorSetLayout);
	}

private:
	void UpdateWindowDimensions()
	{
		win.GetClientWidth(windowWidth);
		win.GetClientHeight(windowHeight);
	}

	void InitializeGraphics()
	{
		InitializeVertexBuffer();
		// TODO: Part 1g
		InitializeIndexBuffer();
		// TODO: Part 2d // TODO: Part 3d
		initializeUniformBuffer();
		initializeStorageBuffer();
		initializeDescriptorPool();
		initializeDescriptorSets();
		linkDescriptorSetUniformBuffer();
		CompileShaders();
		InitializeGraphicsPipeline();
	}

	void GetHandlesFromSurface()
	{
		vlk.GetDevice((void**)&device);
		vlk.GetPhysicalDevice((void**)&physicalDevice);
		vlk.GetRenderPass((void**)&renderPass);
	}

	void InitializeVertexBuffer()
	{
		// TODO: Part 1c
		float verts[] = 
		{
			0,   0.5f,
			0.5f, -0.5f,
			-0.5f, -0.5f
		};

		CreateVertexBuffer(&FSLogo_vertices[0], sizeof(FSLogo_vertices));
	}

	void CreateVertexBuffer(const void* data, unsigned int sizeInBytes)
	{
		GvkHelper::create_buffer(physicalDevice, device, sizeInBytes,
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
			VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &vertexHandle, &vertexData);
		// Transfer triangle data to the vertex buffer. (staging would be prefered here)
		GvkHelper::write_to_buffer(device, vertexData, data, sizeInBytes);
	}

	void InitializeIndexBuffer() //part of 1g
	{
		GvkHelper::create_buffer(physicalDevice, device, sizeof(FSLogo_indices),
			VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
			VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &indexHandle, &indexData);
		GvkHelper::write_to_buffer(device, indexData, &FSLogo_indices[0], sizeof(FSLogo_indices));
	}

	//part 2d
	void initializeUniformBuffer()
	{
		unsigned int bufferSize = sizeof(shaderVars);  //size of the uniform data

		//gets the number of active frames
		uint32_t imageCount;
		vlk.GetSwapchainImageCount(imageCount);

		//resizes the vectors for the uniform buffers for each frame
		uniformBufferHandle.resize(imageCount);
		uniformBufferData.resize(imageCount);

		for (size_t i = 0; i < imageCount; i++) //loops through each active frame and creates a buffer for each
		{
			GvkHelper::create_buffer(physicalDevice, device, bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &uniformBufferHandle[i], &uniformBufferData[i]);
			GvkHelper::write_to_buffer(device, uniformBufferData[i], &shader, bufferSize);
		}
	}

	//part 3d
	void initializeStorageBuffer()
	{
		unsigned int bufferSize = sizeof(instanceData) * instances.size();  //size of the storage data

		//gets the number of active frames
		uint32_t imageCount;
		vlk.GetSwapchainImageCount(imageCount);

		//resizes the vectors for the uniform buffers for each frame
		storageBufferHandle.resize(imageCount);
		storageBufferData.resize(imageCount);

		for (size_t i = 0; i < imageCount; i++) //loops through each active frame and creates a buffer for each
		{
			GvkHelper::create_buffer(physicalDevice, device, bufferSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &storageBufferHandle[i], &storageBufferData[i]);
			GvkHelper::write_to_buffer(device, storageBufferData[i], instances.data(), bufferSize);
		}
	}

	void initializeDescriptorPool()
	{
		VkDescriptorPoolSize uniformPoolSize = {};
		uniformPoolSize.descriptorCount = uniformBufferHandle.size();
		uniformPoolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;

		VkDescriptorPoolSize storagePoolSize = {};
		storagePoolSize.descriptorCount = storageBufferHandle.size();
		storagePoolSize.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;

		std::array<VkDescriptorPoolSize, 2> poolSizes = { uniformPoolSize, storagePoolSize };

		VkDescriptorPoolCreateInfo descriptorPoolInfo = {};
		descriptorPoolInfo.flags = 0;
		descriptorPoolInfo.maxSets = uniformBufferHandle.size();
		descriptorPoolInfo.pNext = nullptr;
		descriptorPoolInfo.poolSizeCount = 2;
		descriptorPoolInfo.pPoolSizes = poolSizes.data();
		descriptorPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;

		vkCreateDescriptorPool(device, &descriptorPoolInfo, nullptr, &descriptorPool);
	}

	void initializeDescriptorSets()
	{
		VkDescriptorSetAllocateInfo descriptorAllocateInfo = {};
		descriptorAllocateInfo.descriptorPool = descriptorPool;
		descriptorAllocateInfo.descriptorSetCount = 1;
		descriptorAllocateInfo.pNext = nullptr;
		descriptorAllocateInfo.pSetLayouts = &descriptorSetLayout;
		descriptorAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;

		descriptorSets.resize(uniformBufferHandle.size());

		for (int i = 0; i < uniformBufferData.size(); i++)
		{
			vkAllocateDescriptorSets(device, &descriptorAllocateInfo, &descriptorSets[i]);
		}
	}

	void linkDescriptorSetUniformBuffer()
	{
		for (int i = 0; i < uniformBufferData.size(); i++)
		{
			VkDescriptorBufferInfo uniformDescriptorBuffer = {};
			uniformDescriptorBuffer.buffer = uniformBufferHandle[i];
			uniformDescriptorBuffer.offset = 0;
			uniformDescriptorBuffer.range = sizeof(shaderVars);

			VkDescriptorBufferInfo storageDescriptorBuffer = {};
			storageDescriptorBuffer.buffer = storageBufferHandle[i];
			storageDescriptorBuffer.offset = 0;
			storageDescriptorBuffer.range = sizeof(instanceData) * instances.size();

			VkWriteDescriptorSet writeUniformDescriptor = {};
			writeUniformDescriptor.descriptorCount = 1;
			writeUniformDescriptor.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			writeUniformDescriptor.dstArrayElement = 0;
			writeUniformDescriptor.dstBinding = 0;
			writeUniformDescriptor.dstSet = descriptorSets[i];
			writeUniformDescriptor.pBufferInfo = &uniformDescriptorBuffer;
			writeUniformDescriptor.pImageInfo = nullptr;
			writeUniformDescriptor.pNext = nullptr;
			writeUniformDescriptor.pTexelBufferView = nullptr;
			writeUniformDescriptor.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;


			VkWriteDescriptorSet writeStorageDescriptor = {};
			writeStorageDescriptor.descriptorCount = 1;
			writeStorageDescriptor.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			writeStorageDescriptor.dstArrayElement = 0;
			writeStorageDescriptor.dstBinding = 1;
			writeStorageDescriptor.dstSet = descriptorSets[i];
			writeStorageDescriptor.pBufferInfo = &storageDescriptorBuffer;
			writeStorageDescriptor.pImageInfo = nullptr;
			writeStorageDescriptor.pNext = nullptr;
			writeStorageDescriptor.pTexelBufferView = nullptr;
			writeStorageDescriptor.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;

			std::array<VkWriteDescriptorSet, 2> writeDescriptors = { writeUniformDescriptor, writeStorageDescriptor };

			vkUpdateDescriptorSets(device, 2, writeDescriptors.data(), 0, nullptr);
		}
	}

	void CompileShaders()
	{
		// Intialize runtime shader compiler HLSL -> SPIRV
		shaderc_compiler_t compiler = shaderc_compiler_initialize();
		shaderc_compile_options_t options = CreateCompileOptions();

		CompileVertexShader(compiler, options);
		CompilePixelShader(compiler, options);

		// Free runtime shader compiler resources
		shaderc_compile_options_release(options);
		shaderc_compiler_release(compiler);
	}

	shaderc_compile_options_t CreateCompileOptions()
	{
		shaderc_compile_options_t retval = shaderc_compile_options_initialize();
		shaderc_compile_options_set_source_language(retval, shaderc_source_language_hlsl);
		shaderc_compile_options_set_invert_y(retval, true); // TODO: Part 2g
#ifndef NDEBUG
		shaderc_compile_options_set_generate_debug_info(retval);
#endif
		return retval;	
	}

	void CompileVertexShader(const shaderc_compiler_t& compiler, const shaderc_compile_options_t& options)
	{
		std::string vertexShaderSource = ReadFileIntoString("../VertexShader.hlsl");

		shaderc_compilation_result_t result = shaderc_compile_into_spv( // compile
			compiler, vertexShaderSource.c_str(), vertexShaderSource.length(),
			shaderc_vertex_shader, "main.vert", "main", options);
		
		if (shaderc_result_get_compilation_status(result) != shaderc_compilation_status_success) // errors?
		{
			PrintLabeledDebugString("Vertex Shader Errors:\n", shaderc_result_get_error_message(result));
			abort();
			return;
		}
		
		GvkHelper::create_shader_module(device, shaderc_result_get_length(result), // load into Vulkan
			(char*)shaderc_result_get_bytes(result), &vertexShader);
		
		shaderc_result_release(result); // done
	}
	
	void CompilePixelShader(const shaderc_compiler_t& compiler, const shaderc_compile_options_t& options)
	{
		std::string fragmentShaderSource = ReadFileIntoString("../FragmentShader.hlsl");
		
		shaderc_compilation_result_t result;
		
		result = shaderc_compile_into_spv( // compile
			compiler, fragmentShaderSource.c_str(), fragmentShaderSource.length(),
			shaderc_fragment_shader, "main.frag", "main", options);

		if (shaderc_result_get_compilation_status(result) != shaderc_compilation_status_success) // errors?
		{
			PrintLabeledDebugString("Fragment Shader Errors:\n", shaderc_result_get_error_message(result));
			abort();
			return;
		}
		
		GvkHelper::create_shader_module(device, shaderc_result_get_length(result), // load into Vulkan
			(char*)shaderc_result_get_bytes(result), &fragmentShader);
		
		shaderc_result_release(result); // done
	}

	void InitializeGraphicsPipeline()
	{
		// Create Pipeline & Layout (Thanks Tiny!)
		VkPipelineShaderStageCreateInfo stage_create_info[2] = {};
		// Create Stage Info for Vertex Shader
		stage_create_info[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		stage_create_info[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
		stage_create_info[0].module = vertexShader;
		stage_create_info[0].pName = "main";

		// Create Stage Info for Fragment Shader
		stage_create_info[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		stage_create_info[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		stage_create_info[1].module = fragmentShader;
		stage_create_info[1].pName = "main";

		VkPipelineInputAssemblyStateCreateInfo assembly_create_info = CreateVkPipelineInputAssemblyStateCreateInfo();
		VkVertexInputBindingDescription vertex_binding_description = CreateVkVertexInputBindingDescription();

		std::array<VkVertexInputAttributeDescription, 3> vertex_attribute_description;
		vertex_attribute_description[0].binding = 0;
		vertex_attribute_description[0].location = 0;
		vertex_attribute_description[0].format = VK_FORMAT_R32G32B32_SFLOAT;
		vertex_attribute_description[0].offset = 0;

		vertex_attribute_description[1].binding = 0;
		vertex_attribute_description[1].location = 1;
		vertex_attribute_description[1].format = VK_FORMAT_R32G32B32_SFLOAT;
		vertex_attribute_description[1].offset = sizeof(OBJ_VEC3);

		vertex_attribute_description[2].binding = 0;
		vertex_attribute_description[2].location = 2;
		vertex_attribute_description[2].format = VK_FORMAT_R32G32B32_SFLOAT;
		vertex_attribute_description[2].offset = sizeof(OBJ_VERT);

		VkPipelineVertexInputStateCreateInfo input_vertex_info = CreateVkPipelineVertexInputStateCreateInfo(&vertex_binding_description, 1, vertex_attribute_description.data(), 3);
		VkViewport viewport = CreateViewportFromWindowDimensions();
		VkRect2D scissor = CreateScissorFromWindowDimensions(); 
		VkPipelineViewportStateCreateInfo viewport_create_info = CreateVkPipelineViewportStateCreateInfo(&viewport, 1, &scissor, 1);
		VkPipelineRasterizationStateCreateInfo rasterization_create_info = CreateVkPipelineRasterizationStateCreateInfo();
		VkPipelineMultisampleStateCreateInfo multisample_create_info = CreateVkPipelineMultisampleStateCreateInfo();
		VkPipelineDepthStencilStateCreateInfo depth_stencil_create_info = CreateVkPipelineDepthStencilStateCreateInfo();
		VkPipelineColorBlendAttachmentState color_blend_attachment_state = CreateVkPipelineColorBlendAttachmentState();
		VkPipelineColorBlendStateCreateInfo color_blend_create_info = CreateVkPipelineColorBlendStateCreateInfo(&color_blend_attachment_state, 1);

		// Dynamic State 
		VkDynamicState dynamic_states[2] = 
		{
			// By setting these we do not need to re-create the pipeline on Resize
			VK_DYNAMIC_STATE_VIEWPORT, 
			VK_DYNAMIC_STATE_SCISSOR
		};
		VkPipelineDynamicStateCreateInfo dynamic_create_info = CreateVkPipelineDynamicStateCreateInfo(dynamic_states, 2);

		CreatePipelineLayout();

		// Pipeline State... (FINALLY) 
		VkGraphicsPipelineCreateInfo pipeline_create_info = {};
		pipeline_create_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipeline_create_info.stageCount = 2;
		pipeline_create_info.pStages = stage_create_info;
		pipeline_create_info.pInputAssemblyState = &assembly_create_info;
		pipeline_create_info.pVertexInputState = &input_vertex_info;
		pipeline_create_info.pViewportState = &viewport_create_info;
		pipeline_create_info.pRasterizationState = &rasterization_create_info;
		pipeline_create_info.pMultisampleState = &multisample_create_info;
		pipeline_create_info.pDepthStencilState = &depth_stencil_create_info;
		pipeline_create_info.pColorBlendState = &color_blend_create_info;
		pipeline_create_info.pDynamicState = &dynamic_create_info;
		pipeline_create_info.layout = pipelineLayout;
		pipeline_create_info.renderPass = renderPass;
		pipeline_create_info.subpass = 0;
		pipeline_create_info.basePipelineHandle = VK_NULL_HANDLE;

		vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipeline_create_info, nullptr, &pipeline);
	}

	VkPipelineInputAssemblyStateCreateInfo CreateVkPipelineInputAssemblyStateCreateInfo()
	{
		VkPipelineInputAssemblyStateCreateInfo retval = {};
		retval.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		retval.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		retval.primitiveRestartEnable = false;
		return retval;
	}

	VkVertexInputBindingDescription CreateVkVertexInputBindingDescription()
	{
		// TODO: Part 1e
		VkVertexInputBindingDescription retval = {};
		retval.binding = 0;
		retval.stride = sizeof(OBJ_VERT);
		retval.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
		return retval;
	}

	VkPipelineVertexInputStateCreateInfo CreateVkPipelineVertexInputStateCreateInfo(
		VkVertexInputBindingDescription* bindingDescriptions, uint32_t bindingCount,
		VkVertexInputAttributeDescription* attributeDescriptions, uint32_t attributeCount)
	{
		VkPipelineVertexInputStateCreateInfo retval = {};
		retval.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		retval.vertexBindingDescriptionCount = bindingCount;
		retval.pVertexBindingDescriptions = bindingDescriptions;
		retval.vertexAttributeDescriptionCount = attributeCount;
		retval.pVertexAttributeDescriptions = attributeDescriptions;
		return retval;
	}

	VkViewport CreateViewportFromWindowDimensions()
	{
		VkViewport retval = {};
		retval.x = 0;
		retval.y = 0;
		retval.width = static_cast<float>(windowWidth);
		retval.height = static_cast<float>(windowHeight);
		retval.minDepth = 0;
		retval.maxDepth = 1;
		return retval;
	}

	VkRect2D CreateScissorFromWindowDimensions()
	{
		VkRect2D retval = {};
		retval.offset.x = 0;
		retval.offset.y = 0;
		retval.extent.width = windowWidth;
		retval.extent.height = windowHeight;
		return retval;
	}

	VkPipelineViewportStateCreateInfo CreateVkPipelineViewportStateCreateInfo(VkViewport* viewports, uint32_t viewportCount, VkRect2D* scissors, uint32_t scissorCount)
	{
		VkPipelineViewportStateCreateInfo retval = {};
		retval.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		retval.viewportCount = viewportCount;
		retval.pViewports = viewports;
		retval.scissorCount = scissorCount;
		retval.pScissors = scissors;
		return retval;
	}

	VkPipelineRasterizationStateCreateInfo CreateVkPipelineRasterizationStateCreateInfo()
	{
		VkPipelineRasterizationStateCreateInfo retval = {};
		retval.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		retval.rasterizerDiscardEnable = VK_FALSE;
		retval.polygonMode = VK_POLYGON_MODE_FILL;
		retval.lineWidth = 1.0f;
		retval.cullMode = VK_CULL_MODE_BACK_BIT;
		retval.frontFace = VK_FRONT_FACE_CLOCKWISE;
		retval.depthClampEnable = VK_FALSE;
		retval.depthBiasEnable = VK_FALSE;
		retval.depthBiasClamp = 0.0f;
		retval.depthBiasConstantFactor = 0.0f;
		retval.depthBiasSlopeFactor = 0.0f;
		return retval;
	}

	VkPipelineMultisampleStateCreateInfo CreateVkPipelineMultisampleStateCreateInfo()
	{
		VkPipelineMultisampleStateCreateInfo retval = {};
		retval.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		retval.sampleShadingEnable = VK_FALSE;
		retval.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
		retval.minSampleShading = 1.0f;
		retval.pSampleMask = VK_NULL_HANDLE;
		retval.alphaToCoverageEnable = VK_FALSE;
		retval.alphaToOneEnable = VK_FALSE;
		return retval;
	}

	VkPipelineDepthStencilStateCreateInfo CreateVkPipelineDepthStencilStateCreateInfo()
	{
		VkPipelineDepthStencilStateCreateInfo retval = {};
		retval.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		retval.depthTestEnable = VK_TRUE;
		retval.depthWriteEnable = VK_TRUE;
		retval.depthCompareOp = VK_COMPARE_OP_LESS;
		retval.depthBoundsTestEnable = VK_FALSE;
		retval.minDepthBounds = 0.0f;
		retval.maxDepthBounds = 1.0f;
		retval.stencilTestEnable = VK_FALSE;
		return retval;
	}

	VkPipelineColorBlendAttachmentState CreateVkPipelineColorBlendAttachmentState()
	{
		VkPipelineColorBlendAttachmentState retval = {};
		retval.colorWriteMask = 0xF;
		retval.blendEnable = VK_FALSE;
		retval.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_COLOR;
		retval.dstColorBlendFactor = VK_BLEND_FACTOR_DST_COLOR;
		retval.colorBlendOp = VK_BLEND_OP_ADD;
		retval.srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		retval.dstAlphaBlendFactor = VK_BLEND_FACTOR_DST_ALPHA;
		retval.alphaBlendOp = VK_BLEND_OP_ADD;
		return retval;
	}

	VkPipelineColorBlendStateCreateInfo CreateVkPipelineColorBlendStateCreateInfo(VkPipelineColorBlendAttachmentState* attachmentStates, uint32_t attachmentCount)
	{
		VkPipelineColorBlendStateCreateInfo retval = {};
		retval.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		retval.logicOpEnable = VK_FALSE;
		retval.logicOp = VK_LOGIC_OP_COPY;
		retval.attachmentCount = attachmentCount;
		retval.pAttachments = attachmentStates;
		retval.blendConstants[0] = 0.0f;
		retval.blendConstants[1] = 0.0f;
		retval.blendConstants[2] = 0.0f;
		retval.blendConstants[3] = 0.0f;
		return retval;
	}

	VkPipelineDynamicStateCreateInfo CreateVkPipelineDynamicStateCreateInfo(VkDynamicState* dynamicStates, uint32_t dynamicStateCount)
	{
		VkPipelineDynamicStateCreateInfo retval = {};
		retval.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		retval.dynamicStateCount = dynamicStateCount;
		retval.pDynamicStates = dynamicStates;
		return retval;
	}

	void CreatePipelineLayout()
	{
		// Descriptor pipeline layout
		// TODO: Part 2d // TODO: Part 3d
		VkPipelineLayoutCreateInfo pipeline_layout_create_info = {};
		pipeline_layout_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipeline_layout_create_info.setLayoutCount = 1;
		pipeline_layout_create_info.pSetLayouts = &descriptorSetLayout;
		pipeline_layout_create_info.pushConstantRangeCount = 0;
		pipeline_layout_create_info.pPushConstantRanges = nullptr;

		vkCreatePipelineLayout(device, &pipeline_layout_create_info,nullptr, &pipelineLayout);
	}

	void BindShutdownCallback()
	{
		// GVulkanSurface will inform us when to release any allocated resources
		shutdown.Create(vlk, [&]() {
			if (+shutdown.Find(GW::GRAPHICS::GVulkanSurface::Events::RELEASE_RESOURCES, true)) {
				CleanUp(); // unlike D3D we must be careful about destroy timing
			}
			});
	}


public:
	void Render()
	{
		// TODO: Part 3i
		VkCommandBuffer commandBuffer = GetCurrentCommandBuffer();
		SetUpPipeline(commandBuffer);
		// TODO: Part 3i
		vkCmdBindIndexBuffer(commandBuffer, indexHandle, 0, VK_INDEX_TYPE_UINT32);

		// TODO: Part 2e
		uint32_t activeImage;
		vlk.GetSwapchainCurrentImage(activeImage);
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSets[activeImage], 0, 0);

		// TODO: Part 1h
		//vkCmdDrawIndexed(commandBuffer, FSLogo_indexcount, 1, 0, 0, 0);
		// TODO: Part 3f
		for (int i = 0; i < FSLogo_meshcount; i++)
		{
			vkCmdDrawIndexed(commandBuffer, FSLogo_meshes[i].indexCount, 1, 0, FSLogo_meshes[i].indexOffset, 0);
		}
		//vkCmdDraw(commandBuffer, FSLogo_vertexcount, 1, 0, 0); // TODO: Part 1d
	}

private:

	VkCommandBuffer GetCurrentCommandBuffer()
	{
		VkCommandBuffer retval;
		unsigned int currentBuffer;
		vlk.GetSwapchainCurrentImage(currentBuffer);
		vlk.GetCommandBuffer(currentBuffer, (void**)&retval);
		return retval;
	}

	void SetUpPipeline(VkCommandBuffer& commandBuffer)
	{
		UpdateWindowDimensions(); // what is the current client area dimensions?
		SetViewport(commandBuffer);
		SetScissor(commandBuffer);
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
		BindVertexBuffers(commandBuffer);
	}

	void SetViewport(const VkCommandBuffer& commandBuffer)
	{
		VkViewport viewport = CreateViewportFromWindowDimensions();
		vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
	}

	void SetScissor(const VkCommandBuffer& commandBuffer)
	{
		VkRect2D scissor = CreateScissorFromWindowDimensions();
		vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
	}

	void BindVertexBuffers(VkCommandBuffer& commandBuffer)
	{
		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexHandle, offsets);
	}


	void CleanUp()
	{
		// wait till everything has completed
		vkDeviceWaitIdle(device);
		// Release allocated buffers, shaders & pipeline
		// TODO: Part 1g
		vkDestroyBuffer(device, indexHandle, nullptr);
		vkFreeMemory(device, indexData, nullptr);

		// TODO: Part 2d
		for (int i = 0; i < uniformBufferHandle.size(); i++)
		{
			vkDestroyBuffer(device, uniformBufferHandle[i], nullptr);

			vkFreeMemory(device, uniformBufferData[i], nullptr);
		}
		uniformBufferHandle.clear();
		uniformBufferData.clear();

		vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
		vkDestroyDescriptorPool(device, descriptorPool, nullptr);

		// TODO: Part 3d
		for (int i = 0; i < storageBufferHandle.size(); i++) 
		{
			vkDestroyBuffer(device, storageBufferHandle[i], nullptr);
			vkFreeMemory(device, storageBufferData[i], nullptr);
		}
		storageBufferHandle.clear();
		storageBufferData.clear();

		vkDestroyBuffer(device, vertexHandle, nullptr);
		vkFreeMemory(device, vertexData, nullptr);
		vkDestroyShaderModule(device, vertexShader, nullptr);
		vkDestroyShaderModule(device, fragmentShader, nullptr);
		vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
		vkDestroyPipeline(device, pipeline, nullptr);
	}
};
