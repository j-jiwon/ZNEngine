#pragma once

#include <dxgi1_6.h>
#include <D3Dcompiler.h>
#include "d3dx12.h"
#include <string>
#include <wrl.h>
#include <system_error>

inline void ThrowIfFailed(HRESULT hr)
{
    if (FAILED(hr))
    {
        auto msg = std::system_category().message(hr);
        throw std::exception(msg.c_str());
    } 
}
