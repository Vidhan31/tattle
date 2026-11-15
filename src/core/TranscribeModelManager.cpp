#include "TranscribeModelManager.h"

#include "LoggingCategories.h"

#include "common/SettingsKeys.h"
#include "SettingsManager.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSettings>
#include <QStandardPaths>
#include <QStorageInfo>

#include <algorithm>

using namespace Qt::StringLiterals;

namespace {
const auto kDefaultSelectedModel = u"whisper-base.en"_s;
TranscribeModelManager* s_instance = nullptr;
} // namespace

TranscribeModelManager* TranscribeModelManager::instance() {
    return s_instance;
}

TranscribeModelManager::TranscribeModelManager(QObject* parent)
    : TranscribeModelManager(std::make_unique<ModelDownloader>(), parent) { }

TranscribeModelManager::TranscribeModelManager(std::unique_ptr<ModelDownloader> downloader, QObject* parent)
    : QAbstractListModel(parent)
    , m_downloader(std::move(downloader)) {
    s_instance = this;
    if (!m_downloader) {
        m_downloader = std::make_unique<ModelDownloader>(this);
    } else {
        m_downloader->setParent(this);
    }

    initPresets();

    QDir().mkpath(modelsDirectory());
    cleanupOrphanedPartFiles();
    scanInstalledModels();
    checkDiskSpace();
    setupDownloaderConnections();

    if (auto* inst = SettingsManager::instance()) {
        setSettingsManager(inst);
    } else {
        loadSettings();
    }
}

void TranscribeModelManager::setSettingsManager(SettingsManager* settings) {
    if (m_settingsManager == settings) {
        return;
    }
    if (m_settingsManager) {
        disconnect(m_settingsManager, nullptr, this, nullptr);
    }
    m_settingsManager = settings;
    if (m_settingsManager) {
        connect(m_settingsManager, &SettingsManager::transcribeConfigChanged, this,
                [this](const SettingsManager::TranscribeConfig& config) {
                    if (!config.selectedModel.isEmpty() && m_selectedModelId != config.selectedModel &&
                        isModelInstalled(config.selectedModel)) {
                        setSelectedModelId(config.selectedModel);
                    }
                });
    }
    loadSettings();
}

SettingsManager* TranscribeModelManager::settingsManager() const noexcept {
    return m_settingsManager;
}

TranscribeModelManager::~TranscribeModelManager() {
    if (s_instance == this) {
        s_instance = nullptr;
    }
}

void TranscribeModelManager::initPresets() {
    m_models = {
        TranscribeModelItem {
            .id = u"whisper-tiny.en"_s,
            .name = tr("Whisper Tiny (English)"),
            .fileName = u"whisper-tiny.en-Q8_0.gguf"_s,
            .downloadUrl =
                u"https://huggingface.co/handy-computer/whisper-tiny.en-gguf/resolve/main/whisper-tiny.en-Q8_0.gguf"_s,
            .sizeBytes = 45904544,
            .sizeFormatted = u"~44 MiB"_s,
            .memoryFormatted = u"~120 MB RAM/VRAM"_s,
            .description = tr("Fastest English dictation with lowest resource usage and minimal latency."),
            .installedSizeFormatted = {},
            .speedFormatted = {}},
        TranscribeModelItem {
            .id = u"whisper-tiny"_s,
            .name = tr("Whisper Tiny (Multilingual)"),
            .fileName = u"whisper-tiny-Q8_0.gguf"_s,
            .downloadUrl =
                u"https://huggingface.co/handy-computer/whisper-tiny-gguf/resolve/main/whisper-tiny-Q8_0.gguf"_s,
            .sizeBytes = 45904544,
            .sizeFormatted = u"~44 MiB"_s,
            .memoryFormatted = u"~120 MB RAM/VRAM"_s,
            .description = tr("Ultra-fast multilingual dictation across 99+ languages."),
            .installedSizeFormatted = {},
            .speedFormatted = {}},
        TranscribeModelItem {
            .id = u"whisper-base.en"_s,
            .name = tr("Whisper Base (English)"),
            .fileName = u"whisper-base.en-Q8_0.gguf"_s,
            .downloadUrl =
                u"https://huggingface.co/handy-computer/whisper-base.en-gguf/resolve/main/whisper-base.en-Q8_0.gguf"_s,
            .sizeBytes = 84886208,
            .sizeFormatted = u"~81 MiB"_s,
            .memoryFormatted = u"~220 MB RAM/VRAM"_s,
            .description = tr("Fast English transcription with improved accuracy for general speech."),
            .installedSizeFormatted = {},
            .speedFormatted = {}},
        TranscribeModelItem {
            .id = u"whisper-base"_s,
            .name = tr("Whisper Base (Multilingual)"),
            .fileName = u"whisper-base-Q8_0.gguf"_s,
            .downloadUrl =
                u"https://huggingface.co/handy-computer/whisper-base-gguf/resolve/main/whisper-base-Q8_0.gguf"_s,
            .sizeBytes = 84886208,
            .sizeFormatted = u"~81 MiB"_s,
            .memoryFormatted = u"~220 MB RAM/VRAM"_s,
            .description = tr("Fast multilingual transcription with solid baseline accuracy."),
            .installedSizeFormatted = {},
            .speedFormatted = {}},
        TranscribeModelItem {
            .id = u"whisper-small"_s,
            .name = tr("Whisper Small (Multilingual)"),
            .fileName = u"whisper-small-Q8_0.gguf"_s,
            .downloadUrl =
                u"https://huggingface.co/handy-computer/whisper-small-gguf/resolve/main/whisper-small-Q8_0.gguf"_s,
            .sizeBytes = 269751136,
            .sizeFormatted = u"~257 MiB"_s,
            .memoryFormatted = u"~550 MB RAM/VRAM"_s,
            .description = tr("High accuracy multilingual model; recommended sweet spot for desktop dictation."),
            .installedSizeFormatted = {},
            .speedFormatted = {}},
        TranscribeModelItem {
            .id = u"whisper-large-v3-turbo"_s,
            .name = tr("Whisper Large v3 Turbo (Multilingual)"),
            .fileName = u"whisper-large-v3-turbo-Q8_0.gguf"_s,
            .downloadUrl =
                u"https://huggingface.co/handy-computer/whisper-large-v3-turbo-gguf/resolve/main/whisper-large-v3-turbo-Q8_0.gguf"_s,
            .sizeBytes = 886381728,
            .sizeFormatted = u"~845 MiB"_s,
            .memoryFormatted = u"~1.2 GB RAM/VRAM"_s,
            .description = tr("State-of-the-art multilingual accuracy with optimized 4-layer fast decoder."),
            .installedSizeFormatted = {},
            .speedFormatted = {}},
        TranscribeModelItem {
            .id = u"parakeet-tdt-0.6b-v3"_s,
            .name = tr("Parakeet TDT 0.6B (Multilingual)"),
            .fileName = u"parakeet-tdt-0.6b-v3-Q8_0.gguf"_s,
            .downloadUrl =
                u"https://huggingface.co/handy-computer/parakeet-tdt-0.6b-v3-gguf/resolve/main/parakeet-tdt-0.6b-v3-Q8_0.gguf"_s,
            .sizeBytes = 739508576,
            .sizeFormatted = u"~705 MiB"_s,
            .memoryFormatted = u"~1.0 GB RAM/VRAM"_s,
            .description = tr("High-speed FastConformer model for European languages and English."),
            .installedSizeFormatted = {},
            .speedFormatted = {}},
        TranscribeModelItem {
            .id = u"moonshine-tiny"_s,
            .name = tr("Moonshine Tiny (English)"),
            .fileName = u"moonshine-tiny-Q8_0.gguf"_s,
            .downloadUrl =
                u"https://huggingface.co/handy-computer/moonshine-tiny-gguf/resolve/main/moonshine-tiny-Q8_0.gguf"_s,
            .sizeBytes = 35466912,
            .sizeFormatted = u"~34 MiB"_s,
            .memoryFormatted = u"~100 MB RAM/VRAM"_s,
            .description = tr("Ultra-compact edge model designed for fast desktop dictation."),
            .installedSizeFormatted = {},
            .speedFormatted = {}},
        TranscribeModelItem {
            .id = u"moonshine-base"_s,
            .name = tr("Moonshine Base (English)"),
            .fileName = u"moonshine-base-Q8_0.gguf"_s,
            .downloadUrl =
                u"https://huggingface.co/handy-computer/moonshine-base-gguf/resolve/main/moonshine-base-Q8_0.gguf"_s,
            .sizeBytes = 77476480,
            .sizeFormatted = u"~74 MiB"_s,
            .memoryFormatted = u"~180 MB RAM/VRAM"_s,
            .description = tr("Balanced edge model with higher accuracy for English speech."),
            .installedSizeFormatted = {},
            .speedFormatted = {}}};
}

void TranscribeModelManager::setupDownloaderConnections() {
    connect(m_downloader.get(), &ModelDownloader::isDownloadingAnyChanged, this,
            &TranscribeModelManager::isDownloadingAnyChanged);

    connect(m_downloader.get(), &ModelDownloader::lastErrorChanged, this, &TranscribeModelManager::setLastError);

    connect(m_downloader.get(), &ModelDownloader::downloadProgressChanged, this,
            [this](const QString& modelId, qreal progress, qint64 bytesReceived, qint64 totalBytes,
                   const QString& speedFormatted) {
                const int idx = findModelIndex(modelId);
                if (idx >= 0 && idx < m_models.size()) {
                    m_models[idx].isDownloading = true;
                    m_models[idx].progress = progress;
                    m_models[idx].bytesReceived = bytesReceived;
                    m_models[idx].totalBytes = totalBytes;
                    m_models[idx].speedFormatted = speedFormatted;

                    const QModelIndex modelIdx = index(idx);
                    emit dataChanged(
                        modelIdx, modelIdx,
                        {IsDownloadingRole, ProgressRole, BytesReceivedRole, TotalBytesRole, SpeedFormattedRole});
                }
                emit downloadProgressChanged();
            });

    connect(m_downloader.get(), &ModelDownloader::downloadFinished, this,
            [this](const QString& modelId, bool success, const QString& error) {
                const int idx = findModelIndex(modelId);
                if (idx >= 0 && idx < m_models.size()) {
                    m_models[idx].isDownloading = false;
                    if (success) {
                        m_models[idx].isInstalled = true;
                        m_models[idx].progress = 1.0;
                        const QFileInfo fi(getModelPath(modelId));
                        m_models[idx].installedSizeBytes = fi.size();
                        m_models[idx].installedSizeFormatted = formatBytes(fi.size());
                        m_models[idx].bytesReceived = fi.size();
                        m_models[idx].totalBytes = fi.size();
                        m_models[idx].speedFormatted.clear();
                    } else {
                        m_models[idx].progress = 0.0;
                        m_models[idx].bytesReceived = 0;
                        m_models[idx].speedFormatted.clear();
                    }

                    const QModelIndex modelIdx = index(idx);
                    emit dataChanged(modelIdx, modelIdx,
                                     {IsInstalledRole, ProgressRole, BytesReceivedRole, TotalBytesRole,
                                      SpeedFormattedRole, InstalledSizeFormattedRole, CanDeleteRole});

                    if (success && m_selectedModelId.isEmpty()) {
                        setSelectedModelId(modelId);
                    }
                }

                checkDiskSpace();
                emit modelStatusChanged();
                emit modelDownloadFinished(modelId, success, error);
            });
}

void TranscribeModelManager::setLastError(const QString& error) {
    if (m_lastError != error) {
        m_lastError = error;
        emit lastErrorChanged();
    }
}

int TranscribeModelManager::rowCount(const QModelIndex& parent) const {
    if (parent.isValid()) {
        return 0;
    }
    return static_cast<int>(m_models.size());
}

QVariant TranscribeModelManager::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= m_models.size()) {
        return {};
    }

    const auto& item = m_models.at(index.row());

    switch (role) {
        case IdRole:
            return item.id;
        case NameRole:
            return item.name;
        case FileNameRole:
            return item.fileName;
        case DownloadUrlRole:
            return item.downloadUrl;
        case SizeBytesRole:
            return item.sizeBytes;
        case SizeFormattedRole:
            return item.sizeFormatted;
        case MemoryFormattedRole:
            return item.memoryFormatted;
        case DescriptionRole:
            return item.description;
        case IsInstalledRole:
            return item.isInstalled;
        case IsSelectedRole:
            return item.isInstalled && (item.id == m_selectedModelId);
        case IsDownloadingRole:
            return item.isDownloading;
        case ProgressRole:
            return item.progress;
        case BytesReceivedRole:
            return item.bytesReceived;
        case TotalBytesRole:
            return item.totalBytes;
        case SpeedFormattedRole:
            return item.speedFormatted;
        case InstalledSizeFormattedRole:
            return item.installedSizeFormatted;
        case CanDeleteRole:
            return item.isInstalled;
        default:
            return {};
    }
}

QHash<int, QByteArray> TranscribeModelManager::roleNames() const {
    return {
        {IdRole, "modelId"},
        {NameRole, "name"},
        {FileNameRole, "fileName"},
        {DownloadUrlRole, "downloadUrl"},
        {SizeBytesRole, "sizeBytes"},
        {SizeFormattedRole, "sizeFormatted"},
        {MemoryFormattedRole, "memoryFormatted"},
        {DescriptionRole, "description"},
        {IsInstalledRole, "isInstalled"},
        {IsSelectedRole, "isSelected"},
        {IsDownloadingRole, "isDownloading"},
        {ProgressRole, "progress"},
        {BytesReceivedRole, "bytesReceived"},
        {TotalBytesRole, "totalBytes"},
        {SpeedFormattedRole, "speedFormatted"},
        {InstalledSizeFormattedRole, "installedSizeFormatted"},
        {CanDeleteRole, "canDelete"},
    };
}

QString TranscribeModelManager::modelsDirectory() const {
    if (!m_customModelsDirectory.isEmpty()) {
        return m_customModelsDirectory;
    }
    const QString genericData = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation);
    return genericData + u"/qtscribe/models"_s;
}

void TranscribeModelManager::setModelsDirectory(const QString& path) {
    if (m_customModelsDirectory != path) {
        m_customModelsDirectory = path;
        QDir().mkpath(modelsDirectory());
        cleanupOrphanedPartFiles();
        scanInstalledModels();
        checkDiskSpace();
        emit modelsDirectoryChanged();
    }
}

const QList<TranscribeModelItem>& TranscribeModelManager::models() const {
    return m_models;
}

std::optional<TranscribeModelItem> TranscribeModelManager::model(const QString& modelId) const {
    const int idx = findModelIndex(modelId);
    if (idx >= 0 && idx < m_models.size()) {
        return m_models.at(idx);
    }
    return std::nullopt;
}

int TranscribeModelManager::modelCount() const {
    return static_cast<int>(m_models.size());
}

int TranscribeModelManager::findModelIndex(const QString& modelId) const {
    for (int i = 0; i < m_models.size(); ++i) {
        if (m_models.at(i).id == modelId) {
            return i;
        }
    }
    return -1;
}

QString TranscribeModelManager::selectedModelId() const {
    return m_selectedModelId;
}

QString TranscribeModelManager::selectedModelPath() const {
    if (m_selectedModelId.isEmpty()) {
        return QString();
    }
    return getModelPath(m_selectedModelId);
}

QString TranscribeModelManager::selectedModelName() const {
    const auto item = model(m_selectedModelId);
    return item ? item->name : QString();
}

bool TranscribeModelManager::isSelectedModelInstalled() const {
    return isModelInstalled(m_selectedModelId);
}

bool TranscribeModelManager::hasAnyModelInstalled() const {
    return std::ranges::any_of(m_models, [](const auto& m) { return m.isInstalled; });
}

QString TranscribeModelManager::firstInstalledModelId() const {
    for (const auto& item : m_models) {
        if (item.isInstalled) {
            return item.id;
        }
    }
    return QString();
}

bool TranscribeModelManager::isDownloadingAny() const {
    return m_downloader && m_downloader->isDownloadingAny();
}

QString TranscribeModelManager::downloadingModelId() const {
    return m_downloader ? m_downloader->downloadingModelId() : QString();
}

qreal TranscribeModelManager::downloadProgress() const {
    return m_downloader ? m_downloader->downloadProgress() : 0.0;
}

QString TranscribeModelManager::downloadSpeedFormatted() const {
    return m_downloader ? m_downloader->downloadSpeedFormatted() : QString();
}

QString TranscribeModelManager::downloadBytesFormatted() const {
    return m_downloader ? m_downloader->downloadBytesFormatted() : QString();
}

QString TranscribeModelManager::lastError() const {
    return m_lastError;
}

qint64 TranscribeModelManager::availableDiskSpace() const {
    return m_availableDiskSpace;
}

QString TranscribeModelManager::availableDiskSpaceFormatted() const {
    return formatBytes(m_availableDiskSpace);
}

bool TranscribeModelManager::isModelInstalled(const QString& modelId) const {
    if (modelId.isEmpty()) {
        return false;
    }
    const QString path = getModelPath(modelId);
    return !path.isEmpty() && QFile::exists(path);
}

QString TranscribeModelManager::getModelPath(const QString& modelId) const {
    const auto item = model(modelId);
    if (!item) {
        return QString();
    }
    return modelsDirectory() + u"/"_s + item->fileName;
}

QString TranscribeModelManager::formatBytes(qint64 bytes) {
    if (bytes <= 0) {
        return u"0 B"_s;
    }
    const auto kib = static_cast<double>(1024);
    const auto mib = static_cast<double>(1024 * 1024);
    const auto gib = static_cast<double>(1024LL * 1024LL * 1024LL);

    const auto dBytes = static_cast<double>(bytes);
    if (dBytes >= gib) {
        return QString::asprintf("%.2f GiB", dBytes / gib);
    }
    if (dBytes >= mib) {
        return QString::asprintf("%.1f MiB", dBytes / mib);
    }
    if (dBytes >= kib) {
        return QString::asprintf("%.1f KiB", dBytes / kib);
    }
    return QString::asprintf("%lld B", bytes);
}

void TranscribeModelManager::setSelectedModelId(const QString& id) {
    if (m_selectedModelId == id) {
        return;
    }

    const QString oldId = m_selectedModelId;
    m_selectedModelId = id;

    if (m_settingsManager) {
        m_settingsManager->setTranscribeSelectedModel(id);
    } else {
        QSettings settings;
        settings.setValue(SettingsKeys::kTranscribeSelectedModel.toString(), id);
    }

    const int oldIdx = findModelIndex(oldId);
    const int newIdx = findModelIndex(id);
    if (oldIdx >= 0) {
        const QModelIndex idx = index(oldIdx);
        emit dataChanged(idx, idx, {IsSelectedRole});
    }
    if (newIdx >= 0) {
        const QModelIndex idx = index(newIdx);
        emit dataChanged(idx, idx, {IsSelectedRole});
    }

    emit selectedModelChanged();
}

bool TranscribeModelManager::startDownload(const QString& modelId) {
    setLastError(QString());
    const auto item = model(modelId);
    if (!item) {
        setLastError(tr("Model '%1' not found in catalog.").arg(modelId));
        return false;
    }

    if (item->sizeBytes > 0 && m_availableDiskSpace > 0 && item->sizeBytes > m_availableDiskSpace) {
        setLastError(tr("Insufficient disk space. Required: %1, Available: %2")
                         .arg(item->sizeFormatted, availableDiskSpaceFormatted()));
        return false;
    }

    const QString destinationPath = getModelPath(modelId);
    const bool started =
        m_downloader->startDownload(modelId, item->name, item->downloadUrl, destinationPath, item->sizeBytes);
    if (started) {
        const int idx = findModelIndex(modelId);
        if (idx >= 0 && idx < m_models.size()) {
            m_models[idx].isDownloading = true;
            m_models[idx].progress = 0.0;
            m_models[idx].bytesReceived = 0;
            m_models[idx].totalBytes = item->sizeBytes;
            m_models[idx].speedFormatted.clear();

            const QModelIndex modelIdx = index(idx);
            emit dataChanged(modelIdx, modelIdx,
                             {IsDownloadingRole, ProgressRole, BytesReceivedRole, TotalBytesRole, SpeedFormattedRole});
        }
        emit isDownloadingAnyChanged();
    }
    return started;
}

void TranscribeModelManager::cancelDownload(const QString& modelId) {
    const QString targetId = modelId.isEmpty() && m_downloader ? m_downloader->downloadingModelId() : modelId;
    if (m_downloader) {
        m_downloader->cancelDownload(targetId);
    }
    const int idx = findModelIndex(targetId);
    if (idx >= 0 && idx < m_models.size()) {
        m_models[idx].isDownloading = false;
        m_models[idx].progress = 0.0;
        m_models[idx].bytesReceived = 0;
        m_models[idx].speedFormatted.clear();

        const QModelIndex modelIdx = index(idx);
        emit dataChanged(modelIdx, modelIdx,
                         {IsDownloadingRole, ProgressRole, BytesReceivedRole, TotalBytesRole, SpeedFormattedRole});
    }
    emit isDownloadingAnyChanged();
}

void TranscribeModelManager::clearLastError() {
    setLastError(QString());
}

bool TranscribeModelManager::deleteModel(const QString& modelId) {
    const auto item = model(modelId);
    if (!item) {
        return false;
    }

    const QString path = getModelPath(modelId);
    if (QFile::exists(path)) {
        if (!QFile::remove(path)) {
            setLastError(tr("Failed to delete model file: %1").arg(path));
            return false;
        }
    }

    const int idx = findModelIndex(modelId);
    if (idx >= 0 && idx < m_models.size()) {
        m_models[idx].isInstalled = false;
        m_models[idx].installedSizeBytes = 0;
        m_models[idx].installedSizeFormatted.clear();

        const QModelIndex modelIdx = index(idx);
        emit dataChanged(modelIdx, modelIdx, {IsInstalledRole, InstalledSizeFormattedRole, CanDeleteRole});
    }

    if (m_selectedModelId == modelId) {
        const QString nextId = firstInstalledModelId();
        setSelectedModelId(nextId);
    }

    checkDiskSpace();
    emit modelStatusChanged();
    return true;
}

void TranscribeModelManager::refreshModelList() {
    scanInstalledModels();
    checkDiskSpace();
}

void TranscribeModelManager::checkDiskSpace() {
    const QStorageInfo storage(modelsDirectory());
    if (storage.isValid()) {
        const qint64 available = storage.bytesAvailable();
        if (m_availableDiskSpace != available) {
            m_availableDiskSpace = available;
            emit diskSpaceChanged();
        }
    }
}

void TranscribeModelManager::loadSettings() {
    QString savedId;
    if (m_settingsManager) {
        savedId = m_settingsManager->transcribeConfig().selectedModel;
    } else {
        QSettings settings;
        savedId = settings.value(SettingsKeys::kTranscribeSelectedModel.toString(), QString()).toString();
    }

    if (savedId.isEmpty() || !isModelInstalled(savedId)) {
        savedId = firstInstalledModelId();
        if (savedId.isEmpty()) {
            savedId = kDefaultSelectedModel;
        }
    }

    setSelectedModelId(savedId);
}

void TranscribeModelManager::scanInstalledModels() {
    bool changed = false;
    for (int i = 0; i < m_models.size(); ++i) {
        const QString path = modelsDirectory() + u"/"_s + m_models[i].fileName;
        const QFileInfo fi(path);
        const bool installed = fi.exists() && fi.size() > 0;
        if (m_models[i].isInstalled != installed || m_models[i].installedSizeBytes != fi.size()) {
            m_models[i].isInstalled = installed;
            m_models[i].installedSizeBytes = installed ? fi.size() : 0;
            m_models[i].installedSizeFormatted = installed ? formatBytes(fi.size()) : QString();
            changed = true;
            const QModelIndex idx = index(i);
            emit dataChanged(idx, idx, {IsInstalledRole, InstalledSizeFormattedRole, CanDeleteRole});
        }
    }

    if (changed) {
        emit modelStatusChanged();
        if (!isModelInstalled(m_selectedModelId)) {
            const QString first = firstInstalledModelId();
            if (!first.isEmpty()) {
                setSelectedModelId(first);
            }
        }
    }
}

void TranscribeModelManager::cleanupOrphanedPartFiles() const {
    const QDir dir(modelsDirectory());
    const auto partFiles = dir.entryList({u"*.part"_s}, QDir::Files);
    for (const auto& part : partFiles) {
        QFile::remove(dir.filePath(part));
    }
}
