#include "operatorrunner.h"
#include "pluginaction.h"
#include "pluginactionlineage.h"
#include "pluginactionmanager.h"
#include "settingsmanager.h"

QSharedPointer<OperatorRunner> OperatorRunner::create(
        QSharedPointer<const HobbitsPluginManager> pluginManager,
        QSharedPointer<PluginAction> action)
{
    if (action->getPluginType() != PluginAction::Operator) {
        return nullptr;
    }
    auto plugin = pluginManager->getOperator(action->getPluginName());
    if (!plugin) {
        return nullptr;
    }

    auto runner = QSharedPointer<OperatorRunner>(new OperatorRunner());

    runner->m_id = QUuid::createUuid();
    runner->m_op = plugin;
    runner->m_action = action;
    runner->m_pluginFileLocation = pluginManager->getPluginLocation(plugin->getName());

    return runner;
}

QUuid OperatorRunner::id() const
{
    return m_id;
}

QSharedPointer<ActionWatcher<QSharedPointer<const OperatorResult>>> OperatorRunner::getWatcher()
{
    return m_actionWatcher;
}

QSharedPointer<ActionWatcher<QSharedPointer<const OperatorResult>>> OperatorRunner::run(QList<QSharedPointer<BitContainer>> inputContainers)
{
    if (m_actionWatcher->watcher()->future().isRunning()) {
        emit reportError(QString("Operator runner '%1' is already running").arg(m_id.toString()));
        return QSharedPointer<ActionWatcher<QSharedPointer<const OperatorResult>>>();
    }

    QList<QSharedPointer<const BitContainer>> inputContainersConst;
    for (QSharedPointer<BitContainer> input : inputContainers) {
        inputContainersConst.append(input);
    }

    auto pluginState = m_action->getPluginState();

    if (pluginState.isEmpty()) {
        pluginState = m_op->getStateFromUi();
        if (pluginState.isEmpty() || pluginState.contains("error")) {
            if (pluginState.contains("error")) {
                emit reportError(QString("Plugin '%1' reported an error with its current state: '%2'").arg(
                        m_op->getName()).arg(pluginState.value("error").toString()));
            }
            else if (pluginState.isEmpty()) {
                emit reportError(QString(
                        "Plugin '%1' is in an invalid state and can't be executed.  Double check the input fields.").arg(
                                         m_op->getName()));
            }
            return QSharedPointer<ActionWatcher<QSharedPointer<const OperatorResult>>>();
        }
    }

    QVariant previousRunning = SettingsManager::getInstance().getPrivateSetting(SettingsData::PLUGINS_RUNNING_KEY);
    QStringList runningPlugins;
    if (previousRunning.isValid() && previousRunning.canConvert<QStringList>()) {
        runningPlugins = previousRunning.toStringList();
    }
    runningPlugins.append(m_pluginFileLocation);
    SettingsManager::getInstance().setPrivateSetting(SettingsData::PLUGINS_RUNNING_KEY, QVariant(runningPlugins));

    QSharedPointer<ActionProgress> progress(new ActionProgress());

    auto future = QtConcurrent::run(
            QThreadPool::globalInstance(),
            OperatorRunner::operatorCall,
            m_op,
            inputContainersConst,
            pluginState,
            progress);

    m_actionWatcher = QSharedPointer<ActionWatcher<QSharedPointer<const OperatorResult>>>(
            new ActionWatcher<QSharedPointer<const OperatorResult>>(
                    future,
                    progress));

    m_inputContainers = inputContainers;
    m_outputContainers.clear();

    connect(m_actionWatcher->watcher(), SIGNAL(finished()), this, SLOT(postProcess()));

    return m_actionWatcher;
}

void OperatorRunner::postProcess()
{
    disconnect(m_actionWatcher->watcher(), SIGNAL(finished()), this, SLOT(postProcess()));

    QSharedPointer<const OperatorResult> result = m_actionWatcher->watcher()->future().result();

    if (result.isNull()) {
        QString errorString = QString("Plugin '%1' failed to execute.  Double check the input fields.").arg(m_op->getName());
        emit reportError(errorString);
        emit finishedFail(m_id, errorString);
        return;
    }

    if (result->getPluginState().contains("error")) {
        QString errorString = QString("Plugin '%1' reported an error with its processing: %2").arg(m_op->getName()).arg(
                    result->getPluginState().value("error").toString());
        emit reportError(errorString);
        emit finishedFail(m_id, errorString);
        return;
    }

    // Set output containers
    m_outputContainers = result->getOutputContainers();

    // Apply action lineage
    if (!result->getPluginState().isEmpty()) {
        QSharedPointer<PluginAction> action =
            QSharedPointer<PluginAction>(
                    new PluginAction(
                            PluginAction::Operator,
                            m_op->getName(),
                            result->getPluginState()));
        PluginActionLineage::recordLineage(action, m_inputContainers, result->getOutputContainers());
    }

    // Set Parent/Child Relationships
    for (QSharedPointer<BitContainer> inputContainer : m_inputContainers) {
        for (QSharedPointer<BitContainer> outputContainer : result->getOutputContainers()) {
            inputContainer->addChild(outputContainer->getId());
            outputContainer->addParent(inputContainer->getId());
        }
    }

    // Add output containers to container manager
    if (result->getOutputContainers().size() > 0) {
        int number = 1;
        for (QSharedPointer<BitContainer> output : result->getOutputContainers()) {
            QString containerName = m_op->getName() + " Output";
            if (output->nameWasSet()) {
                containerName = output->name();
            }
            if (result->getOutputContainers().length() > 1) {
                output->setName(QString("%2: %1").arg(containerName).arg(number));
            }
            else {
                output->setName(containerName);
            }
            number++;
        }
    }

    emit finished(m_id);
}

QSharedPointer<const OperatorResult> OperatorRunner::operatorCall(
        QSharedPointer<OperatorInterface> op,
        QList<QSharedPointer<const BitContainer>> inputContainers,
        QJsonObject pluginState,
        QSharedPointer<ActionProgress> progressTracker)
{
    return op->operateOnContainers(inputContainers, pluginState, progressTracker);
}
