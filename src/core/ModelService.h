#pragma once

#include <QObject>
#include <QString>

class TranscribeModelManager;
class TranscribeSttClient;
class DictationCoordinator;

class ModelService : public QObject {
    Q_OBJECT

    Q_PROPERTY(bool isModelLoaded READ isModelLoaded NOTIFY modelStatusChanged FINAL)
    Q_PROPERTY(bool isModelBusy READ isModelBusy NOTIFY modelStatusChanged FINAL)
    Q_PROPERTY(QString loadedModelPath READ loadedModelPath NOTIFY modelStatusChanged FINAL)
    Q_PROPERTY(QString modelPath READ modelPath NOTIFY modelStatusChanged FINAL)
    Q_PROPERTY(QString modelFileName READ modelFileName NOTIFY modelStatusChanged FINAL)
    Q_PROPERTY(QString computeDevice READ computeDevice NOTIFY computeDeviceChanged FINAL)
    Q_PROPERTY(bool isVulkanSupported READ isVulkanSupported CONSTANT FINAL)
    Q_PROPERTY(bool isModelInstalled READ isModelInstalled NOTIFY modelStatusChanged FINAL)

    Q_PROPERTY(QString modelsDirectory READ modelsDirectory NOTIFY modelCatalogChanged FINAL)
    Q_PROPERTY(QString selectedModelName READ selectedModelName NOTIFY modelCatalogChanged FINAL)
    Q_PROPERTY(QString modelCatalogJson READ modelCatalogJson NOTIFY modelCatalogChanged FINAL)
    Q_PROPERTY(bool modelDownloadingAny READ modelDownloadingAny NOTIFY modelCatalogChanged FINAL)
    Q_PROPERTY(QString modelDownloadBytes READ modelDownloadBytes NOTIFY modelCatalogChanged FINAL)
    Q_PROPERTY(QString modelCatalogLastError READ modelCatalogLastError NOTIFY modelCatalogChanged FINAL)
    Q_PROPERTY(QString modelDiskSpace READ modelDiskSpace NOTIFY modelCatalogChanged FINAL)

public:
    explicit ModelService(QObject* parent = nullptr);
    ~ModelService() override = default;

    void setModelManager(TranscribeModelManager* manager);
    [[nodiscard]] TranscribeModelManager* modelManager() const noexcept;

    void setTranscribeClient(TranscribeSttClient* client);
    [[nodiscard]] TranscribeSttClient* transcribeClient() const noexcept;

    void setDictationCoordinator(DictationCoordinator* coordinator);
    [[nodiscard]] DictationCoordinator* dictationCoordinator() const noexcept;

    [[nodiscard]] bool isModelLoaded() const;
    [[nodiscard]] bool isModelBusy() const;
    [[nodiscard]] QString loadedModelPath() const;
    [[nodiscard]] QString modelPath() const;
    [[nodiscard]] QString modelFileName() const;
    [[nodiscard]] QString computeDevice() const;
    [[nodiscard]] static bool isVulkanSupported();
    [[nodiscard]] bool isModelInstalled() const;

    [[nodiscard]] QString modelsDirectory() const;
    [[nodiscard]] QString selectedModelName() const;
    [[nodiscard]] QString modelCatalogJson() const;
    [[nodiscard]] bool modelDownloadingAny() const;
    [[nodiscard]] QString modelDownloadBytes() const;
    [[nodiscard]] QString modelCatalogLastError() const;
    [[nodiscard]] QString modelDiskSpace() const;

    void loadModel(const QString& customPath = QString());
    void unloadModel();
    void checkModelStatus();

    void selectModel(const QString& modelId);
    bool startModelDownload(const QString& modelId);
    void cancelModelDownload(const QString& modelId);
    bool deleteModel(const QString& modelId);
    void refreshModelCatalog();
    void checkModelDiskSpace();
    void clearModelCatalogLastError();

signals:
    void modelStatusChanged();
    void computeDeviceChanged();
    void modelCatalogChanged();

private:
    TranscribeModelManager* m_modelManager = nullptr;
    TranscribeSttClient* m_transcribeClient = nullptr;
    DictationCoordinator* m_coordinator = nullptr;
};
