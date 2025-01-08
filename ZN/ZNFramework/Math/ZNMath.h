#pragma once

constexpr float XM_PI = 3.141592654f;
constexpr float ConvertDegreesToRadians(float fDegrees) noexcept { return fDegrees * (XM_PI / 180.0f); }
constexpr float ConvertRadiansToDegrees(float fRadians) noexcept { return fRadians * (180.0f / XM_PI); }
