#pragma once

#include <QtGlobal>
namespace Platform {
    class ShortCut;
}

#ifdef Q_OS_MAC
#include "platform/mac/shortcut_mac.h"
#elif defined Q_OS_LINUX // Q_OS_MAC
#include "platform/linux/shortcut_linux.h"
#elif defined Q_OS_WIN // Q_OS_MAC || Q_OS_LINUX
#include "platform/win/shortcut_win.h"
#endif
