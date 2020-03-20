#ifndef SHORT_CUT_MAC_CPP
#define SHORT_CUT_MAC_CPP

#include "platform/mac/shortcut_mac.h"
#include <QMap>
#include <QHash>
#include <QtDebug>

typedef QPair<uint, uint> Identifier;
static QMap<quint32, EventHotKeyRef> keyRefs;
static QHash<Identifier, quint32> keyIDs;
static quint32 hotKeySerial = 0;
static bool qxt_mac_handler_installed = false;

// WId to return when error
#define WINDOW_NOT_FOUND (WId)(0)

WindowList qxt_getWindowsForPSN(ProcessSerialNumber *psn)
{
    static CGSConnection connection = _CGSDefaultConnection();

    WindowList wlist;
    if (!psn) return wlist;

    CGError err((CGError)noErr);

    // get onnection for given process psn
    CGSConnection procConnection;
    err = CGSGetConnectionIDForPSN(connection, psn, &procConnection);
    if (err != noErr) return wlist;

    /* get number of windows open by given process
       in Mac OS X an application may have multiple windows, which generally
       represent documents. It is also possible that there is no window even
       though there is an application, it can simply not have any documents open. */

    int windowCount(0);
    err = CGSGetOnScreenWindowCount(connection, procConnection, &windowCount);
    // if there are no windows open by this application, skip
    if (err != noErr || windowCount == 0) return wlist;

    // get list of windows
    int windowList[windowCount];
    int outCount(0);
    err = CGSGetOnScreenWindowList(connection, procConnection, windowCount, windowList, &outCount);

    if (err != noErr || outCount == 0) return wlist;

    for (int i=0; i<outCount; ++i)
    {
        wlist += windowList[i];
    }

    return wlist;
}


WindowList QxtWindowSystem::windows()
{
    WindowList wlist;
    ProcessSerialNumber psn = {0, kNoProcess};

    // iterate over list of processes
    OSErr err;
    while ((err = ::GetNextProcess(&psn)) == noErr)
    {
        wlist += qxt_getWindowsForPSN(&psn);
    }

    return wlist;
}

WId QxtWindowSystem::activeWindow()
{
    ProcessSerialNumber psn;
    OSErr err(noErr);
    err = ::GetFrontProcess(&psn);
    if (err != noErr) return WINDOW_NOT_FOUND;

    // in Mac OS X, first window for given PSN is always the active one
    WindowList wlist = qxt_getWindowsForPSN(&psn);

    if (wlist.count() > 0)
        return wlist.at(0);

    return WINDOW_NOT_FOUND;
}

QString QxtWindowSystem::windowTitle(WId window)
{
    CGSValue windowTitle;
    CGError err((CGError)noErr);
    static CGSConnection connection = _CGSDefaultConnection();

    // This code is so dirty I had to wash my hands after writing it.

    // most of CoreGraphics private definitions ask for CGSValue as key but since
    // converting strings to/from CGSValue was dropped in 10.5, I use CFString, which
    // apparently also works.

    // FIXME: Not public API function. Can't compile with OS X 10.8
    // err = CGSGetWindowProperty(connection, window, (CGSValue)CFSTR("kCGSWindowTitle"), &windowTitle);
    if (err != noErr) return QString();

    // this is UTF8 encoded
    return QCFString::toQString((CFStringRef)windowTitle);
}

QRect QxtWindowSystem::windowGeometry(WId window)
{
    CGRect rect;
    static CGSConnection connection = _CGSDefaultConnection();

    CGError err = CGSGetWindowBounds(connection, window, &rect);
    if (err != noErr) return QRect();

    return QRect(rect.origin.x, rect.origin.y, rect.size.width, rect.size.height);
}

/* This method is the only one that is not a complete hack
   from Quartz Event Services
   http://developer.apple.com/library/mac/#documentation/Carbon/Reference/QuartzEventServicesRef/Reference/reference.html
*/
uint QxtWindowSystem::idleTime()
{
    // CGEventSourceSecondsSinceLastEventType returns time in seconds as a double
    // also has extremely long name
    double idle = 1000 * ::CGEventSourceSecondsSinceLastEventType(kCGEventSourceStateCombinedSessionState, kCGAnyInputEventType);
    return (uint)idle;
}


// these are copied from X11 implementation
WId QxtWindowSystem::findWindow(const QString& title)
{
    WId result = 0;
    WindowList list = windows();
    foreach(const WId &wid, list)
    {
        if (windowTitle(wid) == title)
        {
            result = wid;
            break;
        }
    }
    return result;
}

WId QxtWindowSystem::windowAt(const QPoint& pos)
{
    WId result = 0;
    WindowList list = windows();
    for (int i = list.size() - 1; i >= 0; --i)
    {
        WId wid = list.at(i);
        if (windowGeometry(wid).contains(pos))
        {
            result = wid;
            break;
        }
    }
    return result;
}

OSStatus qxt_mac_handle_hot_key(EventHandlerCallRef nextHandler, EventRef event, void* data)
{
    Q_UNUSED(nextHandler);
    Q_UNUSED(data);
    if (GetEventClass(event) == kEventClassKeyboard && GetEventKind(event) == kEventHotKeyPressed)
    {
        EventHotKeyID keyID;
        GetEventParameter(event, kEventParamDirectObject, typeEventHotKeyID, NULL, sizeof(keyID), NULL, &keyID);
        Identifier id = keyIDs.key(keyID.id);
        QxtGlobalShortcutPrivate::activateShortcut(id.second, id.first);
    }
    return noErr;
}

quint32 QxtGlobalShortcutPrivate::nativeModifiers(Qt::KeyboardModifiers modifiers)
{
    quint32 native = 0;
    if (modifiers & Qt::ShiftModifier)
        native |= shiftKey;
    if (modifiers & Qt::ControlModifier)
        native |= cmdKey;
    if (modifiers & Qt::AltModifier)
        native |= optionKey;
    if (modifiers & Qt::MetaModifier)
        native |= controlKey;
    if (modifiers & Qt::KeypadModifier)
        native |= kEventKeyModifierNumLockMask;
    return native;
}

quint32 QxtGlobalShortcutPrivate::nativeKeycode(Qt::Key key)
{
    UTF16Char ch;
    // Constants found in NSEvent.h from AppKit.framework
    switch (key)
    {
    case Qt::Key_Return:
        return kVK_Return;
    case Qt::Key_Enter:
        return kVK_ANSI_KeypadEnter;
    case Qt::Key_Tab:
        return kVK_Tab;
    case Qt::Key_Space:
        return kVK_Space;
    case Qt::Key_Backspace:
        return kVK_Delete;
    case Qt::Key_Control:
        return kVK_Command;
    case Qt::Key_Shift:
        return kVK_Shift;
    case Qt::Key_CapsLock:
        return kVK_CapsLock;
    case Qt::Key_Option:
        return kVK_Option;
    case Qt::Key_Meta:
        return kVK_Control;
    case Qt::Key_F17:
        return kVK_F17;
    case Qt::Key_VolumeUp:
        return kVK_VolumeUp;
    case Qt::Key_VolumeDown:
        return kVK_VolumeDown;
    case Qt::Key_F18:
        return kVK_F18;
    case Qt::Key_F19:
        return kVK_F19;
    case Qt::Key_F20:
        return kVK_F20;
    case Qt::Key_F5:
        return kVK_F5;
    case Qt::Key_F6:
        return kVK_F6;
    case Qt::Key_F7:
        return kVK_F7;
    case Qt::Key_F3:
        return kVK_F3;
    case Qt::Key_F8:
        return kVK_F8;
    case Qt::Key_F9:
        return kVK_F9;
    case Qt::Key_F11:
        return kVK_F11;
    case Qt::Key_F13:
        return kVK_F13;
    case Qt::Key_F16:
        return kVK_F16;
    case Qt::Key_F14:
        return kVK_F14;
    case Qt::Key_F10:
        return kVK_F10;
    case Qt::Key_F12:
        return kVK_F12;
    case Qt::Key_F15:
        return kVK_F15;
    case Qt::Key_Help:
        return kVK_Help;
    case Qt::Key_Home:
        return kVK_Home;
    case Qt::Key_PageUp:
        return kVK_PageUp;
    case Qt::Key_Delete:
        return kVK_ForwardDelete;
    case Qt::Key_F4:
        return kVK_F4;
    case Qt::Key_End:
        return kVK_End;
    case Qt::Key_F2:
        return kVK_F2;
    case Qt::Key_PageDown:
        return kVK_PageDown;
    case Qt::Key_F1:
        return kVK_F1;
    case Qt::Key_Left:
        return kVK_LeftArrow;
    case Qt::Key_Right:
        return kVK_RightArrow;
    case Qt::Key_Down:
        return kVK_DownArrow;
    case Qt::Key_Up:
        return kVK_UpArrow;
    default:
        ;
    }

    if (key == Qt::Key_Escape)	ch = 27;
    else if (key == Qt::Key_Return) ch = 13;
    else if (key == Qt::Key_Enter) ch = 3;
    else if (key == Qt::Key_Tab) ch = 9;
    else ch = key;

    CFDataRef currentLayoutData;
    TISInputSourceRef currentKeyboard = TISCopyCurrentKeyboardInputSource();

    if (currentKeyboard == NULL)
        return 0;

    currentLayoutData = (CFDataRef)TISGetInputSourceProperty(currentKeyboard, kTISPropertyUnicodeKeyLayoutData);
    CFRelease(currentKeyboard);
    if (currentLayoutData == NULL)
        return 0;

    UCKeyboardLayout* header = (UCKeyboardLayout*)CFDataGetBytePtr(currentLayoutData);
    UCKeyboardTypeHeader* table = header->keyboardTypeList;

    uint8_t *data = (uint8_t*)header;
    // God, would a little documentation for this shit kill you...
    for (quint32 i=0; i < header->keyboardTypeCount; i++)
    {
        UCKeyStateRecordsIndex* stateRec = 0;
        if (table[i].keyStateRecordsIndexOffset != 0)
        {
            stateRec = reinterpret_cast<UCKeyStateRecordsIndex*>(data + table[i].keyStateRecordsIndexOffset);
            if (stateRec->keyStateRecordsIndexFormat != kUCKeyStateRecordsIndexFormat) stateRec = 0;
        }

        UCKeyToCharTableIndex* charTable = reinterpret_cast<UCKeyToCharTableIndex*>(data + table[i].keyToCharTableIndexOffset);
        if (charTable->keyToCharTableIndexFormat != kUCKeyToCharTableIndexFormat) continue;

        for (quint32 j=0; j < charTable->keyToCharTableCount; j++)
        {
            UCKeyOutput* keyToChar = reinterpret_cast<UCKeyOutput*>(data + charTable->keyToCharTableOffsets[j]);
            for (quint32 k=0; k < charTable->keyToCharTableSize; k++)
            {
                if (keyToChar[k] & kUCKeyOutputTestForIndexMask)
                {
                    long idx = keyToChar[k] & kUCKeyOutputGetIndexMask;
                    if (stateRec && idx < stateRec->keyStateRecordCount)
                    {
                        UCKeyStateRecord* rec = reinterpret_cast<UCKeyStateRecord*>(data + stateRec->keyStateRecordOffsets[idx]);
                        if (rec->stateZeroCharData == ch) return k;
                    }
                }
                else if (!(keyToChar[k] & kUCKeyOutputSequenceIndexMask) && keyToChar[k] < 0xFFFE)
                {
                    if (keyToChar[k] == ch) return k;
                }
            } // for k
        } // for j
    } // for i
    return 0;
}

bool QxtGlobalShortcutPrivate::registerShortcut(quint32 nativeKey, quint32 nativeMods)
{
    if (!qxt_mac_handler_installed)
    {
        EventTypeSpec t;
        t.eventClass = kEventClassKeyboard;
        t.eventKind = kEventHotKeyPressed;
        InstallApplicationEventHandler(&qxt_mac_handle_hot_key, 1, &t, NULL, NULL);
    }

    EventHotKeyID keyID;
    keyID.signature = 'cute';
    keyID.id = ++hotKeySerial;

    EventHotKeyRef ref = 0;
    bool rv = !RegisterEventHotKey(nativeKey, nativeMods, keyID, GetApplicationEventTarget(), 0, &ref);
    if (rv)
    {
        keyIDs.insert(Identifier(nativeMods, nativeKey), keyID.id);
        keyRefs.insert(keyID.id, ref);
    }
    return rv;
}

bool QxtGlobalShortcutPrivate::unregisterShortcut(quint32 nativeKey, quint32 nativeMods)
{
    Identifier id(nativeMods, nativeKey);
    if (!keyIDs.contains(id)) return false;

    EventHotKeyRef ref = keyRefs.take(keyIDs[id]);
    keyIDs.remove(id);
    return !UnregisterEventHotKey(ref);
}


#endif
