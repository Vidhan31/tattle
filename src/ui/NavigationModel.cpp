#include "NavigationModel.h"

using namespace Qt::StringLiterals;

NavigationModel::NavigationModel(QObject* parent)
    : QAbstractListModel(parent) {
    rebuildItems();
}

int NavigationModel::rowCount(const QModelIndex& parent) const {
    if (parent.isValid()) {
        return 0;
    }
    return static_cast<int>(m_items.size());
}

QVariant NavigationModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= static_cast<int>(m_items.size())) {
        return {};
    }

    const auto& item = m_items.at(index.row());
    switch (role) {
        case SectionRole:
            return item.section;
        case TitleRole:
            return item.title;
        case IconSourceRole:
            return item.iconSource;
        case SectionIdRole:
            return item.sectionId;
        case IsFirstInSectionRole:
            return item.isFirstInSection;
        default:
            return {};
    }
}

QHash<int, QByteArray> NavigationModel::roleNames() const {
    return {{SectionRole, "section"},
            {TitleRole, "title"},
            {IconSourceRole, "iconSource"},
            {SectionIdRole, "sectionId"},
            {IsFirstInSectionRole, "isFirstInSection"}};
}

bool NavigationModel::isCloud() const {
    return m_isCloud;
}

void NavigationModel::setIsCloud(bool cloud) {
    if (m_isCloud == cloud) {
        return;
    }
    m_isCloud = cloud;
    rebuildItems();
    emit isCloudChanged();
    emit isOnlineChanged();
}

bool NavigationModel::isOnline() const {
    return isCloud();
}

void NavigationModel::setIsOnline(bool online) {
    setIsCloud(online);
}

int NavigationModel::count() const {
    return static_cast<int>(m_items.size());
}

void NavigationModel::rebuildItems() {
    beginResetModel();
    m_items.clear();

    m_items.append({u"MAIN"_s, tr("Dictate"), u"qrc:/qt/qml/Tattle/assets/icons/mic.svg"_s, u"dictate"_s, true});

    m_items.append({u"MAIN"_s, tr("History"), u"qrc:/qt/qml/Tattle/assets/icons/history.svg"_s, u"history"_s, false});

    m_items.append({u"PREFERENCES"_s, tr("Transcription Mode"), u"qrc:/qt/qml/Tattle/assets/icons/sparkles.svg"_s,
                    u"engine"_s, true});

    if (m_isCloud) {
        m_items.append({u"PREFERENCES"_s, tr("Cloud Transcription"), u"qrc:/qt/qml/Tattle/assets/icons/cloud.svg"_s,
                        u"cloud"_s, false});
    } else {
        m_items.append({u"PREFERENCES"_s, tr("Offline Transcription"), u"qrc:/qt/qml/Tattle/assets/icons/cpu.svg"_s,
                        u"offline"_s, false});
    }

    m_items.append({u"PREFERENCES"_s, tr("Audio & Recording"), u"qrc:/qt/qml/Tattle/assets/icons/speech.svg"_s,
                    u"audio"_s, false});

    m_items.append(
        {u"PREFERENCES"_s, tr("Shortcuts"), u"qrc:/qt/qml/Tattle/assets/icons/keyboard.svg"_s, u"shortcuts"_s, false});

    m_items.append({u"PREFERENCES"_s, tr("Notifications"), u"qrc:/qt/qml/Tattle/assets/icons/bell.svg"_s,
                    u"notifications"_s, false});

    m_items.append({u"PREFERENCES"_s, tr("System & Privacy"), u"qrc:/qt/qml/Tattle/assets/icons/sliders.svg"_s,
                    u"system"_s, false});

    m_items.append({u"INFO"_s, tr("About"), u"qrc:/qt/qml/Tattle/assets/icons/info.svg"_s, u"about"_s, true});

    endResetModel();
    emit countChanged();
}
