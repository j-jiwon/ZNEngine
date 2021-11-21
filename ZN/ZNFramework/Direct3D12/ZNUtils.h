#pragma once

#include <windows.h>
#include <wrl.h>
#include <dxgi1_4.h>
#include "../Direct3D12/d3dx12.h"
#include <system_error>
#include <assert.h>
#include <D3Dcompiler.h>


inline void ThrowIfFailed(HRESULT hr)
{
    if (FAILED(hr))
    {
        auto msg = std::system_category().message(hr);
        throw std::exception(msg.c_str());
    }
}

#ifndef ReleaseCom
#define ReleaseCom(x) { if(x){ x->Release(); x = 0; } }
#endif
