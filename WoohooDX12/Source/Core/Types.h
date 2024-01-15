#pragma once

#include <DirectXMath.h>
#include <string>

typedef unsigned int uint32;
typedef unsigned short uint16;
typedef unsigned long long uint64;
typedef unsigned char uint8;

typedef std::wstring WString;
typedef std::string String;
typedef DirectX::XMMATRIX Mat; // 4x4 matrix aligned on a 16-byte boundary
typedef DirectX::XMVECTOR Vec; // 4 components, 32 bit floating point
typedef DirectX::XMFLOAT4X4 Mat4x4;
typedef DirectX::XMFLOAT3X3 Mat3x3;
typedef DirectX::XMFLOAT4 Vec4;
typedef DirectX::XMFLOAT3 Vec3;
typedef DirectX::XMFLOAT2 Vec2;
