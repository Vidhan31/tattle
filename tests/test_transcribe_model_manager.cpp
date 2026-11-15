#include "ModelDownloader.h"
#include "TranscribeModelManager.h"

#include <QFile>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>

using namespace Qt::StringLiterals;

class TranscribeModelManagerTest : public QObject {
    Q_OBJECT

private slots:
    void testPresetsCountAndMetadata() {
        TranscribeModelManager manager;
        QCOMPARE(manager.modelCount(), 9);
        QCOMPARE(manager.models().size(), 9);

        for (const auto& item : manager.models()) {
            QVERIFY(!item.id.isEmpty());
            QVERIFY(!item.name.isEmpty());
            QVERIFY(!item.fileName.isEmpty());
            QVERIFY(item.fileName.endsWith(u".gguf"_s));
            QVERIFY(item.downloadUrl.startsWith(u"https://huggingface.co/"_s));
            QVERIFY(item.sizeBytes > 0);
            QVERIFY(!item.sizeFormatted.isEmpty());
            QVERIFY(!item.memoryFormatted.isEmpty());
            QVERIFY(!item.description.isEmpty());
        }

        const int tinyIdx = manager.findModelIndex(u"whisper-tiny.en"_s);
        QVERIFY(tinyIdx >= 0);

        const auto modelOpt = manager.model(u"whisper-tiny.en"_s);
        QVERIFY(modelOpt.has_value());
        QCOMPARE(modelOpt->id, u"whisper-tiny.en"_s);
        QCOMPARE(modelOpt->fileName, u"whisper-tiny.en-Q8_0.gguf"_s);

        const auto invalidOpt = manager.model(u"non_existent_model"_s);
        QVERIFY(!invalidOpt.has_value());
        QCOMPARE(manager.findModelIndex(u"non_existent_model"_s), -1);
    }

    void testModelDownloaderBasics() {
        ModelDownloader downloader;
        QVERIFY(!downloader.isDownloadingAny());
        QVERIFY(downloader.downloadingModelId().isEmpty());
        QCOMPARE(downloader.downloadProgress(), 0.0);
        QCOMPARE(downloader.downloadBytesReceived(), 0);
        QCOMPARE(downloader.downloadTotalBytes(), 0);
        QVERIFY(downloader.downloadSpeedFormatted().isEmpty());
        QVERIFY(downloader.downloadBytesFormatted().isEmpty());

        downloader.cancelDownload(u"non_existent"_s);
        QVERIFY(!downloader.isDownloadingAny());

        QCOMPARE(ModelDownloader::formatBytes(0), u"0 B"_s);
        QCOMPARE(ModelDownloader::formatBytes(512), u"512 B"_s);
        QCOMPARE(ModelDownloader::formatBytes(1024), u"1.0 KiB"_s);
        QCOMPARE(ModelDownloader::formatBytes(1024 * 1024), u"1.0 MiB"_s);
        QCOMPARE(ModelDownloader::formatBytes(1024LL * 1024LL * 1024LL), u"1.00 GiB"_s);
    }

    void testCustomDirectoryAndScanning() {
        QTemporaryDir tempDir;
        QVERIFY(tempDir.isValid());

        TranscribeModelManager manager;
        manager.setModelsDirectory(tempDir.path());
        QCOMPARE(manager.modelsDirectory(), tempDir.path());

        QVERIFY(!manager.isModelInstalled(u"whisper-tiny.en"_s));

        const QString filePath = tempDir.path() + u"/whisper-tiny.en-Q8_0.gguf"_s;
        {
            QFile file(filePath);
            QVERIFY(file.open(QIODevice::WriteOnly));
            constexpr quint32 kGgufMagic = 0x46554747;
            file.write(reinterpret_cast<const char*>(&kGgufMagic), sizeof(kGgufMagic));
            file.write("dummy_gguf_content_bytes");
            file.close();
        }

        QSignalSpy statusSpy(&manager, &TranscribeModelManager::modelStatusChanged);
        QSignalSpy dataSpy(&manager, &QAbstractItemModel::dataChanged);
        manager.scanInstalledModels();

        QVERIFY(manager.isModelInstalled(u"whisper-tiny.en"_s));
        QCOMPARE(manager.getModelPath(u"whisper-tiny.en"_s), filePath);
        QVERIFY(statusSpy.count() >= 1);
        QVERIFY(dataSpy.count() >= 1);
    }

    void testModelSelection() {
        QTemporaryDir tempDir;
        QVERIFY(tempDir.isValid());

        TranscribeModelManager manager;
        manager.setModelsDirectory(tempDir.path());

        const QString tinyPath = tempDir.path() + u"/whisper-tiny.en-Q8_0.gguf"_s;
        const QString basePath = tempDir.path() + u"/whisper-base.en-Q8_0.gguf"_s;
        {
            QFile file(tinyPath);
            QVERIFY(file.open(QIODevice::WriteOnly));
            file.write("dummy_tiny_data");
            file.close();
        }
        {
            QFile file(basePath);
            QVERIFY(file.open(QIODevice::WriteOnly));
            file.write("dummy_base_data");
            file.close();
        }
        manager.scanInstalledModels();
        QVERIFY(manager.isModelInstalled(u"whisper-tiny.en"_s));
        QVERIFY(manager.isModelInstalled(u"whisper-base.en"_s));

        QCOMPARE(manager.selectedModelId(), u"whisper-base.en"_s);

        QSignalSpy selectSpy(&manager, &TranscribeModelManager::selectedModelChanged);
        manager.setSelectedModelId(u"whisper-tiny.en"_s);

        QCOMPARE(manager.selectedModelId(), u"whisper-tiny.en"_s);
        QCOMPARE(manager.selectedModelPath(), tinyPath);
        QVERIFY(manager.isSelectedModelInstalled());
        QCOMPARE(selectSpy.count(), 1);
    }

    void testModelDeletion() {
        QTemporaryDir tempDir;
        QVERIFY(tempDir.isValid());

        TranscribeModelManager manager;
        manager.setModelsDirectory(tempDir.path());

        const QString filePath = tempDir.path() + u"/whisper-tiny.en-Q8_0.gguf"_s;
        {
            QFile file(filePath);
            QVERIFY(file.open(QIODevice::WriteOnly));
            file.write("dummy");
            file.close();
        }

        manager.scanInstalledModels();
        QVERIFY(manager.isModelInstalled(u"whisper-tiny.en"_s));

        QSignalSpy statusSpy(&manager, &TranscribeModelManager::modelStatusChanged);
        const bool deleted = manager.deleteModel(u"whisper-tiny.en"_s);

        QVERIFY(deleted);
        QVERIFY(!QFile::exists(filePath));
        QVERIFY(!manager.isModelInstalled(u"whisper-tiny.en"_s));
        QCOMPARE(statusSpy.count(), 1);
    }

    void testOrphanPartFileCleanup() {
        QTemporaryDir tempDir;
        QVERIFY(tempDir.isValid());

        const QString partFile = tempDir.path() + u"/whisper-base.en-Q8_0.gguf.part"_s;
        {
            QFile file(partFile);
            QVERIFY(file.open(QIODevice::WriteOnly));
            file.write("incomplete_chunk");
            file.close();
        }
        QVERIFY(QFile::exists(partFile));

        TranscribeModelManager manager;
        manager.setModelsDirectory(tempDir.path());

        QVERIFY(!QFile::exists(partFile));
    }

    void testQAbstractListModelInterface() {
        TranscribeModelManager manager;
        QCOMPARE(manager.rowCount(), 9);

        const QModelIndex firstIndex = manager.index(0);
        QVERIFY(firstIndex.isValid());
        QCOMPARE(manager.data(firstIndex, TranscribeModelManager::IdRole).toString(), u"whisper-tiny.en"_s);
        QCOMPARE(manager.data(firstIndex, TranscribeModelManager::FileNameRole).toString(), u"whisper-tiny.en-Q8_0.gguf"_s);
        QCOMPARE(manager.data(firstIndex, TranscribeModelManager::CanDeleteRole).toBool(), false);

        const auto roles = manager.roleNames();
        QCOMPARE(roles.value(TranscribeModelManager::IdRole), "modelId");
        QCOMPARE(roles.value(TranscribeModelManager::NameRole), "name");
        QCOMPARE(roles.value(TranscribeModelManager::FileNameRole), "fileName");
        QCOMPARE(roles.value(TranscribeModelManager::DownloadUrlRole), "downloadUrl");
        QCOMPARE(roles.value(TranscribeModelManager::SizeBytesRole), "sizeBytes");
        QCOMPARE(roles.value(TranscribeModelManager::SizeFormattedRole), "sizeFormatted");
        QCOMPARE(roles.value(TranscribeModelManager::IsInstalledRole), "isInstalled");
        QCOMPARE(roles.value(TranscribeModelManager::IsSelectedRole), "isSelected");
        QCOMPARE(roles.value(TranscribeModelManager::IsDownloadingRole), "isDownloading");
    }

    void testFormatBytes() {
        QCOMPARE(TranscribeModelManager::formatBytes(0), u"0 B"_s);
        QCOMPARE(TranscribeModelManager::formatBytes(512), u"512 B"_s);
        QCOMPARE(TranscribeModelManager::formatBytes(1024), u"1.0 KiB"_s);
        QCOMPARE(TranscribeModelManager::formatBytes(1024 * 1024), u"1.0 MiB"_s);
        QCOMPARE(TranscribeModelManager::formatBytes(1024LL * 1024LL * 1024LL), u"1.00 GiB"_s);
    }
};

QTEST_GUILESS_MAIN(TranscribeModelManagerTest)
#include "test_transcribe_model_manager.moc"
