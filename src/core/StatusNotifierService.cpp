#include "StatusNotifierService.h"

#include "DictationCoordinator.h"
#include "LoggingCategories.h"

#include "SystemService.h"

#include <QColor>
#include <QCoreApplication>
#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusMetaType>
#include <QDBusServiceWatcher>
#include <QDir>
#include <QFile>
#include <QIcon>
#include <QImage>
#include <QPainter>

using namespace Qt::StringLiterals;

QDBusArgument& operator<<(QDBusArgument& argument, const KDbusImageStruct& icon) {
    argument.beginStructure();
    argument << icon.width << icon.height << icon.data;
    argument.endStructure();
    return argument;
}

const QDBusArgument& operator>>(const QDBusArgument& argument, KDbusImageStruct& icon) {
    argument.beginStructure();
    argument >> icon.width >> icon.height >> icon.data;
    argument.endStructure();
    return argument;
}

QDBusArgument& operator<<(QDBusArgument& argument, const KDbusToolTipStruct& toolTip) {
    argument.beginStructure();
    argument << toolTip.icon << toolTip.image << toolTip.title << toolTip.subTitle;
    argument.endStructure();
    return argument;
}

const QDBusArgument& operator>>(const QDBusArgument& argument, KDbusToolTipStruct& toolTip) {
    argument.beginStructure();
    argument >> toolTip.icon >> toolTip.image >> toolTip.title >> toolTip.subTitle;
    argument.endStructure();
    return argument;
}

QDBusArgument& operator<<(QDBusArgument& argument, const DBusMenuLayoutItem& item) {
    argument.beginStructure();
    argument << item.id << item.properties;
    argument.beginArray(qMetaTypeId<QDBusVariant>());
    for (const auto& child : item.children) {
        QDBusVariant var;
        var.setVariant(QVariant::fromValue(child));
        argument << var;
    }
    argument.endArray();
    argument.endStructure();
    return argument;
}

const QDBusArgument& operator>>(const QDBusArgument& argument, DBusMenuLayoutItem& item) {
    argument.beginStructure();
    argument >> item.id >> item.properties;
    argument.beginArray();
    while (!argument.atEnd()) {
        QDBusVariant var;
        argument >> var;
        item.children.append(qvariant_cast<DBusMenuLayoutItem>(var.variant()));
    }
    argument.endArray();
    argument.endStructure();
    return argument;
}

QDBusArgument& operator<<(QDBusArgument& argument, const DBusMenuItem& item) {
    argument.beginStructure();
    argument << item.id << item.properties;
    argument.endStructure();
    return argument;
}

const QDBusArgument& operator>>(const QDBusArgument& argument, DBusMenuItem& item) {
    argument.beginStructure();
    argument >> item.id >> item.properties;
    argument.endStructure();
    return argument;
}

QDBusArgument& operator<<(QDBusArgument& argument, const DBusMenuItemKeys& item) {
    argument.beginStructure();
    argument << item.id << item.keys;
    argument.endStructure();
    return argument;
}

const QDBusArgument& operator>>(const QDBusArgument& argument, DBusMenuItemKeys& item) {
    argument.beginStructure();
    argument >> item.id >> item.keys;
    argument.endStructure();
    return argument;
}

StatusNotifierItemAdaptor::StatusNotifierItemAdaptor(StatusNotifierService* parent)
    : QDBusAbstractAdaptor(parent)
    , m_service(parent) { }

QString StatusNotifierItemAdaptor::category() {
    return u"ApplicationStatus"_s;
}

QString StatusNotifierItemAdaptor::id() {
    return u"tattle"_s;
}

QString StatusNotifierItemAdaptor::title() {
    return u"Tattle"_s;
}

QString StatusNotifierItemAdaptor::status() const {
    const bool rec = m_service && m_service->controller() && m_service->controller()->recording();
    return rec ? u"NeedsAttention"_s : u"Active"_s;
}

int StatusNotifierItemAdaptor::windowId() {
    return 0;
}

QString StatusNotifierItemAdaptor::iconThemePath() const {
    const QString localShare = QDir::homePath() + u"/.local/share/icons"_s;
    if (QFile::exists(localShare + u"/hicolor/32x32/apps/io.github.tattle.png"_s)) {
        return localShare;
    }
    return {};
}

QDBusObjectPath StatusNotifierItemAdaptor::menu() {
    return QDBusObjectPath(u"/MenuBar"_s);
}

bool StatusNotifierItemAdaptor::itemIsMenu() {
    return false;
}

QString StatusNotifierItemAdaptor::iconName() const {
    const bool rec = m_service && m_service->controller() && m_service->controller()->recording();
    if (rec) {
        return u"media-record"_s;
    }
    return u"io.github.tattle"_s;
}

KDbusImageVector StatusNotifierItemAdaptor::iconPixmap() const {
    if (!m_service) {
        return {};
    }
    const bool rec = m_service->controller() && m_service->controller()->recording();
    if (rec && !m_service->recordingIconPixmap().isEmpty()) {
        return m_service->recordingIconPixmap();
    }
    return m_service->iconPixmap();
}

QString StatusNotifierItemAdaptor::overlayIconName() {
    return {};
}

KDbusImageVector StatusNotifierItemAdaptor::overlayIconPixmap() {
    return {};
}

QString StatusNotifierItemAdaptor::attentionIconName() const {
    return u"media-record"_s;
}

KDbusImageVector StatusNotifierItemAdaptor::attentionIconPixmap() const {
    return m_service ? m_service->recordingIconPixmap() : KDbusImageVector {};
}

QString StatusNotifierItemAdaptor::attentionMovieName() {
    return {};
}

KDbusToolTipStruct StatusNotifierItemAdaptor::toolTip() const {
    KDbusToolTipStruct tip;
    const bool rec = m_service && m_service->controller() && m_service->controller()->recording();
    tip.icon = rec ? u"media-record"_s : u"audio-input-microphone"_s;
    tip.image = m_service ? m_service->iconPixmap() : KDbusImageVector {};
    tip.title = u"Tattle"_s;
    tip.subTitle = rec ? u"Recording speech..."_s : u"Ready"_s;
    return tip;
}

void StatusNotifierItemAdaptor::ContextMenu(int x, int y) {
    Q_UNUSED(x);
    Q_UNUSED(y);
}

void StatusNotifierItemAdaptor::Activate(int x, int y) {
    Q_UNUSED(x);
    Q_UNUSED(y);
    emit m_service->requestShowWindow();
}

void StatusNotifierItemAdaptor::SecondaryActivate(int x, int y) {
    Q_UNUSED(x);
    Q_UNUSED(y);
}

void StatusNotifierItemAdaptor::Scroll(int delta, const QString& orientation) {
    Q_UNUSED(delta);
    Q_UNUSED(orientation);
}

void StatusNotifierItemAdaptor::ProvideXdgActivationToken(const QString& token) {
    if (!token.isEmpty()) {
        qputenv("XDG_ACTIVATION_TOKEN", token.toUtf8());
    }
}

DBusMenuAdaptor::DBusMenuAdaptor(StatusNotifierService* parent)
    : QDBusAbstractAdaptor(parent)
    , m_service(parent) { }

QString DBusMenuAdaptor::status() {
    return u"normal"_s;
}

bool DBusMenuAdaptor::AboutToShow(int id) {
    Q_UNUSED(id);
    return false;
}

void DBusMenuAdaptor::Event(int id, const QString& eventId, const QDBusVariant& data, uint timestamp) {
    Q_UNUSED(data);
    Q_UNUSED(timestamp);
    if (eventId == u"clicked"_s) {
        if (id == 1) {
            emit m_service->requestShowWindow();
        } else if (id == 3) {
            emit m_service->requestQuitApp();
        }
    }
}

QList<int> DBusMenuAdaptor::EventGroup(const QList<int>& ids, const QString& eventId, const QDBusVariant& data,
                                       uint timestamp) {
    for (const int id : ids) {
        Event(id, eventId, data, timestamp);
    }
    return {};
}

bool DBusMenuAdaptor::AboutToShowGroup(const QList<int>& ids, QList<int>& updatesNeeded, QList<int>& idErrors) {
    Q_UNUSED(ids);
    updatesNeeded.clear();
    idErrors.clear();
    return true;
}

QDBusVariant DBusMenuAdaptor::GetProperty(int id, const QString& name) {
    if (id == 1) {
        if (name == u"label"_s) {
            return QDBusVariant(u"Open Tattle"_s);
        }
        if (name == u"enabled"_s) {
            return QDBusVariant(true);
        }
        if (name == u"visible"_s) {
            return QDBusVariant(true);
        }
    } else if (id == 2) {
        if (name == u"type"_s) {
            return QDBusVariant(u"separator"_s);
        }
        if (name == u"visible"_s) {
            return QDBusVariant(true);
        }
    } else if (id == 3) {
        if (name == u"label"_s) {
            return QDBusVariant(u"Quit"_s);
        }
        if (name == u"enabled"_s) {
            return QDBusVariant(true);
        }
        if (name == u"visible"_s) {
            return QDBusVariant(true);
        }
    }
    return {};
}

uint DBusMenuAdaptor::GetLayout(int parentId, int recursionDepth, const QStringList& propertyNames,
                                DBusMenuLayoutItem& layout) {
    Q_UNUSED(recursionDepth);
    Q_UNUSED(propertyNames);

    layout.id = parentId;
    layout.properties.clear();
    layout.children.clear();

    if (parentId != 0) {
        return m_service->revision();
    }

    DBusMenuLayoutItem showItem;
    showItem.id = 1;
    showItem.properties[u"label"_s] = u"Open Tattle"_s;
    showItem.properties[u"enabled"_s] = true;
    showItem.properties[u"visible"_s] = true;

    DBusMenuLayoutItem sepItem;
    sepItem.id = 2;
    sepItem.properties[u"type"_s] = u"separator"_s;
    sepItem.properties[u"visible"_s] = true;

    DBusMenuLayoutItem quitItem;
    quitItem.id = 3;
    quitItem.properties[u"label"_s] = u"Quit"_s;
    quitItem.properties[u"enabled"_s] = true;
    quitItem.properties[u"visible"_s] = true;

    layout.children.append(showItem);
    layout.children.append(sepItem);
    layout.children.append(quitItem);

    return m_service->revision();
}

DBusMenuItemList DBusMenuAdaptor::GetGroupProperties(const QList<int>& ids, const QStringList& propertyNames) {
    Q_UNUSED(propertyNames);
    DBusMenuItemList list;

    for (const int id : ids) {
        DBusMenuItem item;
        item.id = id;
        if (id == 1) {
            item.properties[u"label"_s] = u"Open Tattle"_s;
            item.properties[u"enabled"_s] = true;
            item.properties[u"visible"_s] = true;
        } else if (id == 2) {
            item.properties[u"type"_s] = u"separator"_s;
            item.properties[u"visible"_s] = true;
        } else if (id == 3) {
            item.properties[u"label"_s] = u"Quit"_s;
            item.properties[u"enabled"_s] = true;
            item.properties[u"visible"_s] = true;
        }
        list.append(item);
    }
    return list;
}

void StatusNotifierService::registerMetaTypes() {
    static bool registered = false;
    if (registered) {
        return;
    }
    registered = true;

    qDBusRegisterMetaType<KDbusImageStruct>();
    qDBusRegisterMetaType<KDbusImageVector>();
    qDBusRegisterMetaType<KDbusToolTipStruct>();
    qDBusRegisterMetaType<DBusMenuLayoutItem>();
    qDBusRegisterMetaType<DBusMenuItem>();
    qDBusRegisterMetaType<DBusMenuItemList>();
    qDBusRegisterMetaType<DBusMenuItemKeys>();
    qDBusRegisterMetaType<DBusMenuItemKeysList>();
}

static KDbusImageStruct imageToDbusPixmap(const QImage& img) {
    KDbusImageStruct item;
    if (img.isNull()) {
        return item;
    }
    const QImage argb = img.convertToFormat(QImage::Format_ARGB32);
    item.width = argb.width();
    item.height = argb.height();
    item.data.resize(static_cast<qsizetype>(item.width) * item.height * 4);
    auto* dest = reinterpret_cast<uchar*>(item.data.data());
    for (int y = 0; y < item.height; ++y) {
        const auto* src = reinterpret_cast<const QRgb*>(argb.constScanLine(y));
        for (int x = 0; x < item.width; ++x) {
            *dest++ = static_cast<uchar>(qAlpha(src[x]));
            *dest++ = static_cast<uchar>(qRed(src[x]));
            *dest++ = static_cast<uchar>(qGreen(src[x]));
            *dest++ = static_cast<uchar>(qBlue(src[x]));
        }
    }
    return item;
}

void StatusNotifierService::loadIcons() {
    m_iconPixmap.clear();
    m_recordingIconPixmap.clear();

    const QList<int> sizes = {16, 24, 32, 64};
    for (const int sz : sizes) {
        const QString path = QString(u":/qt/qml/Tattle/assets/speech-to-text-%1.png"_s).arg(sz);
        const QImage img(path);
        if (!img.isNull()) {
            m_iconPixmap.append(imageToDbusPixmap(img));

            QImage recImg = img.copy();
            QPainter p(&recImg);
            p.setRenderHint(QPainter::Antialiasing);
            const int dotSize = qMax(5, sz / 3);
            const QRect dotRect(sz - dotSize - 1, sz - dotSize - 1, dotSize, dotSize);
            p.setBrush(QColor(0xf4, 0x3f, 0x5e));
            p.setPen(QPen(QColor(0xff, 0xff, 0xff), 1));
            p.drawEllipse(dotRect);
            p.end();
            m_recordingIconPixmap.append(imageToDbusPixmap(recImg));
        }
    }
}

StatusNotifierService::StatusNotifierService(QObject* parent)
    : QObject(parent) {
    registerMetaTypes();
    loadIcons();
    m_sniAdaptor = new StatusNotifierItemAdaptor(this);
    m_menuAdaptor = new DBusMenuAdaptor(this);
}

StatusNotifierService::~StatusNotifierService() {
    if (m_registered && QDBusConnection::sessionBus().isConnected()) {
        QDBusConnection::sessionBus().unregisterObject(u"/StatusNotifierItem"_s);
        QDBusConnection::sessionBus().unregisterObject(u"/MenuBar"_s);
        QDBusConnection::sessionBus().unregisterService(m_serviceName);
    }
}

bool StatusNotifierService::registerController(DictationCoordinator* coordinator, SystemService* systemService) {
    if (!coordinator || !QDBusConnection::sessionBus().isConnected()) {
        return false;
    }

    m_coordinator = coordinator;
    m_systemService = systemService;
    connect(m_coordinator, &DictationCoordinator::recordingChanged, this, &StatusNotifierService::onRecordingChanged);

    m_serviceName = QString(u"org.kde.StatusNotifierItem-%1-1"_s).arg(QCoreApplication::applicationPid());

    if (!QDBusConnection::sessionBus().registerService(m_serviceName)) {
        qWarning() << "StatusNotifierService: Failed to register D-Bus service" << m_serviceName;
        return false;
    }

    if (!QDBusConnection::sessionBus().registerObject(u"/StatusNotifierItem"_s, this)) {
        qWarning() << "StatusNotifierService: Failed to register /StatusNotifierItem";
        return false;
    }

    if (!QDBusConnection::sessionBus().registerObject(u"/MenuBar"_s, this)) {
        qWarning() << "StatusNotifierService: Failed to register /MenuBar";
        return false;
    }

    m_registered = true;
    qCDebug(lcSpeech) << "StatusNotifierService: Registered D-Bus SNI service" << m_serviceName;

    registerWithWatcher();

    auto* serviceWatcher = new QDBusServiceWatcher(u"org.kde.StatusNotifierWatcher"_s, QDBusConnection::sessionBus(),
                                                   QDBusServiceWatcher::WatchForRegistration, this);
    connect(serviceWatcher, &QDBusServiceWatcher::serviceRegistered, this, &StatusNotifierService::registerWithWatcher);

    QDBusConnection::sessionBus().connect(u"org.kde.StatusNotifierWatcher"_s, u"/StatusNotifierWatcher"_s,
                                          u"org.kde.StatusNotifierWatcher"_s, u"StatusNotifierWatcherRegistered"_s,
                                          this, SLOT(registerWithWatcher()));

    return true;
}

void StatusNotifierService::registerWithWatcher() {
    if (!QDBusConnection::sessionBus().isConnected() || m_serviceName.isEmpty()) {
        return;
    }

    QDBusInterface watcher(u"org.kde.StatusNotifierWatcher"_s, u"/StatusNotifierWatcher"_s,
                           u"org.kde.StatusNotifierWatcher"_s, QDBusConnection::sessionBus());

    if (watcher.isValid()) {
        const QDBusMessage reply = watcher.call(u"RegisterStatusNotifierItem"_s, m_serviceName);
        if (reply.type() == QDBusMessage::ErrorMessage) {
            qCDebug(lcSpeech) << "StatusNotifierService: RegisterStatusNotifierItem returned:" << reply.errorMessage();
        } else {
            qCDebug(lcSpeech) << "StatusNotifierService: Registered with StatusNotifierWatcher";
        }
    }
}

void StatusNotifierService::onRecordingChanged() {
    m_revision++;
    if (m_sniAdaptor) {
        emit m_sniAdaptor->NewStatus(m_sniAdaptor->status());
        emit m_sniAdaptor->NewIcon();
        emit m_sniAdaptor->NewAttentionIcon();
        emit m_sniAdaptor->NewToolTip();
    }
    if (m_menuAdaptor) {
        emit m_menuAdaptor->LayoutUpdated(m_revision, 0);
    }
}
