#pragma once

/* Common */
#pragma region Common
#include <vector>
#include <cassert>
#include <stdexcept>
#include <chrono>
#include <string>
#pragma endregion

/* Core */
#pragma region Core
#include <windows.h>
#include <wrl.h> //for ComPtr smart pointers for DXGI and D3D12 "objects
#include "Debuger.h"
#pragma endregion

/* Graphics */
#pragma region Graphics
#include <d3d12.h> //D3D12 header
#include <dxgi1_4.h> //DXGI interfaces "fundemental D3D12 Objects"
#include <D3Dcompiler.h> //for runtime shader compilation
#include <DirectXMath.h>
#include "d3dx12.h"
#pragma endregion

/* Audio */
#pragma region Audio
#include <dsound.h>
#pragma endregion

/* Physics */
#pragma region Physics
#include "RigidBody.h"
#pragma endregion