#pragma once
#include "precomp.h"
#include "Structs.hpp"

namespace lv {
    void createAppContext(AppContextInfo& info, AppContext& ctx);
    void destroyAppContext(AppContext& ctx);
}

