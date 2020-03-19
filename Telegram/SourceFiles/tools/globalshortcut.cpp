#include "globalshortcut.h"

#include <QAbstractEventDispatcher>
#include <QKeySequence>
#include <QHash>
#include <QDebug>

#ifndef Q_OS_MAC
int QxtGlobalShortcutPrivate::ref = 0;
#   if QT_VERSION < QT_VERSION_CHECK(5,0,0)
QAbstractEventDispatcher::EventFilter QxtGlobalShortcutPrivate::prevEventFilter = 0;
#   endif
#endif // Q_OS_MAC
QHash<QPair<quint32, quint32>, QxtGlobalShortcut*> QxtGlobalShortcutPrivate::shortcuts;

QxtGlobalShortcutPrivate::QxtGlobalShortcutPrivate() : enabled(true), key(Qt::Key(0)), mods(Qt::NoModifier)
{
#ifndef Q_OS_MAC
    if (ref == 0) {
#   if QT_VERSION < QT_VERSION_CHECK(5,0,0)
        prevEventFilter = QAbstractEventDispatcher::instance()->setEventFilter(eventFilter);
#   else
        QAbstractEventDispatcher::instance()->installNativeEventFilter(this);
#endif
    }
    ++ref;
#endif // Q_OS_MAC
}

QxtGlobalShortcutPrivate::~QxtGlobalShortcutPrivate()
{
#ifndef Q_OS_MAC
    --ref;
    if (ref == 0) {
        QAbstractEventDispatcher *ed = QAbstractEventDispatcher::instance();
        if (ed != nullptr) {
#   if QT_VERSION < QT_VERSION_CHECK(5,0,0)
            ed->setEventFilter(prevEventFilter);
#   else
            ed->removeNativeEventFilter(this);
#   endif
        }
    }
#endif // Q_OS_MAC
}

bool QxtGlobalShortcutPrivate::setShortcut(const QKeySequence& shortcut)
{
    Qt::KeyboardModifiers allMods = Qt::ShiftModifier | Qt::ControlModifier | Qt::AltModifier | Qt::MetaModifier;
    key = shortcut.isEmpty() ? Qt::Key(0) : Qt::Key((shortcut[0] ^ allMods) & shortcut[0]);
    mods = shortcut.isEmpty() ? Qt::KeyboardModifiers(0) : Qt::KeyboardModifiers(shortcut[0] & allMods);
    const quint32 nativeKey = nativeKeycode(key);
    const quint32 nativeMods = nativeModifiers(mods);
    const bool res = registerShortcut(nativeKey, nativeMods);
    if (res)
        shortcuts.insert(qMakePair(nativeKey, nativeMods), &qxt_p());
    else
        qWarning() << "QxtGlobalShortcut failed to register:" << QKeySequence(key + mods).toString();
    return res;
}

bool QxtGlobalShortcutPrivate::unsetShortcut()
{
    bool res = false;
    const quint32 nativeKey = nativeKeycode(key);
    const quint32 nativeMods = nativeModifiers(mods);
    if (shortcuts.value(qMakePair(nativeKey, nativeMods)) == &qxt_p())
        res = unregisterShortcut(nativeKey, nativeMods);
    if (res)
        shortcuts.remove(qMakePair(nativeKey, nativeMods));
    else
        qWarning() << "QxtGlobalShortcut failed to unregister:" << QKeySequence(key + mods).toString();
    key = Qt::Key(0);
    mods = Qt::KeyboardModifiers(0);
    return res;
}

void QxtGlobalShortcutPrivate::activateShortcut(quint32 nativeKey, quint32 nativeMods)
{
    QxtGlobalShortcut* shortcut = shortcuts.value(qMakePair(nativeKey, nativeMods));
    if (shortcut && shortcut->isEnabled())
        emit shortcut->activated();
}

QxtGlobalShortcut::QxtGlobalShortcut(QObject* parent)
        : QObject(parent)
{
    QXT_INIT_PRIVATE(QxtGlobalShortcut);
}

QxtGlobalShortcut::QxtGlobalShortcut(const QKeySequence& shortcut, QObject* parent)
        : QObject(parent)
{
    QXT_INIT_PRIVATE(QxtGlobalShortcut);
    setShortcut(shortcut);
}

QxtGlobalShortcut::~QxtGlobalShortcut()
{
    if (qxt_d().key != 0)
        qxt_d().unsetShortcut();
}

QKeySequence QxtGlobalShortcut::shortcut() const
{
    return QKeySequence(qxt_d().key | qxt_d().mods);
}

bool QxtGlobalShortcut::setShortcut(const QKeySequence& shortcut)
{
    if (qxt_d().key != 0)
        qxt_d().unsetShortcut();
    return qxt_d().setShortcut(shortcut);
}

bool QxtGlobalShortcut::isEnabled() const
{
    return qxt_d().enabled;
}

void QxtGlobalShortcut::setEnabled(bool enabled)
{
    qxt_d().enabled = enabled;
}

void QxtGlobalShortcut::setDisabled(bool disabled)
{
    qxt_d().enabled = !disabled;
}
