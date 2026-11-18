#include "ModelCatalogModel.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSignalSpy>
#include <QTest>

using namespace Qt::StringLiterals;

namespace {

QJsonObject catalogEntry(const QString& id, const QString& name, bool installed, bool selected, bool downloading,
                         double progress, qint64 bytesReceived) {
    QJsonObject entry;
    entry[u"modelId"_s] = id;
    entry[u"name"_s] = name;
    entry[u"fileName"_s] = u"ggml-%1.bin"_s.arg(id);
    entry[u"downloadUrl"_s] = u"https://example.invalid/%1.bin"_s.arg(id);
    entry[u"sizeBytes"_s] = 1024;
    entry[u"sizeFormatted"_s] = u"1 KiB"_s;
    entry[u"memoryFormatted"_s] = u"1 MiB"_s;
    entry[u"description"_s] = u"Test model"_s;
    entry[u"isInstalled"_s] = installed;
    entry[u"isSelected"_s] = selected;
    entry[u"isDownloading"_s] = downloading;
    entry[u"progress"_s] = progress;
    entry[u"bytesReceived"_s] = bytesReceived;
    entry[u"totalBytes"_s] = 1024;
    entry[u"speedFormatted"_s] = u"1 MiB/s"_s;
    entry[u"installedSizeFormatted"_s] = u"1 MiB"_s;
    entry[u"canDelete"_s] = installed;
    return entry;
}

QString catalogJson(const QJsonArray& models) {
    return QString::fromUtf8(QJsonDocument(models).toJson(QJsonDocument::Compact));
}

QJsonArray initialCatalog() {
    return {catalogEntry(u"tiny.en"_s, u"Tiny"_s, false, true, true, 0.1, 100),
            catalogEntry(u"base.en"_s, u"Base"_s, true, false, false, 0.0, 0)};
}

} // namespace

class TestModelCatalogModel : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void testProgressUpdatesInPlace();
    void testSelectionUpdatesInPlace();
    void testStructuralChangeResetsModel();
    void testIdenticalSnapshotIsSilent();
    void testPrefixCountsAndMatching();
    void testRevisionIncrementsOnUpdate();
};

void TestModelCatalogModel::initTestCase() {
    QCoreApplication::setApplicationName(u"TattleTestApp_ModelCatalogModel"_s);
    qputenv("TATTLE_DISABLE_REMOTE_ATTACH", "1");
}

void TestModelCatalogModel::testProgressUpdatesInPlace() {
    ModelCatalogModel model;
    QSignalSpy resetSpy(&model, &QAbstractItemModel::modelReset);
    QSignalSpy dataSpy(&model, &QAbstractItemModel::dataChanged);
    QSignalSpy progressSpy(&model, &ModelCatalogModel::downloadProgressChanged);

    model.applyCatalogJson(catalogJson(initialCatalog()));
    QCOMPARE(model.rowCount(), 2);

    resetSpy.clear();
    dataSpy.clear();
    progressSpy.clear();

    QJsonArray updated = initialCatalog();
    updated[0] = catalogEntry(u"tiny.en"_s, u"Tiny"_s, false, true, true, 0.35, 350);
    model.applyCatalogJson(catalogJson(updated));

    QCOMPARE(resetSpy.count(), 0);
    QCOMPARE(dataSpy.count(), 1);
    QCOMPARE(progressSpy.count(), 1);

    const QList<QVariant> arguments = dataSpy.at(0);
    QCOMPARE(arguments.at(0).toModelIndex().row(), 0);
    QCOMPARE(arguments.at(1).toModelIndex().row(), 0);

    const QList<int> roles = arguments.at(2).value<QList<int>>();
    QVERIFY(roles.contains(ModelCatalogModel::ProgressRole));
    QVERIFY(roles.contains(ModelCatalogModel::BytesReceivedRole));
    QVERIFY(!roles.contains(ModelCatalogModel::IsDownloadingRole));

    QCOMPARE(model.data(model.index(0, 0), ModelCatalogModel::ProgressRole).toDouble(), 0.35);
    QCOMPARE(model.data(model.index(0, 0), ModelCatalogModel::BytesReceivedRole).toLongLong(), qint64(350));
}

void TestModelCatalogModel::testSelectionUpdatesInPlace() {
    ModelCatalogModel model;
    QSignalSpy resetSpy(&model, &QAbstractItemModel::modelReset);
    QSignalSpy dataSpy(&model, &QAbstractItemModel::dataChanged);
    QSignalSpy selectedSpy(&model, &ModelCatalogModel::selectedModelNameChanged);

    model.applyCatalogJson(catalogJson(initialCatalog()));
    QCOMPARE(model.selectedModelName(), u"Tiny"_s);

    resetSpy.clear();
    dataSpy.clear();
    selectedSpy.clear();

    QJsonArray updated = initialCatalog();
    updated[0] = catalogEntry(u"tiny.en"_s, u"Tiny"_s, true, false, false, 0.0, 0);
    updated[1] = catalogEntry(u"base.en"_s, u"Base"_s, true, true, false, 0.0, 0);
    model.applyCatalogJson(catalogJson(updated));

    QCOMPARE(resetSpy.count(), 0);
    QCOMPARE(dataSpy.count(), 2);
    QCOMPARE(selectedSpy.count(), 1);
    QCOMPARE(model.selectedModelName(), u"Base"_s);
}

void TestModelCatalogModel::testStructuralChangeResetsModel() {
    ModelCatalogModel model;
    QSignalSpy resetSpy(&model, &QAbstractItemModel::modelReset);
    QSignalSpy dataSpy(&model, &QAbstractItemModel::dataChanged);

    model.applyCatalogJson(catalogJson(initialCatalog()));
    resetSpy.clear();
    dataSpy.clear();

    QJsonArray expanded = initialCatalog();
    expanded.append(catalogEntry(u"small.en"_s, u"Small"_s, false, false, false, 0.0, 0));
    model.applyCatalogJson(catalogJson(expanded));

    QCOMPARE(resetSpy.count(), 1);
    QCOMPARE(dataSpy.count(), 0);
    QCOMPARE(model.rowCount(), 3);
}

void TestModelCatalogModel::testIdenticalSnapshotIsSilent() {
    ModelCatalogModel model;
    QSignalSpy resetSpy(&model, &QAbstractItemModel::modelReset);
    QSignalSpy dataSpy(&model, &QAbstractItemModel::dataChanged);
    QSignalSpy progressSpy(&model, &ModelCatalogModel::downloadProgressChanged);

    const QString snapshot = catalogJson(initialCatalog());
    model.applyCatalogJson(snapshot);
    resetSpy.clear();
    dataSpy.clear();
    progressSpy.clear();

    model.applyCatalogJson(snapshot);

    QCOMPARE(resetSpy.count(), 0);
    QCOMPARE(dataSpy.count(), 0);
    QCOMPARE(progressSpy.count(), 0);
}

void TestModelCatalogModel::testPrefixCountsAndMatching() {
    ModelCatalogModel model;
    QJsonArray models = {
        catalogEntry(u"whisper-tiny"_s, u"Whisper Tiny (Multilingual)"_s, true, true, false, 0.0, 0),
        catalogEntry(u"whisper-base"_s, u"Whisper Base (Multilingual)"_s, false, false, false, 0.0, 0),
        catalogEntry(u"whisper-small"_s, u"Whisper Small (Multilingual)"_s, false, false, false, 0.0, 0),
        catalogEntry(u"moonshine-tiny"_s, u"Moonshine Tiny (English)"_s, false, false, false, 0.0, 0),
        catalogEntry(u"moonshine-base"_s, u"Moonshine Base (English)"_s, true, false, false, 0.0, 0),
        catalogEntry(u"parakeet-tdt"_s, u"Parakeet TDT (Multilingual)"_s, false, false, false, 0.0, 0),
    };
    model.applyCatalogJson(catalogJson(models));

    // Whisper counts
    QCOMPARE(model.totalCountForPrefix(u"whisper-"_s), 3);
    QCOMPARE(model.installedCountForPrefix(u"whisper-"_s), 1);
    QCOMPARE(model.availableCountForPrefix(u"whisper-"_s), 2);

    // Moonshine counts
    QCOMPARE(model.totalCountForPrefix(u"moonshine-"_s), 2);
    QCOMPARE(model.installedCountForPrefix(u"moonshine-"_s), 1);
    QCOMPARE(model.availableCountForPrefix(u"moonshine-"_s), 1);

    // Parakeet counts
    QCOMPARE(model.totalCountForPrefix(u"parakeet-"_s), 1);
    QCOMPARE(model.installedCountForPrefix(u"parakeet-"_s), 0);
    QCOMPARE(model.availableCountForPrefix(u"parakeet-"_s), 1);

    // Total counts across all families
    QCOMPARE(model.totalCountForPrefix(QString()), 6);
    QCOMPARE(model.totalCountForPrefix(u""_s), 6);

    // Matching with status filter
    QCOMPARE(model.matchingCount(u"whisper-"_s, u"all"_s, QString()), 3);
    QCOMPARE(model.matchingCount(u"whisper-"_s, u"installed"_s, QString()), 1);
    QCOMPARE(model.matchingCount(u"whisper-"_s, u"available"_s, QString()), 2);

    // Matching with search filter
    QCOMPARE(model.matchingCount(u"whisper-"_s, u"all"_s, u"tiny"_s), 1);
    QCOMPARE(model.matchingCount(u"moonshine-"_s, u"all"_s, u"tiny"_s), 1);
    QCOMPARE(model.matchingCount(u"parakeet-"_s, u"all"_s, u"tiny"_s), 0);
}

void TestModelCatalogModel::testRevisionIncrementsOnUpdate() {
    ModelCatalogModel model;
    QSignalSpy revisionSpy(&model, &ModelCatalogModel::revisionChanged);
    const int initialRev = model.revision();

    model.applyCatalogJson(catalogJson(initialCatalog()));
    QCOMPARE(model.revision(), initialRev + 1);
    QCOMPARE(revisionSpy.count(), 1);

    QJsonArray updated = initialCatalog();
    updated[0] = catalogEntry(u"tiny.en"_s, u"Tiny"_s, false, true, true, 0.5, 500);
    model.applyCatalogJson(catalogJson(updated));
    QCOMPARE(model.revision(), initialRev + 2);
    QCOMPARE(revisionSpy.count(), 2);
}

QTEST_MAIN(TestModelCatalogModel)

#include "test_model_catalog_model.moc"
