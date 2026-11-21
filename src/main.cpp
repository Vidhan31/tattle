#include "DaemonClient.h"

#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QDBusConnection>
#include <QDBusInterface>
#include <QFile>
#include <QGuiApplication>
#include <QIcon>
#include <QProcess>
#include <QQmlApplicationEngine>
#include <QQuickStyle>
#include <QTimer>
#include <QtQml/QQmlExtensionPlugin>

#include <array>

Q_IMPORT_QML_PLUGIN(TattlePlugin)

using namespace Qt::StringLiterals;

#ifndef TATTLE_VERSION
#define TATTLE_VERSION "0.0.0-dev"
#endif

class SettingsGuiService : public QObject {
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "io.github.tattle.SettingsGui")

public:
    explicit SettingsGuiService(QObject* parent = nullptr)
        : QObject(parent) { }

    Q_INVOKABLE static void activateWindow() {
        if (auto* client = DaemonClient::instance()) {
            client->showWindow();
        }
    }

    Q_INVOKABLE static void navigateToSection(const QString& section) {
        if (auto* client = DaemonClient::instance()) {
            client->showWindow();
            client->navigateToSection(section);
        }
    }
};

/**
 * @brief Application entry point for the Tattle GUI client.
 *
 * Configures the Wayland platform environment, single-instance D-Bus registration,
 * registers QML types, initializes the daemon client façade, and launches the main
 * QML application engine.
 *
 * All speech, audio, model download, and notification work runs in tattled; the
 * GUI is a pure D-Bus presentation layer.
 */
int main(int argc, char* argv[]) {
    if (qEnvironmentVariableIsEmpty("QT_QPA_PLATFORM")) {
        qputenv("QT_QPA_PLATFORM", "wayland");
    }

    QGuiApplication app(argc, argv);
    if (qEnvironmentVariableIsEmpty("QT_QUICK_CONTROLS_STYLE")) {
        QQuickStyle::setStyle(u"Fusion"_s);
        QQuickStyle::setFallbackStyle(u"Fusion"_s);
    }
    QGuiApplication::setApplicationName(u"Tattle"_s);
    QGuiApplication::setApplicationVersion(u"" TATTLE_VERSION ""_s);
    QGuiApplication::setOrganizationName(u"Tattle"_s);
    QGuiApplication::setOrganizationDomain(u"io.github.tattle"_s);
    QGuiApplication::setDesktopFileName(u"io.github.tattle"_s);
    QGuiApplication::setQuitOnLastWindowClosed(true);

    QCommandLineParser parser;
    parser.setApplicationDescription(u"Tattle — Wayland native Speech-to-Text application"_s);
    parser.addHelpOption();
    parser.addVersionOption();

    const QCommandLineOption daemonOption(u"daemon"_s, u"Launch the headless daemon (tattled)"_s);
    const QCommandLineOption toggleOption({u"t"_s, u"toggle"_s}, u"Toggle speech-to-text recording"_s);
    const QCommandLineOption startOption(u"start"_s, u"Start speech-to-text recording"_s);
    const QCommandLineOption stopOption(u"stop"_s, u"Stop speech-to-text recording"_s);
    const QCommandLineOption showOption({u"s"_s, u"show"_s}, u"Show and activate the main window"_s);
    const QCommandLineOption quitOption({u"q"_s, u"quit"_s}, u"Quit the running application"_s);
    const QCommandLineOption sectionOption(u"section"_s, u"Navigate to settings section on startup"_s, u"section"_s);

    parser.addOption(daemonOption);
    parser.addOption(toggleOption);
    parser.addOption(startOption);
    parser.addOption(stopOption);
    parser.addOption(showOption);
    parser.addOption(quitOption);
    parser.addOption(sectionOption);

    parser.process(app);

    const auto launchDaemon = [](const QStringList& args = {}) -> bool {
        const QString appDir = QCoreApplication::applicationDirPath();
        const QString daemonInAppDir = appDir + u"/tattled"_s;
        const QString daemonExec = QFile::exists(daemonInAppDir) ? daemonInAppDir : u"tattled"_s;
        qInfo() << "Tattle: Starting background daemon:" << daemonExec << args;
        return QProcess::startDetached(daemonExec, args);
    };

    if (parser.isSet(daemonOption)) {
        return launchDaemon({}) ? 0 : 1;
    }

    if (parser.isSet(toggleOption)) {
        if (DaemonClient::sendRemoteCommand(u"toggleRecording"_s)) {
            return 0;
        }
        return launchDaemon({u"--toggle"_s}) ? 0 : 1;
    }
    if (parser.isSet(startOption)) {
        if (DaemonClient::sendRemoteCommand(u"startRecording"_s)) {
            return 0;
        }
        return launchDaemon({u"--start"_s}) ? 0 : 1;
    }
    if (parser.isSet(stopOption)) {
        return DaemonClient::sendRemoteCommand(u"stopRecording"_s) ? 0 : 1;
    }
    if (parser.isSet(quitOption)) {
        return DaemonClient::sendRemoteCommand(u"quitApp"_s) ? 0 : 1;
    }

    const QString kGuiService = u"io.github.tattle.SettingsGui"_s;
    const QString kGuiPath = u"/io/github/tattle/SettingsGui"_s;

    if (QDBusConnection::sessionBus().isConnected()) {
        auto* guiService = new SettingsGuiService(&app);
        QDBusConnection::sessionBus().registerObject(
            kGuiPath, guiService, QDBusConnection::ExportAllSlots | QDBusConnection::ExportAllInvokables);
        if (!QDBusConnection::sessionBus().registerService(kGuiService)) {
            QDBusConnection::sessionBus().unregisterObject(kGuiPath);
            QDBusInterface guiIface(kGuiService, kGuiPath, kGuiService, QDBusConnection::sessionBus());
            if (guiIface.isValid()) {
                if (parser.isSet(sectionOption)) {
                    guiIface.call(QDBus::NoBlock, u"navigateToSection"_s, parser.value(sectionOption));
                } else {
                    guiIface.call(QDBus::NoBlock, u"activateWindow"_s);
                }
            }
            return 0;
        }
    }

    if (!DaemonClient::isServiceRunning()) {
        launchDaemon({});
    }

    QIcon appIcon;
    appIcon.addFile(u":/qt/qml/Tattle/assets/tattle_logo.svg"_s);
    constexpr auto kIconSizes = std::to_array({16, 24, 32, 64, 128, 256, 512});
    for (const int sz : kIconSizes) {
        appIcon.addFile(QString(u":/qt/qml/Tattle/assets/tattle_logo_%1.png"_s).arg(sz), QSize(sz, sz));
    }
    QGuiApplication::setWindowIcon(appIcon);

    QQmlApplicationEngine engine;

    QObject::connect(&engine, &QQmlEngine::quit, []() { QCoreApplication::exit(0); });

    QObject::connect(&app, &QGuiApplication::aboutToQuit, []() {
        if (auto* client = DaemonClient::instance()) {
            client->setClientWindowFocused(false);
        }
    });

    QObject::connect(
        &engine, &QQmlApplicationEngine::objectCreationFailed, &app, []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);

    DaemonClient::instance();

    engine.loadFromModule("Tattle", "Main");

    if (parser.isSet(sectionOption)) {
        const QString section = parser.value(sectionOption);
        QTimer::singleShot(0, [section]() {
            if (auto* client = DaemonClient::instance()) {
                client->navigateToSection(section);
            }
        });
    }

    return QGuiApplication::exec();
}

#include "main.moc"
