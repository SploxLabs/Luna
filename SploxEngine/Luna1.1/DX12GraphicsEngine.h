#pragma once
#include "pch.h"

using namespace Microsoft::WRL;
using namespace DirectX;

class DX12GraphicsEngine {
public:
	DX12GraphicsEngine();
	~DX12GraphicsEngine();
	HWND render_window; //render window handle
	//core functions
	void Init(); //init DX12
	void Update();
	void Render(); //draw and present to render window
	void Destroy();
	RigidBody camera_rb;
	bool paused;
private:
	//"settings"
	static const UINT num_frame_resources;
	static const UINT REFRESH_RATE = 144; //hz
	static const UINT FRAME_COUNT = 2;
	static const UINT SAMPLE_COUNT = 1;
	static const UINT SAMPLE_QUALITY = 0;
	static const DXGI_FORMAT BACK_BUFFER_FORMAT = DXGI_FORMAT_R8G8B8A8_UNORM;
	static const DXGI_FORMAT DEPTH_STENCIL_BUFFER_FORMAT = DXGI_FORMAT_D24_UNORM_S8_UINT;

	//static const UINT TEXTURE_WIDTH = 256;
	//static const UINT TEXTURE_HEIGHT = 256;
	//static const UINT TEXTURE_PIXEL_SIZE = 4;    // The number of bytes used to represent a pixel in the texture.

	float MOON_SIZE;
	XMFLOAT4 MOON_COLOR;
	XMFLOAT4 CRATER_COLOR;
	XMFLOAT4 CABIN_COLOR;
	XMFLOAT4 FUEL_COLOR;
	UINT NUM_CRATERS;
	//windows
	int client_height;//pixel height of client area
	int client_width;//pixel width of client area
	void UpdateClientDimensions(); //updates client_height & client_width from render window
	
	CD3DX12_VIEWPORT view_port;
	CD3DX12_RECT scissor_rect;

	//Scene Objects
	struct Vertex {
		Vertex(XMFLOAT3 POS, XMFLOAT4 COLOR) {
			position = POS;
			color = COLOR;
		}
		XMFLOAT3 position;
		XMFLOAT4 color;
	};

	struct SceneConstantBuffer {
		XMFLOAT4 offset;
	};

	struct CameraConstant {
		XMFLOAT4X4 world_view_proj;
	};

	//Pipeline Objects
	DXGI_SAMPLE_DESC SAMPLE_DESC; //use this for consistency, SwapChain and PSO use this and must match

	ComPtr<IDXGIFactory4> dxgi_factory;
	ComPtr<ID3D12Device> graphics_device;
	ComPtr<ID3D12CommandQueue> command_queue;
	ComPtr<ID3D12CommandAllocator> command_allocators[FRAME_COUNT];
	ComPtr<ID3D12GraphicsCommandList> command_list;
	ComPtr<ID3D12GraphicsCommandList> hud_command_list_lines;
	ComPtr<ID3D12GraphicsCommandList> hud_command_list_triangles;

	ComPtr<IDXGISwapChain3> swap_chain;
	ComPtr<ID3D12Resource> render_targets[FRAME_COUNT];

	ComPtr<ID3D12DescriptorHeap> render_target_view_desc_heap;
	UINT render_target_view_desc_size;
	ComPtr<ID3D12DescriptorHeap> cbv_srv_desc_heap;
	UINT cbv_srv_desc_size;
	
	ComPtr<ID3D12RootSignature> root_signature;
	ComPtr<ID3D12RootSignature> root_signature_hud;
	ComPtr<ID3D12PipelineState> pipeline_state;
	ComPtr<ID3D12PipelineState> pipeline_state_hud_lines;
	ComPtr<ID3D12PipelineState> pipeline_state_hud_triangles;

	//Synchornization Objects
	UINT frame_index;
	HANDLE fence_event;
	ComPtr<ID3D12Fence> fence;
	UINT64 fence_values[FRAME_COUNT];

	//Assets
	ComPtr<ID3D12Resource> vertex_buffer;
	ComPtr<ID3D12Resource> hud_vertex_buffer_lines;
	ComPtr<ID3D12Resource> hud_vertex_buffer_triangles;
	D3D12_VERTEX_BUFFER_VIEW vertex_buffer_view;
	D3D12_VERTEX_BUFFER_VIEW hud_vertex_buffer_lines_view;
	D3D12_VERTEX_BUFFER_VIEW hud_vertex_buffer_triangles_view;
	UINT num_scene_triangle_verts;
	UINT num_hud_line_verts;
	UINT num_hud_triangle_verts;

	void UpdateHud();
	void UpdateCameraMartrix();

	ComPtr<ID3D12Resource> constant_buffer;
	CameraConstant camera_constant;
	UINT8* constant_buffer_data_begin;

	void InitPipline();
	void InitAssets();
	void MoveToNextFrame();
	void FlushCommandQueue();
	void PopulateCommandList();

	std::vector<DX12GraphicsEngine::Vertex> GenerateCraterAtPos(float SIZE, XMFLOAT2 POS, XMFLOAT4 COLOR);

	std::chrono::high_resolution_clock clock;
	std::chrono::high_resolution_clock::time_point start;
	std::chrono::high_resolution_clock::time_point last_update;

	bool first_update;

};

inline XMFLOAT4X4 Identity4x4() {
	static DirectX::XMFLOAT4X4 I(
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f);

	return I;
}