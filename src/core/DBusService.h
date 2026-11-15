#pragma once

#include <QObject>
#include <QString>

class DictationCoordinator;
class ModelService;
class SystemService;
class SettingsService;

class DBusService : public QObject {
    Q_OBJECT

public:
    explicit DBusService(QObject* parent = nullptr);
    ~DBusService() override = default;

    bool registerService();
    bool registerServices(DictationCoordinator* coordinator, ModelService* modelService, SystemService* systemService,
                          SettingsService* settingsService = nullptr);
    bool registerController(DictationCoordinator* coordinator);
    bool isRegistered() const;

    static bool sendRemoteCommand(const QString& method);
    static bool isRemoteRunning();

    static constexpr QStringView kDbusServiceName = u"io.github.tattle.SpeechService";
    static constexpr QStringView kDbusObjectPath = u"/io/github/tattle/SpeechService";
    static constexpr QStringView kDbusInterfaceName = u"io.github.tattle.SpeechService";
    static constexpr QStringView kDbusModelInterfaceName = u"io.github.tattle.ModelService";
    static constexpr QStringView kDbusSystemInterfaceName = u"io.github.tattle.SystemService";
    static constexpr QStringView kDbusSettingsInterfaceName = u"io.github.tattle.SettingsService";

private:
    bool m_registered = false;
};
