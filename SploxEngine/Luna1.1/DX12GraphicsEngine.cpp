#include "DX12GraphicsEngine.h"

DX12GraphicsEngine::DX12GraphicsEngine() :
	frame_index(0),
	first_update(TRUE) {

	start = clock.now();
	last_update = clock.now();
	camera_constant.world_view_proj = Identity4x4();
	camera_rb.orientation = { 0, 0, 0 };
	camera_rb.position = { 0, 400, -2 };
	camera_rb.velocity = { 0, -5, 0 };
	camera_rb.mass = 1200.0f;
	camera_rb.forces.clear();
	camera_rb.moment_of_inertia_coefficient = 2.0f / 5.0f; //sphere
	camera_rb.angular_velocity = { 0, 0, 0 };
	camera_rb.radius = 3.9624f;
	camera_rb.angluar_acceleration_strength = 300.0f;

	camera_rb.pitch_up = false;
	camera_rb.pitch_down = false;
	camera_rb.roll_left = false;
	camera_rb.roll_right = false;
	camera_rb.yaw_left = false;
	camera_rb.yaw_right = false;
	camera_rb.thurster_activation = false;
	camera_rb.thruster_force = 500.0f;
	camera_rb.fuel = 2000.0f;
	camera_rb.fuel_max = 2000.0f;
	camera_rb.fuel_rate = 100.0f;

	paused = false;

	MOON_SIZE = 5000.0f;
	MOON_COLOR = { 0.8f,0.8f,0.8f, 1.0f };
	CRATER_COLOR = { 0.2f, 0.2f, 0.2f, 1.0f };
	CABIN_COLOR = { 0.6f, 0.7f, 0.7f, 1.0f };
	FUEL_COLOR = { 1.0f, 0.0f, 0.0f, 1.0f};
	NUM_CRATERS = 500;
	XMVECTOR moon_gravity = XMVectorSet(0.0f, -1.62f, 0.0f, 0.0f);
	camera_rb.AddConstantForce(moon_gravity);
	
}
DX12GraphicsEngine::~DX12GraphicsEngine() { 
	Destroy(); 
}

void DX12GraphicsEngine::Init(){
	InitPipline();
	InitAssets();
}

void DX12GraphicsEngine::Update() {
	if (camera_rb.position.y < camera_rb.radius) { 
		paused = true;
		if (abs(camera_rb.velocity.y) < 5.0f) {
			SetWindowText(render_window, L"LANDED SAFELY: Vy < 5.0 m/s");
		}
		else if (abs(camera_rb.velocity.y) < 10.0f) {
			SetWindowText(render_window, L"LANDED HAZARDLESSLY: Vy < 10.0 m/s");
		}
		else {
			SetWindowText(render_window, L"LANDED FATALLY: Vy > 10.0 m/s");
		}
	}
	if (!paused) {

		if (first_update) {
			last_update = clock.now();
			first_update = false;
		}
		std::chrono::duration<double> time_span = std::chrono::duration_cast<std::chrono::duration<double>>(clock.now() - last_update);
		double time_since_last_update = time_span.count();

		camera_rb.Update(time_since_last_update);

		UpdateCameraMartrix();
		UpdateHud();
		last_update = clock.now();
	}
}

void DX12GraphicsEngine::Render() {
	// Record all the commands we need to render the scene into the command list.
	PopulateCommandList();

	// Execute the command list.
	ID3D12CommandList* ppCommandLists[] = { command_list.Get(), hud_command_list_lines.Get(), hud_command_list_triangles.Get() };
	command_queue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

	// Present the frame.
	ThrowIfFailed(swap_chain->Present(1, 0));

	MoveToNextFrame();
}

void DX12GraphicsEngine::Destroy() {
	// Ensure that the GPU is no longer referencing resources that are about to be
	// cleaned up by the destructor.
	FlushCommandQueue();

	CloseHandle(fence_event);
}

void DX12GraphicsEngine::UpdateClientDimensions() {
	RECT tmp_rect;
	GetClientRect(render_window, &tmp_rect);//store ClientRect info from render window into tmp_rect
	client_height = tmp_rect.bottom - tmp_rect.top; //calc & store client height from tmp_rect
	client_width = tmp_rect.right - tmp_rect.left; //calc & store client width from tmp_rect
	assert(client_height > 0); //error check client_height
	assert(client_width > 0); //error check client_width
}

void DX12GraphicsEngine::UpdateHud() {
	/* Create the HUD Vertex Buffer */
	//line stuff
	{
		Vertex line_vertices[] =
		{
			{ { -0.1f, 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f, 0.75f } },
			{ { 0.1f, 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f, 0.75f } },

			{ { 0.0f, -0.1f, 0.0f }, { 1.0f, 1.0f, 1.0f, 0.75f } },
			{ { 0.0f, 0.1f, 0.0f }, { 1.0f, 1.0f, 1.0f, 0.75f } },

			{ { -0.3f, -0.3f, 0.0f }, { 1.0f, 1.0f, 1.0f, 0.75f } },
			{ { 0.3f, 0.3f, 0.0f }, { 1.0f, 1.0f, 1.0f, 0.75f } },

			{ { -0.3f, 0.3f, 0.0f }, { 1.0f, 1.0f, 1.0f, 0.75f } },
			{ { 0.3f, -0.3f, 0.0f }, { 1.0f, 1.0f, 1.0f, 0.75f } },
		};

		num_hud_line_verts = _countof(line_vertices);

		const UINT vertex_buffer_size = sizeof(line_vertices);

		// Copy the triangle data to the vertex buffer.
		UINT8* vertex_data_begin;
		CD3DX12_RANGE read_range(0, 0);
		ThrowIfFailed(hud_vertex_buffer_lines->Map(0,
			&read_range,
			reinterpret_cast<void**>(&vertex_data_begin)));
		memcpy(vertex_data_begin, line_vertices, sizeof(line_vertices));
		hud_vertex_buffer_lines->Unmap(0, nullptr);

		//Init vertex buffer view
		hud_vertex_buffer_lines_view.BufferLocation = hud_vertex_buffer_lines->GetGPUVirtualAddress();
		hud_vertex_buffer_lines_view.StrideInBytes = sizeof(Vertex);
		hud_vertex_buffer_lines_view.SizeInBytes = vertex_buffer_size;
	}


	///////////////////////////////////////////////////////////////////////////////////
	//triangle stuff
	///////////////////////////////////////////////////////////////////////////////////

	{
		Vertex triangle_vertices[] =
		{
			//bot-left triangle
			{ { -1.0f, -1.0f, 0.0f }, CABIN_COLOR },
			{ { -1.0f, -0.75f, 0.0f }, CABIN_COLOR },
			{ { -0.75f, -1.0f, 0.0f }, CABIN_COLOR },
			//top-left triangle
			{ { -1.0f, 1.0f, 0.0f }, CABIN_COLOR },
			{ { -0.75f, 1.0f, 0.0f }, CABIN_COLOR },
			{ { -1.0f, 0.75f, 0.0f }, CABIN_COLOR },
			//top-right triangle
			{ { 1.0f, 1.0f, 0.0f }, CABIN_COLOR },
			{ { 1.0f, 0.75f, 0.0f }, CABIN_COLOR },
			{ { 0.75f, 1.0f, 0.0f }, CABIN_COLOR },
			//bot-right triangle
			{ { 1.0f, -1.0f, 0.0f }, CABIN_COLOR },
			{ { 0.75f, -1.0f, 0.0f }, CABIN_COLOR },
			{ { 1.0f, -0.75f, 0.0f }, CABIN_COLOR },

			//bot-right fuel indicator
			{ { 0.8f, 0.5f * camera_rb.fuel / camera_rb.fuel_max, 0.0f }, FUEL_COLOR },
			{ { 0.8f, -0.5f * camera_rb.fuel / camera_rb.fuel_max, 0.0f }, FUEL_COLOR },
			{ { 0.75f, 0.5f * camera_rb.fuel / camera_rb.fuel_max, 0.0f }, FUEL_COLOR },

			{ { 0.75f, 0.5f * camera_rb.fuel / camera_rb.fuel_max, 0.0f }, FUEL_COLOR },
			{ { 0.8f, -0.5f * camera_rb.fuel / camera_rb.fuel_max, 0.0f }, FUEL_COLOR },
			{ { 0.75f, -0.5f * camera_rb.fuel / camera_rb.fuel_max, 0.0f }, FUEL_COLOR }

		};

		const UINT vertex_buffer_size = sizeof(triangle_vertices);

		// Copy the triangle data to the vertex buffer.
		UINT8* vertex_data_begin;
		CD3DX12_RANGE read_range(0, 0);
		ThrowIfFailed(hud_vertex_buffer_triangles->Map(0,
			&read_range,
			reinterpret_cast<void**>(&vertex_data_begin)));
		memcpy(vertex_data_begin, triangle_vertices, sizeof(triangle_vertices));
		hud_vertex_buffer_triangles->Unmap(0, nullptr);

		//Init vertex buffer view
		hud_vertex_buffer_triangles_view.BufferLocation = hud_vertex_buffer_triangles->GetGPUVirtualAddress();
		hud_vertex_buffer_triangles_view.StrideInBytes = sizeof(Vertex);
		hud_vertex_buffer_triangles_view.SizeInBytes = vertex_buffer_size;
	}
	

}
void DX12GraphicsEngine::UpdateCameraMartrix() {
	// Build the view matrix.
	XMVECTOR pos = XMLoadFloat3(&camera_rb.position);
	XMVECTOR target = XMVectorAdd(pos, camera_rb.GetOrientation());
	XMVECTOR up = XMVector3Transform(XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f), camera_rb.GetOrientationMatrix());

	XMMATRIX view = XMMatrixLookAtLH(pos, target, up);
	XMMATRIX proj = XMMatrixPerspectiveFovLH(0.25f * XM_PI, client_width / client_height, 1.0f, 3000.0f);

	XMMATRIX world_view_proj = XMMatrixMultiply(view, proj);

	// Update the constant buffer with the latest worldViewProj matrix.
	XMStoreFloat4x4(&camera_constant.world_view_proj, XMMatrixTranspose(world_view_proj));

	memcpy(constant_buffer_data_begin, &camera_constant, sizeof(camera_constant));
}

void DX12GraphicsEngine::InitPipline() {
	/* Sample Desc (USED IN MULTIPLE AREAS AND MUST BE SAME)*/
	DXGI_SAMPLE_DESC SAMPLE_DESC = {};
	SAMPLE_DESC.Count = SAMPLE_COUNT;
	SAMPLE_DESC.Quality = 0;

	/* Enable Dubugging */
	UINT dxgi_factory_flags = 0; //by defulat do not build factory with debugging

	ComPtr<ID3D12Debug> debug_controller;
	ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&debug_controller)));
	debug_controller->EnableDebugLayer(); // I don't know what the "Debug Layer" is
	//debug_controller->SetEnableGPUBasedValidation(TRUE);
	
	//Replace defuat factory flag for additional debug layers.
	dxgi_factory_flags |= DXGI_CREATE_FACTORY_DEBUG;
	
	/* Create The Factory */
	ThrowIfFailed(CreateDXGIFactory2(
		dxgi_factory_flags, //factory flags from above
		IID_PPV_ARGS(&dxgi_factory)));

	/* Create The Device*/
	// No warp device fallback option/functionality atm
	ThrowIfFailed(D3D12CreateDevice(
		NULL, //use primary GPU on OS
		D3D_FEATURE_LEVEL_12_0, //minimum feature level
		IID_PPV_ARGS(&graphics_device)));

	/* Create The Command Queue */
	D3D12_COMMAND_QUEUE_DESC command_queue_desc = {};
	command_queue_desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	command_queue_desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

	ThrowIfFailed(graphics_device->CreateCommandQueue(
		&command_queue_desc,
		IID_PPV_ARGS(&command_queue)));

	/* Create The Swap Chain*/
	//
	DXGI_SWAP_CHAIN_DESC1 swap_chain_desc = {};
	swap_chain_desc.BufferCount = FRAME_COUNT; 
	swap_chain_desc.Width = client_width; //same as client area
	swap_chain_desc.Height = client_height; //same as client area
	swap_chain_desc.Format = BACK_BUFFER_FORMAT;
	swap_chain_desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swap_chain_desc.SampleDesc = SAMPLE_DESC;

	ComPtr<IDXGISwapChain1> tmp_swap_chain;
	ThrowIfFailed(dxgi_factory->CreateSwapChainForHwnd(
		command_queue.Get(),
		render_window,
		&swap_chain_desc,
		nullptr,
		nullptr,
		&tmp_swap_chain));

	ThrowIfFailed(dxgi_factory->MakeWindowAssociation( //I don't know what this is
		render_window, 
		DXGI_MWA_NO_ALT_ENTER));

	ThrowIfFailed(tmp_swap_chain.As(&swap_chain));
	//now that the swap_chain is created we can set the frame index to the current back buffer
	frame_index = swap_chain->GetCurrentBackBufferIndex();

	/* Create The Descriptor Heaps */
	{
		/* Create the Render Target View Heap */
		{
			D3D12_DESCRIPTOR_HEAP_DESC render_target_view_heap_desc = {};
			render_target_view_heap_desc.NumDescriptors = FRAME_COUNT;
			render_target_view_heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
			render_target_view_heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

			ThrowIfFailed(graphics_device->CreateDescriptorHeap(
				&render_target_view_heap_desc,
				IID_PPV_ARGS(&render_target_view_desc_heap)));

			render_target_view_desc_size = graphics_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		}

		/* Create the Constant Buffer View Heap */ 
		{
			D3D12_DESCRIPTOR_HEAP_DESC constant_buffer_view_desc = {};
			constant_buffer_view_desc.NumDescriptors = 1;
			constant_buffer_view_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
			constant_buffer_view_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

			ThrowIfFailed(graphics_device->CreateDescriptorHeap(
				&constant_buffer_view_desc,
				IID_PPV_ARGS(&cbv_srv_desc_heap)));

			cbv_srv_desc_size = graphics_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		}
	}

	/* Create the Frame Resources */
	{
		/*Create a handle to the render_target_view_desc_heap*/
		CD3DX12_CPU_DESCRIPTOR_HANDLE render_target_view_handle(render_target_view_desc_heap->GetCPUDescriptorHandleForHeapStart());

		// Create a RTV and a command allocator for each frame.
		for (UINT i = 0; i < FRAME_COUNT; ++i) //every frame
		{
			ThrowIfFailed(swap_chain->GetBuffer(i, IID_PPV_ARGS(&render_targets[i])));
			graphics_device->CreateRenderTargetView(render_targets[i].Get(), nullptr, render_target_view_handle);
			render_target_view_handle.Offset(1, render_target_view_desc_size);

			ThrowIfFailed(graphics_device->CreateCommandAllocator(
				D3D12_COMMAND_LIST_TYPE_DIRECT, 
				IID_PPV_ARGS(&command_allocators[i])));
		}
	}

}

void DX12GraphicsEngine::InitAssets() {
	UpdateClientDimensions();
	view_port = CD3DX12_VIEWPORT(
		0.0f,
		0.0f,
		static_cast<float>(client_width),
		static_cast<float>(client_height));
	scissor_rect = CD3DX12_RECT(
		0,
		0,
		static_cast<LONG>(client_width),
		static_cast<LONG>(client_height));

	/* Create The Root Signature (input parameters for PSO)*/ //NOT sure about alot here
	{
		D3D12_FEATURE_DATA_ROOT_SIGNATURE feature_data = {};

		// This is the highest version the sample supports. If CheckFeatureSupport succeeds, the HighestVersion returned will not be greater than this.
		feature_data.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;

		if (FAILED(graphics_device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &feature_data, sizeof(feature_data))))
		{
			feature_data.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
		}

		CD3DX12_DESCRIPTOR_RANGE1 ranges[1];
		ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);

		CD3DX12_ROOT_PARAMETER1 root_parameters[1];
		root_parameters[0].InitAsDescriptorTable(1, &ranges[0], D3D12_SHADER_VISIBILITY_VERTEX);

		// Allow input layout and deny uneccessary access to certain pipeline stages.
		D3D12_ROOT_SIGNATURE_FLAGS root_signature_flags =
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS;

		CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC root_signature_desc;
		root_signature_desc.Init_1_1(_countof(root_parameters), root_parameters, 0, nullptr, root_signature_flags);

		ComPtr<ID3DBlob> signature;
		ComPtr<ID3DBlob> error;
		ThrowIfFailed(D3DX12SerializeVersionedRootSignature(&root_signature_desc, feature_data.HighestVersion, &signature, &error));
		ThrowIfFailed(graphics_device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&root_signature)));

	}

	/* Complie + Load Shaders and create The PSO*/
	{
		/* Complie and Store Shaders*/
		ComPtr<ID3DBlob> vertex_shader_byte_code;
		ComPtr<ID3DBlob> pixel_shader_byte_code;

		//enable debugging
#if defined(_DEBUG)
		// Enable better shader debugging with the graphics debugging tools.
		UINT compile_flags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
		UINT compile_flags = 0;
#endif
		ThrowIfFailed(D3DCompileFromFile(
			L"camera.hlsl",
			nullptr,
			nullptr,
			"VSMain",
			"vs_5_0",
			compile_flags,
			0,
			&vertex_shader_byte_code,
			nullptr));

		ThrowIfFailed(D3DCompileFromFile(
			L"camera.hlsl",
			nullptr,
			nullptr,
			"PSMain",
			"ps_5_0",
			compile_flags,
			0,
			&pixel_shader_byte_code,
			nullptr));

		/* Define the Vertex Input Layout*/
		D3D12_INPUT_ELEMENT_DESC input_element_desc[] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
		};

		/* Create the Graphics Pipeline State Object PSO */
		D3D12_GRAPHICS_PIPELINE_STATE_DESC pso_desc = {};
		pso_desc.InputLayout = { input_element_desc , _countof(input_element_desc) };
		pso_desc.pRootSignature = root_signature.Get();
		pso_desc.VS = CD3DX12_SHADER_BYTECODE(vertex_shader_byte_code.Get());
		pso_desc.PS = CD3DX12_SHADER_BYTECODE(pixel_shader_byte_code.Get());
		pso_desc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		pso_desc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
		pso_desc.DepthStencilState.DepthEnable = FALSE;
		pso_desc.DepthStencilState.StencilEnable = FALSE;
		pso_desc.SampleMask = UINT_MAX;
		pso_desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		pso_desc.NumRenderTargets = 1;
		pso_desc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
		pso_desc.SampleDesc.Count = 1;
		
		ThrowIfFailed(graphics_device->CreateGraphicsPipelineState(
			&pso_desc,
			IID_PPV_ARGS(&pipeline_state)));

	}

	/* Init HUD PSO */

	 // Rebuild the root_signature to that of nothing
	{
		CD3DX12_ROOT_SIGNATURE_DESC root_signature_desc;
		root_signature_desc.Init(0, nullptr, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

		ComPtr<ID3DBlob> signature;
		ComPtr<ID3DBlob> error;
		ThrowIfFailed(D3D12SerializeRootSignature(&root_signature_desc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error));
		ThrowIfFailed(graphics_device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&root_signature_hud)));
	}

	/* Complie + Load Shaders and create The PSO*/ //HUD PSO
	{
		/* Complie and Store Shaders*/
		ComPtr<ID3DBlob> vertex_shader_byte_code;
		ComPtr<ID3DBlob> pixel_shader_byte_code;

		//enable debugging
#if defined(_DEBUG)
		// Enable better shader debugging with the graphics debugging tools.
		UINT compile_flags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
		UINT compile_flags = 0;
#endif
		ThrowIfFailed(D3DCompileFromFile(
			L"hud.hlsl",
			nullptr,
			nullptr,
			"VSMain",
			"vs_5_0",
			compile_flags,
			0,
			&vertex_shader_byte_code,
			nullptr));

		ThrowIfFailed(D3DCompileFromFile(
			L"hud.hlsl",
			nullptr,
			nullptr,
			"PSMain",
			"ps_5_0",
			compile_flags,
			0,
			&pixel_shader_byte_code,
			nullptr));

		/* Define the Vertex Input Layout*/
		D3D12_INPUT_ELEMENT_DESC input_element_desc[] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
		};

		/* Create the Graphics Pipeline State Object PSO */
		D3D12_GRAPHICS_PIPELINE_STATE_DESC pso_desc = {};
		pso_desc.InputLayout = { input_element_desc , _countof(input_element_desc) };
		pso_desc.pRootSignature = root_signature_hud.Get();
		pso_desc.VS = CD3DX12_SHADER_BYTECODE(vertex_shader_byte_code.Get());
		pso_desc.PS = CD3DX12_SHADER_BYTECODE(pixel_shader_byte_code.Get());
		pso_desc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		pso_desc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
		pso_desc.DepthStencilState.DepthEnable = FALSE;
		pso_desc.DepthStencilState.StencilEnable = FALSE;
		pso_desc.SampleMask = UINT_MAX;
		pso_desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		pso_desc.NumRenderTargets = 1;
		pso_desc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
		pso_desc.SampleDesc.Count = 1;

		ThrowIfFailed(graphics_device->CreateGraphicsPipelineState(
			&pso_desc,
			IID_PPV_ARGS(&pipeline_state_hud_triangles)));

		pso_desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;

		ThrowIfFailed(graphics_device->CreateGraphicsPipelineState(
			&pso_desc,
			IID_PPV_ARGS(&pipeline_state_hud_lines)));

	}


	/* Create the Command Lists */
	
	//main command list
	ThrowIfFailed(graphics_device->CreateCommandList(
		0,
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		command_allocators[frame_index].Get(),
		pipeline_state.Get(),
		IID_PPV_ARGS(&command_list)));
	ThrowIfFailed(command_list->Close());

	//hud command list
	ThrowIfFailed(graphics_device->CreateCommandList(
		0,
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		command_allocators[frame_index].Get(),
		pipeline_state_hud_lines.Get(),
		IID_PPV_ARGS(&hud_command_list_lines)));
	ThrowIfFailed(hud_command_list_lines->Close());

	ThrowIfFailed(graphics_device->CreateCommandList(
		0,
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		command_allocators[frame_index].Get(),
		pipeline_state_hud_lines.Get(),
		IID_PPV_ARGS(&hud_command_list_triangles)));
	ThrowIfFailed(hud_command_list_triangles->Close());


	/* Create the Vertex Buffer */
	{
		std::vector<Vertex> all_verts;

		std::vector<Vertex> triangle_vertices =
		{
			//moon ground
			Vertex( { MOON_SIZE, 0.0f, MOON_SIZE }, MOON_COLOR ),
			Vertex( { MOON_SIZE, 0.0f, -MOON_SIZE }, MOON_COLOR ),
			Vertex( { -MOON_SIZE, 0.0f, MOON_SIZE }, MOON_COLOR ),

			Vertex( { MOON_SIZE, 0.0f, -MOON_SIZE }, MOON_COLOR ),
			Vertex( { -MOON_SIZE, 0.0f, -MOON_SIZE }, MOON_COLOR ),
			Vertex( { -MOON_SIZE, 0.0f, MOON_SIZE }, MOON_COLOR )

		};

		all_verts.insert(all_verts.end(), triangle_vertices.begin(), triangle_vertices.end());
		std::vector<Vertex> crater_verts;
		for (int i = 0, end = NUM_CRATERS; i < end; ++i) {
			crater_verts.clear();
			crater_verts = GenerateCraterAtPos(10.0f + 5.0f * (rand() / static_cast<float>(RAND_MAX) - 0.5f), { (rand() / static_cast<float>(RAND_MAX) -0.5f) * 6000.0f , (rand() / static_cast<float>(RAND_MAX) - 0.5f) * 6000.0f }, CRATER_COLOR);
			all_verts.insert(all_verts.end(), crater_verts.begin(), crater_verts.end());
		}
	
		num_scene_triangle_verts = all_verts.size();

		const UINT vertex_buffer_size = all_verts.size() * sizeof(Vertex);

		ThrowIfFailed(graphics_device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(vertex_buffer_size),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&vertex_buffer)));

		// Copy the triangle data to the vertex buffer.
		UINT8* vertex_data_begin;
		CD3DX12_RANGE read_range(0, 0);
		ThrowIfFailed(vertex_buffer->Map(0,
			&read_range,
			reinterpret_cast<void**>(&vertex_data_begin)));
		memcpy(vertex_data_begin, all_verts.data(), vertex_buffer_size);
		vertex_buffer->Unmap(0, nullptr);

		//Init vertex buffer view
		vertex_buffer_view.BufferLocation = vertex_buffer->GetGPUVirtualAddress();
		vertex_buffer_view.StrideInBytes = sizeof(Vertex);
		vertex_buffer_view.SizeInBytes = vertex_buffer_size;
		
	};

	/* Create the HUD Vertex Buffer */
	//line stuff
	{
		Vertex line_vertices[] =
		{
			{ { -0.1f, 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f, 0.75f } },
			{ { 0.1f, 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f, 0.75f } },

			{ { 0.0f, -0.1f, 0.0f }, { 1.0f, 1.0f, 1.0f, 0.75f } },
			{ { 0.0f, 0.1f, 0.0f }, { 1.0f, 1.0f, 1.0f, 0.75f } },

			{ { -0.3f, -0.3f, 0.0f }, { 1.0f, 1.0f, 1.0f, 0.75f } },
			{ { 0.3f, 0.3f, 0.0f }, { 1.0f, 1.0f, 1.0f, 0.75f } },

			{ { -0.3f, 0.3f, 0.0f }, { 1.0f, 1.0f, 1.0f, 0.75f } },
			{ { 0.3f, -0.3f, 0.0f }, { 1.0f, 1.0f, 1.0f, 0.75f } },
		};

		num_hud_line_verts = _countof(line_vertices);

		const UINT vertex_buffer_size = sizeof(line_vertices);


		ThrowIfFailed(graphics_device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(vertex_buffer_size),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&hud_vertex_buffer_lines)));

		// Copy the triangle data to the vertex buffer.
		UINT8* vertex_data_begin;
		CD3DX12_RANGE read_range(0, 0);
		ThrowIfFailed(hud_vertex_buffer_lines->Map(0,
			&read_range,
			reinterpret_cast<void**>(&vertex_data_begin)));
		memcpy(vertex_data_begin, line_vertices, sizeof(line_vertices));
		hud_vertex_buffer_lines->Unmap(0, nullptr);

		//Init vertex buffer view
		hud_vertex_buffer_lines_view.BufferLocation = hud_vertex_buffer_lines->GetGPUVirtualAddress();
		hud_vertex_buffer_lines_view.StrideInBytes = sizeof(Vertex);
		hud_vertex_buffer_lines_view.SizeInBytes = vertex_buffer_size;
	}


	///////////////////////////////////////////////////////////////////////////////////
	//triangle stuff
	///////////////////////////////////////////////////////////////////////////////////

	{
		Vertex triangle_vertices[] =
		{
			//bot-left triangle
			{ { -1.0f, -1.0f, 0.0f }, { 0.75f, 0.75f, 0.75f, 1.0f } },
			{ { -1.0f, -0.75f, 0.0f }, { 0.75f, 0.75f, 0.75f, 1.0f } },
			{ { -0.75f, -1.0f, 0.0f }, { 0.75f, 0.75f, 0.75f, 1.0f } },
			//top-left triangle
			{ { -1.0f, 1.0f, 0.0f }, { 0.75f, 0.75f, 0.75f, 1.0f } },
			{ { -0.75f, 1.0f, 0.0f }, { 0.75f, 0.75f, 0.75f, 1.0f } },
			{ { -1.0f, 0.75f, 0.0f }, { 0.75f, 0.75f, 0.75f, 1.0f } },
			//top-right triangle
			{ { 1.0f, 1.0f, 0.0f }, { 0.75f, 0.75f, 0.75f, 1.0f } },
			{ { 1.0f, 0.75f, 0.0f }, { 0.75f, 0.75f, 0.75f, 1.0f } },
			{ { 0.75f, 1.0f, 0.0f }, { 0.75f, 0.75f, 0.75f, 1.0f } },
			//bot-right triangle
			{ { 1.0f, -1.0f, 0.0f }, { 0.75f, 0.75f, 0.75f, 1.0f } },
			{ { 0.75f, -1.0f, 0.0f }, { 0.75f, 0.75f, 0.75f, 1.0f } },
			{ { 1.0f, -0.75f, 0.0f }, { 0.75f, 0.75f, 0.75f, 1.0f } },

			//bot-right fuel indicator
			{ { 0.8f, 0.5f * camera_rb.fuel / camera_rb.fuel_max, 0.0f }, FUEL_COLOR },
			{ { 0.8f, -0.5f * camera_rb.fuel / camera_rb.fuel_max, 0.0f }, FUEL_COLOR },
			{ { 0.75f, 0.5f * camera_rb.fuel / camera_rb.fuel_max, 0.0f }, FUEL_COLOR },

			{ { 0.75f, 0.5f * camera_rb.fuel / camera_rb.fuel_max, 0.0f }, FUEL_COLOR },
			{ { 0.8f, -0.5f * camera_rb.fuel / camera_rb.fuel_max, 0.0f }, FUEL_COLOR },
			{ { 0.75f, -0.5f * camera_rb.fuel / camera_rb.fuel_max, 0.0f }, FUEL_COLOR }

		};

		num_hud_triangle_verts = _countof(triangle_vertices);

		const UINT vertex_buffer_size = sizeof(triangle_vertices);

		ThrowIfFailed(graphics_device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(vertex_buffer_size),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&hud_vertex_buffer_triangles)));

		// Copy the triangle data to the vertex buffer.
		UINT8* vertex_data_begin;
		CD3DX12_RANGE read_range(0, 0);
		ThrowIfFailed(hud_vertex_buffer_triangles->Map(0,
			&read_range,
			reinterpret_cast<void**>(&vertex_data_begin)));
		memcpy(vertex_data_begin, triangle_vertices, sizeof(triangle_vertices));
		hud_vertex_buffer_triangles->Unmap(0, nullptr);

		//Init vertex buffer view
		hud_vertex_buffer_triangles_view.BufferLocation = hud_vertex_buffer_triangles->GetGPUVirtualAddress();
		hud_vertex_buffer_triangles_view.StrideInBytes = sizeof(Vertex);
		hud_vertex_buffer_triangles_view.SizeInBytes = vertex_buffer_size;
	}

	/* Create the Constant Buffer*/
	{
		ThrowIfFailed(graphics_device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer((sizeof(CameraConstant) + 255) & ~255),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&constant_buffer)));

		// Describe and create a constant buffer view.
		D3D12_CONSTANT_BUFFER_VIEW_DESC cbv_desc = {};
		cbv_desc.BufferLocation = constant_buffer->GetGPUVirtualAddress();
		cbv_desc.SizeInBytes = (sizeof(CameraConstant) + 255) & ~255;    // CB size is required to be 256-byte aligned.
		graphics_device->CreateConstantBufferView(&cbv_desc, cbv_srv_desc_heap->GetCPUDescriptorHandleForHeapStart());

		// Map and initialize the constant buffer. We don't unmap this until the
		// app closes. Keeping things mapped for the lifetime of the resource is okay.
		CD3DX12_RANGE readRange(0, 0); // We do not intend to read from this resource on the CPU.
		ThrowIfFailed(constant_buffer->Map(
			0, 
			&readRange, 
			reinterpret_cast<void**>(&constant_buffer_data_begin)));

		memcpy(constant_buffer_data_begin, &camera_constant, sizeof(camera_constant));
	}

	/* Create Synchornization Objects and Wait untill assets have been uploaded to the GPU */
	{
		ThrowIfFailed(graphics_device->CreateFence(fence_values[frame_index], D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)));
		fence_values[frame_index]++;

		//Create an event handle to use for frame synchronization
		fence_event = CreateEvent(nullptr, FALSE, FALSE, nullptr);
		if (fence_event == nullptr) {
			ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
		}

		// Wait for the command list to execute; we are reusing the same command 
		// list in our main loop but for now, we just want to wait for setup to 
		// complete before continuing.
		FlushCommandQueue();
	}

}

// Prepare to render the next frame.
void DX12GraphicsEngine::MoveToNextFrame() {
	// Schedule a Signal command in the queue.
	const UINT64 currentFenceValue = fence_values[frame_index];
	ThrowIfFailed(command_queue->Signal(fence.Get(), currentFenceValue));

	// Update the frame index.
	frame_index = swap_chain->GetCurrentBackBufferIndex();

	// If the next frame is not ready to be rendered yet, wait until it is ready.
	if (fence->GetCompletedValue() < fence_values[frame_index])
	{
		ThrowIfFailed(fence->SetEventOnCompletion(fence_values[frame_index], fence_event));
		WaitForSingleObjectEx(fence_event, INFINITE, FALSE);
	}

	// Set the fence value for the next frame.
	fence_values[frame_index] = currentFenceValue + 1;
}

void DX12GraphicsEngine::FlushCommandQueue() {
	// Schedule a Signal command in the queue.
	ThrowIfFailed(command_queue->Signal(fence.Get(), fence_values[frame_index]));

	// Wait until the fence has been processed.
	ThrowIfFailed(fence->SetEventOnCompletion(fence_values[frame_index], fence_event));
	WaitForSingleObjectEx(fence_event, INFINITE, FALSE);

	// Increment the fence value for the current frame.
	fence_values[frame_index]++;
}


void DX12GraphicsEngine::PopulateCommandList() {
	// Command list allocators can only be reset when the associated 
	// command lists have finished execution on the GPU; apps should use 
	// fences to determine GPU execution progress.
	ThrowIfFailed(command_allocators[frame_index]->Reset());

	// However, when ExecuteCommandList() is called on a particular command 
	// list, that command list can then be reset at any time and must be before 
	// re-recording.

	ThrowIfFailed(command_list->Reset(command_allocators[frame_index].Get(), pipeline_state.Get()));

	// Set necessary state.
	command_list->SetGraphicsRootSignature(root_signature.Get());

	ID3D12DescriptorHeap* heap_pointers[] = { cbv_srv_desc_heap.Get() };
	command_list->SetDescriptorHeaps(_countof(heap_pointers), heap_pointers);
	command_list->SetGraphicsRootDescriptorTable(0, cbv_srv_desc_heap->GetGPUDescriptorHandleForHeapStart());
	command_list->RSSetViewports(1, &view_port);
	command_list->RSSetScissorRects(1, &scissor_rect);

	// Indicate that the back buffer will be used as a render target.
	command_list->ResourceBarrier(
		1, 
		&CD3DX12_RESOURCE_BARRIER::Transition(render_targets[frame_index].Get(), 
		D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(render_target_view_desc_heap->GetCPUDescriptorHandleForHeapStart(), frame_index, render_target_view_desc_size);
	command_list->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);

	// Record commands.
	const float clearColor[] = { 0.0f, 0.0f, 0.0f, 1.0f };
	command_list->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
	command_list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	command_list->IASetVertexBuffers(0, 1, &vertex_buffer_view);
	command_list->DrawInstanced(num_scene_triangle_verts, 1, 0, 0);

	// Indicate that the back buffer will now be used to present.
	command_list->ResourceBarrier(
		1, 
		&CD3DX12_RESOURCE_BARRIER::Transition(render_targets[frame_index].Get(), 
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));


	ThrowIfFailed(command_list->Close());

	//HUD LINES//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	ThrowIfFailed(hud_command_list_lines->Reset(command_allocators[frame_index].Get(), pipeline_state_hud_lines.Get()));
	hud_command_list_lines->SetGraphicsRootSignature(root_signature_hud.Get());
	hud_command_list_lines->RSSetViewports(1, &view_port);
	hud_command_list_lines->RSSetScissorRects(1, &scissor_rect);

	// Indicate that the back buffer will be used as a render target.
	hud_command_list_lines->ResourceBarrier(
		1,
		&CD3DX12_RESOURCE_BARRIER::Transition(render_targets[frame_index].Get(),
			D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

	hud_command_list_lines->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);
	hud_command_list_lines->IASetVertexBuffers(0, 1, &hud_vertex_buffer_lines_view);
	hud_command_list_lines->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINELIST);
	hud_command_list_lines->DrawInstanced(num_hud_line_verts, 1, 0, 0);

	// Indicate that the back buffer will now be used to present.
	hud_command_list_lines->ResourceBarrier(
		1,
		&CD3DX12_RESOURCE_BARRIER::Transition(render_targets[frame_index].Get(),
			D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

	ThrowIfFailed(hud_command_list_lines->Close());

	//TRIANGLE/////////////////////////////////////////////////////////////////////////////////////////////////////
	ThrowIfFailed(hud_command_list_triangles->Reset(command_allocators[frame_index].Get(), pipeline_state_hud_triangles.Get()));
	hud_command_list_triangles->SetGraphicsRootSignature(root_signature_hud.Get());
	hud_command_list_triangles->RSSetViewports(1, &view_port);
	hud_command_list_triangles->RSSetScissorRects(1, &scissor_rect);

	// Indicate that the back buffer will be used as a render target.
	hud_command_list_triangles->ResourceBarrier(
		1,
		&CD3DX12_RESOURCE_BARRIER::Transition(render_targets[frame_index].Get(),
			D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

	hud_command_list_triangles->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);
	hud_command_list_triangles->IASetVertexBuffers(0, 1, &hud_vertex_buffer_triangles_view);
	hud_command_list_triangles->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	hud_command_list_triangles->DrawInstanced(num_hud_triangle_verts, 1, 0, 0);

	// Indicate that the back buffer will now be used to present.
	hud_command_list_triangles->ResourceBarrier(
		1,
		&CD3DX12_RESOURCE_BARRIER::Transition(render_targets[frame_index].Get(),
			D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

	ThrowIfFailed(hud_command_list_triangles->Close());


	FlushCommandQueue();
}

std::vector<DX12GraphicsEngine::Vertex> DX12GraphicsEngine::GenerateCraterAtPos(float SIZE, XMFLOAT2 POS, XMFLOAT4 COLOR) {
	std::vector<Vertex> verts = 
	{
		Vertex({POS.x + SIZE, 0, POS.y + SIZE }, COLOR),
		Vertex({POS.x + SIZE, 0, POS.y - SIZE }, COLOR),
		Vertex({POS.x - SIZE, 0, POS.y - SIZE }, COLOR),

		Vertex({POS.x + SIZE, 0, POS.y + SIZE }, COLOR),
		Vertex({POS.x - SIZE, 0, POS.y - SIZE }, COLOR),
		Vertex({POS.x - SIZE, 0, POS.y + SIZE }, COLOR)
	};

	return verts;
}
