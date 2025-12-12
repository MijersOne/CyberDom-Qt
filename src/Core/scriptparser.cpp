#include "scriptparser.h"
#include "ScriptUtils.h"
#include <QSettings>
#include <QFile>
#include <QTextStream>
#include <QDebug>
#include <QFileInfo>
#include <QDir>
#include <QRandomGenerator>
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#include <QStringConverter>
#endif

QMap<QString, QMap<QString, QStringList>> ScriptParser::parseIniFile(const QString& path)
{
    QFile file(path);
    QMap<QString, QMap<QString, QStringList>> sections;

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Failed to open script file:" << path;
        return sections;
    }

    QTextStream in(&file);
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    in.setEncoding(QStringConverter::Utf8);
#else
    in.setCodec("UTF-8");
#endif
    QStringList lines;

    QString basePath = QFileInfo(path).absolutePath();

    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();

        if (line.startsWith("%include=", Qt::CaseInsensitive)) {
            QString includedFile = resolveIncludePath(path, line);
            QMap<QString, QMap<QString, QStringList>> includedSections = parseIniFile(includedFile);

            // Merge includedSections into sections
            for (auto it = includedSections.begin(); it != includedSections.end(); ++it) {
                QString section = it.key();
                QMap<QString, QStringList> values = it.value();

                for (auto innerIt = values.begin(); innerIt != values.end(); ++innerIt) {
                    sections[section][innerIt.key()] += innerIt.value();
                }
            }

            continue;
        }

        lines << line;
    }

    file.close();

    // Now parse the combined lines
    QString currentSection = "general";
    for (const QString& line : lines) {
        if (line.isEmpty() || line.startsWith("#") || line.startsWith(";"))
            continue;

        if (line.startsWith("[") && line.endsWith("]")) {
            currentSection = line.mid(1, line.length() - 2).trimmed().toLower();
            continue;
        }

        int equalsIndex = line.indexOf('=');
        if (equalsIndex != -1) {
            QString key = line.left(equalsIndex).trimmed();
            QString value = line.mid(equalsIndex + 1).trimmed();
            sections[currentSection][key].append(value);
        }
    }

    return sections;
}

QStringList ScriptParser::readIniLines(const QString &path) {
    QFile file(path);
    QStringList lines;

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Failed to open script file:" << path;
        return lines;
    }

    QTextStream in(&file);
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    in.setEncoding(QStringConverter::Utf8);
#else
    in.setCodec("UTF-8");
#endif
    QString basePath = QFileInfo(path).absolutePath();

    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();

        if (line.startsWith("%include=", Qt::CaseInsensitive)) {
            QString includedFile = resolveIncludePath(path, line);
            lines += readIniLines(includedFile);
            continue;
        }

        lines << line;
    }

    file.close();
    return lines;
}

bool ScriptParser::parseScript(const QString& path) {
    scriptFilePath = path;
    auto sections = parseIniFile(path);
    scriptData.rawSections = sections;

    // Section names are stored in lower-case by parseIniFile()
    if (!sections.contains("general") || !sections["general"].contains("MinVersion")) {
        qWarning() << "Script missing required [General] section or MinVersion key.";
        return false;
    }

    if (sections.contains("general"))
        parseGeneralSection("general", sections["general"]);
    if (sections.contains("init"))
        parseInitSection(sections["init"]);

    QStringList iniLines = readIniLines(path);
    parseClothingTypes(iniLines);
    parseStatusSections(sections);
    parseReportSections(iniLines);
    parseConfessionSections(iniLines);
    parsePermissionSections(iniLines);
    parsePunishmentSections(sections);
    parseJobSections(sections);
    parseInstructionSets(iniLines);
    parseInstructionSections(iniLines);
    parseProcedureSections(iniLines);
    parsePopupGroupSections(sections);
    parsePopupSections(sections);
    parseTimerSections(iniLines);
    parseQuestionSections(sections);

    if (sections.contains("events")) {
        // Parse basic event settings (FirstRun, Signin) as well as
        // individual event handlers defined in the [events] section.
        parseEventsSection(sections["events"]);
        parseEventSection(sections["events"]);
    }

    if (sections.contains("ftp"))
        parseFtpSection(sections["ftp"]);

    if (sections.contains("font"))
        parseFontSection(sections["font"]);

    for (auto it = sections.begin(); it != sections.end(); ++it) {
        QString key = it.key();
        if (key.startsWith("flag-", Qt::CaseInsensitive)) {
            parseFlagSection(key.mid(5), it.value());
        }
    }

    return true;
}

void ScriptParser::parseGeneralSection(const QString &sectionName, const QMap<QString, QStringList>& section) {
    auto& g = scriptData.general;

    // This loop populates generalSettings and handles a few specific loops
    for (auto it = section.begin(); it != section.end(); ++it) {
        const QString &key = it.key().trimmed();
        const QStringList &values = it.value();

        if (!values.isEmpty())
            scriptData.generalSettings[key] = values.first();

        if (key.compare("ValidateAll", Qt::CaseInsensitive) == 0) {
            if (!values.isEmpty())
                validateAll = (values.first().trimmed() == "1");
        } else if (key.compare("MinVersion", Qt::CaseInsensitive) == 0) {
            if (!values.isEmpty()) {
                bool ok;
                float version = values.first().toFloat(&ok);
                if (ok) minVersion = version;
            }
        }
    }

    if (section.contains("Master"))
        g.masterName = section["Master"].value(0);
    if (section.contains("SubName"))
        g.subNames = section["SubName"];
    if (section.contains("MinVersion"))
        g.minVersion = section["MinVersion"].value(0);
    if (section.contains("Version"))
        g.version = section["Version"].value(0);
    if (section.contains("ReportTimeFormat")) {
        QString val = section["ReportTimeFormat"].value(0).trimmed();
        if (val == "12") g.reportTimeFormat = 12;
        else g.reportTimeFormat = 24;
    }
    if (section.contains("ForgetConfession"))
        g.forgetConfessionEnabled = section["ForgetConfession"].value(0).trimmed() != "0";
    if (section.contains("IgnoreConfession"))
        g.ignoreConfessionEnabled = section["IgnoreConfession"].value(0).trimmed() != "0";
    if (section.contains("ForgetPenalty")) {
        QStringList range = section["ForgetPenalty"].value(0).split(',', Qt::SkipEmptyParts);
        if (range.size() == 2) {
            g.forgetPenaltyMin = range[0].toInt();
            g.forgetPenaltyMax = range[1].toInt();
        } else if (range.size() == 1) {
            g.forgetPenaltyMin = g.forgetPenaltyMax = range[0].toInt();
        }
    }

    if (section.contains("IgnorePenalty")) {
        QStringList range = section["IgnorePenalty"].value(0).split(',', Qt::SkipEmptyParts);
        if (range.size() == 2) {
            g.ignorePenaltyMin = range[0].toInt();
            g.ignorePenaltyMax = range[1].toInt();
        } else if (range.size() == 1) {
            g.ignorePenaltyMin = g.ignorePenaltyMax = range[0].toInt();
        }
    }

    if (section.contains("ForgetPenaltyGroup"))
        g.forgetPenaltyGroup = section["ForgetPenaltyGroup"].value(0);
    if (section.contains("IgnorePenaltyGroup"))
        g.ignorePenaltyGroup = section["IgnorePenaltyGroup"].value(0);

    if (section.contains("InterruptStatus")) {
        for (const QString& val : section["InterruptStatus"])
            g.interruptStatuses += val.split(',', Qt::SkipEmptyParts);
    }

    if (section.contains("MinMerits"))
        g.minMerits = section["MinMerits"].value(0).toInt();

    if (section.contains("MaxMerits"))
        g.maxMerits = section["MaxMerits"].value(0).toInt();

    if (section.contains("Yellow"))
        g.yellowMerits = section["Yellow"].value(0).toInt();

    if (section.contains("Red"))
        g.redMerits = section["Red"].value(0).toInt();

    if (section.contains("DayMerits"))
        g.dayMerits = section["DayMerits"].value(0).toInt();

    if (section.contains("HideMerits"))
        g.hideMerits = section["HideMerits"].value(0).trimmed() == "1";

    if (section.contains("ReportPassword"))
        g.reportPassword = section["ReportPassword"].value(0);

    if (section.contains("Restrict"))
        g.restrictMode = section["Restrict"].value(0).trimmed() == "1";

    if (section.contains("AutoEncrypt"))
        g.autoEncrypt = section["AutoEncrypt"].value(0).trimmed() == "1";

    if (section.contains("CenterRandom"))
        g.centerRandom = section["CenterRandom"].value(0).trimmed() == "1";

    if (section.contains("ClothExport"))
        g.allowClothExport = section["ClothExport"].value(0).trimmed() == "1";

    if (section.contains("ClothReport"))
        g.allowClothReport = section["ClothReport"].value(0).trimmed() != "0";

    if (section.contains("Alarm"))
        g.popupAlarm = section["Alarm"].value(0);

    if (section.contains("TopText"))
        g.topText += section["TopText"];

    if (section.contains("BottomText"))
        g.bottomText += section["BottomText"];

    if (section.contains("PopupMessage"))
        g.popupMessage = section["PopupMessage"].value(0);

    if (section.contains("SigninPenalty1"))
        g.signinPenalty1 = section["SigninPenalty1"].value(0).toInt();

    if (section.contains("SigninPenalty2"))
        g.signinPenalty2 = section["SigninPenalty2"].value(0).toInt();

    if (section.contains("SigninPenaltyGroup"))
        g.signinPenaltyGroup = section["SigninPenaltyGroup"].value(0);

    if (section.contains("AutoClothFlags"))
        g.autoClothFlags = section["AutoClothFlags"].value(0).trimmed() == "1";

    if (section.contains("smtp")) g.smtpServer = section["smtp"].value(0);

    if (section.contains("smtpUser")) g.smtpUser = section["smtpUser"].value(0);

    if (section.contains("smtpPassword")) g.smtpPassword = section["smtpPassword"].value(0);

    if (section.contains("senderEmail")) g.senderEmail = section["senderEmail"].value(0);

    if (section.contains("subEmail")) g.subEmail = section["subEmail"].value(0);

    if (section.contains("masterEmail")) g.masterEmail = section["masterEmail"].value(0);

    if (section.contains("masterEmail2")) g.masterEmail2 = section["masterEmail2"].value(0);

    if (section.contains("smtpPort")) g.smtpPort = section["smtpPort"].value(0).toInt();

    if (section.contains("tlsPort")) g.tlsPort = section["tlsPort"].value(0).toInt();

    if (section.contains("AutoMailReport")) g.autoMailReport = section["AutoMailReport"].value(0).toInt() == 1;

    if (section.contains("ShowMailWindow")) g.showMailWindow = section["ShowMailWindow"].value(0).toInt() != 0;

    if (section.contains("TestMail")) g.testMail = section["TestMail"].value(0).toInt() == 1;

    if (section.contains("MailLog")) g.mailLog = section["MailLog"].value(0).toInt() == 1;

    if (section.contains("MaxLineBreak"))
        g.maxLineBreak = section["MaxLineBreak"].value(0);

    if (section.contains("DirectShow"))
        g.directShow = section["DirectShow"].value(0).toInt() == 1;

    if (section.contains("CameraFolder"))
        g.cameraFolder = section["CameraFolder"].value(0);

    if (section.contains("SavePictures"))
        g.savePictures = section["SavePictures"].value(0).trimmed();

    if (section.contains("OpenPassword"))
        g.openPassword = section["OpenPassword"].value(0);

    if (section.contains("UseIcon"))
        g.useIcon = section["UseIcon"].value(0).toInt() == 1;

    if (section.contains("StartMinimized"))
        g.startMinimized = section["StartMinimized"].value(0).toInt() == 1;

    if (section.contains("MinimizePopup"))
        g.minimizePopup = section["MinimizePopup"].value(0).trimmed();

    if (section.contains("Rules"))
        g.rulesEnabled = section["Rules"].value(0).toInt() != 0;

    if (section.contains("QuickReport"))
        g.quickReport = section["QuickReport"].value(0).trimmed();

    if (section.contains("QuickLabel"))
        g.quickLabel = section["QuickLabel"].value(0).trimmed();

    if (section.contains("AutoMailReport"))
        g.autoMailReport = section["AutoMailReport"].value(0).toInt() == 1;

    if (section.contains("FlagOnText"))
        g.flagOnText = section["FlagOnText"].value(0);

    if (section.contains("FlagOffText"))
        g.flagOffText = section["FlagOffText"].value(0);

    if (section.contains("DefaultStatus"))
        g.defaultStatus = section["DefaultStatus"].value(0);

    if (section.contains("RemindInterval"))
        g.globalRemindInterval = section["RemindInterval"].value(0);

    if (section.contains("RemindPenalty"))
        g.globalRemindPenalty = section["RemindPenalty"].value(0);

    if (section.contains("RemindPenaltyGroup"))
        g.globalRemindPenaltyGroup = section["RemindPenaltyGroup"].value(0);

    if (section.contains("ExpirePenaltyGroup"))
        g.globalExpirePenaltyGroup = section["ExpirePenaltyGroup"].value(0);

    if (section.contains("AnnounceJobs")) {
        // Set to false only if it's explictly "0"
        g.announceJobs = (section["AnnounceJobs"].value(0).trimmed() != "0");
    }

    if (section.contains("MinPunishment")) {
        g.minPunishment = section["MinPunishment"].value(0).toInt();
    }

    if (section.contains("MaxPunishment")) {
        g.maxPunishment = section["MaxPunishment"].value(0).toInt();
    }

    if (section.contains("AskPunishment")) {
        QStringList parts = section["AskPunishment"].value(0).split(',', Qt::SkipEmptyParts);
        if (parts.size() >= 2) {
            g.askPunishmentMin = parts[0].trimmed().toInt();
            g.askPunishmentMax = parts[1].trimmed().toInt();
        } else if (parts.size() == 1) {
            // Handle single value cases
            g.askPunishmentMin = g.askPunishmentMax = parts[0].trimmed().toInt();
        }
    }

    if (section.contains("AskPunishmentGroup")) {
        // Split by comma to get the list of groups
        g.askPunishmentGroups = section["AskPunishmentGroup"].value(0).split(',', Qt::SkipEmptyParts);
    }

    if (section.contains("MaxDecline")) {
        g.maxDecline = section["MaxDecline"].value(0).toInt();
    }

    if (section.contains("HideTime")) {
        g.hideTimeGlobal = !section["HideTime"].isEmpty() && section["HideTime"].first().trimmed() == "1";
    }

    if (!section.contains("ValidateAll")) {
        validateAll = (minVersion >= 3.4f);
    }
}

void ScriptParser::parseInitSection(const QMap<QString, QStringList>& section) {
    auto& i = scriptData.init;

    if (section.contains("NewStatus"))
        i.newStatus = section["NewStatus"].value(0);
    if (section.contains("Merits")) {
        bool ok;
        int value = section["Merits"].value(0).toInt(&ok);
        if (ok) i.merits = value;
    }
    if (section.contains("Procedure"))
        i.procedure = section["Procedure"].value(0);
}

void ScriptParser::parseEventsSection(const QMap<QString, QStringList>& section) {
    auto& e = scriptData.events;

    if (section.contains("FirstRun"))
        e.firstRunProcedure = section["FirstRun"].value(0);
    if (section.contains("Signin"))
        e.signIn = section["Signin"].value(0);
}

void ScriptParser::parseStatusSections(const QMap<QString, QMap<QString, QStringList>>& sections) {
    for (auto it = sections.begin(); it != sections.end(); ++it) {
        const QString& sectionName = it.key();
        if (!sectionName.startsWith("status-"))
            continue;

        QString statusName = sectionName.mid(QString("status-").length());
        const QMap<QString, QStringList>& entries = it.value();

        StatusDefinition status;
        status.name = statusName;

        if (entries.contains("Group"))
            status.group = entries["Group"].value(0);

        if (entries.contains("SubStatus"))
            status.isSubStatus = entries["SubStatus"].value(0).trimmed() == "1";

        if (entries.contains("ReportsOnly"))
            status.reportsOnly = entries["ReportsOnly"].value(0).trimmed() != "0";

        if (entries.contains("Assignments"))
            status.assignmentsEnabled = entries["Assignments"].value(0).trimmed() != "0";

        if (entries.contains("Permissions"))
            status.permissionsEnabled = entries["Permissions"].value(0).trimmed() != "0";

        if (entries.contains("Confessions"))
            status.confessionsEnabled = entries["Confessions"].value(0).trimmed() != "0";

        if (entries.contains("Reports"))
            status.reportsEnabled = entries["Reports"].value(0).trimmed() != "0";

        if (entries.contains("EndReport"))
            status.endReport = entries["EndReport"].value(0);

        if (entries.contains("ClothReport"))
            status.allowClothReport = entries["ClothReport"].value(0).trimmed() != "0";

        if (entries.contains("Title"))
            status.title = entries["Title"].value(0);

        if (entries.contains("PopupInterval")) {
            QStringList vals = entries["PopupInterval"].value(0).split(',', Qt::SkipEmptyParts);
            status.popupIntervalMin = vals.value(0);
            status.popupIntervalMax = vals.value(1, vals.value(0));
        }
        if (entries.contains("PopupIf"))
            status.popupIfFlags += entries["PopupIf"].value(0).split(',', Qt::SkipEmptyParts);
        if (entries.contains("NoPopupIf"))
            status.noPopupIfFlags += entries["NoPopupIf"].value(0).split(',', Qt::SkipEmptyParts);
        if (entries.contains("PopupGroup"))
            status.popupGroup = entries["PopupGroup"].value(0);
        if (entries.contains("PopupAlarm"))
            status.popupAlarm = entries["PopupAlarm"].value(0);

        MessageGroup currentGroup;
        bool inMessageBlock = false;

        for (auto keyIt = entries.begin(); keyIt != entries.end(); ++keyIt) {
            QString key = keyIt.key();
            const QStringList& values = keyIt.value();

            for (const QString& val : values) {
                if (key == "Select") {
                    if (val.compare("Random", Qt::CaseInsensitive) == 0)
                        currentGroup.mode = MessageSelectMode::Random;
                    else
                        currentGroup.mode = MessageSelectMode::All;

                    inMessageBlock = true;
                    if (!currentGroup.messages.isEmpty()) {
                        status.messages.append(currentGroup);
                        currentGroup = MessageGroup();
                    }
                }
                else if (key == "Message") {
                    inMessageBlock = true;
                    currentGroup.messages.append(val.trimmed());
                }
                else if (key == "Text") {
                    status.statusTexts.append(val.trimmed());
                }
            }
        }
        if (!currentGroup.messages.isEmpty()) {
            status.messages.append(currentGroup);
        }

        if (entries.contains("Input"))
            status.inputQuestions.append(entries["Input"]);

        if (entries.contains("NoInputProcedure"))
            status.noInputProcedure = entries["NoInputProcedure"].value(0);

        if (entries.contains("Question"))
            status.advancedQuestions.append(entries["Question"]);

        QString signinKey;
        for (auto keyIt = entries.constBegin(); keyIt != entries.constEnd(); ++keyIt) {
            QString lower = keyIt.key().toLower();
            if (lower == "signininterval" || lower == "signinininterval") {
                signinKey = keyIt.key();
                break;
            }
        }
        if (!signinKey.isEmpty()) {
            QStringList parts = entries[signinKey].value(0).split(',', Qt::SkipEmptyParts);
            status.signinIntervalMin = parts.value(0).trimmed();
            status.signinIntervalMax = parts.value(1, parts.value(0)).trimmed();
        }

        if (entries.contains("SigninPenalty1"))
            status.signinPenalty1 = entries["SigninPenalty1"].value(0).toInt();

        if (entries.contains("SigninPenalty2"))
            status.signinPenalty2 = entries["SigninPenalty2"].value(0).toInt();

        if (entries.contains("SigninPenaltyGroup"))
            status.signinPenaltyGroup = entries["SigninPenaltyGroup"].value(0);

        if (entries.contains("AutoAssign"))
            status.allowAutoAssign = entries["AutoAssign"].value(0).trimmed() == "1";

        if (entries.contains("PointCamera"))
            status.pointCameraText = entries["PointCamera"].value(0);

        if (entries.contains("CameraInteral")) {
            QStringList parts = entries["CameraInterval"].value(0).split(',', Qt::SkipEmptyParts);
            status.cameraIntervalMin = parts.value(0);
            status.cameraIntervalMax = parts.value(1, parts.value(0));
        }

        if (entries.contains("PoseCamera"))
            status.poseCameraText = entries["PoseCamera"].value(0);

        if (entries.contains("Picture"))
            status.pictureList = entries["Picture"];

        if (entries.contains("Statistics")) {
            QString val = entries["Statistics"].value(0).trimmed();
            if (val == "1") {
                status.trackStatistics = true;
                status.statisticsLabel = sectionName;
            } else {
                status.trackStatistics = true;
                status.statisticsLabel = val;
            }
        }

        if (entries.contains("Away"))
            status.isAway = entries["Away"].value(0).toInt() == 1;

        if (entries.contains("Rules"))
            status.rulesOverride = entries["Rules"].value(0).toInt() != 0;

        if (entries.contains("QuickReport"))
            status.quickReportName = entries["QuickReport"].value(0);

        if (entries.contains("SetFlag"))
            status.setFlags = entries["SetFlag"];

        if (entries.contains("ClearFlag"))
            status.clearFlags = entries["ClearFlag"];

        if (entries.contains("Set$"))
            status.setStringVars = entries["Set$"];

        if (entries.contains("Set#"))
            status.setCounterVars = entries["Set#"];

        if (entries.contains("Set@"))
            status.setTimeVars = entries["Set@"];

        if (entries.contains("Counter+"))
            status.incrementCounters = entries["Counter+"];

        if (entries.contains("Counter-"))
            status.decrementCounters = entries["Counter-"];

        if (entries.contains("Random#"))
            status.randomCounters = entries["Random#"];

        if (entries.contains("Random$"))
            status.randomStrings = entries["Random$"];

        if (entries.contains("SetFlag"))
            status.setFlags = entries["SetFlag"];

        if (entries.contains("RemoveFlag"))
            status.removeFlags = entries["RemoveFlag"];

        if (entries.contains("SetFlagGroup"))
            status.setFlagGroups = entries["SetFlagGroup"];

        if (entries.contains("RemoveFlagGroup"))
            status.removeFlagGroups = entries["RemoveFlagGroup"];

        if (entries.contains("If"))
            status.ifFlags = entries["If"];

        if (entries.contains("NotIf"))
            status.notIfFlags = entries["NotIf"];

        if (entries.contains("DenyIf"))
            status.denyIfFlags = entries["DenyIf"];

        if (entries.contains("PermitIf"))
            status.permitIfFlags = entries["PermitIf"];

        if (entries.contains("Set$"))
            status.setStrings = entries["Set$"];

        if (entries.contains("Input$"))
            status.inputStrings = entries["Input$"];

        if (entries.contains("InputLong$"))
            status.inputLongStrings = entries["InputLong$"];

        if (entries.contains("Drop$"))
            status.dropStrings = entries["Drop$"];

        if (entries.contains("Set#"))
            status.setCounters = entries["Set#"];

        if (entries.contains("Add#"))
            status.addCounters = entries["Add#"];

        if (entries.contains("Subtract#"))
            status.subtractCounters = entries["Subtract#"];

        if (entries.contains("Multiply#"))
            status.multiplyCounters = entries["Multiply#"];

        if (entries.contains("Divide#"))
            status.divideCounters = entries["Divide#"];

        if (entries.contains("Drop#"))
            status.dropCounters = entries["Drop#"];

        if (entries.contains("Input#"))
            status.inputCounters = entries["Input#"];

        if (entries.contains("InputNeg#"))
            status.inputNegCounters = entries["InputNeg#"];

        if (entries.contains("Random#"))
            status.randomCounters = entries["Random#"];

        if (entries.contains("Set!"))
            status.setTimeVars = entries["Set!"];
        if (entries.contains("Add!"))
            status.addTimeVars = entries["Add!"];
        if (entries.contains("Subtract!"))
            status.subtractTimeVars = entries["Subtract!"];
        if (entries.contains("Multiply!"))
            status.multiplyTimeVars = entries["Multiply!"];
        if (entries.contains("Divide!"))
            status.divideTimeVars = entries["Divide!"];
        if (entries.contains("Round!"))
            status.roundTimeVars = entries["Round!"];
        if (entries.contains("Drop!"))
            status.dropTimeVars = entries["Drop!"];
        if (entries.contains("InputDate!"))
            status.inputDateVars = entries["InputDate!"];
        if (entries.contains("InputDateDef!"))
            status.inputDateDefVars = entries["InputDateDef!"];
        if (entries.contains("InputTime!"))
            status.inputTimeVars = entries["InputTime!"];
        if (entries.contains("InputTimeDef!"))
            status.inputTimeDefVars = entries["InputTimeDef!"];
        if (entries.contains("InputInterval!"))
            status.inputIntervalVars = entries["InputInterval!"];
        if (entries.contains("Random!"))
            status.randomTimeVars = entries["Random!"];
        if (entries.contains("AddDays!"))
            status.addDaysVars = entries["AddDays!"];
        if (entries.contains("SubtractDays!"))
            status.subtractDaysVars = entries["SubtractDays!"];
        if (entries.contains("Days#"))
            status.extractToCounter += entries["Days#"];
        if (entries.contains("Hours#"))
            status.extractToCounter += entries["Hours#"];
        if (entries.contains("Minutes#"))
            status.extractToCounter += entries["Minutes#"];
        if (entries.contains("Seconds#"))
            status.extractToCounter += entries["Seconds#"];
        if (entries.contains("Days!"))
            status.convertFromCounter += entries["Days!"];
        if (entries.contains("Hours!"))
            status.convertFromCounter += entries["Hours!"];
        if (entries.contains("Minutes!"))
            status.convertFromCounter += entries["Minutes!"];
        if (entries.contains("Seconds!"))
            status.convertFromCounter += entries["Seconds!"];
        if (entries.contains("HideTime") && !entries["HideTime"].isEmpty()) {
            status.hideTime = entries["HideTime"].first().trimmed() == "1";
        }
        if (entries.contains("AutoAssignMessage")) {
            status.autoAssignMessage = (entries["AutoAssignMessage"].value(0).trimmed() != "0");
        }

        parseDurationControl(entries, status.duration);

        scriptData.statuses.insert(statusName, status);

        scriptData.statuses[statusName] = status;
    }
}

void ScriptParser::parseReportSections(const QStringList& lines) {
    ReportDefinition currentReport;
    bool inSection = false;
    QString currentSectionName;
    QString currentPunishMessage;

    // Loop through every single line from the .ini in its original order
    for (const QString& line : lines) {
        if (line.isEmpty() || line.startsWith("#") || line.startsWith(";"))
            continue;

        // Check for section headers
        if (line.startsWith("[") && line.endsWith("]")) {
            // Save the previous report
            if (inSection) {
                scriptData.reports.insert(currentReport.name, currentReport);
            }
            inSection = false; // Reset

            currentSectionName = line.mid(1, line.length() - 2).trimmed();

            // Check if this new section is a report
            if (currentSectionName.startsWith("report-", Qt::CaseInsensitive)) {
                inSection = true;
                currentReport = ReportDefinition();
                currentReport.name = currentSectionName.mid(7).toLower();
                currentPunishMessage.clear();
            }
            continue;
        }

        // If we are not in a report section, skip this line
        if (!inSection) {
            continue;
        }

        // If we ARE in a report, this must be a key-value pair
        int equalsIndex = line.indexOf('=');
        if (equalsIndex == -1) continue;

        QString key = line.left(equalsIndex).trimmed();
        QString value = line.mid(equalsIndex + 1).trimmed();

        // We append actions to the list *in the order we find them*
        if (key.compare("Title", Qt::CaseInsensitive) == 0) {
            currentReport.title = value;
        } else if (key.compare("OnTop", Qt::CaseInsensitive) == 0) {
            currentReport.onTop = (value != "0");
        } else if (key.compare("PreStatus", Qt::CaseInsensitive) == 0) {
            currentReport.preStatuses << value.split(',', Qt::SkipEmptyParts);
        } else if (key.compare("AddMerit", Qt::CaseInsensitive) == 0 || key.compare("AddMerits", Qt::CaseInsensitive) == 0) {
            currentReport.merits.add = value;
        } else if (key.compare("SubtractMerit", Qt::CaseInsensitive) == 0 || key.compare("SubtractMerits", Qt::CaseInsensitive) == 0) {
            currentReport.merits.subtract = value;
        } else if (key.compare("Group", Qt::CaseInsensitive) == 0) {
            currentReport.group = value;
        }

        // --- Handle ACTIONS ---
        else if (key.compare("Procedure", Qt::CaseInsensitive) == 0) {
            currentReport.actions.append({ScriptActionType::ProcedureCall, value});
        } else if (key.compare("If", Qt::CaseInsensitive) == 0) {
            currentReport.actions.append({ScriptActionType::If, value});
        } else if (key.compare("NotIf", Qt::CaseInsensitive) == 0) {
            currentReport.actions.append({ScriptActionType::NotIf, value});
        } else if (key.compare("SetFlag", Qt::CaseInsensitive) == 0) {
            currentReport.actions.append({ScriptActionType::SetFlag, value});
        } else if (key.compare("RemoveFlag", Qt::CaseInsensitive) == 0) {
            currentReport.actions.append({ScriptActionType::RemoveFlag, value});
        } else if (key.compare("ClearFlag", Qt::CaseInsensitive) == 0) {
            currentReport.actions.append({ScriptActionType::ClearFlag, value});
        } else if (key.compare("Set#", Qt::CaseInsensitive) == 0) {
            currentReport.actions.append({ScriptActionType::SetCounterVar, value});
        } else if (key.compare("Input#", Qt::CaseInsensitive) == 0) {
            currentReport.actions.append({ScriptActionType::InputCounter, value});
        } else if (key.compare("Change#", Qt::CaseInsensitive) == 0) {
            currentReport.actions.append({ScriptActionType::ChangeCounter, value});
        } else if (key.compare("InputNeg#", Qt::CaseInsensitive) == 0) {
            currentReport.actions.append({ScriptActionType::InputNegCounter, value});
        } else if (key.compare("Random#", Qt::CaseInsensitive) == 0) {
            currentReport.actions.append({ScriptActionType::RandomCounter, value});
        } else if (key.compare("Drop#", Qt::CaseInsensitive) == 0) {
            currentReport.actions.append({ScriptActionType::DropCounter, value});
        } else if (key.compare("Set$", Qt::CaseInsensitive) == 0) {
            currentReport.actions.append({ScriptActionType::SetString, value});
        } else if (key.compare("Input$", Qt::CaseInsensitive) == 0) {
            currentReport.actions.append({ScriptActionType::InputString, value});
        } else if (key.compare("Change$", Qt::CaseInsensitive) == 0) {
            currentReport.actions.append({ScriptActionType::ChangeString, value});
        } else if (key.compare("InputLong$", Qt::CaseInsensitive) == 0) {
            currentReport.actions.append({ScriptActionType::InputLongString, value});
        } else if (key.compare("ChangeLong$", Qt::CaseInsensitive) == 0) {
            currentReport.actions.append({ScriptActionType::ChangeLongString, value});
        } else if (key.compare("Drop$", Qt::CaseInsensitive) == 0) {
            currentReport.actions.append({ScriptActionType::DropString, value});
        } else if (key.compare("Set!", Qt::CaseInsensitive) == 0) {
            currentReport.actions.append({ScriptActionType::SetTimeVar, value});
        } else if (key.compare("Input!", Qt::CaseInsensitive) == 0) {
            currentReport.actions.append({ScriptActionType::InputDate, value});
        } else if (key.compare("InputDateDef!", Qt::CaseInsensitive) == 0) {
            currentReport.actions.append({ScriptActionType::InputDateDef, value});
        } else if (key.compare("ChangeDate!", Qt::CaseInsensitive) == 0) {
            currentReport.actions.append({ScriptActionType::ChangeDate, value});
        } else if (key.compare("InputTime!", Qt::CaseInsensitive) == 0) {
            currentReport.actions.append({ScriptActionType::InputTime, value});
        } else if (key.compare("InputTimeDef!", Qt::CaseInsensitive) == 0) {
            currentReport.actions.append({ScriptActionType::InputTimeDef, value});
        } else if (key.compare("ChangeTime!", Qt::CaseInsensitive) == 0) {
            currentReport.actions.append({ScriptActionType::ChangeTime, value});
        } else if (key.compare("InputInterval!", Qt::CaseInsensitive) == 0) {
            currentReport.actions.append({ScriptActionType::InputInterval, value});
        } else if (key.compare("ChangeInterval!", Qt::CaseInsensitive) == 0) {
            currentReport.actions.append({ScriptActionType::ChangeInterval, value});
        } else if (key.compare("AddDays!", Qt::CaseInsensitive) == 0) {
            currentReport.actions.append({ScriptActionType::AddDaysTime, value});
        } else if (key.compare("SubtractDays!", Qt::CaseInsensitive) == 0) {
            currentReport.actions.append({ScriptActionType::SubtractDaysTime, value});
        } else if (key.compare("Days!", Qt::CaseInsensitive) == 0) {
            currentReport.actions.append({ScriptActionType::ConvertDays, value});
        } else if (key.compare("Hours!", Qt::CaseInsensitive) == 0) {
            currentReport.actions.append({ScriptActionType::ConvertHours, value});
        } else if (key.compare("Minutes!", Qt::CaseInsensitive) == 0) {
            currentReport.actions.append({ScriptActionType::ConvertMinutes, value});
        } else if (key.compare("Seconds!", Qt::CaseInsensitive) == 0) {
            currentReport.actions.append({ScriptActionType::ConvertSeconds, value});
        } else if (key.compare("Round!", Qt::CaseInsensitive) == 0) {
            currentReport.actions.append({ScriptActionType::RoundTime, value});
        } else if (key.compare("Random!", Qt::CaseInsensitive) == 0) {
            currentReport.actions.append({ScriptActionType::RandomTime, value});
        } else if (key.compare("Drop!", Qt::CaseInsensitive) == 0) {
            currentReport.actions.append({ScriptActionType::DropTime, value});
        } else if (key.compare("Add#", Qt::CaseInsensitive) == 0) {
            currentReport.actions.append({ScriptActionType::AddCounter, value});
        } else if (key.compare("Subtract#", Qt::CaseInsensitive) == 0) {
            currentReport.actions.append({ScriptActionType::SubtractCounter, value});
        } else if (key.compare("Multiply#", Qt::CaseInsensitive) == 0) {
            currentReport.actions.append({ScriptActionType::MultiplyCounter, value});
        } else if (key.compare("Divide#", Qt::CaseInsensitive) == 0) {
            currentReport.actions.append({ScriptActionType::DivideCounter, value});
        } else if (key.compare("Days#", Qt::CaseInsensitive) == 0) {
            currentReport.actions.append({ScriptActionType::ExtractDays, value});
        } else if (key.compare("Hours#", Qt::CaseInsensitive) == 0) {
            currentReport.actions.append({ScriptActionType::ExtractHours, value});
        } else if (key.compare("Minutes#", Qt::CaseInsensitive) == 0) {
            currentReport.actions.append({ScriptActionType::ExtractMinutes, value});
        } else if (key.compare("Seconds#", Qt::CaseInsensitive) == 0) {
            currentReport.actions.append({ScriptActionType::ExtractSeconds, value});
        } else if (key.compare("Message", Qt::CaseInsensitive) == 0) {
            currentReport.actions.append({ScriptActionType::Message, value});
        } else if (key.compare("Question", Qt::CaseInsensitive) == 0) {
            currentReport.actions.append({ScriptActionType::Question, value});
        } else if (key.compare("Input", Qt::CaseInsensitive) == 0) {
            currentReport.actions.append({ScriptActionType::Input, value});
        } else if (key.compare("Clothing", Qt::CaseInsensitive) == 0) {
            if (currentReport.clothingInstruction.isEmpty()) {
                currentReport.actions.append({ScriptActionType::Clothing, value});
            } else {
                currentReport.actions.append({ScriptActionType::Clothing, value});
            }
        } else if (key.compare("Instructions", Qt::CaseInsensitive) == 0) {
            currentReport.actions.append({ScriptActionType::Instructions, value});
        } else if (key.compare("NewStatus", Qt::CaseInsensitive) == 0) {
            currentReport.actions.append({ScriptActionType::NewStatus, value});
        } else if (key.compare("NewSubStatus", Qt::CaseInsensitive) == 0) {
            currentReport.actions.append({ScriptActionType::NewSubStatus, value});
        } else if (key.compare("Job", Qt::CaseInsensitive) == 0) {
            currentReport.actions.append({ScriptActionType::AnnounceJob, value});
        } else if (key.compare("MarkDone", Qt::CaseInsensitive) == 0) {
            currentReport.actions.append({ScriptActionType::MarkDone, value});
        } else if (key.compare("Abort", Qt::CaseInsensitive) == 0) {
            currentReport.actions.append({ScriptActionType::Abort, value});
        } else if (key.compare("Delete", Qt::CaseInsensitive) == 0) {
            currentReport.actions.append({ScriptActionType::Delete, value});
        } else if (key.compare("Group", Qt::CaseInsensitive) == 0) {
            currentReport.stopAutoAssign = (value.trimmed() == "1");
        } else if (key.compare("AddMerit", Qt::CaseInsensitive) == 0 || key.compare("AddMerits", Qt::CaseInsensitive) == 0) {
            currentReport.actions.append({ScriptActionType::AddMerit, value});
        } else if (key.compare("SubtractMerit", Qt::CaseInsensitive) == 0 || key.compare("SubtractMerits", Qt::CaseInsensitive) == 0) {
            currentReport.actions.append({ScriptActionType::SubtractMerit, value});
        } else if (key.compare("SetMerit", Qt::CaseInsensitive) == 0 || key.compare("SetMerits", Qt::CaseInsensitive) == 0) {
            currentReport.actions.append({ScriptActionType::SetMerit, value});
        } else if (key.compare("PunishMessage", Qt::CaseInsensitive) == 0) {
            if (currentPunishMessage.isEmpty()) {
                currentPunishMessage = value;
            }
        } else if (key.compare("PunishmentGroup", Qt::CaseInsensitive) == 0) {
            if (currentReport.punishGroup.isEmpty()) {
                currentReport.punishGroup = value;
            }
        } else if (key.compare("Punish", Qt::CaseInsensitive) == 0) {
            currentReport.actions.append({ScriptActionType::Punish, value});
        } else if (key.compare("ClothReport", Qt::CaseInsensitive) == 0) {
            // Add as an action (value is "1" or "Title Text")
            if (value != "0") {
                currentReport.actions.append({ScriptActionType::ClothReport, value});
            }
        }
        else if (key.compare("ClearCloth", Qt::CaseInsensitive) == 0) {
            currentReport.actions.append({ScriptActionType::ClearCloth, value});
        }
        else if (key.compare("CameraInterval", Qt::CaseInsensitive) == 0) {
            QStringList parts = value.split(',', Qt::SkipEmptyParts);
            if (parts.size() == 2) {
                currentReport.cameraIntervalMin = parts[0].trimmed();
                currentReport.cameraIntervalMax = parts[1].trimmed();
            } else {
                currentReport.cameraIntervalMin = currentReport.cameraIntervalMax = value.trimmed();
            }
        }
        else if (key.compare("PointCamera", Qt::CaseInsensitive) == 0) {
            currentReport.pointCameraText = value;
            currentReport.actions.append({ScriptActionType::PointCamera, value});
        }
        else if (key.compare("PoseCamera", Qt::CaseInsensitive) == 0) {
            currentReport.poseCameraText = value;
            currentReport.actions.append({ScriptActionType::PoseCamera, value});
        }
        else if (key.compare("input$", Qt::CaseInsensitive) == 0) {
            currentReport.actions.append({ScriptActionType::InputString, value});
        }
    }

    // Save the very last report in the file
    if (inSection) {
        scriptData.reports.insert(currentReport.name, currentReport);
    }
}

void ScriptParser::parseConfessionSections(const QStringList& lines) {
    ConfessionDefinition currentConf;
    bool inSection = false;
    QString currentSectionName;
    QString currentPunishMessage;

    // Helper lambda for parsing time ranges
    auto parseTimeRange = [](const QString& val) -> TimeRange {
        QStringList parts = val.split(',', Qt::SkipEmptyParts);
        TimeRange tr;
        if (parts.size() == 2) {
            tr.min = parts[0].trimmed();
            tr.max = parts[1].trimmed();
        } else if (parts.size() == 1) {
            tr.min = tr.max = parts[0].trimmed();
        }
        return tr;
    };

    for (const QString& line : lines) {
        if (line.isEmpty() || line.startsWith("#") || line.startsWith(";"))
            continue; // Skip comments and empty lines

        // Check for section headers
        if (line.startsWith("[") && line.endsWith("]")) {
            // Save the previous confession
            if (inSection) {
                scriptData.confessions.insert(currentConf.name, currentConf);
            }
            inSection = false; // Reset

            currentSectionName = line.mid(1, line.length() - 2).trimmed();

            if (currentSectionName.startsWith("confession-", Qt::CaseInsensitive)) {
                inSection = true;
                currentConf = ConfessionDefinition(); // Start a new, clean confession
                currentConf.name = currentSectionName.mid(11).toLower();
                currentPunishMessage.clear();
            }
            continue;
        }

        if (!inSection) continue; // Skip lines not in a confession section

        int equalsIndex = line.indexOf('=');
        if (equalsIndex == -1) continue; // Not a key-value pair

        QString key = line.left(equalsIndex).trimmed();
        QString value = line.mid(equalsIndex + 1).trimmed();

        // --- 1. Handle PROPERTIES (not actions) ---
        if (key.compare("Title", Qt::CaseInsensitive) == 0) {
            currentConf.title = value;
        } else if (key.compare("OnTop", Qt::CaseInsensitive) == 0) {
            currentConf.onTop = (value != "0");
        } else if (key.compare("ShowInMenu", Qt::CaseInsensitive) == 0) {
            currentConf.showInMenu = (value != "0");
        } else if (key.compare("PreStatus", Qt::CaseInsensitive) == 0) {
            currentConf.preStatuses << value.split(',', Qt::SkipEmptyParts);
        } else if (key.compare("AddMerit", Qt::CaseInsensitive) == 0 || key.compare("AddMerits", Qt::CaseInsensitive) == 0) {
            currentConf.merits.add = value;
        } else if (key.compare("SubtractMerit", Qt::CaseInsensitive) == 0 || key.compare("SubtractMerits", Qt::CaseInsensitive) == 0) {
            currentConf.merits.subtract = value;
        } else if (key.compare("SetMerit", Qt::CaseInsensitive) == 0 || key.compare("SetMerits", Qt::CaseInsensitive) == 0) {
            currentConf.merits.set = value;
        } else if (key.compare("CenterRandom", Qt::CaseInsensitive) == 0) {
            currentConf.centerRandom = (value == "1");
        } else if (key.compare("Group", Qt::CaseInsensitive) == 0) {
            currentConf.group = value;
        } else if (key.compare("MasterMail", Qt::CaseInsensitive) == 0) {
            currentConf.masterMailSubject = value;
        } else if (key.compare("MasterAttach", Qt::CaseInsensitive) == 0) {
            currentConf.masterAttachments.append(value);
        } else if (key.compare("SubMail", Qt::CaseInsensitive) == 0) {
            currentConf.subMailLines.append(value);
        } else if (key.compare("Text", Qt::CaseInsensitive) == 0) {
            currentConf.statusTexts.append(value); // This is a property
        }
        // ... (add other non-action properties like TimeWindowControl) ...

        // --- 2. Handle ACTIONS (add to ordered list) ---
        else if (key.compare("Procedure", Qt::CaseInsensitive) == 0) {
            currentConf.actions.append({ScriptActionType::ProcedureCall, value});
        } else if (key.compare("If", Qt::CaseInsensitive) == 0) {
            currentConf.actions.append({ScriptActionType::If, value});
        } else if (key.compare("NotIf", Qt::CaseInsensitive) == 0) {
            currentConf.actions.append({ScriptActionType::NotIf, value});
        } else if (key.compare("SetFlag", Qt::CaseInsensitive) == 0) {
            currentConf.actions.append({ScriptActionType::SetFlag, value});
        } else if (key.compare("RemoveFlag", Qt::CaseInsensitive) == 0) {
            currentConf.actions.append({ScriptActionType::RemoveFlag, value});
        } else if (key.compare("ClearFlag", Qt::CaseInsensitive) == 0) {
            currentConf.actions.append({ScriptActionType::ClearFlag, value});
        } else if (key.compare("Set#", Qt::CaseInsensitive) == 0) {
            currentConf.actions.append({ScriptActionType::SetCounterVar, value});
        } else if (key.compare("Input#", Qt::CaseInsensitive) == 0) {
            currentConf.actions.append({ScriptActionType::InputCounter, value});
        } else if (key.compare("Change#", Qt::CaseInsensitive) == 0) {
            currentConf.actions.append({ScriptActionType::ChangeCounter, value});
        } else if (key.compare("InputNeg#", Qt::CaseInsensitive) == 0) {
            currentConf.actions.append({ScriptActionType::InputNegCounter, value});
        } else if (key.compare("Random#", Qt::CaseInsensitive) == 0) {
            currentConf.actions.append({ScriptActionType::RandomCounter, value});
        } else if (key.compare("Drop#", Qt::CaseInsensitive) == 0) {
            currentConf.actions.append({ScriptActionType::DropCounter, value});
        } else if (key.compare("Set!", Qt::CaseInsensitive) == 0) {
            currentConf.actions.append({ScriptActionType::SetTimeVar, value});
        } else if (key.compare("Input!", Qt::CaseInsensitive) == 0) {
            currentConf.actions.append({ScriptActionType::InputDate, value});
        } else if (key.compare("InputDateDef!", Qt::CaseInsensitive) == 0) {
            currentConf.actions.append({ScriptActionType::InputDateDef, value});
        } else if (key.compare("ChangeDate!", Qt::CaseInsensitive) == 0) {
            currentConf.actions.append({ScriptActionType::ChangeDate, value});
        } else if (key.compare("InputTime!", Qt::CaseInsensitive) == 0) {
            currentConf.actions.append({ScriptActionType::InputTime, value});
        } else if (key.compare("InputTimeDef!", Qt::CaseInsensitive) == 0) {
            currentConf.actions.append({ScriptActionType::InputTimeDef, value});
        } else if (key.compare("ChangeTime!", Qt::CaseInsensitive) == 0) {
            currentConf.actions.append({ScriptActionType::ChangeTime, value});
        } else if (key.compare("InputInterval!", Qt::CaseInsensitive) == 0) {
            currentConf.actions.append({ScriptActionType::InputInterval, value});
        } else if (key.compare("ChangeInterval!", Qt::CaseInsensitive) == 0) {
            currentConf.actions.append({ScriptActionType::ChangeInterval, value});
        } else if (key.compare("AddDays!", Qt::CaseInsensitive) == 0) {
            currentConf.actions.append({ScriptActionType::AddDaysTime, value});
        } else if (key.contains("SubtractDays!", Qt::CaseInsensitive) == 0) {
            currentConf.actions.append({ScriptActionType::SubtractDaysTime, value});
        } else if (key.compare("Days#", Qt::CaseInsensitive) == 0) {
            currentConf.actions.append({ScriptActionType::ExtractDays, value});
        } else if (key.contains("Hours#", Qt::CaseInsensitive) == 0) {
            currentConf.actions.append({ScriptActionType::ExtractHours, value});
        } else if (key.compare("Minutes#", Qt::CaseInsensitive) == 0) {
            currentConf.actions.append({ScriptActionType::ExtractMinutes, value});
        } else if (key.compare("Seconds#", Qt::CaseInsensitive) == 0) {
            currentConf.actions.append({ScriptActionType::ExtractSeconds, value});
        } else if (key.compare("Days!", Qt::CaseInsensitive) == 0) {
            currentConf.actions.append({ScriptActionType::ConvertDays, value});
        } else if (key.compare("Hours!", Qt::CaseInsensitive) == 0) {
            currentConf.actions.append({ScriptActionType::ConvertHours, value});
        } else if (key.compare("Minutes!", Qt::CaseInsensitive) == 0) {
            currentConf.actions.append({ScriptActionType::ConvertMinutes, value});
        } else if (key.compare("Seconds!", Qt::CaseInsensitive) == 0) {
            currentConf.actions.append({ScriptActionType::ConvertSeconds, value});
        } else if (key.compare("Round!", Qt::CaseInsensitive) == 0) {
            currentConf.actions.append({ScriptActionType::RoundTime, value});
        } else if (key.compare("Random!", Qt::CaseInsensitive) == 0) {
            currentConf.actions.append({ScriptActionType::RandomTime, value});
        } else if (key.compare("Drop!", Qt::CaseInsensitive) == 0) {
            currentConf.actions.append({ScriptActionType::DropTime, value});
        }
        // ... (add all other action types just like we did for parseProcedureSections) ...
        else if (key.compare("Add#", Qt::CaseInsensitive) == 0) {
            currentConf.actions.append({ScriptActionType::AddCounter, value});
        } else if (key.compare("Subtract#", Qt::CaseInsensitive) == 0) {
            currentConf.actions.append({ScriptActionType::SubtractCounter, value});
        } else if (key.compare("Multiply#", Qt::CaseInsensitive) == 0) {
            currentConf.actions.append({ScriptActionType::MultiplyCounter, value});
        } else if (key.compare("Divide#", Qt::CaseInsensitive) == 0) {
            currentConf.actions.append({ScriptActionType::DivideCounter, value});
        } else if (key.compare("Message", Qt::CaseInsensitive) == 0) {
            currentConf.actions.append({ScriptActionType::Message, value});
        } else if (key.compare("Question", Qt::CaseInsensitive) == 0) {
            currentConf.actions.append({ScriptActionType::Question, value});
        } else if (key.compare("Input", Qt::CaseInsensitive) == 0) {
            currentConf.actions.append({ScriptActionType::Input, value});
        } else if (key.compare("NewStatus", Qt::CaseInsensitive) == 0) {
            currentConf.actions.append({ScriptActionType::NewStatus, value});
        } else if (key.compare("NewSubStatus", Qt::CaseInsensitive) == 0) {
            currentConf.actions.append({ScriptActionType::NewSubStatus, value});
        } else if (key.compare("MarkDone", Qt::CaseInsensitive) == 0) {
            currentConf.actions.append({ScriptActionType::MarkDone, value});
        } else if (key.compare("Abort", Qt::CaseInsensitive) == 0) {
            currentConf.actions.append({ScriptActionType::Abort, value});
        } else if (key.compare("Delete", Qt::CaseInsensitive) == 0) {
            currentConf.actions.append({ScriptActionType::Delete, value});
        } else if (key.compare("AddMerit", Qt::CaseInsensitive) == 0 || key.compare("AddMerits", Qt::CaseInsensitive) == 0) {
            currentConf.actions.append({ScriptActionType::AddMerit, value});
        } else if (key.compare("SubtractMerit", Qt::CaseInsensitive) == 0 || key.compare("SubtractMerits", Qt::CaseInsensitive) == 0) {
            currentConf.actions.append({ScriptActionType::SubtractMerit, value});
        } else if (key.compare("SetMerit", Qt::CaseInsensitive) == 0 || key.compare("SetMerits", Qt::CaseInsensitive) == 0) {
            currentConf.actions.append({ScriptActionType::SetMerit, value});
        } else if (key.compare("PunishMessage", Qt::CaseInsensitive) == 0) {
            if (currentPunishMessage.isEmpty()) {
                currentPunishMessage = value;
            }
        } else if (key.compare("PunishmentGroup", Qt::CaseInsensitive) == 0) {
            if (currentConf.punishGroup.isEmpty()) {
                currentConf.punishGroup = value;
            }
        } else if (key.compare("Punish", Qt::CaseInsensitive) == 0) {
            currentConf.actions.append({ScriptActionType::Punish, value});
        } else if (key.compare("Clothing", Qt::CaseInsensitive) == 0) {
            if (currentConf.clothingInstruction.isEmpty()) {
                currentConf.actions.append({ScriptActionType::Clothing, value});
            } else {
                currentConf.actions.append({ScriptActionType::Clothing, value});
            }
        } else if (key.compare("Instructions", Qt::CaseInsensitive) == 0) {
            currentConf.actions.append({ScriptActionType::Instructions, value});
        } else if (key.compare("ClothReport", Qt::CaseInsensitive) == 0) {
            // Add as an action (value is "1" or "Title Text")
            if (value != "0") {
                currentConf.actions.append({ScriptActionType::ClothReport, value});
            }
        }
        else if (key.compare("ClearCloth", Qt::CaseInsensitive) == 0) {
            currentConf.actions.append({ScriptActionType::ClearCloth, value});
        }
        else if (key.compare("CameraInterval", Qt::CaseInsensitive) == 0) {
            QStringList parts = value.split(',', Qt::SkipEmptyParts);
            if (parts.size() == 2) {
                currentConf.cameraIntervalMin = parts[0].trimmed();
                currentConf.cameraIntervalMax = parts[1].trimmed();
            } else {
                currentConf.cameraIntervalMin = currentConf.cameraIntervalMax = value.trimmed();
            }
        }
        else if (key.compare("PointCamera", Qt::CaseInsensitive) == 0) {
            currentConf.pointCameraText = value;
            currentConf.actions.append({ScriptActionType::PointCamera, value});
        }
        else if (key.compare("PoseCamera", Qt::CaseInsensitive) == 0) {
            currentConf.poseCameraText = value;
            currentConf.actions.append({ScriptActionType::PoseCamera, value});
        }
        else if (key.compare("Set$", Qt::CaseInsensitive) == 0) {
            currentConf.actions.append({ScriptActionType::SetString, value});
        }
        else if (key.compare("Input$", Qt::CaseInsensitive) == 0) {
            currentConf.actions.append({ScriptActionType::InputString, value});
        }
        else if (key.compare("Change$", Qt::CaseInsensitive) == 0) {
            currentConf.actions.append({ScriptActionType::ChangeString, value});
        }
        else if (key.compare("InputLong$", Qt::CaseInsensitive) == 0) {
            currentConf.actions.append({ScriptActionType::InputLongString, value});
        }
        else if (key.compare("ChangeLong$", Qt::CaseInsensitive) == 0) {
            currentConf.actions.append({ScriptActionType::ChangeLongString, value});
        }
        else if (key.compare("Drop$", Qt::CaseInsensitive) == 0) {
            currentConf.actions.append({ScriptActionType::DropString, value});
        }
    }

    // Save the very last confession in the file
    if (inSection) {
        scriptData.confessions.insert(currentConf.name, currentConf);
    }
}

void ScriptParser::parsePermissionSections(const QStringList& lines) {
    PermissionDefinition currentPermission;
    bool inSection = false;
    QString currentSectionName;
    QString currentPunishMessage;

    // Helper lambda for parsing time ranges
    auto parseTimeRange = [](const QString& val) -> TimeRange {
        QStringList parts = val.split(',', Qt::SkipEmptyParts);
        TimeRange tr;
        if (parts.size() == 2) {
            tr.min = parts[0].trimmed();
            tr.max = parts[1].trimmed();
        } else if (parts.size() == 1) {
            tr.min = tr.max = parts[0].trimmed();
        }
        return tr;
    };

    // Helper lambda for parsing int ranges
    auto parseRange = [](const QString& v, int& minVal, int& maxVal) {
        QStringList parts = v.split(',', Qt::SkipEmptyParts);
        minVal = parts.value(0).toInt();
        maxVal = parts.size() > 1 ? parts.value(1).toInt() : minVal;
    };

    for (const QString& line : lines) {
        if (line.isEmpty() || line.startsWith("#") || line.startsWith(";"))
            continue;

        // Check for section headers
        if (line.startsWith("[") && line.endsWith("]")) {
            if (inSection) {
                // Save the previous permission
                scriptData.permissions.insert(currentPermission.name, currentPermission);
            }
            inSection = false; // Reset

            currentSectionName = line.mid(1, line.length() - 2).trimmed();

            if (currentSectionName.startsWith("permission-", Qt::CaseInsensitive)) {
                inSection = true;
                currentPermission = PermissionDefinition();
                currentPermission.name = currentSectionName.mid(11).toLower();
                currentPunishMessage.clear();
            }
            continue;
        }

        if (!inSection) continue; // Skip lines not in a permission section

        int equalsIndex = line.indexOf('=');
        if (equalsIndex == -1) continue; // Not a key-value pair

        QString key = line.left(equalsIndex).trimmed();
        QString value = line.mid(equalsIndex + 1).trimmed();

        // --- Handle PROPERTIES ---
        if (key.compare("Title", Qt::CaseInsensitive) == 0) {
            currentPermission.title = value;
        } else if (key.compare("PreStatus", Qt::CaseInsensitive) == 0) {
            currentPermission.preStatuses << value.split(',', Qt::SkipEmptyParts);
        } else if (key.compare("Pct", Qt::CaseInsensitive) == 0) {
            if (value.compare("Var", Qt::CaseInsensitive) == 0) {
                currentPermission.pctIsVariable = true;
            } else {
                parseRange(value, currentPermission.pctMin, currentPermission.pctMax);
                if (currentPermission.pctMin = currentPermission.pctMax) {
                    currentPermission.pct = currentPermission.pctMin;
                }
            }
        } else if (key.compare("MinInterval", Qt::CaseInsensitive) == 0) {
            currentPermission.minInterval = parseTimeRange(value);
        } else if (key.compare("Delay", Qt::CaseInsensitive) == 0) {
            currentPermission.delay = parseTimeRange(value);
        } else if (key.compare("MaxWait", Qt::CaseInsensitive) == 0) {
            currentPermission.maxWait = parseTimeRange(value);
        } else if (key.compare("Notify", Qt::CaseInsensitive) == 0) {
            currentPermission.notify = value.toInt();
        } else if (key.compare("AddMerit", Qt::CaseInsensitive) == 0 || key.compare("AddMerits", Qt::CaseInsensitive) == 0) {
            currentPermission.merits.add = value;
        } else if (key.compare("SubtractMerit", Qt::CaseInsensitive) == 0 || key.compare("SubtractMerits", Qt::CaseInsensitive) == 0) {
            currentPermission.merits.subtract = value;
        } else if (key.compare("SetMerit", Qt::CaseInsensitive) == 0 || key.compare("SetMerits", Qt::CaseInsensitive) == 0) {
            currentPermission.merits.set = value;
        } else if (key.compare("CenterRandom", Qt::CaseInsensitive) == 0) {
            currentPermission.centerRandom = (value == "1");
        } else if (key.compare("Group", Qt::CaseInsensitive) == 0) {
            currentPermission.group = value;
        } else if (key.compare("DenyBefore", Qt::CaseInsensitive) == 0) {
            QStringList parts = value.split(',', Qt::SkipEmptyParts);
            currentPermission.denyBeforeStart = parts.value(0);
            currentPermission.denyBeforeEnd = parts.value(1, parts.value(0));
        } else if (key.compare("DenyAfter", Qt::CaseInsensitive) == 0) {
            QStringList parts = value.split(',', Qt::SkipEmptyParts);
            currentPermission.denyAfterStart = parts.value(0);
            currentPermission.denyAfterEnd = parts.value(1, parts.value(0));
        } else if (key.compare("DenyBetween", Qt::CaseInsensitive) == 0) {
            QStringList parts = value.split(',', Qt::SkipEmptyParts);
            if (parts.size() == 2)
                currentPermission.denyBetweenRanges.append({ parts[0].trimmed(), parts[1].trimmed() });
        } else if (key.compare("DenyBelow", Qt::CaseInsensitive) == 0) {
            parseRange(value, currentPermission.denyBelowMin, currentPermission.denyBelowMax);
        } else if (key.compare("DenyAbove", Qt::CaseInsensitive) == 0) {
            parseRange(value, currentPermission.denyAboveMin, currentPermission.denyAboveMax);
        } else if (key.compare("HighMerits", Qt::CaseInsensitive) == 0) {
            parseRange(value, currentPermission.highMeritsMin, currentPermission.highMeritsMax);
        } else if (key.compare("HighPct", Qt::CaseInsensitive) == 0) {
            parseRange(value, currentPermission.highPctMin, currentPermission.highPctMax);
        } else if (key.compare("LowMerits", Qt::CaseInsensitive) == 0) {
            parseRange(value, currentPermission.lowMeritsMin, currentPermission.lowMeritsMax);
        } else if (key.compare("LowPct", Qt::CaseInsensitive) == 0) {
            parseRange(value, currentPermission.lowPctMin, currentPermission.lowPctMax);
        } else if (key.compare("BeforeProcedure", Qt::CaseInsensitive) == 0) {
            currentPermission.beforeProcedure = value;
        } else if (key.compare("PermitMessage", Qt::CaseInsensitive) == 0) {
            currentPermission.permitMessage = value;
        } else if (key.compare("DenyProcedure", Qt::CaseInsensitive) == 0) {
            currentPermission.denyProcedure = value;
        } else if (key.compare("DenyFlag", Qt::CaseInsensitive) == 0) {
            currentPermission.denyFlags += value.split(',', Qt::SkipEmptyParts);
        } else if (key.compare("DenyLaunch", Qt::CaseInsensitive) == 0) {
            currentPermission.denyLaunch = value;
        } else if (key.compare("DenyStatus", Qt::CaseInsensitive) == 0) {
            currentPermission.denyStatus = value;
        } else if (key.compare("MasterMail", Qt::CaseInsensitive) == 0) {
            currentPermission.masterMailSubject = value;
        } else if (key.compare("MasterAttach", Qt::CaseInsensitive) == 0) {
            currentPermission.masterAttachments.append(value);
        } else if (key.compare("SubMail", Qt::CaseInsensitive) == 0) {
            currentPermission.subMailLines.append(value);
        } else if (key.compare("PointCamera", Qt::CaseInsensitive) == 0) {
            currentPermission.pointCameraText = value;
        }

        // --- Handle ACTIONS
        else if (key.compare("Procedure", Qt::CaseInsensitive) == 0) {
            currentPermission.actions.append({ScriptActionType::ProcedureCall, value});
        } else if (key.compare("If", Qt::CaseInsensitive) == 0) {
            currentPermission.actions.append({ScriptActionType::If, value});
        } else if (key.compare("NotIf", Qt::CaseInsensitive) == 0) {
            currentPermission.actions.append({ScriptActionType::NotIf, value});
        } else if (key.compare("SetFlag", Qt::CaseInsensitive) == 0) {
            currentPermission.actions.append({ScriptActionType::SetFlag, value});
        } else if (key.compare("RemoveFlag", Qt::CaseInsensitive) == 0) {
            currentPermission.actions.append({ScriptActionType::RemoveFlag, value});
        } else if (key.compare("ClearFlag", Qt::CaseInsensitive) == 0) {
            currentPermission.actions.append({ScriptActionType::ClearFlag, value});
        } else if (key.compare("Set#", Qt::CaseInsensitive) == 0) {
            currentPermission.actions.append({ScriptActionType::SetCounterVar, value});
        } else if (key.compare("Input#", Qt::CaseInsensitive) == 0) {
            currentPermission.actions.append({ScriptActionType::InputCounter, value});
        } else if (key.compare("Change#", Qt::CaseInsensitive) == 0) {
            currentPermission.actions.append({ScriptActionType::ChangeCounter, value});
        } else if (key.compare("InputNeg#", Qt::CaseInsensitive) == 0) {
            currentPermission.actions.append({ScriptActionType::InputNegCounter, value});
        } else if (key.compare("Random#", Qt::CaseInsensitive) == 0) {
            currentPermission.actions.append({ScriptActionType::RandomCounter, value});
        } else if (key.compare("Drop#", Qt::CaseInsensitive) == 0) {
            currentPermission.actions.append({ScriptActionType::DropCounter, value});
        } else if (key.compare("Set$", Qt::CaseInsensitive) == 0) {
            currentPermission.actions.append({ScriptActionType::SetString, value});
        } else if (key.compare("Input$", Qt::CaseInsensitive) == 0) {
            currentPermission.actions.append({ScriptActionType::InputString, value});
        } else if (key.compare("Change$", Qt::CaseInsensitive) == 0) {
            currentPermission.actions.append({ScriptActionType::ChangeString, value});
        } else if (key.compare("InputLong$", Qt::CaseInsensitive) == 0) {
            currentPermission.actions.append({ScriptActionType::InputLongString, value});
        } else if (key.compare("ChangeLong$", Qt::CaseInsensitive) == 0) {
            currentPermission.actions.append({ScriptActionType::ChangeLongString, value});
        } else if (key.compare("Drop$", Qt::CaseInsensitive) == 0) {
            currentPermission.actions.append({ScriptActionType::DropString, value});
        } else if (key.compare("Set!", Qt::CaseInsensitive) == 0) {
            currentPermission.actions.append({ScriptActionType::SetTimeVar, value});
        } else if (key.compare("Input!", Qt::CaseInsensitive) == 0) {
            currentPermission.actions.append({ScriptActionType::InputDate, value});
        } else if (key.compare("InputDateDef!", Qt::CaseInsensitive) == 0) {
            currentPermission.actions.append({ScriptActionType::InputDateDef, value});
        } else if (key.compare("ChangeDate!", Qt::CaseInsensitive) == 0) {
            currentPermission.actions.append({ScriptActionType::ChangeDate, value});
        } else if (key.compare("InputTime!", Qt::CaseInsensitive) == 0) {
            currentPermission.actions.append({ScriptActionType::InputTime, value});
        } else if (key.compare("InputTimeDef!", Qt::CaseInsensitive) == 0) {
            currentPermission.actions.append({ScriptActionType::InputTimeDef, value});
        } else if (key.compare("ChangeTime!", Qt::CaseInsensitive) == 0) {
            currentPermission.actions.append({ScriptActionType::ChangeTime, value});
        } else if (key.compare("InputInterval!", Qt::CaseInsensitive) == 0) {
            currentPermission.actions.append({ScriptActionType::InputInterval, value});
        } else if (key.compare("ChangeInterval!", Qt::CaseInsensitive) == 0) {
            currentPermission.actions.append({ScriptActionType::ChangeInterval, value});
        } else if (key.compare("AddDays!", Qt::CaseInsensitive) == 0) {
            currentPermission.actions.append({ScriptActionType::AddDaysTime, value});
        } else if (key.compare("SubtractDays!", Qt::CaseInsensitive) == 0) {
            currentPermission.actions.append({ScriptActionType::SubtractDaysTime, value});
        } else if (key.compare("Days!", Qt::CaseInsensitive) == 0) {
            currentPermission.actions.append({ScriptActionType::ConvertDays, value});
        } else if (key.compare("Hours!", Qt::CaseInsensitive) == 0) {
            currentPermission.actions.append({ScriptActionType::ConvertHours, value});
        } else if (key.compare("Minutes!", Qt::CaseInsensitive) == 0) {
            currentPermission.actions.append({ScriptActionType::ConvertMinutes, value});
        } else if (key.compare("Seconds!", Qt::CaseInsensitive) == 0) {
            currentPermission.actions.append({ScriptActionType::ConvertSeconds, value});
        } else if (key.compare("Round!", Qt::CaseInsensitive) == 0) {
            currentPermission.actions.append({ScriptActionType::RoundTime, value});
        } else if (key.compare("Random!", Qt::CaseInsensitive) == 0) {
            currentPermission.actions.append({ScriptActionType::RandomTime, value});
        } else if (key.compare("Drop!", Qt::CaseInsensitive) == 0) {
            currentPermission.actions.append({ScriptActionType::DropTime, value});
        } else if (key.compare("Add#", Qt::CaseInsensitive) == 0) {
            currentPermission.actions.append({ScriptActionType::AddCounter, value});
        } else if (key.compare("Subtract#", Qt::CaseInsensitive) == 0) {
            currentPermission.actions.append({ScriptActionType::SubtractCounter, value});
        } else if (key.compare("Multiply#", Qt::CaseInsensitive) == 0) {
            currentPermission.actions.append({ScriptActionType::MultiplyCounter, value});
        } else if (key.compare("Divide#", Qt::CaseInsensitive) == 0) {
            currentPermission.actions.append({ScriptActionType::DivideCounter, value});
        } else if (key.compare("Days#", Qt::CaseInsensitive) == 0) {
            currentPermission.actions.append({ScriptActionType::ExtractDays, value});
        } else if (key.compare("Hours#", Qt::CaseInsensitive) == 0) {
            currentPermission.actions.append({ScriptActionType::ExtractHours, value});
        } else if (key.contains("Minutes#", Qt::CaseInsensitive) == 0) {
            currentPermission.actions.append({ScriptActionType::ExtractMinutes, value});
        } else if (key.compare("Seconds#", Qt::CaseInsensitive) == 0) {
            currentPermission.actions.append({ScriptActionType::ExtractSeconds, value});
        } else if (key.compare("Message", Qt::CaseInsensitive) == 0) {
            currentPermission.actions.append({ScriptActionType::Message, value});
        } else if (key.compare("Question", Qt::CaseInsensitive) == 0) {
            currentPermission.actions.append({ScriptActionType::Question, value});
        } else if (key.compare("Input", Qt::CaseInsensitive) == 0) {
            currentPermission.actions.append({ScriptActionType::Input, value});
        } else if (key.compare("Clothing", Qt::CaseInsensitive) == 0) {
            if (currentPermission.clothingInstruction.isEmpty()) {
                currentPermission.actions.append({ScriptActionType::Clothing, value});
            } else {
                currentPermission.actions.append({ScriptActionType::Clothing, value});
            }
        } else if (key.compare("Instructions", Qt::CaseInsensitive) == 0) {
            currentPermission.actions.append({ScriptActionType::Instructions, value});
        } else if (key.compare("NewStatus", Qt::CaseInsensitive) == 0) {
            currentPermission.actions.append({ScriptActionType::NewStatus, value});
        } else if (key.compare("NewSubStatus", Qt::CaseInsensitive) == 0) {
            currentPermission.actions.append({ScriptActionType::NewSubStatus, value});
        } else if (key.compare("Job", Qt::CaseInsensitive) == 0) {
            currentPermission.actions.append({ScriptActionType::AnnounceJob, value});
        } else if (key.compare("MarkDone", Qt::CaseInsensitive) == 0) {
            currentPermission.actions.append({ScriptActionType::MarkDone, value});
        } else if (key.compare("Abort", Qt::CaseInsensitive) == 0) {
            currentPermission.actions.append({ScriptActionType::Abort, value});
        } else if (key.compare("Delete", Qt::CaseInsensitive) == 0) {
            currentPermission.actions.append({ScriptActionType::Delete, value});
        } else if (key.compare("AddMerit", Qt::CaseInsensitive) == 0 || key.compare("AddMerits", Qt::CaseInsensitive) == 0) {
            currentPermission.actions.append({ScriptActionType::AddMerit, value});
        } else if (key.compare("SubtractMerit", Qt::CaseInsensitive) == 0 || key.compare("SubtractMerits", Qt::CaseInsensitive) == 0) {
            currentPermission.actions.append({ScriptActionType::SubtractMerit, value});
        } else if (key.compare("SetMerit", Qt::CaseInsensitive) == 0 || key.compare("SetMerits", Qt::CaseInsensitive) == 0) {
            currentPermission.actions.append({ScriptActionType::SetMerit, value});
        } else if (key.compare("PunishMessage", Qt::CaseInsensitive) == 0) {
            if (currentPunishMessage.isEmpty()) {
                currentPunishMessage = value;
            }
        } else if (key.compare("PunishmentGroup", Qt::CaseInsensitive) == 0) {
            if (currentPermission.punishGroup.isEmpty()) {
                currentPermission.punishGroup = value;
            }
        } else if (key.compare("Punish", Qt::CaseInsensitive) == 0) {
            currentPermission.actions.append({ScriptActionType::Punish, value});
        } else if (key.compare("ClothReport", Qt::CaseInsensitive) == 0) {
            // Add as an action (value is "1" or "Title Text")
            if (value != "0") {
                currentPermission.actions.append({ScriptActionType::ClothReport, value});
            }
        }
        else if (key.compare("ClearCloth", Qt::CaseInsensitive) == 0) {
            currentPermission.actions.append({ScriptActionType::ClearCloth, value});
        }
        else if (key.compare("CameraInterval", Qt::CaseInsensitive) == 0) {
            QStringList parts = value.split(',', Qt::SkipEmptyParts);
            if (parts.size() == 2) {
                currentPermission.cameraIntervalMin = parts[0].trimmed();
                currentPermission.cameraIntervalMax = parts[1].trimmed();
            } else {
                currentPermission.cameraIntervalMin = currentPermission.cameraIntervalMax = value.trimmed();
            }
        }
        else if (key.compare("PointCamera", Qt::CaseInsensitive) == 0) {
            currentPermission.pointCameraText = value;
            currentPermission.actions.append({ScriptActionType::PointCamera, value});
        }
        else if (key.compare("PoseCamera", Qt::CaseInsensitive) == 0) {
            currentPermission.poseCameraText = value;
            currentPermission.actions.append({ScriptActionType::PoseCamera, value});
        }
        else if (key.compare("input$", Qt::CaseInsensitive) == 0) {
            currentPermission.actions.append({ScriptActionType::InputString, value});
        }
    }

    // Save the very last permission in the file
    if (inSection) {
        scriptData.permissions.insert(currentPermission.name, currentPermission);
    }
}

void ScriptParser::parsePunishmentSections(const QMap<QString, QMap<QString, QStringList>>& sections) {
    for (auto it = sections.begin(); it != sections.end(); ++it) {
        const QString& sectionName = it.key();
        if (!sectionName.startsWith("punishment-"))
            continue;

        QString punishmentName = sectionName.mid(QString("punishment-").length());

        // --- FIX: Create Case-Insensitive Entry Map ---
        QMap<QString, QStringList> entries;
        const QMap<QString, QStringList>& rawEntries = it.value();
        for (auto e = rawEntries.begin(); e != rawEntries.end(); ++e) {
            entries.insert(e.key().toLower(), e.value());
        }
        // ----------------------------------------------

        PunishmentDefinition p;
        p.name = punishmentName;

        // Now use lowercase keys for EVERYTHING
        if (entries.contains("valueunit"))
            p.valueUnit = entries["valueunit"].value(0).trimmed().toLower();

        if (entries.contains("value"))
            p.value = entries["value"].value(0).toDouble();

        if (entries.contains("min"))
            p.min = entries["min"].value(0).toInt();

        if (entries.contains("max"))
            p.max = entries["max"].value(0).toInt();

        if (entries.contains("minseverity"))
            p.minSeverity = entries["minseverity"].value(0).toInt();

        if (entries.contains("maxseverity"))
            p.maxSeverity = entries["maxseverity"].value(0).toInt();

        if (entries.contains("weight")) {
            QStringList parts = entries["weight"].value(0).split(',', Qt::SkipEmptyParts);
            p.weightMin = parts.value(0).trimmed();
            p.weightMax = parts.value(1, p.weightMin).trimmed();
        }

        if (entries.contains("group")) {
            for (const QString& g : entries["group"])
                p.groups.append(g.split(',', Qt::SkipEmptyParts));
        }

        if (entries.contains("grouponly"))
            p.groupOnly = entries["grouponly"].value(0).trimmed() == "1";

        if (entries.contains("longrunning"))
            p.longRunning = entries["longrunning"].value(0).trimmed() == "1";

        if (entries.contains("muststart"))
            p.mustStart = entries["muststart"].value(0).trimmed() == "1";

        if (entries.contains("deleteallowed"))
            p.deleteAllowed = entries["deleteallowed"].value(0).trimmed() == "1";

        if (entries.contains("startflag"))
            p.startFlag = entries["startflag"].value(0).trimmed();

        if (entries.contains("interruptable") || entries.contains("interruptible")) {
            QString val = entries.value("interruptable", entries.value("interruptible")).value(0).trimmed();
            if (val == "0") p.interruptable = false;
        }

        if (entries.contains("accumulative"))
            p.accumulative = entries["accumulative"].value(0).trimmed() == "1";

        if (entries.contains("respite"))
            p.respite = entries["respite"].value(0).trimmed();
        if (entries.contains("respit"))
            p.respite = entries["respit"].value(0).trimmed();

        if (entries.contains("estimate"))
            p.estimate = entries["estimate"].value(0).trimmed();

        if (entries.contains("forbid")) {
            for (const QString& f : entries["forbid"])
                p.forbids.append(f.split(',', Qt::SkipEmptyParts));
        }

        // Procedures
        if (entries.contains("startprocedure")) p.startProcedure = entries["startprocedure"].value(0).trimmed();
        if (entries.contains("beforedoneprocedure")) p.beforeDoneProcedure = entries["beforedoneprocedure"].value(0).trimmed();
        if (entries.contains("abortprocedure")) p.abortProcedure = entries["abortprocedure"].value(0).trimmed();
        if (entries.contains("doneprocedure")) p.doneProcedure = entries["doneprocedure"].value(0).trimmed();
        if (entries.contains("remindprocedure")) p.remindProcedure = entries["remindprocedure"].value(0).trimmed();
        if (entries.contains("announceprocedure")) p.announceProcedure = entries["announceprocedure"].value(0).trimmed();
        if (entries.contains("beforeprocedure")) p.beforeProcedure = entries["beforeprocedure"].value(0).trimmed();
        if (entries.contains("deleteprocedure")) p.deleteProcedure = entries["deleteprocedure"].value(0).trimmed();
        if (entries.contains("beforedeleteprocedure")) p.beforeDeleteProcedure = entries["beforedeleteprocedure"].value(0).trimmed();

        // Merits
        if (entries.contains("addmerit") || entries.contains("addmerits"))
            p.merits.add = entries.value("addmerit", entries.value("addmerits")).value(0);
        if (entries.contains("subtractmerit") || entries.contains("subtractmerits"))
            p.merits.subtract = entries.value("subtractmerit", entries.value("subtractmerits")).value(0);
        if (entries.contains("setmerit") || entries.contains("setmerits"))
            p.merits.set = entries.value("setmerit", entries.value("setmerits")).value(0);

        if (entries.contains("centerrandom"))
            p.centerRandom = entries["centerrandom"].value(0).trimmed() == "1";

        if (entries.contains("title"))
            p.title = entries["title"].value(0);

        if (entries.contains("newstatus"))
            p.newStatus = entries["newstatus"].value(0);

        // Messages (Use rawEntries to preserve casing/order of values)
        MessageGroup currentGroup;
        bool inMessageBlock = false;
        for (auto keyIt = rawEntries.begin(); keyIt != rawEntries.end(); ++keyIt) {
            QString key = keyIt.key();
            const QStringList& values = keyIt.value();
            for (const QString& val : values) {
                if (key.compare("Select", Qt::CaseInsensitive) == 0) {
                    if (val.compare("Random", Qt::CaseInsensitive) == 0)
                        currentGroup.mode = MessageSelectMode::Random;
                    else
                        currentGroup.mode = MessageSelectMode::All;
                    inMessageBlock = true;
                    if (!currentGroup.messages.isEmpty()) {
                        p.messages.append(currentGroup);
                        currentGroup = MessageGroup();
                    }
                }
                else if (key.compare("Message", Qt::CaseInsensitive) == 0) {
                    inMessageBlock = true;
                    currentGroup.messages.append(val.trimmed());
                }
                else if (key.compare("Text", Qt::CaseInsensitive) == 0) {
                    p.statusTexts.append(val.trimmed());
                }
            }
        }
        if (!currentGroup.messages.isEmpty()) {
            p.messages.append(currentGroup);
        }

        if (entries.contains("input"))
            p.inputQuestions.append(entries["input"]);
        if (entries.contains("noinputprocedure"))
            p.noInputProcedure = entries["noinputprocedure"].value(0);
        if (entries.contains("question"))
            p.advancedQuestions.append(entries["question"]);

        if (entries.contains("resource")) {
            for (const QString& value : entries["resource"])
                p.resources.append(value.trimmed());
        }

        if (entries.contains("autoassign")) {
            QStringList parts = entries["autoassign"].value(0).split(',', Qt::SkipEmptyParts);
            p.autoAssignMin = parts.value(0).toInt();
            p.autoAssignMax = parts.value(1, parts.value(0)).toInt();
        }

        if (entries.contains("clothing"))
            p.clothingInstruction = entries["clothing"].value(0);
        if (entries.contains("clearcheck"))
            p.clearClothingCheck = entries["clearcheck"].value(0).trimmed() == "1";

        // --- Line Writing Checks ---
        if (entries.contains("type") && entries["type"].value(0).trimmed().compare("Lines", Qt::CaseInsensitive) == 0)
            p.isLineWriting = true;

        if (entries.contains("line"))
            p.lines = entries["line"];

        if (entries.contains("select"))
            p.lineSelectMode = entries["select"].value(0).trimmed();

        if (entries.contains("pagesize")) {
            QStringList parts = entries["pagesize"].value(0).split(',', Qt::SkipEmptyParts);
            p.pageSizeMin = parts.value(0);
            p.pageSizeMax = parts.value(1, parts.value(0));
        }
        // ---------------------------

        if (entries.contains("type") && entries["type"].value(0).trimmed().compare("Detention", Qt::CaseInsensitive) == 0)
            p.isDetention = true;

        if (entries.contains("text")) {
            // Note: statusTexts logic above handles this, but detention text might need this loop if separate
            for (const QString& line : entries["text"])
                p.detentionText.append(line.trimmed());
        }

        if (entries.contains("posecamera"))
            p.poseCameraText = entries["posecamera"].value(0);
        if (entries.contains("pointcamera"))
            p.pointCameraText = entries["pointcamera"].value(0);

        if (entries.contains("camerainterval")) {
            QStringList parts = entries["camerainterval"].value(0).split(',', Qt::SkipEmptyParts);
            p.cameraIntervalMin = parts.value(0).trimmed();
            p.cameraIntervalMax = parts.value(1, p.cameraIntervalMin).trimmed();
        }

        // (Include all your other flag/counter parsing here, using 'entries' and lowercase keys)
        if (entries.contains("setflag")) p.setFlags = entries["setflag"];
        if (entries.contains("clearflag")) p.clearFlags = entries["clearflag"];
        if (entries.contains("set$")) p.setStringVars = entries["set$"];
        if (entries.contains("set#")) p.setCounterVars = entries["set#"];
        if (entries.contains("set@")) p.setTimeVars = entries["set@"];
        if (entries.contains("counter+")) p.incrementCounters = entries["counter+"];
        if (entries.contains("counter-")) p.decrementCounters = entries["counter-"];
        if (entries.contains("random#")) p.randomCounters = entries["random#"];
        if (entries.contains("random$")) p.randomStrings = entries["random$"];
        if (entries.contains("removeflag")) p.removeFlags = entries["removeflag"];
        if (entries.contains("setflaggroup")) p.setFlagGroups = entries["setflaggroup"];
        if (entries.contains("removeflaggroup")) p.removeFlagGroups = entries["removeflaggroup"];
        if (entries.contains("if")) p.ifFlags = entries["if"];
        if (entries.contains("notif")) p.notIfFlags = entries["notif"];
        if (entries.contains("denyif")) p.denyIfFlags = entries["denyif"];
        if (entries.contains("permitif")) p.permitIfFlags = entries["permitif"];
        if (entries.contains("set#")) p.setCounters = entries["set#"];
        if (entries.contains("add#")) p.addCounters = entries["add#"];
        if (entries.contains("subtract#")) p.subtractCounters = entries["subtract#"];
        if (entries.contains("multiply#")) p.multiplyCounters = entries["multiply#"];
        if (entries.contains("divide#")) p.divideCounters = entries["divide#"];
        if (entries.contains("drop#")) p.dropCounters = entries["drop#"];
        if (entries.contains("input#")) p.inputCounters = entries["input#"];
        if (entries.contains("inputneg#")) p.inputNegCounters = entries["inputneg#"];

        if (entries.contains("remindinterval")) {
            QStringList parts = entries["remindinterval"].value(0).split(',', Qt::SkipEmptyParts);
            if (parts.size() == 2) {
                p.remindIntervalMin = parts[0].trimmed();
                p.remindIntervalMax = parts[1].trimmed();
            } else {
                p.remindIntervalMin = p.remindIntervalMax = entries["remindinterval"].value(0).trimmed();
            }
        }

        if (entries.contains("remindpenalty")) {
            QStringList parts = entries["remindpenalty"].value(0).split(',', Qt::SkipEmptyParts);
            p.remindPenaltyMin = parts.value(0).trimmed().toInt();
            p.remindPenaltyMax = parts.value(1, parts.value(0)).trimmed().toInt();
        }

        if (entries.contains("remindpenaltygroup")) {
            p.remindPenaltyGroup = entries["remindpenaltygroup"].value(0);
        }

        // Time vars (lower case keys)
        if (entries.contains("set!")) p.setTimeVars = entries["set!"];
        if (entries.contains("add!")) p.addTimeVars = entries["add!"];
        if (entries.contains("subtract!")) p.subtractTimeVars = entries["subtract!"];
        if (entries.contains("multiply!")) p.multiplyTimeVars = entries["multiply!"];
        if (entries.contains("divide!")) p.divideTimeVars = entries["divide!"];
        if (entries.contains("round!")) p.roundTimeVars = entries["round!"];
        if (entries.contains("drop!")) p.dropTimeVars = entries["drop!"];
        if (entries.contains("inputdate!")) p.inputDateVars = entries["inputdate!"];
        if (entries.contains("inputdatedef!")) p.inputDateDefVars = entries["inputdatedef!"];
        if (entries.contains("inputtime!")) p.inputTimeVars = entries["inputtime!"];
        if (entries.contains("inputtimedef!")) p.inputTimeDefVars = entries["inputtimedef!"];
        if (entries.contains("inputinterval!")) p.inputIntervalVars = entries["inputinterval!"];
        if (entries.contains("random!")) p.randomTimeVars = entries["random!"];
        if (entries.contains("adddays!")) p.addDaysVars = entries["adddays!"];
        if (entries.contains("subtractdays!")) p.subtractDaysVars = entries["subtractdays!"];

        if (entries.contains("days#")) p.extractToCounter += entries["days#"];
        if (entries.contains("hours#")) p.extractToCounter += entries["hours#"];
        if (entries.contains("minutes#")) p.extractToCounter += entries["minutes#"];
        if (entries.contains("seconds#")) p.extractToCounter += entries["seconds#"];
        if (entries.contains("days!")) p.convertFromCounter += entries["days!"];
        if (entries.contains("hours!")) p.convertFromCounter += entries["hours!"];
        if (entries.contains("minutes!")) p.convertFromCounter += entries["minutes!"];
        if (entries.contains("seconds!")) p.convertFromCounter += entries["seconds!"];

        if (entries.contains("if")) p.ifConditions = entries["if"];
        if (entries.contains("notif")) p.notIfConditions = entries["notif"];
        if (entries.contains("denyif")) p.denyIfConditions = entries["denyif"];
        if (entries.contains("permitif")) p.permitIfConditions = entries["permitif"];

        parseDurationControl(entries, p.duration); // (Note: ensure parseDurationControl also uses lowercase keys or handle it)

        scriptData.punishments.insert(punishmentName, p);
    }
}

void ScriptParser::parseJobSections(const QMap<QString, QMap<QString, QStringList>>& sections) {
    for (auto it = sections.begin(); it != sections.end(); ++it) {
        const QString& sectionName = it.key();
        if (!sectionName.startsWith("job-"))
            continue;

        QString jobName = sectionName.mid(QString("job-").length());

        // --- Create Case-Insensitive Entry Map ---
        QMap<QString, QStringList> entries;
        const QMap<QString, QStringList>& rawEntries = it.value();
        for (auto e = rawEntries.begin(); e != rawEntries.end(); ++e) {
            entries.insert(e.key().toLower(), e.value());
        }
        // -----------------------------------------

        JobDefinition job;
        job.name = jobName;

        // Update ALL lookups to use lowercase keys
        if (entries.contains("text"))
            job.text = entries["text"].value(0).trimmed().toLower();

        auto parseIntRange = [](const QString& s, int& min, int& max) {
            QStringList parts = s.split(',', Qt::SkipEmptyParts);
            if (parts.size() == 2) {
                min = parts[0].toInt();
                max = parts[1].toInt();
            } else {
                min = max = parts[0].toInt();
            }
        };

        if (entries.contains("interval"))
            parseIntRange(entries["interval"].value(0), job.intervalMin, job.intervalMax);

        if (entries.contains("firstinterval"))
            parseIntRange(entries["firstinterval"].value(0), job.firstIntervalMin, job.firstIntervalMax);

        if (entries.contains("run")) {
            for (const QString& val : entries["run"])
                job.runDays.append(val.split(',', Qt::SkipEmptyParts));
        }

        if (entries.contains("norun")) {
            for (const QString& val : entries["norun"])
                job.noRunDays.append(val.split(',', Qt::SkipEmptyParts));
        }

        if (entries.contains("endtime")) {
            QStringList parts = entries["endtime"].value(0).split(',', Qt::SkipEmptyParts);
            if (parts.size() == 2) {
                job.endTimeMin = parts[0].trimmed();
                job.endTimeMax = parts[1].trimmed();
            } else {
                job.endTimeMin = job.endTimeMax = entries["endtime"].value(0).trimmed();
            }
        }

        if (entries.contains("announce")) {
            QString val = entries["announce"].value(0).trimmed();
            if (val == "0") {
                job.announce = 0;
            } else if (val == "1") {
                job.announce = 1;
            }
        }

        if (entries.contains("respite"))
            job.respite = entries["respite"].value(0).trimmed();
        if (entries.contains("respit"))
            job.respite = entries["respit"].value(0).trimmed();

        if (entries.contains("estimate"))
            job.estimate = entries["estimate"].value(0).trimmed();

        if (entries.contains("remindinterval")) {
            QStringList parts = entries["remindinterval"].value(0).split(',', Qt::SkipEmptyParts);
            if (parts.size() == 2) {
                job.remindIntervalMin = parts[0].trimmed();
                job.remindIntervalMax = parts[1].trimmed();
            } else {
                job.remindIntervalMin = job.remindIntervalMax = entries["remindinterval"].value(0).trimmed();
            }
        }

        if (entries.contains("remindpenalty"))
            parseIntRange(entries["remindpenalty"].value(0), job.remindPenaltyMin, job.remindPenaltyMax);

        if (entries.contains("remindpenaltygroup"))
            job.remindPenaltyGroup = entries["remindpenaltygroup"].value(0);

        if (entries.contains("latemerits")) {
            QStringList parts = entries["latemerits"].value(0).split(',', Qt::SkipEmptyParts);
            job.lateMeritsMin = parts.value(0).trimmed();
            job.lateMeritsMax = parts.value(1, job.lateMeritsMin).trimmed();
        }

        if (entries.contains("expireafter")) {
            QStringList parts = entries["expireafter"].value(0).split(',', Qt::SkipEmptyParts);
            if (parts.size() == 2) {
                job.expireAfterMin = parts[0].trimmed();
                job.expireAfterMax = parts[1].trimmed();
            } else {
                job.expireAfterMin = job.expireAfterMax = entries["expireafter"].value(0).trimmed();
            }
        }

        if (entries.contains("muststart"))
            job.mustStart = entries["muststart"].value(0).trimmed() == "1";

        if (entries.contains("deleteallowed"))
            job.deleteAllowed = (entries["deleteallowed"].value(0).trimmed() == "1");

        if (entries.contains("startflag"))
            job.startFlag = entries["startflag"].value(0).trimmed();

        if (entries.contains("announceprocedure"))
            job.announceProcedure = entries["announceprocedure"].value(0).trimmed();

        if (entries.contains("beforedoneprocedure"))
            job.beforeDoneProcedure = entries["beforedoneprocedure"].value(0).trimmed();

        if (entries.contains("startprocedure"))
            job.startProcedure = entries["startprocedure"].value(0).trimmed();

        if (entries.contains("abortprocedure"))
            job.abortProcedure = entries["abortprocedure"].value(0).trimmed();

        if (entries.contains("doneprocedure"))
            job.doneProcedure = entries["doneprocedure"].value(0).trimmed();

        if (entries.contains("beforeprocedure"))
            job.beforeProcedure = entries["beforeprocedure"].value(0).trimmed();

        if (entries.contains("remindprocedure"))
            job.remindProcedure = entries["remindprocedure"].value(0).trimmed();

        if (entries.contains("deleteprocedure"))
            job.deleteProcedure = entries["deleteprocedure"].value(0).trimmed();

        if (entries.contains("beforedeleteprocedure"))
            job.beforeDeleteProcedure = entries["beforedeleteprocedure"].value(0).trimmed();

        if (entries.contains("interruptable") || entries.contains("interruptible")) {
            QString val = entries.value("interruptable", entries.value("interruptible")).value(0).trimmed();
            if (val == "0") {
                job.interruptable = false;
            }
        }

        if (entries.contains("expirepenalty"))
            parseIntRange(entries["expirepenalty"].value(0), job.expirePenaltyMin, job.expirePenaltyMax);

        if (entries.contains("expirepenaltygroup"))
            job.expirePenaltyGroup = entries["expirepenaltygroup"].value(0);

        if (entries.contains("expireprocedure"))
            job.expireProcedure = entries["expireprocedure"].value(0);

        if (entries.contains("onetime"))
            job.oneTime = entries["onetime"].value(0).trimmed() == "1";

        if (entries.contains("announce"))
            job.announce = entries["announce"].value(0).trimmed() != "0";

        if (entries.contains("addmerit") || entries.contains("addmerits"))
            job.merits.add = entries.value("addmerit", entries.value("addmerits")).value(0);

        if (entries.contains("subtractmerit") || entries.contains("subtractmerits"))
            job.merits.subtract = entries.value("subtractmerit", entries.value("subtractmerits")).value(0);

        if (entries.contains("setmerit") || entries.contains("setmerits"))
            job.merits.set = entries.value("setmerit", entries.value("setmerits")).value(0);

        if (entries.contains("centerrandom"))
            job.centerRandom = entries["centerrandom"].value(0).trimmed() == "1";

        if (entries.contains("title"))
            job.title = entries["title"].value(0);

        if (entries.contains("newstatus"))
            job.newStatus = entries["newstatus"].value(0).trimmed();

        // Re-process messages using the original rawEntries to preserve order/case of values
        // but we check keys case-insensitively
        MessageGroup currentGroup;
        bool inMessageBlock = false;
        for (auto keyIt = rawEntries.begin(); keyIt != rawEntries.end(); ++keyIt) {
            QString key = keyIt.key();
            const QStringList& values = keyIt.value();
            for (const QString& val : values) {
                if (key.compare("Select", Qt::CaseInsensitive) == 0) {
                    if (val.compare("Random", Qt::CaseInsensitive) == 0)
                        currentGroup.mode = MessageSelectMode::Random;
                    else
                        currentGroup.mode = MessageSelectMode::All;
                    inMessageBlock = true;
                    if (!currentGroup.messages.isEmpty()) {
                        job.messages.append(currentGroup);
                        currentGroup = MessageGroup();
                    }
                }
                else if (key.compare("Message", Qt::CaseInsensitive) == 0) {
                    inMessageBlock = true;
                    currentGroup.messages.append(val.trimmed());
                }
                else if (key.compare("Text", Qt::CaseInsensitive) == 0) {
                    job.statusTexts.append(val.trimmed());
                }
            }
        }
        if (!currentGroup.messages.isEmpty()) {
            job.messages.append(currentGroup);
        }

        if (entries.contains("resource")) {
            for (const QString& value : entries["resource"])
                job.resources.append(value.trimmed());
        }

        if (entries.contains("estimate"))
            job.estimate = entries["estimate"].value(0);

        if (entries.contains("autoassign")) {
            QStringList parts = entries["autoassign"].value(0).split(',', Qt::SkipEmptyParts);
            job.autoAssignMin = parts.value(0).toInt();
            job.autoAssignMax = parts.value(1, parts.value(0)).toInt();
        }

        if (entries.contains("clothing"))
            job.clothingInstruction = entries["clothing"].value(0);

        if (entries.contains("clearcheck"))
            job.clearClothingCheck = entries["clearcheck"].value(0).trimmed() == "1";

        // --- Line Writing Checks ---
        if (entries.contains("type")) {
            QString typeVal = entries["type"].value(0).trimmed();
            if (typeVal.compare("Lines", Qt::CaseInsensitive) == 0) {
                job.isLineWriting = true;
            }
        }
        // ---------------------------

        if (entries.contains("line"))
            job.lines = entries["line"];

        if (entries.contains("select"))
            job.lineSelectMode = entries["select"].value(0).trimmed();

        if (entries.contains("linenumber"))
            job.lineCount = entries["linenumber"].value(0).toInt();

        if (entries.contains("pagesize")) {
            QStringList parts = entries["pagesize"].value(0).split(',', Qt::SkipEmptyParts);
            job.pageSizeMin = parts.value(0);
            job.pageSizeMax = parts.value(1, parts.value(0));
        }

        if (entries.contains("posecamera"))
            job.poseCameraText = entries["posecamera"].value(0);

        if (entries.contains("pointcamera"))
            job.pointCameraText = entries["pointcamera"].value(0);

        if (entries.contains("camerainterval")) {
            QStringList parts = entries["camerainterval"].value(0).split(',', Qt::SkipEmptyParts);
            job.cameraIntervalMin = parts.value(0).trimmed();
            job.cameraIntervalMax = parts.value(1, job.cameraIntervalMin).trimmed();
        }

        if (entries.contains("setflag")) job.setFlags = entries["setflag"];
        if (entries.contains("clearflag")) job.clearFlags = entries["clearflag"];
        if (entries.contains("set$")) job.setStringVars = entries["set$"];
        if (entries.contains("set#")) job.setCounterVars = entries["set#"];
        if (entries.contains("set@")) job.setTimeVars = entries["set@"];
        if (entries.contains("counter+")) job.incrementCounters = entries["counter+"];
        if (entries.contains("counter-")) job.decrementCounters = entries["counter-"];
        if (entries.contains("random#")) job.randomCounters = entries["random#"];
        if (entries.contains("random$")) job.randomStrings = entries["random$"];
        if (entries.contains("removeflag")) job.removeFlags = entries["removeflag"];
        if (entries.contains("setflaggroup")) job.setFlagGroups = entries["setflaggroup"];
        if (entries.contains("removeflaggroup")) job.removeFlagGroups = entries["removeflaggroup"];
        if (entries.contains("if")) job.ifFlags = entries["if"];
        if (entries.contains("notif")) job.notIfFlags = entries["notif"];
        if (entries.contains("denyif")) job.denyIfFlags = entries["denyif"];
        if (entries.contains("permitif")) job.permitIfFlags = entries["permitif"];
        if (entries.contains("set#")) job.setCounters = entries["set#"];
        if (entries.contains("add#")) job.addCounters = entries["add#"];
        if (entries.contains("subtract#")) job.subtractCounters = entries["subtract#"];
        if (entries.contains("multiply#")) job.multiplyCounters = entries["multiply#"];
        if (entries.contains("divide#")) job.divideCounters = entries["divide#"];
        if (entries.contains("drop#")) job.dropCounters = entries["drop#"];
        if (entries.contains("input#")) job.inputCounters = entries["input#"];
        if (entries.contains("inputneg#")) job.inputNegCounters = entries["inputneg#"];

        if (entries.contains("set!")) job.setTimeVars = entries["set!"];
        if (entries.contains("add!")) job.addTimeVars = entries["add!"];
        if (entries.contains("subtract!")) job.subtractTimeVars = entries["subtract!"];
        if (entries.contains("multiply!")) job.multiplyTimeVars = entries["multiply!"];
        if (entries.contains("divide!")) job.divideTimeVars = entries["divide!"];
        if (entries.contains("round!")) job.roundTimeVars = entries["round!"];
        if (entries.contains("drop!")) job.dropTimeVars = entries["drop!"];
        if (entries.contains("inputdate!")) job.inputDateVars = entries["inputdate!"];
        if (entries.contains("inputdatedef!")) job.inputDateDefVars = entries["inputdatedef!"];
        if (entries.contains("inputtime!")) job.inputTimeVars = entries["inputtime!"];
        if (entries.contains("inputtimedef!")) job.inputTimeDefVars = entries["inputtimedef!"];
        if (entries.contains("inputinterval!")) job.inputIntervalVars = entries["inputinterval!"];
        if (entries.contains("random!")) job.randomTimeVars = entries["random!"];
        if (entries.contains("adddays!")) job.addDaysVars = entries["adddays!"];
        if (entries.contains("subtractdays!")) job.subtractDaysVars = entries["subtractdays!"];

        if (entries.contains("days#")) job.extractToCounter += entries["days#"];
        if (entries.contains("hours#")) job.extractToCounter += entries["hours#"];
        if (entries.contains("minutes#")) job.extractToCounter += entries["minutes#"];
        if (entries.contains("seconds#")) job.extractToCounter += entries["seconds#"];
        if (entries.contains("days!")) job.convertFromCounter += entries["days!"];
        if (entries.contains("hours!")) job.convertFromCounter += entries["hours!"];
        if (entries.contains("minutes!")) job.convertFromCounter += entries["minutes!"];
        if (entries.contains("seconds!")) job.convertFromCounter += entries["seconds!"];

        if (entries.contains("if")) job.ifConditions = entries["if"];
        if (entries.contains("notif")) job.notIfConditions = entries["notif"];
        if (entries.contains("denyif")) job.denyIfConditions = entries["denyif"];
        if (entries.contains("permitif")) job.permitIfConditions = entries["permitif"];

        if (entries.contains("set*")) job.listSets = entries["set*"];
        if (entries.contains("setsplit*")) job.listSetSplits = entries["setsplit*"];
        if (entries.contains("add*")) job.listAdds = entries["add*"];
        if (entries.contains("addnodub*")) job.listAddNoDubs = entries["addnodub*"];
        if (entries.contains("addsplit*")) job.listAddSplits = entries["addsplit*"];
        if (entries.contains("push*")) job.listPushes = entries["push*"];
        if (entries.contains("remove*")) job.listRemoves = entries["remove*"];
        if (entries.contains("removeall*")) job.listRemoveAlls = entries["removeall*"];
        if (entries.contains("pull*")) job.listPulls = entries["pull*"];
        if (entries.contains("intersect*")) job.listIntersects = entries["intersect*"];
        if (entries.contains("clear*")) job.listClears = entries["clear*"];
        if (entries.contains("drop*")) job.listDrops = entries["drop*"];
        if (entries.contains("sort*")) job.listSorts = entries["sort*"];

        parseDurationControl(entries, job.duration);

        scriptData.jobs.insert(jobName, job);
    }
}

void ScriptParser::parseAssignmentBehavior(const QMap<QString, QStringList>& entries, AssignmentBehavior& a) {
    if (entries.contains("AddMerit"))
        a.addMerit = entries["AddMerit"].value(0).toInt();

    if (entries.contains("LongRunning"))
        a.longRunning = entries["LongRunning"].value(0).trimmed() == "1";

    if (entries.contains("MustStart"))
        a.mustStart = entries["MustStart"].value(0).trimmed() == "1";

    if (entries.contains("Interruptable"))
        a.interruptable = entries["Interruptable"].value(0).trimmed() != "0";
    if (entries.contains("Interruptible"))
        a.interruptable = entries["Interruptible"].value(0).trimmed() != "0";

    if (entries.contains("StartFlag"))
        a.startFlag = entries["StartFlag"].value(0);

    if (entries.contains("NewStatus"))
        a.newStatus = entries["NewStatus"].value(0);

    if (entries.contains("AnnounceProcedure"))
        a.announceProcedure = entries["AnnounceProcedure"].value(0);
    if (entries.contains("RemindProcedure"))
        a.remindProcedure = entries["RemindProcedure"].value(0);
    if (entries.contains("BeforeProcedure"))
        a.beforeProcedure = entries["BeforeProcedure"].value(0);
    if (entries.contains("StartProcedure"))
        a.startProcedure = entries["StartProcedure"].value(0);
    if (entries.contains("AbortProcedure"))
        a.startProcedure = entries["AbortProcedure"].value(0);
    if (entries.contains("BeforeDoneProcedure"))
        a.beforeDoneProcedure = entries["BeforeDoneProcedure"].value(0);
    if (entries.contains("DoneProcedure"))
        a.doneProcedure = entries["DoneProcedure"].value(0);
    if (entries.contains("DeleteProcedure"))
        a.deleteProcedure = entries["DeleteProcedure"].value(0);
    if (entries.contains("BeforeDeleteProcedure"))
        a.beforeDeleteProcedure = entries["BeforeDeleteProcedure"].value(0);

    if (entries.contains("DeleteAllowed"))
        a.deleteAllowed = entries["DeleteAllowed"].value(0).trimmed() == "1";

    MessageGroup currentGroup;
    bool inMessageBlock = false;

    for (auto keyIt = entries.begin(); keyIt != entries.end(); ++keyIt) {
        QString key = keyIt.key();
        const QStringList& values = keyIt.value();

        for (const QString& val : values) {
            if (key == "Select") {
                if (val.compare("Random", Qt::CaseInsensitive) == 0)
                    currentGroup.mode = MessageSelectMode::Random;
                else
                    currentGroup.mode = MessageSelectMode::All;

                inMessageBlock = true;
                if (!currentGroup.messages.isEmpty()) {
                    a.messages.append(currentGroup);
                    currentGroup = MessageGroup();
                }
            }
            else if (key == "Message") {
                inMessageBlock = true;
                currentGroup.messages.append(val.trimmed());
            }
            else if (key == "Text") {
                a.statusTexts.append(val.trimmed());
            }
        }
    }
    if (!currentGroup.messages.isEmpty()) {
        a.messages.append(currentGroup);
    }
}

void ScriptParser::parseInstructionSections(const QStringList& lines) {
    InstructionDefinition currentInstr;
    InstructionSet currentSet; // Implicit set for the instruction
    InstructionChoice currentChoice;

    bool inSection = false;
    bool inChoice = false;
    QString currentSectionName;

    for (const QString& line : lines) {
        if (line.isEmpty() || line.startsWith("#") || line.startsWith(";"))
            continue;

        // Check for section headers
        if (line.startsWith("[") && line.endsWith("]")) {
            if (inSection) {
                // Save previous instruction
                if (inChoice) {
                    // Finish the last choice and add it as a step
                    InstructionStep step;
                    step.type = InstructionStepType::Choice;
                    step.choice = currentChoice;
                    currentSet.steps.append(step);
                }
                currentInstr.sets.append(currentSet);
                scriptData.instructions.insert(currentInstr.name, currentInstr);
            }

            // Reset state
            inSection = false;
            inChoice = false;
            currentInstr = InstructionDefinition();
            currentSet = InstructionSet();
            currentChoice = InstructionChoice();

            currentSectionName = line.mid(1, line.length() - 2).trimmed();

            if (currentSectionName.startsWith("instruction-", Qt::CaseInsensitive)) {
                currentInstr.name = currentSectionName.mid(12).toLower();
                currentInstr.isClothing = false;
                inSection = true;
            } else if (currentSectionName.startsWith("instructions-", Qt::CaseInsensitive)) {
                currentInstr.name = currentSectionName.mid(13).toLower();
                currentInstr.isClothing = false;
                inSection = true;
            } else if (currentSectionName.startsWith("clothing-", Qt::CaseInsensitive)) {
                currentInstr.name = currentSectionName.mid(9).toLower();
                currentInstr.isClothing = true;
                inSection = true;
            }
            continue;
        }

        if (!inSection) continue;

        int equalsIndex = line.indexOf('=');
        if (equalsIndex == -1) continue;

        QString key = line.left(equalsIndex).trimmed();
        QString value = line.mid(equalsIndex + 1).trimmed();

        // --- Properties ---
        if (key.compare("Askable", Qt::CaseInsensitive) == 0) {
            currentInstr.askable = (value != "0");
        } else if (key.compare("Title", Qt::CaseInsensitive) == 0) {
            currentInstr.title = value;
        } else if (key.compare("Select", Qt::CaseInsensitive) == 0) {
            if (value.compare("first", Qt::CaseInsensitive) == 0)
                currentInstr.selectMode = InstructionSelectMode::First;
            else if (value.compare("random", Qt::CaseInsensitive) == 0)
                currentInstr.selectMode = InstructionSelectMode::Random;
            else
                currentInstr.selectMode = InstructionSelectMode::All;
            currentSet.selectMode = currentInstr.selectMode;
        } else if (key.compare("Clothing", Qt::CaseInsensitive) == 0) {
            currentInstr.clothingInstruction = value;
        } else if (key.compare("ClearCheck", Qt::CaseInsensitive) == 0) {
            currentInstr.clearClothingCheck = (value.trimmed() == "1");
        } else if (key.compare("Text", Qt::CaseInsensitive) == 0) {
            currentInstr.statusTexts.append(value);
        }

        // --- Structure (Sets, Choices, Options) ---

        else if (key.compare("Set", Qt::CaseInsensitive) == 0) {
            // 1. Close pending choice if any
            if (inChoice) {
                InstructionStep step;
                step.type = InstructionStepType::Choice;
                step.choice = currentChoice;
                currentSet.steps.append(step);
                currentChoice = InstructionChoice();
                inChoice = false;
            }
            // 2. Add Set Reference Step
            InstructionStep step;
            step.type = InstructionStepType::SetReference;
            step.setReference = "set:" + value.trimmed().toLower();
            currentSet.steps.append(step);
        }
        else if (key.compare("Choice", Qt::CaseInsensitive) == 0) {
            // 1. Close pending choice if any
            if (inChoice) {
                InstructionStep step;
                step.type = InstructionStepType::Choice;
                step.choice = currentChoice;
                currentSet.steps.append(step);
                currentChoice = InstructionChoice();
            }
            inChoice = true;

            // 2. Start new choice
            if (value.contains(',')) {
                // Legacy format
                QStringList parts = value.split(',', Qt::SkipEmptyParts);
                for (const QString& opt : parts) {
                    InstructionOption o;
                    o.text = opt.trimmed();
                    currentChoice.options.append(o);
                }
                // Close immediately
                InstructionStep step;
                step.type = InstructionStepType::Choice;
                step.choice = currentChoice;
                currentSet.steps.append(step);
                currentChoice = InstructionChoice();
                inChoice = false;
            } else {
                if (value.compare("new", Qt::CaseInsensitive) == 0) currentChoice.name.clear();
                else currentChoice.name = value;
            }
        }
        else if (key.compare("Option", Qt::CaseInsensitive) == 0) {
            InstructionOption option;
            option.text = value;
            // ... (keep your existing * skip and % hidden logic) ...
            currentChoice.options.append(option);
            if (!inChoice) inChoice = true;
        }

        // --- WEIGHT LOGIC ---
        else if (key.compare("Weight", Qt::CaseInsensitive) == 0) {
            bool ok = false;
            int w = value.toInt(&ok);
            if (ok) {
                // Because we parse line-by-line, 'last()' is definitely the Option above this line
                if (inChoice && !currentChoice.options.isEmpty()) {
                    currentChoice.options.last().weight = w;
                } else {
                    currentSet.weight = w;
                }
            }
        }
        // --- END WEIGHT LOGIC ---

        else if (key.compare("Check", Qt::CaseInsensitive) == 0) {
            if (!currentChoice.options.isEmpty()) currentChoice.options.last().check.append(value);
        }
        else if (key.compare("CheckOff", Qt::CaseInsensitive) == 0) {
            if (!currentChoice.options.isEmpty()) currentChoice.options.last().checkOff.append(value);
        }
    }

    // Save the very last instruction
    if (inSection) {
        if (inChoice) {
            InstructionStep step;
            step.type = InstructionStepType::Choice;
            step.choice = currentChoice;
            currentSet.steps.append(step);
        }
        currentInstr.sets.append(currentSet);
        scriptData.instructions.insert(currentInstr.name, currentInstr);
    }
}

void ScriptParser::parseInstructionSets(const QStringList& lines) {
    InstructionSet currentSet;
    InstructionChoice currentChoice;

    bool inSection = false;
    bool inChoice = false;
    QString currentSectionName;

    for (const QString& line : lines) {
        if (line.isEmpty() || line.startsWith("#") || line.startsWith(";"))
            continue;

        // Check for section headers
        if (line.startsWith("[") && line.endsWith("]")) {
            if (inSection) {
                // Save the previous set
                if (inChoice) {
                    InstructionStep step;
                    step.type = InstructionStepType::Choice;
                    step.choice = currentChoice;
                    currentSet.steps.append(step);
                }

                // Wrap the set in a definition so resolveInstruction can find it
                InstructionDefinition def;
                def.name = currentSet.name;
                def.sets.append(currentSet);
                scriptData.instructions.insert(def.name, def);
            }

            // Reset state
            inSection = false;
            inChoice = false;
            currentSet = InstructionSet();
            currentChoice = InstructionChoice();

            currentSectionName = line.mid(1, line.length() - 2).trimmed();

            if (currentSectionName.startsWith("set-", Qt::CaseInsensitive)) {
                inSection = true;
                currentSet.name = "set:" + currentSectionName.mid(4).toLower();
            }
            continue;
        }

        if (!inSection) continue;

        int equalsIndex = line.indexOf('=');
        if (equalsIndex == -1) continue;

        QString key = line.left(equalsIndex).trimmed();
        QString value = line.mid(equalsIndex + 1).trimmed();

        // --- Parse Logic ---

        if (key.compare("Select", Qt::CaseInsensitive) == 0) {
            if (value.compare("first", Qt::CaseInsensitive) == 0)
                currentSet.selectMode = InstructionSelectMode::First;
            else if (value.compare("random", Qt::CaseInsensitive) == 0)
                currentSet.selectMode = InstructionSelectMode::Random;
            else
                currentSet.selectMode = InstructionSelectMode::All;
        }
        else if (key.compare("Set", Qt::CaseInsensitive) == 0) {
            // 1. Close pending choice if any
            if (inChoice) {
                InstructionStep step;
                step.type = InstructionStepType::Choice;
                step.choice = currentChoice;
                currentSet.steps.append(step);
                currentChoice = InstructionChoice();
                inChoice = false;
            }

            // 2. Add Set Reference Step (Preserves Order)
            InstructionStep step;
            step.type = InstructionStepType::SetReference;
            step.setReference = "set:" + value.trimmed().toLower(); // Store the set name (e.g., "Bra")
            currentSet.steps.append(step);
        }
        else if (key.compare("Choice", Qt::CaseInsensitive) == 0) {
            // 1. Close pending choice if any
            if (inChoice) {
                InstructionStep step;
                step.type = InstructionStepType::Choice;
                step.choice = currentChoice;
                currentSet.steps.append(step);
                currentChoice = InstructionChoice();
            }
            inChoice = true;

            if (value.contains(',')) {
                // Legacy format: Choice=Option1,Option2
                QStringList parts = value.split(',', Qt::SkipEmptyParts);
                for (const QString& opt : parts) {
                    InstructionOption o;
                    o.text = opt.trimmed();
                    currentChoice.options.append(o);
                }
                InstructionStep step;
                step.type = InstructionStepType::Choice;
                step.choice = currentChoice;
                currentSet.steps.append(step);
                currentChoice = InstructionChoice();
                inChoice = false;
            } else {
                if (value.compare("new", Qt::CaseInsensitive) == 0) {
                    currentChoice.name.clear();
                } else {
                    currentChoice.name = value;
                }
            }
        }
        else if (key.compare("Option", Qt::CaseInsensitive) == 0) {
            InstructionOption option;
            option.text = value;
            if (value == "*") option.skip = true;
            else if (value.startsWith('%')) {
                option.hidden = true;
                option.text = value.mid(1).trimmed();
            }
            currentChoice.options.append(option);
            if (!inChoice) inChoice = true; // Implicit choice if none started
        }
        else if (key.compare("Weight", Qt::CaseInsensitive) == 0) {
            bool ok = false;
            int w = value.toInt(&ok);
            if (ok) {
                if (inChoice && !currentChoice.options.isEmpty()) {
                    currentChoice.options.last().weight = w;
                } else {
                    currentSet.weight = w;
                }
            }
        }
        else if (key.compare("Check", Qt::CaseInsensitive) == 0) {
            if (!currentChoice.options.isEmpty()) currentChoice.options.last().check.append(value);
        }
        else if (key.compare("CheckOff", Qt::CaseInsensitive) == 0) {
            if (!currentChoice.options.isEmpty()) currentChoice.options.last().checkOff.append(value);
        }
        else if (key.compare("OptionSet", Qt::CaseInsensitive) == 0) {
            if (inChoice && !currentChoice.options.isEmpty()) {
                currentChoice.options.last().optionSet = value;
            }
        }
        else if (key.compare("If", Qt::CaseInsensitive) == 0) {
            currentSet.ifFlagGroups.append(value.split(',', Qt::SkipEmptyParts));
        }
        else if (key.compare("NotIf", Qt::CaseInsensitive) == 0) {
            currentSet.notIfFlagGroups.append(value.split(',', Qt::SkipEmptyParts));
        }
        else if (key.compare("IfChosen", Qt::CaseInsensitive) == 0) {
            currentSet.ifChosen.append(value.split(',', Qt::SkipEmptyParts));
        }
        else if (key.compare("IfNotChosen", Qt::CaseInsensitive) == 0) {
            currentSet.ifNotChosen.append(value.split(',', Qt::SkipEmptyParts));
        }
        else if (key.compare("Flag", Qt::CaseInsensitive) == 0) {
            currentSet.setFlags.append(value);
        }
    }

    // Save the very last set
    if (inSection) {
        if (inChoice) {
            InstructionStep step;
            step.type = InstructionStepType::Choice;
            step.choice = currentChoice;
            currentSet.steps.append(step);
        }

        InstructionDefinition def;
        def.name = currentSet.name;
        def.sets.append(currentSet);
        scriptData.instructions.insert(def.name, def);
    }
}

void ScriptParser::parseClothingTypes(const QStringList &lines) {
    QString currentSection;
    ClothingTypeDefinition type;
    ClothingAttribute currentAttr;
    QString currentValue;

    auto finalizeType = [&]() {
        if (!currentAttr.name.isEmpty()) {
            type.attributes.append(currentAttr);
            currentAttr = ClothingAttribute();
        }
        if (!type.name.isEmpty())
            scriptData.clothingTypes.insert(type.name, type);
        type = ClothingTypeDefinition();
    };

    for (const QString &rawLine : lines) {
        QString line = rawLine.trimmed();
        if (line.isEmpty() || line.startsWith('#') || line.startsWith(';'))
            continue;

        if (line.startsWith('[') && line.endsWith(']')) {
            finalizeType();
            currentSection = line.mid(1, line.length() - 2).trimmed().toLower();
            if (currentSection.startsWith("clothtype-")) {
                type.name = currentSection.mid(QString("clothtype-").length());
            } else {
                currentSection.clear();
            }
            currentAttr = ClothingAttribute();
            currentValue.clear();
            continue;
        }

        if (!currentSection.startsWith("clothtype-"))
            continue;

        int eq = line.indexOf('=');
        if (eq == -1)
            continue;

        QString key = line.left(eq).trimmed();
        QString value = line.mid(eq + 1).trimmed();

        if (key.compare("Attr", Qt::CaseInsensitive) == 0) {
            if (!currentAttr.name.isEmpty())
                type.attributes.append(currentAttr);
            currentAttr = ClothingAttribute();
            currentAttr.name = value;
            currentValue.clear();
        } else if (key.compare("Value", Qt::CaseInsensitive) == 0) {
            currentAttr.values.append(value);
            currentValue = value;
        } else if (key.compare("Check", Qt::CaseInsensitive) == 0) {
            if (currentValue.isEmpty())
                type.checks.append(value);
            else
                type.valueChecks[currentValue].append(value);
        } else if (key.compare("Flag", Qt::CaseInsensitive) == 0) {
            if (currentValue.isEmpty())
                type.topLevelFlags.append(value);
            else
                type.valueFlags[currentValue].append(value);
        }
    }

    finalizeType();
}

void ScriptParser::parseProcedureSections(const QStringList& lines) {
    ProcedureDefinition currentProc;
    bool inProcSection = false;
    QString currentSectionName;
    QString currentPunishMessage;

    // Loop through every single line from the .ini in its original order
    for (const QString& line : lines) {
        if (line.isEmpty() || line.startsWith("#") || line.startsWith(";"))
            continue;

        // Check for section headers
        if (line.startsWith("[") && line.endsWith("]")) {
            // If we are in a procedure section, save the one we just finished
            if (inProcSection) {
                scriptData.procedures.insert(currentProc.name, currentProc);
            }
            inProcSection = false;

            currentSectionName = line.mid(1, line.length() - 2).trimmed();

            // Check if this new section is a procedure
            if (currentSectionName.startsWith("procedure-", Qt::CaseInsensitive)) {
                inProcSection = true;
                currentProc = ProcedureDefinition();
                currentProc.name = currentSectionName.mid(10).toLower();
                currentPunishMessage.clear();
            }
            continue;
        }

        // If not is a procedure section, skip
        if (!inProcSection) {
            continue;
        }

        // If we are in a procedure, this must be a key-value pair.
        int equalsIndex = line.indexOf('=');
        if (equalsIndex == -1) continue;

        QString key = line.left(equalsIndex).trimmed();
        QString value = line.mid(equalsIndex + 1).trimmed();

        if (key.compare("Title", Qt::CaseInsensitive) == 0) {
            currentProc.title = value;
        } else if (key.compare("Select", Qt::CaseInsensitive) == 0) {
            if (value.compare("first", Qt::CaseInsensitive) == 0)
                currentProc.selectMode = ProcedureSelectMode::First;
            else if (value.compare("random", Qt::CaseInsensitive) == 0)
                currentProc.selectMode = ProcedureSelectMode::Random;
            else
                currentProc.selectMode = ProcedureSelectMode::All;
        } else if (key.compare("Procedure", Qt::CaseInsensitive) == 0) {
            currentProc.actions.append({ScriptActionType::ProcedureCall, value});
        } else if (key.compare("If", Qt::CaseInsensitive) == 0) {
            currentProc.actions.append({ScriptActionType::If, value});
        } else if (key.compare("NotIf", Qt::CaseInsensitive) == 0) {
            currentProc.actions.append({ScriptActionType::NotIf, value});
        } else if (key.compare("SetFlag", Qt::CaseInsensitive) == 0) {
            currentProc.actions.append({ScriptActionType::SetFlag, value});
        } else if (key.compare("RemoveFlag", Qt::CaseInsensitive) == 0) {
            currentProc.actions.append({ScriptActionType::RemoveFlag, value});
        } else if (key.compare("ClearFlag", Qt::CaseInsensitive) == 0) {
            currentProc.actions.append({ScriptActionType::ClearFlag, value});
        } else if (key.compare("Set#", Qt::CaseInsensitive) == 0) {
            currentProc.actions.append({ScriptActionType::SetCounterVar, value});
        } else if (key.compare("Input#", Qt::CaseInsensitive) == 0) {
            currentProc.actions.append({ScriptActionType::InputCounter, value});
        } else if (key.compare("Change#", Qt::CaseInsensitive) == 0) {
            currentProc.actions.append({ScriptActionType::ChangeCounter, value});
        } else if (key.compare("InputNeg#", Qt::CaseInsensitive) == 0) {
            currentProc.actions.append({ScriptActionType::InputNegCounter, value});
        } else if (key.compare("Random#", Qt::CaseInsensitive) == 0) {
            currentProc.actions.append({ScriptActionType::RandomCounter, value});
        } else if (key.compare("Drop#", Qt::CaseInsensitive) == 0) {
            currentProc.actions.append({ScriptActionType::DropCounter, value});
        } else if (key.compare("Set$", Qt::CaseInsensitive) == 0) {
            currentProc.actions.append({ScriptActionType::SetString, value});
        } else if (key.compare("Input$", Qt::CaseInsensitive) == 0) {
            currentProc.actions.append({ScriptActionType::InputString, value});
        } else if (key.compare("Change$", Qt::CaseInsensitive) == 0) {
            currentProc.actions.append({ScriptActionType::ChangeString, value});
        } else if (key.compare("InputLong$", Qt::CaseInsensitive) == 0) {
            currentProc.actions.append({ScriptActionType::InputLongString, value});
        } else if (key.compare("ChangeLong$", Qt::CaseInsensitive) == 0) {
            currentProc.actions.append({ScriptActionType::ChangeLongString, value});
        } else if (key.compare("Drop$", Qt::CaseInsensitive) == 0) {
            currentProc.actions.append({ScriptActionType::DropString, value});
        } else if (key.compare("Set!", Qt::CaseInsensitive) == 0) {
            currentProc.actions.append({ScriptActionType::SetTimeVar, value});
        } else if (key.compare("Input!", Qt::CaseInsensitive) == 0) {
            currentProc.actions.append({ScriptActionType::InputDate, value});
        } else if (key.compare("InputDateDef!", Qt::CaseInsensitive) == 0) {
            currentProc.actions.append({ScriptActionType::InputDateDef, value});
        } else if (key.compare("ChangeDate!", Qt::CaseInsensitive) == 0) {
            currentProc.actions.append({ScriptActionType::ChangeDate, value});
        } else if (key.compare("InputTime!", Qt::CaseInsensitive) == 0) {
            currentProc.actions.append({ScriptActionType::InputTime, value});
        } else if (key.compare("InputTimeDef!", Qt::CaseInsensitive) == 0) {
            currentProc.actions.append({ScriptActionType::InputTimeDef, value});
        } else if (key.compare("ChangeTime!", Qt::CaseInsensitive) == 0) {
            currentProc.actions.append({ScriptActionType::ChangeTime, value});
        } else if (key.compare("InputInterval!", Qt::CaseInsensitive) == 0) {
            currentProc.actions.append({ScriptActionType::InputInterval, value});
        } else if (key.compare("ChangeInterval!", Qt::CaseInsensitive) == 0) {
            currentProc.actions.append({ScriptActionType::ChangeInterval, value});
        } else if (key.compare("AddDays!", Qt::CaseInsensitive) == 0) {
            currentProc.actions.append({ScriptActionType::AddDaysTime, value});
        } else if (key.compare("SubtractDays!", Qt::CaseInsensitive) == 0) {
            currentProc.actions.append({ScriptActionType::SubtractDaysTime, value});
        } else if (key.compare("Days!", Qt::CaseInsensitive) == 0) {
            currentProc.actions.append({ScriptActionType::ConvertDays, value});
        } else if (key.compare("Hours!", Qt::CaseInsensitive) == 0) {
            currentProc.actions.append({ScriptActionType::ConvertHours, value});
        } else if (key.compare("Minutes!", Qt::CaseInsensitive) == 0) {
            currentProc.actions.append({ScriptActionType::ConvertMinutes, value});
        } else if (key.compare("Seconds!", Qt::CaseInsensitive) == 0) {
            currentProc.actions.append({ScriptActionType::ConvertSeconds, value});
        } else if (key.compare("Round!", Qt::CaseInsensitive) == 0) {
            currentProc.actions.append({ScriptActionType::RoundTime, value});
        } else if (key.compare("Random!", Qt::CaseInsensitive) == 0) {
            currentProc.actions.append({ScriptActionType::RandomTime, value});
        } else if (key.compare("Drop!", Qt::CaseInsensitive) == 0) {
            currentProc.actions.append({ScriptActionType::DropTime, value});
        } else if (key.compare("Add#", Qt::CaseInsensitive) == 0) {
            currentProc.actions.append({ScriptActionType::AddCounter, value});
        } else if (key.compare("Subtract#", Qt::CaseInsensitive) == 0) {
            currentProc.actions.append({ScriptActionType::SubtractCounter, value});
        } else if (key.compare("Multiply#", Qt::CaseInsensitive) == 0) {
            currentProc.actions.append({ScriptActionType::MultiplyCounter, value});
        } else if (key.compare("Divide#", Qt::CaseInsensitive) == 0) {
            currentProc.actions.append({ScriptActionType::DivideCounter, value});
        } else if (key.compare("Days#", Qt::CaseInsensitive) == 0) {
            currentProc.actions.append({ScriptActionType::ExtractDays, value});
        } else if (key.compare("Hours#", Qt::CaseInsensitive) == 0) {
            currentProc.actions.append({ScriptActionType::ExtractHours, value});
        } else if (key.compare("Minutes#", Qt::CaseInsensitive) == 0) {
            currentProc.actions.append({ScriptActionType::ExtractMinutes, value});
        } else if (key.compare("Seconds#", Qt::CaseInsensitive) == 0) {
            currentProc.actions.append({ScriptActionType::ExtractSeconds, value});
        } else if (key.compare("Message", Qt::CaseInsensitive) == 0) {
            currentProc.actions.append({ScriptActionType::Message, value});
        } else if (key.compare("Question", Qt::CaseInsensitive) == 0) {
            currentProc.actions.append({ScriptActionType::Question, value});
        } else if (key.compare("Input", Qt::CaseInsensitive) == 0) {
            currentProc.actions.append({ScriptActionType::Input, value});
        } else if (key.compare("Clothing", Qt::CaseInsensitive) == 0) {
            if (currentProc.clothingInstruction.isEmpty()) {
                currentProc.actions.append({ScriptActionType::Clothing, value});
            } else {
                currentProc.actions.append({ScriptActionType::Clothing, value});
            }
        } else if (key.compare("Instructions", Qt::CaseInsensitive) == 0) {
            currentProc.actions.append({ScriptActionType::Instructions, value});
        } else if (key.compare("Timer", Qt::CaseInsensitive) == 0) {
            currentProc.actions.append({ScriptActionType::Timer, value});
        } else if (key.compare("NewStatus", Qt::CaseInsensitive) == 0) {
            currentProc.actions.append({ScriptActionType::NewStatus, value});
        } else if (key.compare("NewSubStatus", Qt::CaseInsensitive) == 0) {
            currentProc.actions.append({ScriptActionType::NewSubStatus, value});
        } else if (key.compare("Job", Qt::CaseInsensitive) == 0) {
            currentProc.actions.append({ScriptActionType::AnnounceJob, value});
        } else if (key.compare("MarkDone", Qt::CaseInsensitive) == 0) {
            currentProc.actions.append({ScriptActionType::MarkDone, value});
        } else if (key.compare("Abort", Qt::CaseInsensitive) == 0) {
            currentProc.actions.append({ScriptActionType::Delete, value});
        } else if (key.compare("AddMerit", Qt::CaseInsensitive) == 0 || key.compare("AddMerits", Qt::CaseInsensitive) == 0) {
            currentProc.actions.append({ScriptActionType::AddMerit, value});
        } else if (key.compare("SubtractMerit", Qt::CaseInsensitive) == 0 || key.compare("SubtractMerits", Qt::CaseInsensitive) == 0) {
            currentProc.actions.append({ScriptActionType::SubtractMerit, value});
        } else if (key.compare("SetMerit", Qt::CaseInsensitive) == 0 || key.compare("SetMerits", Qt::CaseInsensitive) == 0) {
            currentProc.actions.append({ScriptActionType::SetMerit, value});
        } else if (key.compare("PunishMessage", Qt::CaseInsensitive) == 0) {
            if (currentPunishMessage.isEmpty()) {
                currentPunishMessage = value;
            }
        } else if (key.compare("PunishmentGroup", Qt::CaseInsensitive) == 0) {
            if (currentProc.punishGroup.isEmpty()) {
                currentProc.punishGroup = value;
            }
        } else if (key.compare("Punish", Qt::CaseInsensitive) == 0) {
            currentProc.actions.append({ScriptActionType::Punish, value});
        } else if (key.compare("ClothReport", Qt::CaseInsensitive) == 0) {
            // Add as an action (value is "1" or "Title Text")
            if (value != "0") {
                currentProc.actions.append({ScriptActionType::ClothReport, value});
            }
        }
        else if (key.compare("ClearCloth" , Qt::CaseInsensitive) == 0) {
            currentProc.actions.append({ScriptActionType::ClearCloth, value});
        }
        else if (key.compare("CameraInterval", Qt::CaseInsensitive) == 0) {
            QStringList parts = value.split(',', Qt::SkipEmptyParts);
            if (parts.size() == 2) {
                currentProc.cameraIntervalMin = parts[0].trimmed();
                currentProc.cameraIntervalMax = parts[1].trimmed();
            } else {
                currentProc.cameraIntervalMin = currentProc.cameraIntervalMax = value.trimmed();
            }
        }
        else if (key.compare("PointCamera", Qt::CaseInsensitive) == 0) {
            currentProc.pointCameraText = value;
            currentProc.actions.append({ScriptActionType::PointCamera, value});
        }
        else if (key.compare("PoseCamera", Qt::CaseInsensitive) == 0) {
            currentProc.poseCameraText = value;
            currentProc.actions.append({ScriptActionType::PoseCamera, value});
        }

        if (inProcSection) {
            scriptData.procedures.insert(currentProc.name, currentProc);
        }
    }
}

void ScriptParser::parsePopupSections(const QMap<QString, QMap<QString, QStringList>>& sections) {
    for (auto it = sections.begin(); it != sections.end(); ++it) {
        QString sectionName = it.key();
        if (!sectionName.startsWith("popup-"))
            continue;

        QString name = sectionName.mid(QString("popup-").length());
        const QMap<QString, QStringList>& entries = it.value();

        PopupDefinition p;
        p.name = name;

        for (auto e = entries.begin(); e != entries.end(); ++e) {
            QString key = e.key();
            for (const QString& val : e.value()) {
                QString value = val.trimmed();

                if (key == "Title")
                    p.title = value;
                else if (key == "PreStatus")
                    p.preStatuses += value.split(',', Qt::SkipEmptyParts);
                else if (key == "If")
                    p.ifFlags += value.split(',', Qt::SkipEmptyParts);
                else if (key == "NotIf")
                    p.notIfFlags += value.split(',', Qt::SkipEmptyParts);
                else if (key == "Weight") {
                    QStringList w = value.split(',', Qt::SkipEmptyParts);
                    if (w.size() == 2) {
                        p.weightMin = w[0].toInt();
                        p.weightMax = w[1].toInt();
                    } else {
                        p.weightMin = p.weightMax = w.value(0).toInt();
                    }
                } else if (key == "Alarm")
                    p.alarm = value;
                else if (key == "Group")
                    p.groups += value.split(',', Qt::SkipEmptyParts);
                else if (key == "GroupOnly")
                    p.groupOnly = (value == "1");
            }
        }

        if (entries.contains("PopupMessage"))
            p.popupMessage = entries["PopupMessage"].value(0);

        if (entries.contains("MasterMail"))
            p.masterMailSubject = entries["MasterMail"].value(0);

        if (entries.contains("MasterAttach")) {
            for (const QString& file : entries["MasterAttach"])
                p.masterAttachments.append(file.trimmed());
        }

        if (entries.contains("SubMail")) {
            for (const QString& line : entries["SubMail"])
                p.subMailLines.append(line.trimmed());
        }

        QStringList lines;
        for (auto keyIt = entries.begin(); keyIt != entries.end(); ++keyIt) {
            for (const QString &val : keyIt.value())
                lines.append(val);
        }

        int index = 0;
        while (index < lines.size()) {
            if (lines[index].startsWith("Case=", Qt::CaseInsensitive)) {
                CaseBlock block = parseCaseBlock(lines, index);
                p.cases.append(block);
            } else {
                ++index;
            }
        }

        parseTimeRestrictions(entries, p.notBeforeTimes, p.notAfterTimes, p.notBetweenTimes);

        scriptData.popups.insert(p.name, p);
    }
}

void ScriptParser::parsePopupGroupSections(const QMap<QString, QMap<QString, QStringList>>& sections) {
    for (auto it = sections.begin(); it != sections.end(); ++it) {
        QString sectionName = it.key();
        if (!sectionName.startsWith("popupgroup-"))
            continue;

        QString name = sectionName.mid(QString("popupgroup-").length());
        const QMap<QString, QStringList>& entries = it.value();

        PopupGroupDefinition g;
        g.name = name;

        for (auto e = entries.begin(); e != entries.end(); ++e) {
            QString key = e.key();
            for (const QString& val : e.value()) {
                QString value = val.trimmed();

                if (key == "PopupInterval") {
                    QStringList times = value.split(',', Qt::SkipEmptyParts);
                    g.popupIntervalMin = times.value(0);
                    g.popupIntervalMax = times.value(1, times.value(0));
                } else if (key == "PopupIf")
                    g.noPopupIfFlags += value.split(',', Qt::SkipEmptyParts);
                else if (key == "NoPopupIf")
                    g.noPopupIfFlags += value.split(',', Qt::SkipEmptyParts);
                else if (key == "PopupAlarm")
                    g.popupAlarm = value;
                else if (key == "GroupOnly")
                    g.groupOnly = (value == "1");

                else if (key == "PopupMinTime") g.popupMinTime = value;
                else if (key == "PopupQuickPenalty1") g.quickPenalty1 = value.toInt();
                else if (key == "PopupQuickPenalty2") g.quickPenalty2 = value.toInt();
                else if (key == "PopupQuickMessage") g.quickMessage = value;
                else if (key == "PopupMaxTime") g.popupMinTime = value;
                else if (key == "PopupSlowPenalty1") g.slowPenalty1 = value.toInt();
                else if (key == "PopupSlowPenalty2") g.slowPenalty2 = value.toInt();
                else if (key == "PopupSlowMessage") g.slowMessage = value;
                else if (key == "PopupQuickProcedure") g.quickProcedure = value;
                else if (key == "PopupSlowProcedure") g.slowProcedure = value;
            }
        }

        if (entries.contains("PopupMessage"))
            g.popupMessage = entries["PopupMessage"].value(0);

        parseDurationControl(entries, g.duration);

        scriptData.popupGroups.insert(g.name, g);
    }
}

void ScriptParser::parseTimerSections(const QStringList& lines) {
    TimerDefinition currentTimer;
    bool inSection = false;
    QString currentSectionName;
    QString currentPunishMessage;

    for (const QString& line : lines) {
        if (line.isEmpty() || line.startsWith("#") || line.startsWith(";"))
            continue; // Skip comments and empty lines

        // Check for section headers
        if (line.startsWith("[") && line.endsWith("]")) {
            if (inSection) {
                // Save the previous timer
                scriptData.timers.insert(currentTimer.name, currentTimer);
            }
            inSection = false; // Reset

            currentSectionName = line.mid(1, line.length() - 2).trimmed();

            if (currentSectionName.startsWith("timer-", Qt::CaseInsensitive)) {
                inSection = true;
                currentTimer = TimerDefinition(); // Start a new, clean timer
                currentTimer.name = currentSectionName.mid(6).toLower();
                currentPunishMessage.clear();
            }
            continue;
        }

        if (!inSection) continue; // Skip lines not in a timer section

        int equalsIndex = line.indexOf('=');
        if (equalsIndex == -1) continue; // Not a key-value pair

        QString key = line.left(equalsIndex).trimmed();
        QString value = line.mid(equalsIndex + 1).trimmed();

        // --- 1. Handle PROPERTIES (not actions) ---
        if (key.compare("Start", Qt::CaseInsensitive) == 0) {
            QStringList times = value.split(',', Qt::SkipEmptyParts);
            currentTimer.startTimeMin = times.value(0);
            currentTimer.startTimeMax = times.value(1, times.value(0));
        } else if (key.compare("End", Qt::CaseInsensitive) == 0) {
            currentTimer.endTime = value;
        } else if (key.compare("PreStatus", Qt::CaseInsensitive) == 0) {
            currentTimer.preStatuses += value.split(',', Qt::SkipEmptyParts);
        } else if (key.compare("NotBefore", Qt::CaseInsensitive) == 0) {
            currentTimer.notBefore += value.split(',', Qt::SkipEmptyParts);
        } else if (key.compare("NotAfter", Qt::CaseInsensitive) == 0) {
            currentTimer.notAfter += value.split(',', Qt::SkipEmptyParts);
        } else if (key.compare("NotBetween", Qt::CaseInsensitive) == 0) {
            QStringList parts = value.split(',', Qt::SkipEmptyParts);
            if (parts.size() == 2)
                currentTimer.notBetween.append({ parts[0].trimmed(), parts[1].trimmed() });
        }
        // ... (add other non-action properties like MasterMail, etc.) ...

        // --- 2. Handle ACTIONS (add to ordered list) ---
        else if (key.compare("Procedure", Qt::CaseInsensitive) == 0) {
            currentTimer.actions.append({ScriptActionType::ProcedureCall, value});
        } else if (key.compare("If", Qt::CaseInsensitive) == 0) {
            currentTimer.actions.append({ScriptActionType::If, value});
        } else if (key.compare("NotIf", Qt::CaseInsensitive) == 0) {
            currentTimer.actions.append({ScriptActionType::NotIf, value});
        } else if (key.compare("SetFlag", Qt::CaseInsensitive) == 0) {
            currentTimer.actions.append({ScriptActionType::SetFlag, value});
        } else if (key.compare("Set#", Qt::CaseInsensitive) == 0) {
            currentTimer.actions.append({ScriptActionType::SetCounterVar, value});
        } else if (key.compare("Input#", Qt::CaseInsensitive) == 0) {
            currentTimer.actions.append({ScriptActionType::InputCounter, value});
        } else if (key.compare("Change#", Qt::CaseInsensitive) == 0) {
            currentTimer.actions.append({ScriptActionType::ChangeCounter, value});
        } else if (key.compare("InputNeg#", Qt::CaseInsensitive) == 0) {
            currentTimer.actions.append({ScriptActionType::InputNegCounter, value});
        } else if (key.compare("Random#", Qt::CaseInsensitive) == 0) {
            currentTimer.actions.append({ScriptActionType::RandomCounter, value});
        } else if (key.compare("Drop#", Qt::CaseInsensitive) == 0) {
            currentTimer.actions.append({ScriptActionType::DropCounter, value});
        } else if (key.compare("Add#", Qt::CaseInsensitive) == 0) {
            currentTimer.actions.append({ScriptActionType::AddCounter, value});
        } else if (key.compare("Subtract#", Qt::CaseInsensitive) == 0) {
            currentTimer.actions.append({ScriptActionType::SubtractCounter, value});
        } else if (key.compare("Set!", Qt::CaseInsensitive) == 0) {
            currentTimer.actions.append({ScriptActionType::SetTimeVar, value});
        } else if (key.compare("Input!", Qt::CaseInsensitive) == 0) {
            currentTimer.actions.append({ScriptActionType::InputDate, value});
        } else if (key.compare("InputDateDef!", Qt::CaseInsensitive) == 0) {
            currentTimer.actions.append({ScriptActionType::InputDateDef, value});
        } else if (key.compare("ChangeDate!", Qt::CaseInsensitive) == 0) {
            currentTimer.actions.append({ScriptActionType::ChangeDate, value});
        } else if (key.compare("InputTime!", Qt::CaseInsensitive) == 0) {
            currentTimer.actions.append({ScriptActionType::InputTime, value});
        } else if (key.compare("InputTimeDef!", Qt::CaseInsensitive) == 0) {
            currentTimer.actions.append({ScriptActionType::InputTimeDef, value});
        } else if (key.compare("ChangeTime!", Qt::CaseInsensitive) == 0) {
            currentTimer.actions.append({ScriptActionType::ChangeTime, value});
        } else if (key.compare("InputInterval!", Qt::CaseInsensitive) == 0) {
            currentTimer.actions.append({ScriptActionType::InputInterval, value});
        } else if (key.compare("ChangeInterval!", Qt::CaseInsensitive) == 0) {
            currentTimer.actions.append({ScriptActionType::ChangeInterval, value});
        } else if (key.compare("AddDays!", Qt::CaseInsensitive) == 0) {
            currentTimer.actions.append({ScriptActionType::AddDaysTime, value});
        } else if (key.compare("SubtractDays!", Qt::CaseInsensitive) == 0) {
            currentTimer.actions.append({ScriptActionType::SubtractDaysTime, value});
        } else if (key.compare("Days!", Qt::CaseInsensitive) == 0) {
            currentTimer.actions.append({ScriptActionType::ConvertDays, value});
        } else if (key.compare("Hours!", Qt::CaseInsensitive) == 0) {
            currentTimer.actions.append({ScriptActionType::ConvertHours, value});
        } else if (key.compare("Minutes!", Qt::CaseInsensitive) == 0) {
            currentTimer.actions.append({ScriptActionType::ConvertMinutes, value});
        } else if (key.compare("Seconds!", Qt::CaseInsensitive) == 0) {
            currentTimer.actions.append({ScriptActionType::ConvertSeconds, value});
        } else if (key.compare("Round!", Qt::CaseInsensitive) == 0) {
            currentTimer.actions.append({ScriptActionType::RoundTime, value});
        } else if (key.compare("Random!", Qt::CaseInsensitive) == 0) {
            currentTimer.actions.append({ScriptActionType::RandomTime, value});
        } else if (key.compare("Drop!", Qt::CaseInsensitive) == 0) {
            currentTimer.actions.append({ScriptActionType::DropTime, value});
        } else if (key.compare("Days#", Qt::CaseInsensitive) == 0) {
            currentTimer.actions.append({ScriptActionType::ExtractDays, value});
        } else if (key.compare("Hours#", Qt::CaseInsensitive) == 0) {
            currentTimer.actions.append({ScriptActionType::ExtractHours, value});
        } else if (key.compare("Minutes#", Qt::CaseInsensitive) == 0) {
            currentTimer.actions.append({ScriptActionType::ExtractMinutes, value});
        } else if (key.compare("Seconds#", Qt::CaseInsensitive) == 0) {
            currentTimer.actions.append({ScriptActionType::ExtractSeconds, value});
        } else if (key.compare("Message", Qt::CaseInsensitive) == 0) {
            currentTimer.actions.append({ScriptActionType::Message, value});
        } else if (key.compare("NewStatus", Qt::CaseInsensitive) == 0) {
            currentTimer.actions.append({ScriptActionType::NewStatus, value});
        } else if (key.compare("MarkDone", Qt::CaseInsensitive) == 0) {
            currentTimer.actions.append({ScriptActionType::MarkDone, value});
        } else if (key.compare("Abort", Qt::CaseInsensitive) == 0) {
            currentTimer.actions.append({ScriptActionType::Abort, value});
        } else if (key.compare("Delete", Qt::CaseInsensitive) == 0) {
            currentTimer.actions.append({ScriptActionType::Delete, value});
        } else if (key.compare("AddMerit", Qt::CaseInsensitive) == 0 || key.compare("AddMerits", Qt::CaseInsensitive) == 0) {
            currentTimer.actions.append({ScriptActionType::AddMerit, value});
        } else if (key.compare("SubtractMerit", Qt::CaseInsensitive) == 0 || key.compare("SubtractMerits", Qt::CaseInsensitive) == 0) {
            currentTimer.actions.append({ScriptActionType::SubtractMerit, value});
        } else if (key.compare("SetMerit", Qt::CaseInsensitive) == 0 || key.compare("SetMerits", Qt::CaseInsensitive) == 0) {
            currentTimer.actions.append({ScriptActionType::SetMerit, value});
        } else if (key.compare("PunishMessage", Qt::CaseInsensitive) == 0) {
            if (currentPunishMessage.isEmpty()) {
                currentPunishMessage = value;
            }
        } else if (key.compare("PunishmentGroup", Qt::CaseInsensitive) == 0) {
            if (currentTimer.punishGroup.isEmpty()) {
                currentTimer.punishGroup = value;
            }
        } else if (key.compare("Punish", Qt::CaseInsensitive) == 0) {
            currentTimer.actions.append({ScriptActionType::Punish, value});
        } else if (key.compare("Clothing", Qt::CaseInsensitive) == 0) {
            if (currentTimer.clothingInstruction.isEmpty()) {
                currentTimer.actions.append({ScriptActionType::Clothing, value});
            } else {
                currentTimer.actions.append({ScriptActionType::Clothing, value});
            }
        } else if (key.compare("Instructions", Qt::CaseInsensitive) == 0) {
            currentTimer.actions.append({ScriptActionType::Instructions, value});
        } else if (key.compare("ClothReport", Qt::CaseInsensitive) == 0) {
            // Add as an action (value is "1" or "Title Text"
            if (value != "0") {
                currentTimer.actions.append({ScriptActionType::ClothReport, value});
            }
        }
        else if (key.compare("ClearCloth", Qt::CaseInsensitive) == 0) {
            currentTimer.actions.append({ScriptActionType::ClearCloth, value});
        }

        else if (key.compare("CameraInterval", Qt::CaseInsensitive) == 0) {
            QStringList parts = value.split(',', Qt::SkipEmptyParts);
            if (parts.size() == 2) {
                currentTimer.cameraIntervalMin = parts[0].trimmed();
                currentTimer.cameraIntervalMax = parts[1].trimmed();
            } else {
                currentTimer.cameraIntervalMin = currentTimer.cameraIntervalMax = value.trimmed();
            }
        }
        else if (key.compare("PointCamera", Qt::CaseInsensitive) == 0) {
            currentTimer.pointCameraText = value;
            currentTimer.actions.append({ScriptActionType::PointCamera, value});
        }
        else if (key.compare("PoseCamera", Qt::CaseInsensitive) == 0) {
            currentTimer.poseCameraText = value;
            currentTimer.actions.append({ScriptActionType::PoseCamera, value});
        }
        else if (key.compare("Set$", Qt::CaseInsensitive) == 0) {
            currentTimer.actions.append({ScriptActionType::SetString, value});
        }
        else if (key.compare("Input$", Qt::CaseInsensitive) == 0) {
            currentTimer.actions.append({ScriptActionType::InputString, value});
        }
        else if (key.compare("Change$", Qt::CaseInsensitive) == 0) {
            currentTimer.actions.append({ScriptActionType::ChangeString, value});
        }
        else if (key.compare("InputLong$", Qt::CaseInsensitive) == 0) {
            currentTimer.actions.append({ScriptActionType::InputLongString, value});
        }
        else if (key.compare("ChangeLong$", Qt::CaseInsensitive) == 0) {
            currentTimer.actions.append({ScriptActionType::ChangeLongString, value});
        }
        else if (key.compare("Drop$", Qt::CaseInsensitive) == 0) {
            currentTimer.actions.append({ScriptActionType::DropString, value});
        }
        // ... (add all other action types as needed) ...
    }

    // Save the very last timer in the file
    if (inSection) {
        scriptData.timers.insert(currentTimer.name, currentTimer);
    }
}

void ScriptParser::parseQuestionSections(const QMap<QString, QMap<QString, QStringList>>& sections) {
    for (auto it = sections.begin(); it != sections.end(); ++it) {
        QString sectionName = it.key();
        if (!sectionName.startsWith("question-"))
            continue;

        QString questionName = sectionName.mid(QString("question-").length()).toLower();
        const QMap<QString, QStringList>& entries = it.value();

        QuestionDefinition question;
        question.name = questionName;

        QuestionAnswerBlock* currentAnswer = nullptr;

        for (auto e = entries.begin(); e != entries.end(); ++e) {
            QString key = e.key();
            const QStringList& values = e.value();

            for (const QString& val : values) {
                QString value = val.trimmed();

                if (key.compare("Phrase", Qt::CaseInsensitive) == 0 ||
                    key.compare("Text", Qt::CaseInsensitive) == 0) {
                    question.text = value;
                    question.phrase = value;
                } else if (key.startsWith("?")) {
                    // Save previous answer if exists
                    if (currentAnswer)
                        question.answers.append(*currentAnswer);

                    currentAnswer = new QuestionAnswerBlock();
                    currentAnswer->answerText = key.mid(1).trimmed();

                    if (value == "*") {
                        currentAnswer->hasInlineActions = true;
                    } else {
                        currentAnswer->procedureName = value.trimmed().toLower();
                    }
                } else if (currentAnswer && currentAnswer->hasInlineActions) {
                    if (key == "Message") {
                        MessageGroup msg;
                        msg.mode = MessageSelectMode::All;
                        msg.messages.append(value);
                        currentAnswer->messages.append(msg);
                    } else if (key == "Punish") {
                        QStringList parts = value.split(',', Qt::SkipEmptyParts);
                        if (parts.size() == 2) {
                            currentAnswer->punishMin = parts[0].toInt();
                            currentAnswer->punishMax = parts[1].toInt();
                        } else {
                            currentAnswer->punishMin = currentAnswer->punishMax = value.toInt();
                        }
                    }
                } else if (key.compare("NoInputProcedure", Qt::CaseInsensitive) == 0) {
                    // Store directly in the struct, DO NOT append to answers
                    question.noInputProcedure = value.trimmed().toLower();
                } else if (key.compare("If", Qt::CaseInsensitive) == 0) {
                    question.ifConditions.append(value);
                } else if (key.compare("NotIf", Qt::CaseInsensitive) == 0) {
                    question.notIfConditions.append(value);
                }
            }
        }

        // Ensure a variable name exists
        if (question.variable.isEmpty()) {
            question.variable = question.name; // Default to question name
        }

        // Save last answer
        if (currentAnswer)
            question.answers.append(*currentAnswer);

        scriptData.questions.insert(question.name, question);
    }
}

void ScriptParser::parseDurationControl(const QMap<QString, QStringList>& entries, DurationControl& d) {
    if (entries.contains("MinTime")) {
        QStringList times = entries["MinTime"].value(0).split(',', Qt::SkipEmptyParts);
        d.minTimeStart = times.value(0);
        d.minTimeEnd = times.value(1, times.value(0));
    }
    if (entries.contains("MaxTime")) {
        QStringList times = entries["MaxTime"].value(0).split(',', Qt::SkipEmptyParts);
        d.maxTimeStart = times.value(0);
        d.maxTimeEnd = times.value(1, times.value(0));
    }

    if (entries.contains("QuickPenalty1")) d.quickPenalty1 = entries["QuickPenalty1"].value(0).toInt();
    if (entries.contains("QuickPenalty2")) d.quickPenalty2 = entries["QuickPenalty2"].value(0).toDouble();
    if (entries.contains("QuickPenaltyGroup")) d.quickPenaltyGroup = entries["QuickPenaltyGroup"].value(0);
    if (entries.contains("QuickMessage")) d.quickMessage = entries["QuickMessage"].value(0);
    if (entries.contains("QuickProcedure")) d.quickProcedure = entries["QuickProcedure"].value(0);

    if (entries.contains("SlowPenalty1")) d.slowPenalty1 = entries["SlowPenalty1"].value(0).toInt();
    if (entries.contains("SlowPenalty2")) d.slowPenalty2 = entries["SlowPenalty2"].value(0).toDouble();
    if (entries.contains("SlowPenaltyGroup")) d.slowPenaltyGroup = entries["SlowPenaltyGroup"].value(0);
    if (entries.contains("SlowMessage")) d.slowMessage = entries["SlowMessage"].value(0);
    if (entries.contains("SlowProcedure")) d.slowProcedure = entries["SlowProcedure"].value(0);

    if (entries.contains("MinTimeProcedure")) d.minTimeProcedure = entries["MinTimeProcedure"].value(0);
    if (entries.contains("MaxTimeProcedure")) d.maxTimeProcedure = entries["MaxTimeProcedure"].value(0);
}

void ScriptParser::parseTimeWindowControl(const QMap<QString, QStringList>& entries, TimeWindowControl& tw) {
    if (entries.contains("Earliest")) {
        QStringList times = entries["Earliest"].value(0).split(',', Qt::SkipEmptyParts);
        tw.earliestMin = times.value(0);
        tw.earliestMax = times.value(1, times.value(0));
    }

    if (entries.contains("EarlyPenalty1"))
        tw.earlyPenalty1 = entries["EarlyPenalty1"].value(0).toInt();
    if (entries.contains("EarlyPenalty2"))
        tw.earlyPenalty2 = entries["EarlyPenalty2"].value(0).toDouble();
    if (entries.contains("EarlyPenaltyGroup"))
        tw.earlyPenaltyGroup = entries["EarlyPenaltyGroup"].value(0);
    if (entries.contains("EarlyProcedure"))
        tw.earlyProcedure = entries["EarlyProcedure"].value(0);

    if (entries.contains("Latest")) {
        QStringList times = entries["Latest"].value(0).split(',', Qt::SkipEmptyParts);
        tw.latestMin = times.value(0);
        tw.latestMax = times.value(1, times.value(0));
    }

    if (entries.contains("LatePenalty1"))
        tw.latePenalty1 = entries["LatePenalty1"].value(0).toInt();
    if (entries.contains("LatePenalty2"))
        tw.latePenalty2 = entries["LatePenalty2"].value(0).toDouble();
    if (entries.contains("LatePenaltyGroup"))
        tw.latePenaltyGroup = entries["LatePenaltyGroup"].value(0);
    if (entries.contains("LateProcedure"))
        tw.lateProcedure = entries["LateProcedure"].value(0);
}

void ScriptParser::parseTimeRestrictions(const QMap<QString, QStringList>& entries, QStringList& notBefore, QStringList& notAfter, QList<QPair<QString, QString>>& notBetween) {
    if (entries.contains("NotBefore")) {
        for (const QString& time : entries["NotBefore"])
            notBefore.append(time.trimmed());
    }

    if (entries.contains("NotAfter")) {
        for (const QString& time : entries["NotAfter"])
            notAfter.append(time.trimmed());
    }

    if (entries.contains("NotBetween")) {
        for (const QString& line : entries["NotBetween"]) {
            QStringList parts = line.split(',', Qt::SkipEmptyParts);
            if (parts.size() == 2)
                notBetween.append({ parts[0].trimmed(), parts[1].trimmed() });
        }
    }
}

void ScriptParser::parseEventSection(const QMap<QString, QStringList>& entries) {
    auto& e = scriptData.eventHandlers;

    auto get = [&](const QString& key) -> QString {
        return entries.contains(key) ? entries[key].value(0) : "";
    };

    e.firstRun           = get("FirstRun");
    e.openProgram        = get("OpenProgram");
    e.closeProgram       = get("CloseProgram");
    e.startFromPause     = get("StartFromPause");
    e.deleteStatus       = get("DeleteStatus");
    e.minimize           = get("Minimize");
    e.restore            = get("Restore");

    e.beforeNewReport    = get("BeforeNewReport");
    e.afterNewReport     = get("AfterNewReport");
    e.afterReport        = get("AfterReport");
    e.forgetConfession   = get("ForgetConfession");
    e.ignoreConfession   = get("IgnoreConfession");

    e.permissionGiven    = get("PermissionGiven");
    e.permissionDenied   = get("PermissionDenied");

    e.punishmentGiven    = get("PunishmentGiven");
    e.punishmentAsked    = get("PunishmentAsked");
    e.punishmentDone     = get("PunishmentDone");

    e.jobAnnounced       = get("JobAnnounced");
    e.jobDone            = get("JobDone");

    e.beforeDelete       = get("BeforeDelete");

    e.beforeClothReport  = get("BeforeClothReport");
    e.afterClothReport   = get("AfterClothReport");
    e.checkOn            = get("CheckOn");
    e.checkOff           = get("CheckOff");
    e.checkAll           = get("CheckAll");

    e.signIn             = get("Signin");
    e.meritsChanged      = get("MeritsChanged");
    e.autoAssignEnd      = get("AutoAssignEnd");
    e.autoAssignNone     = get("AutoAssignNone");
    e.mailFailure        = get("MailFailure");
    e.newDay             = get("NewDay");

    e.beforeStatusChange = get("BeforeStatusChange");
    e.afterStatusChange  = get("AfterStatusChange");
}

void ScriptParser::parseFtpSection(const QMap<QString, QStringList>& entries) {
    FTPSettings& ftp = scriptData.ftpSettings;

    auto get = [&](const QString& key) -> QString {
        return entries.contains(key) ? entries[key].value(0).trimmed() : "";
    };

    ftp.url          = get("URL");
    ftp.user         = get("ftpUser");
    ftp.password     = get("ftpPassword");
    ftp.serverType   = get("ftpServerType");
    ftp.directory    = get("ftpDir");

    ftp.updateScript = get("UpdateScript");
    ftp.updateProgram = get("UpdateProgram");

    if (entries.contains("SendReports"))
        ftp.sendReports = entries["SendReports"].value(0).toInt() == 1;

    if (entries.contains("FtpLog"))
        ftp.ftpLog = entries["FtpLog"].value(0).toInt() == 1;

    if (entries.contains("TestFtp"))
        ftp.testFtp = entries["TestFtp"].value(0).toInt() == 1;

    if (entries.contains("SendPictures"))
        ftp.sendPictures = entries["SendPictures"].value(0).toInt() == 1;
}

void ScriptParser::parseFontSection(const QMap<QString, QStringList>& entries) {
    if (entries.contains("TextSize"))
        scriptData.general.textSize = entries["TextSize"].value(0).toInt();

    if (entries.contains("ButtonSize"))
        scriptData.general.buttonSize = entries["ButtonSize"].value(0).toInt();

    if (entries.contains("Border"))
        scriptData.general.border = entries["Border"].value(0).toInt();
}

void ScriptParser::parseFlagSection(const QString& name, const QMap<QString, QStringList>& entries) {
    FlagDefinition flag;
    flag.name = name;

    if (entries.contains("text"))
        flag.textLines = entries["text"];
    if (entries.contains("Group"))
        flag.group = entries["Group"].value(0).trimmed();
    if (entries.contains("Duration")) {
        QStringList dur = entries["Duration"].value(0).split(',', Qt::SkipEmptyParts);
        flag.durationMin = dur.value(0).trimmed();
        flag.durationMax = dur.value(1, dur.value(0)).trimmed();
    }
    if (entries.contains("ExpireMessage"))
        flag.expireMessage = entries["ExpireMessage"].value(0).trimmed();
    if (entries.contains("ExpireProcedure"))
        flag.expireProcedure = entries["ExpireProcedure"].value(0).trimmed();
    if (entries.contains("SetProcedure"))
        flag.setProcedure = entries["SetProcedure"].value(0).trimmed();
    if (entries.contains("RemoveProcedure"))
        flag.removeProcedure = entries["RemoveProcedure"].value(0).trimmed();
    if (entries.contains("ReportFlag"))
        flag.reportFlag = entries["ReportFlag"].value(0).toInt() != 0;

    scriptData.flagDefinitions[name] = flag;
}

void ScriptParser::processListCommands(const QStringList &commands) {
    for (const QString &cmd : commands) {
        QString trimmed = cmd.trimmed();
        if (trimmed.startsWith("Set*=")) {
            auto parts = trimmed.section('=', 1).split(',', Qt::SkipEmptyParts);
            if (parts.isEmpty()) {
                qWarning() << "Malformed list command (no list name):" << trimmed;
                continue;
            }
            QString listName = parts.takeFirst().trimmed();
            QStringList items;
            for (QString &val : parts) {
                val = val.trimmed();
                if (val.startsWith("*")) {
                    items += scriptData.lists.value(val);
                } else {
                    items << val;
                }
            }
            scriptData.lists[listName] = items;
        } else if (trimmed.startsWith("Add*=")) {
            auto parts = trimmed.section('=', 1).split(',', Qt::SkipEmptyParts);
            if (parts.isEmpty()) {
                qWarning() << "Malformed list command (no list name):" << trimmed;
                continue;
            }
            QString listName = parts.takeFirst().trimmed();
            for (QString &val : parts) {
                val = val.trimmed();
                if (val.startsWith("*")) {
                    scriptData.lists[listName] += scriptData.lists.value(val);
                } else {
                    scriptData.lists[listName] << val;
                }
            }
        } else if (trimmed.startsWith("AddNoDub*=")) {
            auto parts = trimmed.section('=', 1).split(',', Qt::SkipEmptyParts);
            if (parts.isEmpty()) {
                qWarning() << "Malformed list command (no list name):" << trimmed;
                continue;
            }
            QString listName = parts.takeFirst().trimmed();
            for (QString &val : parts) {
                val = val.trimmed();
                if (val.startsWith("*")) {
                    scriptData.lists[listName] += scriptData.lists.value(val);
                } else {
                    scriptData.lists[listName] << val;
                }
            }
        } else if (trimmed.startsWith("Push*=")) {
            auto parts = trimmed.section('=', 1).split(',', Qt::SkipEmptyParts);
            if (parts.isEmpty()) {
                qWarning() << "Malformed list command (no list name):" << trimmed;
                continue;
            }
            QString listName = parts.takeFirst().trimmed();
            for (QString &val : parts) {
                val = val.trimmed();
                if (val.startsWith("*")) {
                    scriptData.lists[listName] += scriptData.lists.value(val);
                } else {
                    scriptData.lists[listName] << val;
                }
            }
        } else if (trimmed.startsWith("Pull*=")) {
            auto parts = trimmed.section('=', 1).split(',', Qt::SkipEmptyParts);
            if (parts.isEmpty()) {
                qWarning() << "Malformed list command (no list name):" << trimmed;
                continue;
            }
            QString listName = parts.takeFirst().trimmed();
            for (QString &val : parts) {
                val = val.trimmed();
                if (val.startsWith("*")) {
                    scriptData.lists[listName] += scriptData.lists.value(val);
                } else {
                    scriptData.lists[listName] << val;
                }
            }
        } else if (trimmed.startsWith("Intersect*=")) {
            auto parts = trimmed.section('=', 1).split(',', Qt::SkipEmptyParts);
            if (parts.isEmpty()) {
                qWarning() << "Malformed list command (no list name):" << trimmed;
                continue;
            }
            QString listName = parts.takeFirst().trimmed();
            for (QString &val : parts) {
                val = val.trimmed();
                if (val.startsWith("*")) {
                    scriptData.lists[listName] += scriptData.lists.value(val);
                } else {
                    scriptData.lists[listName] << val;
                }
            }
        } else if (trimmed.startsWith("Sort*=")) {
            auto parts = trimmed.section('=', 1).split(',', Qt::SkipEmptyParts);
            if (parts.isEmpty()) {
                qWarning() << "Malformed list command (no list name):" << trimmed;
                continue;
            }
            QString listName = parts.takeFirst().trimmed();
            for (QString &val : parts) {
                val = val.trimmed();
                if (val.startsWith("*")) {
                    scriptData.lists[listName] += scriptData.lists.value(val);
                } else {
                    scriptData.lists[listName] << val;
                }
            }
        } else if (trimmed.startsWith("Remove*=")) {
            auto parts = trimmed.section('=', 1).split(',', Qt::SkipEmptyParts);
            if (parts.isEmpty()) {
                qWarning() << "Malformed list command (no list name):" << trimmed;
                continue;
            }
            QString listName = parts.takeFirst().trimmed();
            for (QString &val : parts) {
                val = val.trimmed();
                if (val.startsWith("*")) {
                    scriptData.lists[listName] += scriptData.lists.value(val);
                } else {
                    scriptData.lists[listName] << val;
                }
            }
        } else if (trimmed.startsWith("SetSplit*=")) {
            auto parts = trimmed.section('=', 1).split(',', Qt::SkipEmptyParts);
            if (parts.isEmpty()) {
                qWarning() << "Malformed list command (no list name):" << trimmed;
                continue;
            }
            QString listName = parts.takeFirst().trimmed();
            for (QString &val : parts) {
                val = val.trimmed();
                if (val.startsWith("*")) {
                    scriptData.lists[listName] += scriptData.lists.value(val);
                } else {
                    scriptData.lists[listName] << val;
                }
            }
        } else if (trimmed.startsWith("AddSplit*=")) {
            auto parts = trimmed.section('=', 1).split(',', Qt::SkipEmptyParts);
            if (parts.isEmpty()) {
                qWarning() << "Malformed list command (no list name):" << trimmed;
                continue;
            }
            QString listName = parts.takeFirst().trimmed();
            for (QString &val : parts) {
                val = val.trimmed();
                if (val.startsWith("*")) {
                    scriptData.lists[listName] += scriptData.lists.value(val);
                } else {
                    scriptData.lists[listName] << val;
                }
            }
        } else if (trimmed.startsWith("RemoveAll*=")) {
            auto parts = trimmed.section('=', 1).split(',', Qt::SkipEmptyParts);
            if (parts.isEmpty()) {
                qWarning() << "Malformed list command (no list name):" << trimmed;
                continue;
            }
            QString listName = parts.takeFirst().trimmed();
            for (QString &val : parts) {
                val = val.trimmed();
                if (val.startsWith("*")) {
                    scriptData.lists[listName] += scriptData.lists.value(val);
                } else {
                    scriptData.lists[listName] << val;
                }
            }
        } else if (trimmed.startsWith("Clear*=")) {
            auto parts = trimmed.section('=', 1).split(',', Qt::SkipEmptyParts);
            if (parts.isEmpty()) {
                qWarning() << "Malformed list command (no list name):" << trimmed;
                continue;
            }
            QString listName = parts.takeFirst().trimmed();
            for (QString &val : parts) {
                val = val.trimmed();
                if (val.startsWith("*")) {
                    scriptData.lists[listName] += scriptData.lists.value(val);
                } else {
                    scriptData.lists[listName] << val;
                }
            }
        } else if (trimmed.startsWith("Drop*=")) {
            auto parts = trimmed.section('=', 1).split(',', Qt::SkipEmptyParts);
            if (parts.isEmpty()) {
                qWarning() << "Malformed list command (no list name):" << trimmed;
                continue;
            }
            QString listName = parts.takeFirst().trimmed();
            for (QString &val : parts) {
                val = val.trimmed();
                if (val.startsWith("*")) {
                    scriptData.lists[listName] += scriptData.lists.value(val);
                } else {
                    scriptData.lists[listName] << val;
                }
            }
        }
    }
}

bool ScriptParser::shouldHideClock(const QString &currentStatus) const {
    if (scriptData.statuses.contains(currentStatus)) {
        return scriptData.statuses.value(currentStatus).hideTime;
    }
    return scriptData.generalSettings.value("HideTime") == "1";
}

QString ScriptParser::resolveIncludePath(const QString& basePath, const QString& includeLine)
{
    QString fileName = includeLine.mid(QStringLiteral("%include=").length()).trimmed();
    QFileInfo fileInfo(fileName);
    if (fileInfo.isAbsolute()) {
        return fileName;
    } else {
        QDir baseDir = QFileInfo(basePath).absoluteDir();
        return baseDir.absoluteFilePath(fileName);
    }
}

CaseBlock ScriptParser::parseCaseBlock(const QStringList &lines, int &index) {
    CaseBlock block;
    block.mode = CaseMode::All;

    // Parse opening Case line
    QString header = lines[index++].trimmed();
    if (header.contains("First", Qt::CaseInsensitive))
        block.mode = CaseMode::First;
    else if (header.contains("Random", Qt::CaseInsensitive))
        block.mode = CaseMode::Random;

    while (index < lines.size()) {
        QString line = lines[index++].trimmed();
        if (line.compare("Case=End", Qt::CaseInsensitive) == 0)
            break;

        if (line.startsWith("When", Qt::CaseInsensitive)) {
            CaseBranch branch;

            if (line.startsWith("When=Random", Qt::CaseInsensitive)) {
                branch.type = WhenType::WhenRandom;
            } else if (line.startsWith("When=All", Qt::CaseInsensitive)) {
                branch.type = WhenType::WhenAll;
            } else if (line.startsWith("When=NotAll", Qt::CaseInsensitive)) {
                branch.type = WhenType::WhenNotAll;
            } else if (line.startsWith("When=Any", Qt::CaseInsensitive)) {
                branch.type = WhenType::WhenAny;
            } else if (line.startsWith("When=None", Qt::CaseInsensitive)) {
                branch.type = WhenType::WhenNone;
            } else if (line.startsWith("WhenNot=", Qt::CaseInsensitive)) {
                branch.type = WhenType::WhenNot;
                branch.condition = line.section('=', 1).trimmed();
            } else if (line.startsWith("When=", Qt::CaseInsensitive)) {
                branch.type = WhenType::When;
                branch.condition = line.section('=', 1).trimmed();
            }

            // Gather statements until next When or Case=End
            while (index < lines.size()) {
                QString nextLine = lines[index].trimmed();
                if (nextLine.startsWith("When", Qt::CaseInsensitive) || nextLine.startsWith("Case=End", Qt::CaseInsensitive))
                    break;
                branch.statements.append(nextLine);
                ++index;
            }

            block.branches.append(branch);
        }
    }

    return block;
}

bool ScriptParser::loadFromCDS(const QString &cdsPath)
{
    // 1) If there is no .cds file, return false (nothing to load).
    QFile file(cdsPath);
    if (!file.exists()) {
        return false;
    }

    // 2) Open for text‐read. If fail, warn and return false.
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "[ScriptParser] Failed to open .cds:" << cdsPath
                   << "–" << file.errorString();
        return false;
    }

    // 3) Read line by line. Assume each line is “variableName=value”.
    QTextStream in(&file);
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    in.setEncoding(QStringConverter::Utf8);
#else
    in.setCodec("UTF-8");
#endif
    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        // Skip blank lines or comments ("#"/";").
        if (line.isEmpty() || line.startsWith('#') || line.startsWith(';'))
            continue;

        // Split on the first '='.
        QStringList parts = line.split('=', Qt::KeepEmptyParts);
        if (parts.size() < 2)
            continue;

        QString varName  = parts[0].trimmed();
        // Re‐join anything after the first '=' (in case the value contains '=').
        QString varValue = parts.mid(1).join('=').trimmed();

        // Store the loaded variable
        scriptData.stringVariables[varName] = varValue;
    }

    file.close();
    return true;
}

QStringList ScriptParser::getRawSectionNames() const {
    return scriptData.rawSections.keys();
}

QMap<QString, QStringList> ScriptParser::getRawSectionData(const QString &section) const {
    return scriptData.rawSections.value(section.toLower());
}

QString ScriptParser::getIniValue(const QString &section, const QString &key, const QString &defaultValue) const {
    QMap<QString, QStringList> sec = scriptData.rawSections.value(section.toLower());
    if (sec.contains(key) && !sec[key].isEmpty())
        return sec[key].first();
    return defaultValue;
}

QString ScriptParser::getMaster() const {
    return scriptData.general.masterName;
}

QString ScriptParser::getSubName() const {
    const QStringList &names = scriptData.general.subNames;
    if (names.isEmpty())
        return QString();
    if (names.size() == 1)
        return names.first();
    int index = QRandomGenerator::global()->bounded(names.size());
    return names.at(index);
}

int ScriptParser::getMinMerits() const { return scriptData.general.minMerits; }
int ScriptParser::getMaxMerits() const { return scriptData.general.maxMerits; }

bool ScriptParser::isTestMenuEnabled() const {
    return scriptData.generalSettings.value("TestMenu") == "1";
}

QList<StatusSection> ScriptParser::getStatusSections() const {
    return scriptData.statuses.values();
}

StatusSection ScriptParser::getStatus(const QString &name) const {
    return scriptData.statuses.value(name, StatusDefinition());
}

QList<JobSection> ScriptParser::getJobSections() const {
    return scriptData.jobs.values();
}

QList<PunishmentSection> ScriptParser::getPunishmentSections() const {
    return scriptData.punishments.values();
}

QList<PermissionDefinition> ScriptParser::getPermissionSections() const {
    return scriptData.permissions.values();
}

QList<ClothTypeSection> ScriptParser::getClothTypeSections() const {
    return scriptData.clothingTypes.values();
}

QList<ConfessionDefinition> ScriptParser::getConfessionSections() const {
    return scriptData.confessions.values();
}

QList<InstructionDefinition> ScriptParser::getInstructionSections() const {
    QList<InstructionDefinition> result;
    for (auto it = scriptData.instructions.constBegin(); it != scriptData.instructions.constEnd(); ++it) {
        QString lowerName = it.key().toLower();
        if (scriptData.rawSections.contains(QString("set-%1").arg(lowerName)))
            continue;
        result.append(it.value());
    }
    return result;
}

QuestionSection ScriptParser::getQuestion(const QString &name) const {
    return scriptData.questions.value(name, QuestionDefinition());
}

void ScriptParser::setVariable(const QString &name, const QString &value) {
    scriptData.stringVariables[name] = value;
}

QString ScriptParser::getVariable(const QString &varName) const
{
    // Check String Variables
    if (scriptData.stringVariables.contains(varName)) {
        return scriptData.stringVariables.value(varName);
    }

    // Check Time Variables
    if (scriptData.timeVariables.contains(varName)) {
        QVariant val = scriptData.timeVariables.value(varName);
        if (val.userType() == QMetaType::QDateTime) {
            // Format Date: yyyy-MM-dd HH:mm:ss
            return val.toDateTime().toString("yyyy-MM-dd HH:mm:ss");
        } else if (val.userType() == QMetaType::QTime) {
            return val.toTime().toString("HH:mm:ss");
        } else if (val.canConvert<int>()) {
            // Format Duration: 3d or hh:mm:ss
            return ScriptUtils::formatDurationString(val.toInt());
        }
    }

    // Default fallback
    return "0";
}

void ScriptParser::removeVariable(const QString &name) {
    scriptData.stringVariables.remove(name);
}

void ScriptParser::setTimeVariable(const QString &name, const QVariant &value) {
    scriptData.timeVariables[name] = value;
}

QVariant ScriptParser::getTimeVariable(const QString &name) const {
    return scriptData.timeVariables.value(name);
}

void ScriptParser::removeTimeVariable(const QString &name) {
    scriptData.timeVariables.remove(name);
}
