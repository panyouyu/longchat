#ifndef GLOBALSHORTCUT_H
#define GLOBALSHORTCUT_H

#include "ui/twidget.h"
#include <QObject>
#include <QKeySequence>
#include <QAbstractNativeEventFilter>
class QxtGlobalShortcutPrivate;

#define QXT_DECLARE_PRIVATE(PUB) friend class PUB##Private; QxtPrivateInterface<PUB, PUB##Private> qxt_d;
#define QXT_DECLARE_PUBLIC(PUB) friend class PUB;
#define QXT_INIT_PRIVATE(PUB) qxt_d.setPublic(this);
#define QXT_D(PUB) PUB##Private& d = qxt_d()
#define QXT_P(PUB) PUB& p = qxt_p()

template <typename PUB>
class QxtPrivate
{
public:
    virtual ~QxtPrivate()
    {}
    inline void QXT_setPublic(PUB* pub)
    {
        qxt_p_ptr = pub;
    }

protected:
    inline PUB& qxt_p()
    {
        return *qxt_p_ptr;
    }
    inline const PUB& qxt_p() const
    {
        return *qxt_p_ptr;
    }
    inline PUB* qxt_ptr()
    {
        return qxt_p_ptr;
    }
    inline const PUB* qxt_ptr() const
    {
        return qxt_p_ptr;
    }

private:
    PUB* qxt_p_ptr;
};

template <typename PUB, typename PVT>
class QxtPrivateInterface
{
    friend class QxtPrivate<PUB>;
public:
    QxtPrivateInterface()
    {
        pvt = new PVT;
    }
    ~QxtPrivateInterface()
    {
        delete pvt;
    }

    inline void setPublic(PUB* pub)
    {
        pvt->QXT_setPublic(pub);
    }
    inline PVT& operator()()
    {
        return *static_cast<PVT*>(pvt);
    }
    inline const PVT& operator()() const
    {
        return *static_cast<PVT*>(pvt);
    }
    inline PVT * operator->()
    {
    return static_cast<PVT*>(pvt);
    }
    inline const PVT * operator->() const
    {
    return static_cast<PVT*>(pvt);
    }
private:
    QxtPrivateInterface(const QxtPrivateInterface&) { }
    QxtPrivateInterface& operator=(const QxtPrivateInterface&) { }
    QxtPrivate<PUB>* pvt;
};

class QxtGlobalShortcut : public QObject
{
    Q_OBJECT
    QXT_DECLARE_PRIVATE(QxtGlobalShortcut)
    Q_PROPERTY(bool enabled READ isEnabled WRITE setEnabled)
    Q_PROPERTY(QKeySequence shortcut READ shortcut WRITE setShortcut)
public:
    explicit QxtGlobalShortcut(QObject* parent = nullptr);
    explicit QxtGlobalShortcut(const QKeySequence& shortcut, QObject* parent = nullptr);
    virtual ~QxtGlobalShortcut();

    QKeySequence shortcut() const;
    bool setShortcut(const QKeySequence& shortcut);

    bool isEnabled() const;

public Q_SLOTS:
    void setEnabled(bool enabled = true);
    void setDisabled(bool disabled = true);

Q_SIGNALS:
    void activated();
};

class QxtGlobalShortcutPrivate : public QxtPrivate<QxtGlobalShortcut>
#if QT_VERSION >= QT_VERSION_CHECK(5,0,0) && !defined(Q_OS_MAC)
        ,public QAbstractNativeEventFilter
#endif
{
public:
    QXT_DECLARE_PUBLIC(QxtGlobalShortcut)
    QxtGlobalShortcutPrivate();
    ~QxtGlobalShortcutPrivate();

    bool enabled;
    Qt::Key key;
    Qt::KeyboardModifiers mods;

    bool setShortcut(const QKeySequence& shortcut);
    bool unsetShortcut();

    static bool error;
#ifndef Q_OS_MAC
    static int ref;
#if QT_VERSION < QT_VERSION_CHECK(5,0,0)
    static QAbstractEventDispatcher::EventFilter prevEventFilter;
    static bool eventFilter(void* message);
#else
    virtual bool nativeEventFilter(const QByteArray & eventType, void * message, long * result);
#endif // QT_VERSION < QT_VERSION_CHECK(5,0,0)
#endif // Q_OS_MAC

    static void activateShortcut(quint32 nativeKey, quint32 nativeMods);

private:
    static quint32 nativeKeycode(Qt::Key keycode);
    static quint32 nativeModifiers(Qt::KeyboardModifiers modifiers);

    static bool registerShortcut(quint32 nativeKey, quint32 nativeMods);
    static bool unregisterShortcut(quint32 nativeKey, quint32 nativeMods);

    static QHash<QPair<quint32, quint32>, QxtGlobalShortcut*> shortcuts;
};

typedef QList<WId> WindowList;

class QxtWindowSystem
{
public:
    static WindowList windows();
    static WId activeWindow();
    static WId findWindow(const QString& title);
    static WId windowAt(const QPoint& pos);
    static QString windowTitle(WId window);
    static QStringList windowTitles();
    static QRect windowGeometry(WId window);

    static uint idleTime();
};

#ifdef Q_OS_MAC
#include "platform/mac/shortcut_mac.h"
#elif defined Q_OS_LINUX // Q_OS_MAC
#include "platform/linux/shortcut_linux.h"
#elif defined Q_OS_WIN // Q_OS_MAC || Q_OS_LINUX
#include "platform/win/shortcut_win.h"
#endif

#endif // GLOBALSHORTCUT_H
