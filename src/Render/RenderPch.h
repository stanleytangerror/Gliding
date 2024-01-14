#pragma once

#pragma warning( disable : 4819 ) // The file contains a character that cannot be represented in the current code page
#pragma warning( disable : 4251 ) // needs to have dll-interface (declare dllexport for each instantiated template, painful)
#pragma warning( disable : 4996 ) // This function or variable may be unsafe. Consider using _s instead.

#define NOMINMAX

#include "Common/CommonMacros.h"
#include "World/WorldMacros.h"
#include "D3D12Backend/D3D12BackendMacros.h"
#include "Render/RenderMacros.h"

// headers from common
#include "Common/CommonTypes.h"
#include "Common/CommonUtils.h"
#include "Common/AssertUtils.h"
#include "Common/Math.h"
#include "Common/FreeList.h"
#include "Common/StringUtils.h"
#include "Common/Timer.h"
#include "Common/Profile.h"
#include "Common/SuspendedRelease.h"

// std headers
#include <utility>
#include <vector>
#include <set>
#include <unordered_set>
#include <map>
#include <unordered_map>
#include <queue>
#include <array>
#include <string>
#include <sstream>
#include <fstream>
#include <functional>
#include <memory>
#include <thread>
#include <numeric>
#include <filesystem>

#define RENDER_EVENT(context, format) {}
