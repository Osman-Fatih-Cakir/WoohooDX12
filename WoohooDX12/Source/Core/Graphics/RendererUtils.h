#pragma once

#include <d3d12.h>
#include <exception>

#define ReturnIfFailed(hr) { if (FAILED(hr)) return false; }
#define ThrowIfFailed(hr) { if (FAILED(hr)) throw new std::exception(); }