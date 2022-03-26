#pragma once

#pragma warning( disable : 4819 ) // The file contains a character that cannot be represented in the current code page
#pragma warning( disable : 4251 ) // needs to have dll-interface (declare dllexport for each instantiated template, painful)

#include "World/WorldMacros.h"
#include "Common/CommonMacros.h"

// headers from common
#include "Common/CommonTypes.h"
#include "Common/CommonUtils.h"
#include "Common/AssertUtils.h"
#include "Common/Math.h"
#include "Common/FreeList.h"
#include "Common/StringUtils.h"
#include "Common/Timer.h"

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
#include <filesystem>
#include <cstddef>