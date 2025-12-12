#include "datainspectordialog.h"
#include "ui_datainspectordialog.h" // This is generated from your .ui file
#include <QTreeWidgetItem>
#include <QDialogButtonBox> // Make sure to include this

// --- Static Helper Functions ---
// These make it easier to add items to the tree.

/**
 * @brief Creates a simple child item for a QTreeWidgetItem.
 */
static void addChildItem(QTreeWidgetItem* parent, const QString &name, const QString &value = "")
{
    QTreeWidgetItem* item = new QTreeWidgetItem(parent);
    item->setText(0, name);
    item->setText(1, value);
}

/**
 * @brief Creates a parent item for a list of strings.
 */
static void addChildList(QTreeWidgetItem* parent, const QString &name, const QStringList &list)
{
    if (list.isEmpty()) {
        return; // Don't add empty lists
    }

    QTreeWidgetItem* listParent = new QTreeWidgetItem(parent);
    listParent->setText(0, name);
    listParent->setText(1, QString("(%1 items)").arg(list.size()));

    for (const QString &item : list) {
        // Add each list item as a child with no value
        addChildItem(listParent, item);
    }
}

static QString actionTypeToString(ScriptActionType type)
{
    switch (type) {
    case ScriptActionType::ProcedureCall: return "Procedure:";
    case ScriptActionType::If: return "If";
    case ScriptActionType::NotIf: return "NotIf";
    case ScriptActionType::SetFlag: return "SetFlag";
    case ScriptActionType::RemoveFlag: return "RemoveFlag";
    case ScriptActionType::ClearFlag: return "ClearFlag";
    case ScriptActionType::SetCounterVar: return "Set#";
    case ScriptActionType::SetString: return "Set$";
    case ScriptActionType::SetTimeVar: return "Set!";
    case ScriptActionType::AddCounter: return "Add#";
    case ScriptActionType::SubtractCounter: return "Subtract#";
    case ScriptActionType::MultiplyCounter: return "Multiply#";
    case ScriptActionType::DivideCounter: return "Divide#";
    case ScriptActionType::Message: return "Message";
    case ScriptActionType::Question: return "Question";
    case ScriptActionType::Input: return "Input";
    case ScriptActionType::Clothing: return "Clothing";
    case ScriptActionType::NewStatus: return "NewStatus";
    default: return "UnknownAction";
    }
}
// --- End Helper Functions ---


DataInspectorDialog::DataInspectorDialog(ScriptParser* parser, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DataInspectorDialog),
    scriptParser(parser)
{
    ui->setupUi(this);
    setWindowTitle("Script Data Inspector");
    resize(800, 600); // Give it a reasonable default size

    // Set tree header
    ui->treeWidget->setHeaderLabels(QStringList() << "Item" << "Value");
    ui->treeWidget->setColumnWidth(0, 300); // Give more space for the item name

    // Fill the tree with data
    populateTree();

    // Connect the "Close" button (assuming you used "Dialog with Buttons Bottom")
    // If you just have a QDialog, you might need to add a QDialogButtonBox manually
    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

DataInspectorDialog::~DataInspectorDialog()
{
    delete ui;
}

/**
 * @brief The main function that populates the tree widget with all script data.
 */
void DataInspectorDialog::populateTree()
{
    if (!scriptParser) return;

    // Get the fully parsed script data
    const ScriptData &data = scriptParser->getScriptData();
    ui->treeWidget->clear();

    QList<QTreeWidgetItem*> rootItems; // To expand them all at the end

    // --- 1. General Settings ---
    QTreeWidgetItem* generalRoot = new QTreeWidgetItem(ui->treeWidget);
    generalRoot->setText(0, "General Settings");
    rootItems.append(generalRoot);
    addChildItem(generalRoot, "Master Name", data.general.masterName);
    addChildItem(generalRoot, "Sub Names", data.general.subNames.join(", "));
    addChildItem(generalRoot, "Min Merits", QString::number(data.general.minMerits));
    addChildItem(generalRoot, "Max Merits", QString::number(data.general.maxMerits));

    // --- 2. Event Handlers ---
    QTreeWidgetItem* eventsRoot = new QTreeWidgetItem(ui->treeWidget);
    eventsRoot->setText(0, "Event Handlers");
    rootItems.append(eventsRoot);
    addChildItem(eventsRoot, "firstRun", data.eventHandlers.firstRun);
    addChildItem(eventsRoot, "openProgram", data.eventHandlers.openProgram);
    addChildItem(eventsRoot, "closeProgram", data.eventHandlers.closeProgram);
    addChildItem(eventsRoot, "newDay", data.eventHandlers.newDay);
    addChildItem(eventsRoot, "punishmentGiven", data.eventHandlers.punishmentGiven);
    addChildItem(eventsRoot, "jobAnnounced", data.eventHandlers.jobAnnounced);
    // (Add any other handlers you want to see)

    // --- 3. Variables ---
    QTreeWidgetItem* varsRoot = new QTreeWidgetItem(ui->treeWidget);
    varsRoot->setText(0, "Variables (stringVariables)");
    varsRoot->setText(1, QString("(%1)").arg(data.stringVariables.size()));
    rootItems.append(varsRoot);

    for (auto it = data.stringVariables.begin(); it != data.stringVariables.end(); ++it) {
        addChildItem(varsRoot, it.key(), it.value());
    }

    // --- Timers ---
    QTreeWidgetItem* timerRoot = new QTreeWidgetItem(ui->treeWidget);
    timerRoot->setText(0, "Timers");
    timerRoot->setText(1, QString("(%1)").arg(data.timers.size()));
    rootItems.append(timerRoot);

    for (const TimerDefinition &timer : data.timers.values())
    {
        QTreeWidgetItem* timerItem = new QTreeWidgetItem(timerRoot);
        timerItem->setText(0, timer.name);

        // Add the timer's properties as children
        addChildItem(timerItem, "Start", timer.startTimeMin + " - " + timer.startTimeMax);
        addChildItem(timerItem, "End", timer.endTime);

        //addChildList(timerItem, "Procedures", timer.procedures);
    }

    // --- 4. Procedures ---
    QTreeWidgetItem* procsRoot = new QTreeWidgetItem(ui->treeWidget);
    procsRoot->setText(0, "Procedures");
    procsRoot->setText(1, QString("(%1)").arg(data.procedures.size()));
    rootItems.append(procsRoot);

    for (const ProcedureDefinition &proc : data.procedures.values())
    {
        QTreeWidgetItem* procItem = new QTreeWidgetItem(procsRoot);
        procItem->setText(0, proc.name);

        if (!proc.title.isEmpty()) {
            addChildItem(procItem, "Title", proc.title);
        }

        // Create a sub-tree for the ordered actions
        QTreeWidgetItem* actionsRoot = new QTreeWidgetItem(procItem);
        actionsRoot->setText(0, "Actions");
        actionsRoot->setText(1, QString("(%1 steps)").arg(proc.actions.size()));

        // Loop through the new 'actions' list in order
        for (const ScriptAction &action : proc.actions) {
            QString actionName = actionTypeToString(action.type);
            addChildItem(actionsRoot, actionName, action.value);
        }
        actionsRoot->setExpanded(true);
    }

    // --- 5. Questions ---
    QTreeWidgetItem* qRoot = new QTreeWidgetItem(ui->treeWidget);
    qRoot->setText(0, "Questions");
    qRoot->setText(1, QString("(%1)").arg(data.questions.size()));
    rootItems.append(qRoot);

    for (const QuestionDefinition &q : data.questions.values()) {
        QTreeWidgetItem* qItem = new QTreeWidgetItem(qRoot);
        qItem->setText(0, q.name);

        addChildItem(qItem, "Text", q.text);
        addChildItem(qItem, "Variable", q.variable);
        addChildList(qItem, "ifConditions", q.ifConditions);
        addChildList(qItem, "notIfConditions", q.notIfConditions);

        QTreeWidgetItem* ansRoot = new QTreeWidgetItem(qItem);
        ansRoot->setText(0, "Answers");
        for (const QuestionAnswerBlock &ans : q.answers) {
            addChildItem(ansRoot, ans.answerText, "-> " + ans.procedureName);
        }
    }

    // --- 6. Statuses ---
    QTreeWidgetItem* statusRoot = new QTreeWidgetItem(ui->treeWidget);
    statusRoot->setText(0, "Statuses");
    statusRoot->setText(1, QString("(%1)").arg(data.statuses.size()));
    rootItems.append(statusRoot);
    for (const StatusDefinition &status : data.statuses.values()) {
        QTreeWidgetItem* item = new QTreeWidgetItem(statusRoot);
        item->setText(0, status.name);
        addChildItem(item, "Title", status.title);
        addChildItem(item, "Group", status.group);
        addChildItem(item, "Reports Only", status.reportsOnly ? "true" : "false");
    }

    // --- 7. Jobs ---
    QTreeWidgetItem* jobsRoot = new QTreeWidgetItem(ui->treeWidget);
    jobsRoot->setText(0, "Jobs");
    jobsRoot->setText(1, QString("(%1)").arg(data.jobs.size()));
    rootItems.append(jobsRoot);
    for (const JobDefinition &job : data.jobs.values()) {
        QTreeWidgetItem* item = new QTreeWidgetItem(jobsRoot);
        item->setText(0, job.name);
        addChildItem(item, "Title", job.title);
        addChildList(item, "runDays", job.runDays);
        addChildList(item, "notRunDays", job.noRunDays);
    }

    // --- 8. Punishments ---
    QTreeWidgetItem* punRoot = new QTreeWidgetItem(ui->treeWidget);
    punRoot->setText(0, "Punishments");
    punRoot->setText(1, QString("(%1)").arg(data.punishments.size()));
    rootItems.append(punRoot);
    for (const PunishmentDefinition &pun : data.punishments.values()) {
        QTreeWidgetItem* item = new QTreeWidgetItem(punRoot);
        item->setText(0, pun.name);
        addChildItem(item, "Title", pun.title);
        addChildList(item, "Groups", pun.groups);
        addChildItem(item, "Value", QString::number(pun.value) + " " + pun.valueUnit);
    }

    // --- 9. Reports ---
    QTreeWidgetItem* repRoot = new QTreeWidgetItem(ui->treeWidget);
    repRoot->setText(0, "Reports");
    repRoot->setText(1, QString("(%1)").arg(data.reports.size()));
    rootItems.append(repRoot);
    for (const ReportDefinition &rep : data.reports.values()) {
        QTreeWidgetItem* item = new QTreeWidgetItem(repRoot);
        item->setText(0, rep.name);
        addChildItem(item, "Title", rep.title);
        //addChildList(item, "setFlags", rep.setFlags);
    }

    // --- 10. Confessions ---
    QTreeWidgetItem* confRoot = new QTreeWidgetItem(ui->treeWidget);
    confRoot->setText(0, "Confessions");
    confRoot->setText(1, QString("(%1)").arg(data.confessions.size()));
    rootItems.append(confRoot);
    for (const ConfessionDefinition &conf : data.confessions.values()) {
        QTreeWidgetItem* item = new QTreeWidgetItem(confRoot);
        item->setText(0, conf.name);
        addChildItem(item, "Title", conf.title);
    }

    // --- 11. Permissions ---
    QTreeWidgetItem* permRoot = new QTreeWidgetItem(ui->treeWidget);
    permRoot->setText(0, "Permissions");
    permRoot->setText(1, QString("(%1)").arg(data.permissions.size()));
    rootItems.append(permRoot);
    for (const PermissionDefinition &perm : data.permissions.values()) {
        QTreeWidgetItem* item = new QTreeWidgetItem(permRoot);
        item->setText(0, perm.name);
        addChildItem(item, "Title", perm.title);
        addChildItem(item, "Deny Procedure", perm.denyProcedure);
    }

    // --- 12. Flag Definitions ---
    QTreeWidgetItem* flagRoot = new QTreeWidgetItem(ui->treeWidget);
    flagRoot->setText(0, "Flag Definitions");
    flagRoot->setText(1, QString("(%1)").arg(data.flagDefinitions.size()));
    rootItems.append(flagRoot);
    for (const FlagDefinition &flag : data.flagDefinitions.values()) {
        QTreeWidgetItem* item = new QTreeWidgetItem(flagRoot);
        item->setText(0, flag.name);
        addChildItem(item, "Group", flag.group);
        addChildItem(item, "Expire Procedure", flag.expireProcedure);
    }

    // Expand all top-level items for easier viewing
    for (QTreeWidgetItem* item : rootItems) {
        item->setExpanded(false);
    }
}
