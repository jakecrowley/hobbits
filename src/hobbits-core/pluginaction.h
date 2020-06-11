#ifndef PLUGINACTION_H
#define PLUGINACTION_H

#include "actionwatcher.h"
#include "analyzeractor.h"
#include "operatoractor.h"
#include "hobbitspluginmanager.h"
#include <QJsonObject>
#include <QtConcurrent/QtConcurrentRun>

#include "hobbits-core_global.h"

class BitContainerManager;
class HOBBITSCORESHARED_EXPORT PluginAction
{
public:
    enum PluginType {
        Framer = 1 /*Deprecated*/,
        Operator = 2,
        Analyzer = 3,
        Importer = 4,
        Exporter = 5
    };

    PluginAction(PluginType pluginType, QString pluginName, QJsonObject pluginState);

    PluginType getPluginType() const;
    QString getPluginName() const;
    QJsonObject getPluginState() const;

    int minPossibleInputs(QSharedPointer<const HobbitsPluginManager> pluginManager) const;
    int maxPossibleInputs(QSharedPointer<const HobbitsPluginManager> pluginManager) const;

    QJsonObject serialize() const;

    static QSharedPointer<PluginAction> deserialize(QJsonObject data);

    QSharedPointer<ActionWatcher<QSharedPointer<const OperatorResult>>> operatorAct(
            QSharedPointer<OperatorActor> actor,
            QSharedPointer<const HobbitsPluginManager> pluginManager,
            QList<QSharedPointer<BitContainer>> inputContainers,
            QSharedPointer<BitContainerManager> bitContainerManager,
            QString outputName = "",
            QMap<int, QUuid> outputIdMap = QMap<int, QUuid>()) const;

    QSharedPointer<ActionWatcher<QSharedPointer<const AnalyzerResult>>> analyzerAct(
            QSharedPointer<AnalyzerActor> actor,
            QSharedPointer<const HobbitsPluginManager> pluginManager,
            QSharedPointer<BitContainer> container) const;

    QSharedPointer<ImportExportResult> importAct(QSharedPointer<const HobbitsPluginManager> pluginManager, QWidget* parent = nullptr) const;

    QSharedPointer<ImportExportResult> exportAct(QSharedPointer<const HobbitsPluginManager> pluginManager, QSharedPointer<BitContainer> container, QWidget* parent = nullptr) const;

    inline bool operator==(const PluginAction &other) const
    {
        return (
            m_pluginName == other.getPluginName()
            && m_pluginType == other.getPluginType()
            && m_pluginState == other.getPluginState()
            );
    }

private:
    PluginType m_pluginType;
    QString m_pluginName;
    QJsonObject m_pluginState;
};


inline uint qHash(const PluginAction &key, uint seed)
{
    return qHash(key.getPluginState(), seed) ^ uint(key.getPluginType()) ^ qHash(key.getPluginName(), seed);
}

#endif // PLUGINACTION_H
