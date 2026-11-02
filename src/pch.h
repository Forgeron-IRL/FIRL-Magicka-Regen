#pragma once

// Force-included in every source of the target (see CMakeLists.txt, /FIpch.h).
// Purpose: make operator""sv visible AT GLOBAL SCOPE.
//
// The generated __<Project>Plugin.cpp uses "..."sv literals at global scope.
// CommonLibSSE-NG's PCH (SKSE/Impl/PCH.h) does `using namespace std::literals;`
// but INSIDE `namespace SKSE`, so the operator does not leak to global scope → C3688.
// We bring it back globally here.
//
// The full CommonLibSSE-NG PCH is included first: it sets up everything
// (stl namespace, fmt formatters...) that Relocation.h needs when the generated
// file includes it early.
#include <SKSE/Impl/PCH.h>

using namespace std::literals;
