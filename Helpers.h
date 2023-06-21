#pragma once

#include <Windows.h>
#include <exception>

inline void ThrowIfFailedI(HRESULT hr)
{
	if (FAILED(hr))
	{
		throw std::exception();
	}
}
