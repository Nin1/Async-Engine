#pragma once
#include <cstdio>

#define ASSERT(expr) { if (!(expr)) { __debugbreak(); } }
#define ASSERTM(expr, msg, ...) { if (!(expr)) { std::printf(msg, ##__VA_ARGS__); __debugbreak(); } }