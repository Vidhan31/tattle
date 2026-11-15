#include "ModelService.h"

#include "DictationCoordinator.h"
#include "LoggingCategories.h"

#include "TranscribeModelManager.h"
#include "TranscribeSttClient.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

using namespace Qt::StringLiterals;

ModelService::ModelService(QObject* parent)
    : QObject(parent) { }

void ModelService::setModelManager(TranscribeModelManager* manager) {
    if (m_modelManager == manager) {
        return;
    }

    if (m_modelManager) {
        disconnect(m_modelManager, nullptr, this, nullptr);
    }

    m_modelManager = manager;

    if (m_modelManager) {
        connect(m_modelManager, &QAbstractItemModel::dataChanged, this, &ModelService::modelCatalogChanged);
        connect(m_modelManager, &QAbstractItemModel::modelReset, this, &ModelService::modelCatalogChanged);
        connect(m_modelManager, &TranscribeModelManager::modelsChanged, this, &ModelService::modelCatalogChanged);
        connect(m_modelManager, &TranscribeModelManager::selectedModelChanged, this,
                &ModelService::modelCatalogChanged);
        connect(m_modelManager, &TranscribeModelManager::modelStatusChanged, this, &ModelService::modelCatalogChanged);
        connect(m_modelManager, &TranscribeModelManager::isDownloadingAnyChanged, this,
                &ModelService::modelCatalogChanged);
        connect(m_modelManager, &TranscribeModelManager::downloadProgressChanged, this,
                &ModelService::modelCatalogChanged);
        connect(m_modelManager, &TranscribeModelManager::lastErrorChanged, this, &ModelService::modelCatalogChanged);
        connect(m_modelManager, &TranscribeModelManager::diskSpaceChanged, this, &ModelService::modelCatalogChanged);
    }

    emit modelCatalogChanged();
}

TranscribeModelManager* ModelService::modelManager() const noexcept {
    return m_modelManager;
}

void ModelService::setTranscribeClient(TranscribeSttClient* client) {
    if (m_transcribeClient == client) {
        return;
    }

    if (m_transcribeClient) {
        disconnect(m_transcribeClient, nullptr, this, nullptr);
    }

    m_transcribeClient = client;

    if (m_transcribeClient) {
        connect(m_transcribeClient, &TranscribeSttClient::modelStatusChanged, this, &ModelService::modelStatusChanged);
        connect(m_transcribeClient, &AbstractSttClient::busyChanged, this, &ModelService::modelStatusChanged);
        connect(m_transcribeClient, &TranscribeSttClient::computeDeviceChanged, this,
                &ModelService::computeDeviceChanged);
    }

    emit modelStatusChanged();
    emit computeDeviceChanged();
}

TranscribeSttClient* ModelService::transcribeClient() const noexcept {
    return m_transcribeClient;
}

void ModelService::setDictationCoordinator(DictationCoordinator* coordinator) {
    if (m_coordinator == coordinator) {
        return;
    }

    if (m_coordinator) {
        disconnect(m_coordinator, &DictationCoordinator::dictationStateChanged, this,
                   &ModelService::modelStatusChanged);
    }

    m_coordinator = coordinator;

    if (m_coordinator) {
        connect(m_coordinator, &DictationCoordinator::dictationStateChanged, this, &ModelService::modelStatusChanged);
    }

    emit modelStatusChanged();
}

DictationCoordinator* ModelService::dictationCoordinator() const noexcept {
    return m_coordinator;
}

bool ModelService::isModelLoaded() const {
    return m_transcribeClient ? m_transcribeClient->isModelLoaded() : false;
}

bool ModelService::isModelBusy() const {
    const bool coordinatorBusy = m_coordinator ? m_coordinator->isBusy() : false;
    const bool transcribeBusy = m_transcribeClient ? m_transcribeClient->isBusy() : false;
    return coordinatorBusy || transcribeBusy;
}

QString ModelService::loadedModelPath() const {
    return m_transcribeClient ? m_transcribeClient->loadedModelPath() : QString();
}

QString ModelService::modelPath() const {
    return m_transcribeClient ? m_transcribeClient->modelPath() : QString();
}

QString ModelService::modelFileName() const {
    return m_transcribeClient ? m_transcribeClient->modelFileName() : QString();
}

QString ModelService::computeDevice() const {
    return m_transcribeClient ? m_transcribeClient->computeDevice() : QString();
}

bool ModelService::isVulkanSupported() {
    return TranscribeSttClient::isVulkanSupported();
}

bool ModelService::isModelInstalled() const {
    return m_transcribeClient ? m_transcribeClient->isModelInstalled() : false;
}

QString ModelService::modelsDirectory() const {
    return m_modelManager ? m_modelManager->modelsDirectory() : QString();
}

QString ModelService::selectedModelName() const {
    return m_modelManager ? m_modelManager->selectedModelName() : QString();
}

QString ModelService::modelCatalogJson() const {
    if (!m_modelManager) {
        return QStringLiteral("[]");
    }

    QJsonArray catalog;
    const QString selectedId = m_modelManager->selectedModelId();
    for (const auto& item : m_modelManager->models()) {
        QJsonObject entry;
        entry[u"modelId"_s] = item.id;
        entry[u"name"_s] = item.name;
        entry[u"fileName"_s] = item.fileName;
        entry[u"downloadUrl"_s] = item.downloadUrl;
        entry[u"sizeBytes"_s] = item.sizeBytes;
        entry[u"sizeFormatted"_s] = item.sizeFormatted;
        entry[u"memoryFormatted"_s] = item.memoryFormatted;
        entry[u"description"_s] = item.description;
        entry[u"isInstalled"_s] = item.isInstalled;
        entry[u"isSelected"_s] = item.isInstalled && item.id == selectedId;
        entry[u"isDownloading"_s] = item.isDownloading;
        entry[u"progress"_s] = item.progress;
        entry[u"bytesReceived"_s] = item.bytesReceived;
        entry[u"totalBytes"_s] = item.totalBytes;
        entry[u"speedFormatted"_s] = item.speedFormatted;
        entry[u"installedSizeFormatted"_s] = item.installedSizeFormatted;
        entry[u"canDelete"_s] = item.isInstalled;
        catalog.append(entry);
    }
    return QString::fromUtf8(QJsonDocument(catalog).toJson(QJsonDocument::Compact));
}

bool ModelService::modelDownloadingAny() const {
    return m_modelManager && m_modelManager->isDownloadingAny();
}

QString ModelService::modelDownloadBytes() const {
    return m_modelManager ? m_modelManager->downloadBytesFormatted() : QString();
}

QString ModelService::modelCatalogLastError() const {
    return m_modelManager ? m_modelManager->lastError() : QString();
}

QString ModelService::modelDiskSpace() const {
    return m_modelManager ? m_modelManager->availableDiskSpaceFormatted() : QString();
}

void ModelService::loadModel(const QString& customPath) {
    if (isModelBusy()) {
        qCWarning(lcSpeech) << "ModelService: Rejecting loadModel while dictation is active or model is busy";
        return;
    }
    if (m_transcribeClient) {
        m_transcribeClient->loadModel(customPath);
    }
}

void ModelService::unloadModel() {
    if (isModelBusy()) {
        qCWarning(lcSpeech) << "ModelService: Rejecting unloadModel while dictation is active or model is busy";
        return;
    }
    if (m_transcribeClient) {
        m_transcribeClient->unloadModel();
    }
}

void ModelService::checkModelStatus() {
    if (m_transcribeClient) {
        m_transcribeClient->checkModelStatus();
    }
}

void ModelService::selectModel(const QString& modelId) {
    if (isModelBusy()) {
        qCWarning(lcSpeech) << "ModelService: Rejecting selectModel while dictation is active or model is busy";
        return;
    }
    if (m_modelManager) {
        m_modelManager->setSelectedModelId(modelId);
    }
}

bool ModelService::startModelDownload(const QString& modelId) {
    return m_modelManager && m_modelManager->startDownload(modelId);
}

void ModelService::cancelModelDownload(const QString& modelId) {
    if (m_modelManager) {
        m_modelManager->cancelDownload(modelId);
    }
}

bool ModelService::deleteModel(const QString& modelId) {
    if (isModelBusy()) {
        qCWarning(lcSpeech) << "ModelService: Rejecting deleteModel while dictation is active or model is busy";
        return false;
    }
    return m_modelManager && m_modelManager->deleteModel(modelId);
}

void ModelService::refreshModelCatalog() {
    if (m_modelManager) {
        m_modelManager->refreshModelList();
    }
}

void ModelService::checkModelDiskSpace() {
    if (m_modelManager) {
        m_modelManager->checkDiskSpace();
    }
}

void ModelService::clearModelCatalogLastError() {
    if (m_modelManager) {
        m_modelManager->clearLastError();
    }
}
