#include "scriptparser.h"
#include <QSettings>
#include <QFile>
#include <QTextStream>
#include <QDebug>
#include <QFileInfo>
#include <QDir>

QMap<QString, QMap<QString, QStringList>> ScriptParser::parseIniFile(const QString& path)
{
    QFile file(path);
    QMap<QString, QMap<QString, QStringList>> sections;

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Failed to open script file:" << path;
        return sections;
    }

    QTextStream in(&file);
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
    QString currentSection;
    for (const QString& line : lines) {
        if (line.isEmpty() || line.startsWith("#") || line.startsWith(";"))
            continue;

        if (line.startsWith("[") && line.endsWith("]")) {
            currentSection = line.mid(1, line.length() - 2).trimmed().toLower();
            continue;
        }

        int equalsIndex = line.indexOf('=');
        if (equalsIndex != -1 && !currentSection.isEmpty()) {
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
    parseReportSections(sections);
    parseConfessionSections(sections);
    parsePermissionSections(sections);
    parsePunishmentSections(sections);
    parseJobSections(sections);
    parseInstructionSets(sections);
    parseInstructionSections(sections);
    parseProcedureSections(sections);
    parsePopupGroupSections(sections);
    parsePopupSections(sections);
    parseTimerSections(sections);
    parseQuestionSections(sections);

    if (sections.contains("events"))
        parseEventsSection(sections["events"]);

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

        if (section.contains("tslPort")) g.tlsPort = section["tlsPort"].value(0).toInt();

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

        if (section.contains("HideTime")) {
            g.hideTimeGlobal = !section["HideTime"].isEmpty() && section["HideTime"].first().trimmed() == "1";
        }

        if (!section.contains("ValidateAll")) {
            validateAll = (minVersion >= 3.4f);
        }
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
}

void ScriptParser::parseEventsSection(const QMap<QString, QStringList>& section) {
    auto& e = scriptData.events;

    if (section.contains("FirstRun"))
        e.firstRunProcedure = section["FirstRun"].value(0);
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

        if (entries.contains("SigninInterval")) {
            QStringList parts = entries["SigninInterval"].value(0).split(',', Qt::SkipEmptyParts);
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
            status.cameraPrompt = entries["PointCamera"].value(0);

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
        if (entries.contains("HideTime")) {
            status.hideTime = (entries["HideTime"].first().trimmed() == "1");
        }

        parseDurationControl(entries, status.duration);

        scriptData.statuses.insert(statusName, status);

        scriptData.statuses[statusName] = status;
    }
}

void ScriptParser::parseReportSections(const QMap<QString, QMap<QString, QStringList>>& sections) {
    for (auto it = sections.begin(); it != sections.end(); ++it) {
        const QString& sectionName = it.key();
        if (!sectionName.startsWith("report-"))
            continue;

        QString reportName = sectionName.mid(QString("report-").length());
        const QMap<QString, QStringList>& entries = it.value();

        ReportDefinition report;
        report.name = reportName;

        // Parse known report keys
        if (entries.contains("OnTop"))
            report.onTop = entries["OnTop"].value(0).trimmed() != "0";

        if (entries.contains("PreStatus")) {
            for (const QString& val : entries["PreStatus"]) {
                report.preStatuses << val.split(',', Qt::SkipEmptyParts);
            }
        }

        if (entries.contains("AddMerits") || entries.contains("AddMerits"))
            report.merits.add = entries.value("AddMerit", entries.value("AddMerits")).value(0).toInt();

        if (entries.contains("SubtractMerit") || entries.contains("SubtractMerits"))
            report.merits.subtract = entries.value("SubtractMerit", entries.value("SubtractMerits")).value(0).toInt();

        if (entries.contains("SetMerit") || entries.contains("SetMerits"))
            report.merits.set = entries.value("SetMerit", entries.value("SetMerits")).value(0).toInt();

        if (entries.contains("CenterRandom"))
            report.centerRandom = entries["CenterRandom"].value(0).trimmed() == "1";

        if (entries.contains("Title"))
            report.title = entries["Title"].value(0);

        if (entries.contains("Group"))
            report.group = entries["Group"].value(0);

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
                        report.messages.append(currentGroup);
                        currentGroup = MessageGroup();
                    }
                }
                else if (key == "Message") {
                    inMessageBlock = true;
                    currentGroup.messages.append(val.trimmed());
                }
                else if (key == "Text") {
                    report.statusTexts.append(val.trimmed());
                }
            }
        }
        if (!currentGroup.messages.isEmpty()) {
            report.messages.append(currentGroup);
        }

        if (entries.contains("Input"))
            report.inputQuestions.append(entries["Input"]);

        if (entries.contains("NoInputProcedure"))
            report.noInputProcedure = entries["NoInputProcedure"].value(0);

        if (entries.contains("Question"))
            report.advancedQuestions.append(entries["Question"]);

        if (entries.contains("StartAutoAssign")) {
            QString value = entries["StartAutoAssign"].value(0);
            if (value.startsWith("time,", Qt::CaseInsensitive)) {
                report.autoAssignMode = "time";
                report.autoAssignValue = value.mid(QString("time,").length()).trimmed();
            } else if (value.startsWith("interval,", Qt::CaseInsensitive)) {
                report.autoAssignMode = "interval";
                report.autoAssignValue = value.mid(QString("interval,").length()).trimmed();
            } else if (value.startsWith("ask", Qt::CaseInsensitive)) {
                report.autoAssignMode = "ask";
                int commaIndex = value.indexOf(',');
                if (commaIndex >= 0)
                    report.autoAssignValue = value.mid(commaIndex + 1).trimmed();
                else report.autoAssignValue = "";
            }
        }

        if (entries.contains("StopAutoAssign"))
            report.stopAutoAssign = true;

        if (entries.contains("Clothing"))
            report.clothingInstruction = entries["Clothing"].value(0);

        if (entries.contains("ClearCheck"))
            report.clearClothingCheck = entries["ClearCheck"].value(0).trimmed() == "1";

        if (entries.contains("MasterMail"))
            report.masterMailSubject = entries["MasterMail"].value(0);

        if (entries.contains("MasterAttach")) {
            for (const QString& file : entries["MasterAttach"])
                report.masterAttachments.append(file.trimmed());
        }

        if (entries.contains("SubMail")) {
            for (const QString& line : entries["SubMail"])
                report.subMailLines.append(line.trimmed());
        }

        if (entries.contains("Sound"))
            report.soundFiles = entries["Sound"];

        if (entries.contains("LocalSound"))
            report.localSoundFiles = entries["LocalSound"];

        if (entries.contains("WriteReport"))
            report.writeReportLines = entries["WriteReport"];

        if (entries.contains("ShowPicture"))
            report.showPictures = entries["ShowPicture"];

        if (entries.contains("ShowLocalPicture"))
            report.showLocalPictures = entries["ShowLocalPicture"];

        if (entries.contains("RemovePicture"))
            report.removePictures = entries["RemovePicture"];

        if (entries.contains("RemoveLocalPicture"))
            report.removeLocalPictures = entries["RemoveLocalPicture"];

        if (entries.contains("Launch"))
            report.launchCommand = entries["Launch"].value(0);

        if (entries.contains("MakeNewReport")) {
            QString val = entries["MakeNewReport"].value(0);
            report.makeNewReport = val == "1" || val == "2";
            report.makeNewReportSilent = val == "2";
        }

        if (entries.contains("PgmAction"))
            report.programAction = entries["PgmAction"].value(0);

        if (entries.contains("SetFlag"))
            report.setFlags = entries["SetFlag"];

        if (entries.contains("ClearFlag"))
            report.clearFlags = entries["ClearFlag"];

        if (entries.contains("Set$"))
            report.setStringVars = entries["Set$"];

        if (entries.contains("Set#"))
            report.setCounterVars = entries["Set#"];

        if (entries.contains("Set@"))
            report.setTimeVars = entries["Set@"];

        if (entries.contains("Counter+"))
            report.incrementCounters = entries["Counter+"];

        if (entries.contains("Counter-"))
            report.decrementCounters = entries["Counter-"];

        if (entries.contains("Random#"))
            report.randomCounters = entries["Random#"];

        if (entries.contains("Random$"))
            report.randomStrings = entries["Random$"];

        if (entries.contains("SetFlag"))
            report.setFlags = entries["SetFlag"];

        if (entries.contains("RemoveFlag"))
            report.removeFlags = entries["RemoveFlag"];

        if (entries.contains("SetFlagGroup"))
            report.setFlagGroups = entries["SetFlagGroup"];

        if (entries.contains("RemoveFlagGroup"))
            report.removeFlagGroups = entries["RemoveFlagGroup"];

        if (entries.contains("If"))
            report.ifFlags = entries["If"];

        if (entries.contains("NotIf"))
            report.notIfFlags = entries["NotIf"];

        if (entries.contains("DenyIf"))
            report.denyIfFlags = entries["DenyIf"];

        if (entries.contains("PermitIf"))
            report.permitIfFlags = entries["PermitIf"];

        if (entries.contains("Set!"))
            report.setTimeVars = entries["Set!"];
        if (entries.contains("Add!"))
            report.addTimeVars = entries["Add!"];
        if (entries.contains("Subtract!"))
            report.subtractTimeVars = entries["Subtract!"];
        if (entries.contains("Multiply!"))
            report.multiplyTimeVars = entries["Multiply!"];
        if (entries.contains("Divide!"))
            report.divideTimeVars = entries["Divide!"];
        if (entries.contains("Round!"))
            report.roundTimeVars = entries["Round!"];
        if (entries.contains("Drop!"))
            report.dropTimeVars = entries["Drop!"];
        if (entries.contains("InputDate!"))
            report.inputDateVars = entries["InputDate!"];
        if (entries.contains("InputDateDef!"))
            report.inputDateDefVars = entries["InputDateDef!"];
        if (entries.contains("InputTime!"))
            report.inputTimeVars = entries["InputTime!"];
        if (entries.contains("InputTimeDef!"))
            report.inputTimeDefVars = entries["InputTimeDef!"];
        if (entries.contains("InputInterval!"))
            report.inputIntervalVars = entries["InputInterval!"];
        if (entries.contains("Random!"))
            report.randomTimeVars = entries["Random!"];
        if (entries.contains("AddDays!"))
            report.addDaysVars = entries["AddDays!"];
        if (entries.contains("SubtractDays!"))
            report.subtractDaysVars = entries["SubtractDays!"];
        if (entries.contains("Days#"))
            report.extractToCounter += entries["Days#"];
        if (entries.contains("Hours#"))
            report.extractToCounter += entries["Hours#"];
        if (entries.contains("Minutes#"))
            report.extractToCounter += entries["Minutes#"];
        if (entries.contains("Seconds#"))
            report.extractToCounter += entries["Seconds#"];
        if (entries.contains("Days!"))
            report.convertFromCounter += entries["Days!"];
        if (entries.contains("Hours!"))
            report.convertFromCounter += entries["Hours!"];
        if (entries.contains("Minutes!"))
            report.convertFromCounter += entries["Minutes!"];
        if (entries.contains("Seconds!"))
            report.convertFromCounter += entries["Seconds!"];
        if (entries.contains("Set*"))
            report.listSets = entries["Set*"];
        if (entries.contains("SetSplit*"))
            report.listSetSplits = entries["SetSplit*"];
        if (entries.contains("Add*"))
            report.listAdds = entries["Add*"];
        if (entries.contains("AddNoDub*"))
            report.listAddNoDubs = entries["AddNoDub*"];
        if (entries.contains("AddSplit*"))
            report.listAddSplits = entries["AddSplit*"];
        if (entries.contains("Push*"))
            report.listPushes = entries["Push*"];
        if (entries.contains("Remove*"))
            report.listRemoves = entries["Remove*"];
        if (entries.contains("RemoveAll*"))
            report.listRemoveAlls = entries["RemoveAll*"];
        if (entries.contains("Pull*"))
            report.listPulls = entries["Pull*"];
        if (entries.contains("Intersect*"))
            report.listIntersects = entries["Intersect*"];
        if (entries.contains("Clear*"))
            report.listClears = entries["Clear*"];
        if (entries.contains("Drop*"))
            report.listDrops = entries["Drop*"];
        if (entries.contains("Sort*"))
            report.listSorts = entries["Sort*"];

        QStringList lines;
        for (auto keyIt = entries.begin(); keyIt != entries.end(); ++keyIt) {
            for (const QString &val : keyIt.value())
                lines.append(val);
        }

        int index = 0;
        while (index < lines.size()) {
            if (lines[index].startsWith("Case=", Qt::CaseInsensitive)) {
                CaseBlock block = parseCaseBlock(lines, index);
                report.cases.append(block);
            } else {
                ++index;
            }
        }

        parseTimeWindowControl(entries, report.timeWindow);
        parseTimeRestrictions(entries, report.notBeforeTimes, report.notAfterTimes, report.notBetweenTimes);

        scriptData.reports.insert(reportName, report);
    }
}

void ScriptParser::parseConfessionSections(const QMap<QString, QMap<QString, QStringList>>& sections) {
    for (auto it = sections.begin(); it != sections.end(); ++it) {
        const QString& sectionName = it.key();
        if (!sectionName.startsWith("confession-"))
            continue;

        QString confessionName = sectionName.mid(QString("confession-").length());
        const QMap<QString, QStringList>& entries = it.value();

        ConfessionDefinition confession;
        confession.name = confessionName;

        if (entries.contains("OnTop"))
            confession.onTop = entries["OnTop"].value(0).trimmed() == "1";

        if (entries.contains("Menu"))
            confession.showInMenu = entries["Menu"].value(0).trimmed() != "0";

        if (entries.contains("PreStatus")) {
            for (const QString& val : entries["PreStatus"])
                confession.preStatuses << val.split(',', Qt::SkipEmptyParts);
        }

        if (entries.contains("AddMerit") || entries.contains("AddMerits"))
            confession.merits.add = entries.value("AddMerit", entries.value("AddMerits")).value(0).toInt();

        if (entries.contains("SubtractMerit") || entries.contains("SubtractMerits"))
            confession.merits.subtract = entries.value("SubtractMerit", entries.value("SubtractMerits")).value(0).toInt();

        if (entries.contains("SetMerit") || entries.contains("SetMerits"))
            confession.merits.set = entries.value("SetMerit", entries.value("SetMerits")).value(0).toInt();

        if (entries.contains("CenterRandom"))
            confession.centerRandom = entries["CenterRandom"].value(0).trimmed() == "1";

        if (entries.contains("Title"))
            confession.title = entries["Title"].value(0);

        if (entries.contains("Group"))
            confession.group = entries["Group"].value(0);

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
                        confession.messages.append(currentGroup);
                        currentGroup = MessageGroup();
                    }
                }
                else if (key == "Message") {
                    inMessageBlock = true;
                    currentGroup.messages.append(val.trimmed());
                }
                else if (key == "Text") {
                    confession.statusTexts.append(val.trimmed());
                }
            }
        }
        if (!currentGroup.messages.isEmpty()) {
            confession.messages.append(currentGroup);
        }

        if (entries.contains("Input"))
            confession.inputQuestions.append(entries["Input"]);

        if (entries.contains("NoInputProcedure"))
            confession.noInputProcedure = entries["NoInputProcedure"].value(0);

        if (entries.contains("Question"))
            confession.advancedQuestions.append(entries["Question"]);

        if (entries.contains("MasterMail"))
            confession.masterMailSubject = entries["MasterMail"].value(0);

        if (entries.contains("MasterAttach")) {
            for (const QString& file : entries["MasterAttach"])
                confession.masterAttachments.append(file.trimmed());
        }

        if (entries.contains("SubMail")) {
            for (const QString& line : entries["SubMail"])
                confession.subMailLines.append(line.trimmed());
        }

        if (entries.contains("SetFlag"))
            confession.setFlags = entries["SetFlag"];

        if (entries.contains("ClearFlag"))
            confession.clearFlags = entries["ClearFlag"];

        if (entries.contains("Set$"))
            confession.setStringVars = entries["Set$"];

        if (entries.contains("Set#"))
            confession.setCounterVars = entries["Set#"];

        if (entries.contains("Set@"))
            confession.setTimeVars = entries["Set@"];

        if (entries.contains("Counter+"))
            confession.incrementCounters = entries["Counter+"];

        if (entries.contains("Counter-"))
            confession.decrementCounters = entries["Counter-"];

        if (entries.contains("Random#"))
            confession.randomCounters = entries["Random#"];

        if (entries.contains("Random$"))
            confession.randomStrings = entries["Random$"];

        if (entries.contains("Set!"))
            confession.setTimeVars = entries["Set!"];
        if (entries.contains("Add!"))
            confession.addTimeVars = entries["Add!"];
        if (entries.contains("Subtract!"))
            confession.subtractTimeVars = entries["Subtract!"];
        if (entries.contains("Multiply!"))
            confession.multiplyTimeVars = entries["Multiply!"];
        if (entries.contains("Divide!"))
            confession.divideTimeVars = entries["Divide!"];
        if (entries.contains("Round!"))
            confession.roundTimeVars = entries["Round!"];
        if (entries.contains("Drop!"))
            confession.dropTimeVars = entries["Drop!"];
        if (entries.contains("InputDate!"))
            confession.inputDateVars = entries["InputDate!"];
        if (entries.contains("InputDateDef!"))
            confession.inputDateDefVars = entries["InputDateDef!"];
        if (entries.contains("InputTime!"))
            confession.inputTimeVars = entries["InputTime!"];
        if (entries.contains("InputTimeDef!"))
            confession.inputTimeDefVars = entries["InputTimeDef!"];
        if (entries.contains("InputInterval!"))
            confession.inputIntervalVars = entries["InputInterval!"];
        if (entries.contains("Random!"))
            confession.randomTimeVars = entries["Random!"];
        if (entries.contains("AddDays!"))
            confession.addDaysVars = entries["AddDays!"];
        if (entries.contains("SubtractDays!"))
            confession.subtractDaysVars = entries["SubtractDays!"];
        if (entries.contains("Days#"))
            confession.extractToCounter += entries["Days#"];
        if (entries.contains("Hours#"))
            confession.extractToCounter += entries["Hours#"];
        if (entries.contains("Minutes#"))
            confession.extractToCounter += entries["Minutes#"];
        if (entries.contains("Seconds#"))
            confession.extractToCounter += entries["Seconds#"];
        if (entries.contains("Days!"))
            confession.convertFromCounter += entries["Days!"];
        if (entries.contains("Hours!"))
            confession.convertFromCounter += entries["Hours!"];
        if (entries.contains("Minutes!"))
            confession.convertFromCounter += entries["Minutes!"];
        if (entries.contains("Seconds!"))
            confession.convertFromCounter += entries["Seconds!"];
        if (entries.contains("If"))
            confession.ifConditions = entries["If"];
        if (entries.contains("NotIf"))
            confession.notIfConditions = entries["NotIf"];
        if (entries.contains("DenyIf"))
            confession.denyIfConditions = entries["DenyIf"];
        if (entries.contains("PermitIf"))
            confession.permitIfConditions = entries["PermitIf"];

        parseTimeWindowControl(entries, confession.timeWindow);
        parseTimeRestrictions(entries, confession.notBeforeTimes, confession.notAfterTimes, confession.notBetweenTimes);

        scriptData.confessions.insert(confessionName, confession);
    }
}

void ScriptParser::parsePermissionSections(const QMap<QString, QMap<QString, QStringList>>& sections) {
    for (auto it = sections.begin(); it != sections.end(); ++it) {
        const QString& sectionName = it.key();
        if (!sectionName.startsWith("permission-"))
            continue;

        QString permissionName = sectionName.mid(QString("permission-").length());
        const QMap<QString, QStringList>& entries = it.value();

        PermissionDefinition permission;
        permission.name = permissionName;

        if (entries.contains("Pct"))
            permission.pct = entries["Pct"].value(0).toInt();

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

        if (entries.contains("MinInterval"))
            permission.minInterval = parseTimeRange(entries["MinInterval"].value(0));

        if (entries.contains("Delay"))
            permission.delay = parseTimeRange(entries["Delay"].value(0));

        if (entries.contains("MaxWait"))
            permission.maxWait = parseTimeRange(entries["MaxWait"].value(0));

        if (entries.contains("Notify"))
            permission.notify = entries["Notify"].value(0).toInt();

        if (entries.contains("AddMerit") || entries.contains("AddMerits"))
            permission.merits.add = entries.value("AddMerit", entries.value("AddMerits")).value(0).toInt();

        if (entries.contains("SubtractMerit") || entries.contains("SubtractMerits"))
            permission.merits.add = entries.value("SubtractMerit", entries.value("SubtractMerits")).value(0).toInt();

        if (entries.contains("SetMerit") || entries.contains("SetMerits"))
            permission.merits.set = entries.value("SetMerit", entries.value("SetMerits")).value(0).toInt();

        if (entries.contains("CenterRandom"))
            permission.centerRandom = entries["CenterRandom"].value(0).trimmed() == "1";

        if (entries.contains("Title"))
            permission.title = entries["Title"].value(0);

        if (entries.contains("Group"))
            permission.group = entries["Group"].value(0);

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
                        permission.messages.append(currentGroup);
                        currentGroup = MessageGroup();
                    }
                }
                else if (key == "Message") {
                    inMessageBlock = true;
                    currentGroup.messages.append(val.trimmed());
                }
                else if (key == "Text") {
                    permission.statusTexts.append(val.trimmed());
                }
            }
        }
        if (!currentGroup.messages.isEmpty()) {
            permission.messages.append(currentGroup);
        }

        if (entries.contains("Input"))
            permission.inputQuestions.append(entries["Input"]);

        if (entries.contains("NoInputProcedure"))
            permission.noInputProcedure = entries["NoInputProcedure"].value(0);

        if (entries.contains("Question"))
            permission.advancedQuestions.append(entries["Question"]);

        // --- Time Control ---
        if (entries.contains("DenyBefore")) {
            QStringList parts = entries["DenyBefore"].value(0).split(',', Qt::SkipEmptyParts);
            permission.denyBeforeStart = parts.value(0);
            permission.denyBeforeEnd = parts.value(1, parts.value(0));
        }

        if (entries.contains("DenyAfter")) {
            QStringList parts = entries["DenyAfter"].value(0).split(',', Qt::SkipEmptyParts);
            permission.denyAfterStart = parts.value(0);
            permission.denyAfterEnd = parts.value(1, parts.value(0));
        }

        if (entries.contains("DenyBetween")) {
            QStringList parts = entries["DenyBetween"].value(0).split(',', Qt::SkipEmptyParts);
            if (parts.size() == 2)
                permission.denyBetweenRanges.append({ parts[0].trimmed(), parts[1].trimmed() });
        }

        // --- Flag Control ---
        if (entries.contains("DenyIf"))
            permission.denyIfFlags += entries["DenyIf"].value(0).split(',', Qt::SkipEmptyParts);

        if (entries.contains("PermitIf"))
            permission.permitIfFlags += entries["PermitIf"].value(0).split(',', Qt::SkipEmptyParts);

        // --- Merit Control ---
        auto parseRange = [](const QString& v, int& minVal, int& maxVal) {
            QStringList parts = v.split(',', Qt::SkipEmptyParts);
            minVal = parts.value(0).toInt();
            maxVal = parts.size() > 1 ? parts.value(1).toInt() : minVal;
        };

        if (entries.contains("DenyBelow"))
            parseRange(entries["DenyBelow"].value(0), permission.denyBelowMin, permission.denyBelowMax);

        if (entries.contains("DenyAbove"))
            parseRange(entries["DenyAbove"].value(0), permission.denyAboveMin, permission.denyAboveMax);

        // --- Percent Control ---
        if (entries.contains("Pct")) {
            QString value = entries["Pct"].value(0).trimmed();
            if (value.compare("Var", Qt::CaseInsensitive) == 0) {
                permission.pctIsVariable = true;
            } else {
                parseRange(value, permission.pctMin, permission.pctMax);
            }
        }

        if (entries.contains("HighMerits"))
            parseRange(entries["HighMerits"].value(0), permission.highMeritsMin, permission.highMeritsMax);

        if (entries.contains("HighPct"))
            parseRange(entries["HighPct"].value(0), permission.highPctMin, permission.highPctMax);

        if (entries.contains("LowMerits"))
            parseRange(entries["LowMerits"].value(0), permission.lowMeritsMin, permission.lowMeritsMax);

        if (entries.contains("LowPct"))
            parseRange(entries["LowPct"].value(0), permission.lowPctMin, permission.lowPctMax);

        // --- Control Procedure ---
        if (entries.contains("BeforeProcedure"))
            permission.beforeProcedure = entries["BeforeProcedure"].value(0);

        // --- Messages ---
        if (entries.contains("PermitMessage"))
            permission.permitMessage = entries["PermitMessage"].value(0);

        if (entries.contains("DenyMessage"))
            permission.denyMessage = entries["DenyMessage"].value(0);

        // --- Denial Actions ---
        if (entries.contains("DenyProcedure"))
            permission.denyProcedure = entries["DenyProcedure"].value(0);

        if (entries.contains("DenyFlag"))
            permission.denyFlags += entries["DenyFlag"].value(0).split(',', Qt::SkipEmptyParts);

        if (entries.contains("DenyLaunch"))
            permission.denyLaunch = entries["DenyLaunch"].value(0);

        if (entries.contains("DenyStatus"))
            permission.denyStatus = entries["DenyStatus"].value(0);

        if (entries.contains("MasterMail"))
            permission.masterMailSubject = entries["MasterMail"].value(0);

        if (entries.contains("MasterAttach")) {
            for (const QString& file : entries["MasterAttach"])
                permission.masterAttachments.append(file.trimmed());
        }

        if (entries.contains("SubMail")) {
            for (const QString& line : entries["SubMail"])
                permission.subMailLines.append(line.trimmed());
        }

        if (entries.contains("PoseCamera"))
            permission.poseCameraText = entries["PoseCamera"].value(0);

        if (entries.contains("PointCamera"))
            permission.pointCameraText = entries["PointCamera"].value(0);

        if (entries.contains("SetFlag"))
            permission.setFlags = entries["SetFlag"];

        if (entries.contains("ClearFlag"))
            permission.clearFlags = entries["ClearFlag"];

        if (entries.contains("Set$"))
            permission.setStringVars = entries["Set$"];

        if (entries.contains("Set#"))
            permission.setCounterVars = entries["Set#"];

        if (entries.contains("Set@"))
            permission.setTimeVars = entries["Set@"];

        if (entries.contains("Counter+"))
            permission.incrementCounters = entries["Counter+"];

        if (entries.contains("Counter-"))
            permission.decrementCounters = entries["Counter-"];

        if (entries.contains("Random#"))
            permission.randomCounters = entries["Random#"];

        if (entries.contains("Random$"))
            permission.randomStrings = entries["Random$"];

        if (entries.contains("SetFlag"))
            permission.setFlags = entries["SetFlag"];

        if (entries.contains("RemoveFlag"))
            permission.removeFlags = entries["RemoveFlag"];

        if (entries.contains("SetFlagGroup"))
            permission.setFlagGroups = entries["SetFlagGroup"];

        if (entries.contains("RemoveFlagGroup"))
            permission.removeFlagGroups = entries["RemoveFlagGroup"];

        if (entries.contains("If"))
            permission.ifFlags = entries["If"];

        if (entries.contains("NotIf"))
            permission.notIfFlags = entries["NotIf"];

        if (entries.contains("DenyIf"))
            permission.denyIfFlags = entries["DenyIf"];

        if (entries.contains("PermitIf"))
            permission.permitIfFlags = entries["PermitIf"];

        if (entries.contains("Set$"))
            permission.setStrings = entries["Set$"];

        if (entries.contains("Input$"))
            permission.inputStrings = entries["Input$"];

        if (entries.contains("InputLong$"))
            permission.inputLongStrings = entries["InputLong$"];

        if (entries.contains("Drop$"))
            permission.dropStrings = entries["Drop$"];

        if (entries.contains("Set#"))
            permission.setCounters = entries["Set#"];

        if (entries.contains("Add#"))
            permission.addCounters = entries["Add#"];

        if (entries.contains("Subtract#"))
            permission.subtractCounters = entries["Subtract#"];

        if (entries.contains("Multiply#"))
            permission.multiplyCounters = entries["Multiply#"];

        if (entries.contains("Divide#"))
            permission.divideCounters = entries["Divide#"];

        if (entries.contains("Drop#"))
            permission.dropCounters = entries["Drop#"];

        if (entries.contains("Input#"))
            permission.inputCounters = entries["Input#"];

        if (entries.contains("InputNeg#"))
            permission.inputNegCounters = entries["InputNeg#"];

        if (entries.contains("Random#"))
            permission.randomCounters = entries["Random#"];

        if (entries.contains("Set!"))
            permission.setTimeVars = entries["Set!"];
        if (entries.contains("Add!"))
            permission.addTimeVars = entries["Add!"];
        if (entries.contains("Subtract!"))
            permission.subtractTimeVars = entries["Subtract!"];
        if (entries.contains("Multiply!"))
            permission.multiplyTimeVars = entries["Multiply!"];
        if (entries.contains("Divide!"))
            permission.divideTimeVars = entries["Divide!"];
        if (entries.contains("Round!"))
            permission.roundTimeVars = entries["Round!"];
        if (entries.contains("Drop!"))
            permission.dropTimeVars = entries["Drop!"];
        if (entries.contains("InputDate!"))
            permission.inputDateVars = entries["InputDate!"];
        if (entries.contains("InputDateDef!"))
            permission.inputDateDefVars = entries["InputDateDef!"];
        if (entries.contains("InputTime!"))
            permission.inputTimeVars = entries["InputTime!"];
        if (entries.contains("InputTimeDef!"))
            permission.inputTimeDefVars = entries["InputTimeDef!"];
        if (entries.contains("InputInterval!"))
            permission.inputIntervalVars = entries["InputInterval!"];
        if (entries.contains("Random!"))
            permission.randomTimeVars = entries["Random!"];
        if (entries.contains("AddDays!"))
            permission.addDaysVars = entries["AddDays!"];
        if (entries.contains("SubtractDays!"))
            permission.subtractDaysVars = entries["SubtractDays!"];
        if (entries.contains("Days#"))
            permission.extractToCounter += entries["Days#"];
        if (entries.contains("Hours#"))
            permission.extractToCounter += entries["Hours#"];
        if (entries.contains("Minutes#"))
            permission.extractToCounter += entries["Minutes#"];
        if (entries.contains("Seconds#"))
            permission.extractToCounter += entries["Seconds#"];
        if (entries.contains("Days!"))
            permission.convertFromCounter += entries["Days!"];
        if (entries.contains("Hours!"))
            permission.convertFromCounter += entries["Hours!"];
        if (entries.contains("Minutes!"))
            permission.convertFromCounter += entries["Minutes!"];
        if (entries.contains("Seconds!"))
            permission.convertFromCounter += entries["Seconds!"];
        if (entries.contains("If"))
            permission.ifConditions = entries["If"];
        if (entries.contains("NotIf"))
            permission.notIfConditions = entries["NotIf"];
        if (entries.contains("DenyIf"))
            permission.denyIfConditions = entries["DenyIf"];
        if (entries.contains("PermitIf"))
            permission.permitIfConditions = entries["PermitIf"];

        parseTimeWindowControl(entries, permission.timeWindow);
        parseTimeRestrictions(entries, permission.notBeforeTimes, permission.notAfterTimes, permission.notBetweenTimes);

        scriptData.permissions.insert(permissionName, permission);
    }
}

void ScriptParser::parsePunishmentSections(const QMap<QString, QMap<QString, QStringList>>& sections) {
    for (auto it = sections.begin(); it != sections.end(); ++it) {
        const QString& sectionName = it.key();
        if (!sectionName.startsWith("punishment-"))
            continue;

        QString punishmentName = sectionName.mid(QString("punishment-").length());
        const QMap<QString, QStringList>& entries = it.value();

        PunishmentDefinition p;
        p.name = punishmentName;

        if (entries.contains("ValueUnit"))
            p.valueUnit = entries["ValueUnit"].value(0).trimmed().toLower();

        if (entries.contains("value"))
            p.value = entries["value"].value(0).toDouble();

        if (entries.contains("min"))
            p.min = entries["min"].value(0).toInt();

        if (entries.contains("max"))
            p.max = entries["max"].value(0).toInt();

        if (entries.contains("MinSeverity"))
            p.minSeverity = entries["MinSeverity"].value(0).toInt();

        if (entries.contains("MaxSeverity"))
            p.maxSeverity = entries["MaxSeverity"].value(0).toInt();

        if (entries.contains("Weight")) {
            QStringList parts = entries["Weight"].value(0).split(',', Qt::SkipEmptyParts);
            if (parts.size() == 2) {
                p.weightMin = parts[0].toInt();
                p.weightMax = parts[1].toInt();
            } else {
                int w = parts[0].toInt();
                p.weightMin = p.weightMax = (w > 0 ? w : 1);
            }
        }

        if (entries.contains("Group")) {
            for (const QString& g : entries["Group"])
                p.groups.append(g.split(',', Qt::SkipEmptyParts));
        }

        if (entries.contains("GroupOnly"))
            p.groupOnly = entries["GroupOnly"].value(0).trimmed() == "1";

        if (entries.contains("LongRunning"))
            p.longRunning = entries["LongRunning"].value(0).trimmed() == "1";

        if (entries.contains("MustStart"))
            p.mustStart = entries["MustStart"].value(0).trimmed() == "1";

        if (entries.contains("Accumulative"))
            p.accumulative = entries["Accumulative"].value(0).trimmed() == "1";

        if (entries.contains("Respite"))
            p.respite = entries["Respite"].value(0).trimmed();
        if (entries.contains("Respit"))
            p.respite = entries["Respit"].value(0).trimmed();

        if (entries.contains("Estimate"))
            p.estimate = entries["Estimate"].value(0).trimmed();

        if (entries.contains("Forbid")) {
            for (const QString& f : entries["Forbid"])
                p.forbids.append(f.split(',', Qt::SkipEmptyParts));
        }

        if (entries.contains("AddMerit") || entries.contains("AddMerits"))
            p.merits.add = entries.value("AddMerit", entries.value("AddMerits")).value(0).toInt();

        if (entries.contains("SubtractMerit") || entries.contains("SubtractMerits"))
            p.merits.add = entries.value("SubtractMerit", entries.value("SubtractMerits")).value(0).toInt();

        if (entries.contains("SetMerit") || entries.contains("SetMerits"))
            p.merits.set = entries.value("SetMerit", entries.value("SetMerits")).value(0).toInt();

        if (entries.contains("CenterRandom"))
            p.centerRandom = entries["CenterRandom"].value(0).trimmed() == "1";

        if (entries.contains("Title"))
            p.title = entries["Title"].value(0);

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
                        p.messages.append(currentGroup);
                        currentGroup = MessageGroup();
                    }
                }
                else if (key == "Message") {
                    inMessageBlock = true;
                    currentGroup.messages.append(val.trimmed());
                }
                else if (key == "Text") {
                    p.statusTexts.append(val.trimmed());
                }
            }
        }
        if (!currentGroup.messages.isEmpty()) {
            p.messages.append(currentGroup);
        }

        if (entries.contains("Input"))
            p.inputQuestions.append(entries["Input"]);

        if (entries.contains("NoInputProcedure"))
            p.noInputProcedure = entries["NoInputProcedure"].value(0);

        if (entries.contains("Question"))
            p.advancedQuestions.append(entries["Question"]);

        if (entries.contains("Resource")) {
            for (const QString& value : entries["Resource"])
                p.resources.append(value.trimmed());
        }

        if (entries.contains("Estimate"))
            p.estimate = entries["Estimate"].value(0);

        if (entries.contains("AutoAssign")) {
            QStringList parts = entries["AutoAssign"].value(0).split(',', Qt::SkipEmptyParts);
            p.autoAssignMin = parts.value(0).toInt();
            p.autoAssignMax = parts.value(1, parts.value(0)).toInt();
        }

        if (entries.contains("Clothing"))
            p.clothingInstruction = entries["Clothing"].value(0);

        if (entries.contains("ClearCheck"))
            p.clearClothingCheck = entries["ClearCheck"].value(0).trimmed() == "1";

        if (entries.contains("Type") && entries["Type"].value(0).trimmed().compare("Lines", Qt::CaseInsensitive) == 0)
            p.isLineWriting = true;

        if (entries.contains("Line"))
            p.lines = entries["Line"];

        if (entries.contains("Select"))
            p.lineSelectMode = entries["Select"].value(0).trimmed();

        if (entries.contains("PageSize")) {
            QStringList parts = entries["PageSize"].value(0).split(',', Qt::SkipEmptyParts);
            p.pageSizeMin = parts.value(0);
            p.pageSizeMax = parts.value(1, parts.value(0));
        }

        if (entries.contains("Type") && entries["Type"].value(0).trimmed().compare("Detention", Qt::CaseInsensitive) == 0)
            p.isDetention = true;

        if (entries.contains("Text")) {
            for (const QString& line : entries["Text"])
                p.detentionText.append(line.trimmed());
        }

        if (entries.contains("PoseCamera"))
            p.poseCameraText = entries["PoseCamera"].value(0);

        if (entries.contains("PointCamera"))
            p.pointCameraText = entries["PointCamera"].value(0);

        if (entries.contains("SetFlag"))
            p.setFlags = entries["SetFlag"];

        if (entries.contains("ClearFlag"))
            p.clearFlags = entries["ClearFlag"];

        if (entries.contains("Set$"))
            p.setStringVars = entries["Set$"];

        if (entries.contains("Set#"))
            p.setCounterVars = entries["Set#"];

        if (entries.contains("Set@"))
            p.setTimeVars = entries["Set@"];

        if (entries.contains("Counter+"))
            p.incrementCounters = entries["Counter+"];

        if (entries.contains("Counter-"))
            p.decrementCounters = entries["Counter-"];

        if (entries.contains("Random#"))
            p.randomCounters = entries["Random#"];

        if (entries.contains("Random$"))
            p.randomStrings = entries["Random$"];

        if (entries.contains("SetFlag"))
            p.setFlags = entries["SetFlag"];

        if (entries.contains("RemoveFlag"))
            p.removeFlags = entries["RemoveFlag"];

        if (entries.contains("SetFlagGroup"))
            p.setFlagGroups = entries["SetFlagGroup"];

        if (entries.contains("RemoveFlagGroup"))
            p.removeFlagGroups = entries["RemoveFlagGroup"];

        if (entries.contains("If"))
            p.ifFlags = entries["If"];

        if (entries.contains("NotIf"))
            p.notIfFlags = entries["NotIf"];

        if (entries.contains("DenyIf"))
            p.denyIfFlags = entries["DenyIf"];

        if (entries.contains("PermitIf"))
            p.permitIfFlags = entries["PermitIf"];

        if (entries.contains("Set#"))
            p.setCounters = entries["Set#"];

        if (entries.contains("Add#"))
            p.addCounters = entries["Add#"];

        if (entries.contains("Subtract#"))
            p.subtractCounters = entries["Subtract#"];

        if (entries.contains("Multiply#"))
            p.multiplyCounters = entries["Multiply#"];

        if (entries.contains("Divide#"))
            p.divideCounters = entries["Divide#"];

        if (entries.contains("Drop#"))
            p.dropCounters = entries["Drop#"];

        if (entries.contains("Input#"))
            p.inputCounters = entries["Input#"];

        if (entries.contains("InputNeg#"))
            p.inputNegCounters = entries["InputNeg#"];

        if (entries.contains("Random#"))
            p.randomCounters = entries["Random#"];

        if (entries.contains("Set!"))
            p.setTimeVars = entries["Set!"];
        if (entries.contains("Add!"))
            p.addTimeVars = entries["Add!"];
        if (entries.contains("Subtract!"))
            p.subtractTimeVars = entries["Subtract!"];
        if (entries.contains("Multiply!"))
            p.multiplyTimeVars = entries["Multiply!"];
        if (entries.contains("Divide!"))
            p.divideTimeVars = entries["Divide!"];
        if (entries.contains("Round!"))
            p.roundTimeVars = entries["Round!"];
        if (entries.contains("Drop!"))
            p.dropTimeVars = entries["Drop!"];
        if (entries.contains("InputDate!"))
            p.inputDateVars = entries["InputDate!"];
        if (entries.contains("InputDateDef!"))
            p.inputDateDefVars = entries["InputDateDef!"];
        if (entries.contains("InputTime!"))
            p.inputTimeVars = entries["InputTime!"];
        if (entries.contains("InputTimeDef!"))
            p.inputTimeDefVars = entries["InputTimeDef!"];
        if (entries.contains("InputInterval!"))
            p.inputIntervalVars = entries["InputInterval!"];
        if (entries.contains("Random!"))
            p.randomTimeVars = entries["Random!"];
        if (entries.contains("AddDays!"))
            p.addDaysVars = entries["AddDays!"];
        if (entries.contains("SubtractDays!"))
            p.subtractDaysVars = entries["SubtractDays!"];
        if (entries.contains("Days#"))
            p.extractToCounter += entries["Days#"];
        if (entries.contains("Hours#"))
            p.extractToCounter += entries["Hours#"];
        if (entries.contains("Minutes#"))
            p.extractToCounter += entries["Minutes#"];
        if (entries.contains("Seconds#"))
            p.extractToCounter += entries["Seconds#"];
        if (entries.contains("Days!"))
            p.convertFromCounter += entries["Days!"];
        if (entries.contains("Hours!"))
            p.convertFromCounter += entries["Hours!"];
        if (entries.contains("Minutes!"))
            p.convertFromCounter += entries["Minutes!"];
        if (entries.contains("Seconds!"))
            p.convertFromCounter += entries["Seconds!"];
        if (entries.contains("If"))
            p.ifConditions = entries["If"];
        if (entries.contains("NotIf"))
            p.notIfConditions = entries["NotIf"];
        if (entries.contains("DenyIf"))
            p.denyIfConditions = entries["DenyIf"];
        if (entries.contains("PermitIf"))
            p.permitIfConditions = entries["PermitIf"];

        parseDurationControl(entries, p.duration);

        scriptData.punishments.insert(punishmentName, p);
    }
}

void ScriptParser::parseJobSections(const QMap<QString, QMap<QString, QStringList>>& sections) {
    for (auto it = sections.begin(); it != sections.end(); ++it) {
        const QString& sectionName = it.key();
        if (!sectionName.startsWith("job-"))
            continue;

        QString jobName = sectionName.mid(QString("job-").length());
        const QMap<QString, QStringList>& entries = it.value();

        JobDefinition job;
        job.name = jobName;

        if (entries.contains("Text"))
            job.text = entries["Text"].value(0);

        auto parseIntRange = [](const QString& s, int& min, int& max) {
            QStringList parts = s.split(',', Qt::SkipEmptyParts);
            if (parts.size() == 2) {
                min = parts[0].toInt();
                max = parts[1].toInt();
            } else {
                min = max = parts[0].toInt();
            }
        };

        if (entries.contains("Interval"))
            parseIntRange(entries["Interval"].value(0), job.intervalMin, job.intervalMax);

        if (entries.contains("FirstInterval"))
            parseIntRange(entries["FirstInterval"].value(0), job.firstIntervalMin, job.firstIntervalMax);

        if (entries.contains("Run")) {
            for (const QString& val : entries["Run"])
                job.runDays.append(val.split(',', Qt::SkipEmptyParts));
        }

        if (entries.contains("NoRun")) {
            for (const QString& val : entries["NoRun"])
                job.noRunDays.append(val.split(',', Qt::SkipEmptyParts));
        }

        if (entries.contains("EndTime")) {
            QStringList parts = entries["EndTime"].value(0).split(',', Qt::SkipEmptyParts);
            if (parts.size() == 2) {
                job.endTimeMin = parts[0].trimmed();
                job.endTimeMax = parts[1].trimmed();
            } else {
                job.endTimeMin = job.endTimeMax = entries["EndTime"].value(0).trimmed();
            }
        }

        if (entries.contains("Respite"))
            job.respite = entries["Respite"].value(0).trimmed();
        if (entries.contains("Respit"))
            job.respite = entries["Respit"].value(0).trimmed();

        if (entries.contains("Estimate"))
            job.estimate = entries["Estimate"].value(0).trimmed();

        if (entries.contains("RemindInterval")) {
            QStringList parts = entries["RemindInterval"].value(0).split(',', Qt::SkipEmptyParts);
            if (parts.size() == 2) {
                job.remindIntervalMin = parts[0].trimmed();
                job.remindIntervalMax = parts[1].trimmed();
            } else {
                job.remindIntervalMin = job.remindIntervalMax = entries["RemindInterval"].value(0).trimmed();
            }
        }

        if (entries.contains("RemindPenalty"))
            parseIntRange(entries["RemindPenalty"].value(0), job.remindPenaltyMin, job.remindPenaltyMax);

        if (entries.contains("RemindPenaltyGroup"))
            job.remindPenaltyGroup = entries["RemindPenaltyGroup"].value(0);

        if (entries.contains("LateMerits"))
            parseIntRange(entries["LateMerits"].value(0), job.lateMeritsMin, job.lateMeritsMax);

        if (entries.contains("ExpireAfter")) {
            QStringList parts = entries["ExpireAfter"].value(0).split(',', Qt::SkipEmptyParts);
            if (parts.size() == 2) {
                job.expireAfterMin = parts[0].trimmed();
                job.expireAfterMax = parts[1].trimmed();
            } else {
                job.expireAfterMin = job.expireAfterMax = entries["ExpireAfter"].value(0).trimmed();
            }
        }

        if (entries.contains("ExpirePenalty"))
            parseIntRange(entries["ExpirePenalty"].value(0), job.expirePenaltyMin, job.expirePenaltyMax);

        if (entries.contains("ExpirePenaltyGroup"))
            job.expirePenaltyGroup = entries["ExpirePenaltyGroup"].value(0);

        if (entries.contains("ExpireProcedure"))
            job.expireProcedure = entries["ExpireProcedure"].value(0);

        if (entries.contains("OneTime"))
            job.oneTime = entries["OneTime"].value(0).trimmed() == "1";

        if (entries.contains("Announce"))
            job.announce = entries["Announce"].value(0).trimmed() != "0";

        if (entries.contains("AddMerit") || entries.contains("AddMerits"))
            job.merits.add = entries.value("AddMerit", entries.value("AddMerits")).value(0).toInt();

        if (entries.contains("SubtractMerit") || entries.contains("SubtractMerits"))
            job.merits.subtract = entries.value("SubtractMerit", entries.value("SubtractMerits")).value(0).toInt();

        if (entries.contains("SetMerit") || entries.contains("SetMerits"))
            job.merits.set = entries.value("SetMerit", entries.value("SetMerits")).value(0).toInt();

        if (entries.contains("CenterRandom"))
            job.centerRandom = entries["CenterRandom"].value(0).trimmed() == "1";

        if (entries.contains("Title"))
            job.title = entries["Title"].value(0);

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
                        job.messages.append(currentGroup);
                        currentGroup = MessageGroup();
                    }
                }
                else if (key == "Message") {
                    inMessageBlock = true;
                    currentGroup.messages.append(val.trimmed());
                }
                else if (key == "Text") {
                    job.statusTexts.append(val.trimmed());
                }
            }
        }
        if (!currentGroup.messages.isEmpty()) {
            job.messages.append(currentGroup);
        }

        if (entries.contains("Resource")) {
            for (const QString& value : entries["Resource"])
                job.resources.append(value.trimmed());
        }

        if (entries.contains("Estimate"))
            job.estimate = entries["Estimates"].value(0);

        if (entries.contains("AutoAssign")) {
            QStringList parts = entries["AutoAssign"].value(0).split(',', Qt::SkipEmptyParts);
            job.autoAssignMin = parts.value(0).toInt();
            job.autoAssignMax = parts.value(1, parts.value(0)).toInt();
        }

        if (entries.contains("Clothing"))
            job.clothingInstruction = entries["Clothing"].value(0);

        if (entries.contains("ClearCheck"))
            job.clearClothingCheck = entries["ClearCheck"].value(0).trimmed() == "1";

        if (entries.contains("Type") && entries["Type"].value(0).trimmed().compare("Lines", Qt::CaseInsensitive) == 0)
            job.isLineWriting = true;

        if (entries.contains("Line"))
            job.lines = entries["Line"];

        if (entries.contains("Select"))
            job.lineSelectMode = entries["Select"].value(0).trimmed();

        if (entries.contains("Linenumber"))
            job.lineCount = entries["Linenumber"].value(0).toInt();

        if (entries.contains("PageSize")) {
            QStringList parts = entries["PageSize"].value(0).split(',', Qt::SkipEmptyParts);
            job.pageSizeMin = parts.value(0);
            job.pageSizeMax = parts.value(1, parts.value(0));
        }

        if (entries.contains("PoseCamera"))
            job.poseCameraText = entries["PoseCamera"].value(0);

        if (!entries["PointCamera"].isEmpty())
            job.pointCameraText = entries["PointCamera"].first();

        if (entries.contains("SetFlag"))
            job.setFlags = entries["SetFlag"];

        if (entries.contains("ClearFlag"))
            job.clearFlags = entries["ClearFlag"];

        if (entries.contains("Set$"))
            job.setStringVars = entries["Set$"];

        if (entries.contains("Set#"))
            job.setCounterVars = entries["Set#"];

        if (entries.contains("Set@"))
            job.setTimeVars = entries["Set@"];

        if (entries.contains("Counter+"))
            job.incrementCounters = entries["Counter+"];

        if (entries.contains("Counter-"))
            job.decrementCounters = entries["Counter-"];

        if (entries.contains("Random#"))
            job.randomCounters = entries["Random#"];

        if (entries.contains("Random$"))
            job.randomStrings = entries["Random$"];

        if (entries.contains("SetFlag"))
            job.setFlags = entries["SetFlag"];

        if (entries.contains("RemoveFlag"))
            job.removeFlags = entries["RemoveFlag"];

        if (entries.contains("SetFlagGroup"))
            job.setFlagGroups = entries["SetFlagGroup"];

        if (entries.contains("RemoveFlagGroup"))
            job.removeFlagGroups = entries["RemoveFlagGroup"];

        if (entries.contains("If"))
            job.ifFlags = entries["If"];

        if (entries.contains("NotIf"))
            job.notIfFlags = entries["NotIf"];

        if (entries.contains("DenyIf"))
            job.denyIfFlags = entries["DenyIf"];

        if (entries.contains("PermitIf"))
            job.permitIfFlags = entries["PermitIf"];

        if (entries.contains("Set#"))
            job.setCounters = entries["Set#"];

        if (entries.contains("Add#"))
            job.addCounters = entries["Add#"];

        if (entries.contains("Subtract#"))
            job.subtractCounters = entries["Subtract#"];

        if (entries.contains("Multiply#"))
            job.multiplyCounters = entries["Multiply#"];

        if (entries.contains("Divide#"))
            job.divideCounters = entries["Divide#"];

        if (entries.contains("Drop#"))
            job.dropCounters = entries["Drop#"];

        if (entries.contains("Input#"))
            job.inputCounters = entries["Input#"];

        if (entries.contains("InputNeg#"))
            job.inputNegCounters = entries["InputNeg#"];

        if (entries.contains("Random#"))
            job.randomCounters = entries["Random#"];

        if (entries.contains("Set!"))
            job.setTimeVars = entries["Set!"];
        if (entries.contains("Add!"))
            job.addTimeVars = entries["Add!"];
        if (entries.contains("Subtract!"))
            job.subtractTimeVars = entries["Subtract!"];
        if (entries.contains("Multiply!"))
            job.multiplyTimeVars = entries["Multiply!"];
        if (entries.contains("Divide!"))
            job.divideTimeVars = entries["Divide!"];
        if (entries.contains("Round!"))
            job.roundTimeVars = entries["Round!"];
        if (entries.contains("Drop!"))
            job.dropTimeVars = entries["Drop!"];
        if (entries.contains("InputDate!"))
            job.inputDateVars = entries["InputDate!"];
        if (entries.contains("InputDateDef!"))
            job.inputDateDefVars = entries["InputDateDef!"];
        if (entries.contains("InputTime!"))
            job.inputTimeVars = entries["InputTime!"];
        if (entries.contains("InputTimeDef!"))
            job.inputTimeDefVars = entries["InputTimeDef!"];
        if (entries.contains("InputInterval!"))
            job.inputIntervalVars = entries["InputInterval!"];
        if (entries.contains("Random!"))
            job.randomTimeVars = entries["Random!"];
        if (entries.contains("AddDays!"))
            job.addDaysVars = entries["AddDays!"];
        if (entries.contains("SubtractDays!"))
            job.subtractDaysVars = entries["SubtractDays!"];
        if (entries.contains("Days#"))
            job.extractToCounter += entries["Days#"];
        if (entries.contains("Hours#"))
            job.extractToCounter += entries["Hours#"];
        if (entries.contains("Minutes#"))
            job.extractToCounter += entries["Minutes#"];
        if (entries.contains("Seconds#"))
            job.extractToCounter += entries["Seconds#"];
        if (entries.contains("Days!"))
            job.convertFromCounter += entries["Days!"];
        if (entries.contains("Hours!"))
            job.convertFromCounter += entries["Hours!"];
        if (entries.contains("Minutes!"))
            job.convertFromCounter += entries["Minutes!"];
        if (entries.contains("Seconds!"))
            job.convertFromCounter += entries["Seconds!"];
        if (entries.contains("If"))
            job.ifConditions = entries["If"];
        if (entries.contains("NotIf"))
            job.notIfConditions = entries["NotIf"];
        if (entries.contains("DenyIf"))
            job.denyIfConditions = entries["DenyIf"];
        if (entries.contains("PermitIf"))
            job.permitIfConditions = entries["PermitIf"];
        if (entries.contains("Set*"))
            job.listSets = entries["Set*"];
        if (entries.contains("SetSplit*"))
            job.listSetSplits = entries["SetSplit*"];
        if (entries.contains("Add*"))
            job.listAdds = entries["Add*"];
        if (entries.contains("AddNoDub*"))
            job.listAddNoDubs = entries["AddNoDub*"];
        if (entries.contains("AddSplit*"))
            job.listAddSplits = entries["AddSplit*"];
        if (entries.contains("Push*"))
            job.listPushes = entries["Push*"];
        if (entries.contains("Remove*"))
            job.listRemoves = entries["Remove*"];
        if (entries.contains("RemoveAll*"))
            job.listRemoveAlls = entries["RemoveAll*"];
        if (entries.contains("Pull*"))
            job.listPulls = entries["Pull*"];
        if (entries.contains("Intersect*"))
            job.listIntersects = entries["Intersect*"];
        if (entries.contains("Clear*"))
            job.listClears = entries["Clear*"];
        if (entries.contains("Drop*"))
            job.listDrops = entries["Drop*"];
        if (entries.contains("Sort*"))
            job.listSorts = entries["Sort*"];


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

void ScriptParser::parseInstructionSections(const QMap<QString, QMap<QString, QStringList>>& sections) {
    for (auto it = sections.begin(); it != sections.end(); ++it) {
        QString sectionName = it.key();

        bool isClothing = false;
        if (sectionName.startsWith("instruction-")) {
            sectionName = sectionName.mid(QString("instruction-").length());
        } else if (sectionName.startsWith("clothing-")) {
            isClothing = true;
            sectionName = sectionName.mid(QString("clothing-").length());
        } else {
            continue;
        }

        const QMap<QString, QStringList>& entries = it.value();
        InstructionDefinition instr;
        instr.name = sectionName;
        instr.isClothing = isClothing;

        InstructionSet currentSet;
        InstructionChoice currentChoice;
        bool inChoice = false;
        bool inSet = false;
        bool insideSelectBlock = false;

        for (auto keyIt = entries.begin(); keyIt != entries.end(); ++keyIt) {
            QString key = keyIt.key();
            const QStringList& values = keyIt.value();

            for (const QString& rawValue : values) {
                QString value = rawValue.trimmed();

                if (key == "Askable") {
                    instr.askable = (value != "0");
                } else if (key == "Change") {
                    if (value.compare("program", Qt::CaseInsensitive) == 0)
                        instr.changeMode = InstructionChangeMode::Program;
                    else if (value.compare("always", Qt::CaseInsensitive) == 0)
                        instr.changeMode = InstructionChangeMode::Always;
                    else
                        instr.changeMode = InstructionChangeMode::Daily;
                } else if (key == "Select") {
                    insideSelectBlock = true;
                    if (value.compare("first", Qt::CaseInsensitive) == 0)
                        instr.selectMode = InstructionSelectMode::First;
                    else if (value.compare("random", Qt::CaseInsensitive) == 0)
                        instr.selectMode = InstructionSelectMode::Random;
                    else
                        instr.selectMode = InstructionSelectMode::All;
                } else if (key == "None") {
                    instr.noneText = value;
                } else if (key == "Set") {
                    if (inChoice) {
                        currentSet.choices.append(currentChoice);
                        currentChoice = InstructionChoice();
                        inChoice = false;
                    }
                    inSet = true;
                    currentSet.includedSets.append(value);
                } else if (key == "Weight") {
                    bool ok = false;
                    int w = value.toInt(&ok);
                    if (ok) {
                        if (inChoice && !currentChoice.options.isEmpty()) {
                            currentChoice.options.last().weight = w;
                        } else if (inSet) {
                            currentSet.weight = w;
                        }
                    }
                } else if (key == "Choice") {
                    if (inChoice) {
                        currentSet.choices.append(currentChoice);
                        currentChoice = InstructionChoice();
                    }
                    inChoice = true;
                    currentChoice.name = value;
                } else if (key == "Option") {
                    InstructionOption option;
                    option.text = value;

                    if (value == "*") {
                        option.skip = true;
                    } else if (value.startsWith('%')) {
                        option.hidden = true;
                        option.text = value.mid(1).trimmed();
                    }

                    currentChoice.options.append(option);
                } else if (key == "OptionSet") {
                    if (inChoice && !currentChoice.options.isEmpty()) {
                        currentChoice.options.last().optionSet = value;
                    }
                }
            }
        }

        if (inChoice) {
            currentSet.choices.append(currentChoice);
            inChoice = false;
        }
        instr.sets.append(currentSet);

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
                        instr.messages.append(currentGroup);
                        currentGroup = MessageGroup();
                    }
                }
                else if (key == "Message") {
                    inMessageBlock = true;
                    currentGroup.messages.append(val.trimmed());
                }
                else if (key == "Text") {
                    instr.statusTexts.append(val.trimmed());
                }
            }
        }
        if (!currentGroup.messages.isEmpty()) {
            instr.messages.append(currentGroup);
        }

        if (entries.contains("Clothing"))
            instr.clothingInstruction = entries["Clothing"].value(0);

        if (entries.contains("ClearCheck"))
            instr.clearClothingCheck = entries["ClearCheck"].value(0).trimmed() == "1";

        QStringList lines;
        for (auto keyIt = entries.begin(); keyIt != entries.end(); ++keyIt) {
            for (const QString &val : keyIt.value())
                lines.append(val);
        }

        int index = 0;
        while (index < lines.size()) {
            if (lines[index].startsWith("Case=", Qt::CaseInsensitive)) {
                CaseBlock block = parseCaseBlock(lines, index);
                instr.cases.append(block);
            } else {
                ++index;
            }
        }

        scriptData.instructions.insert(instr.name, instr);
    }
}

void ScriptParser::parseInstructionSets(const QMap<QString, QMap<QString, QStringList>>& sections) {
    for (auto it = sections.begin(); it != sections.end(); ++it) {
        QString sectionName = it.key();
        if (!sectionName.startsWith("set-"))
            continue;

        QString setName = sectionName.mid(QString("set-").length());
        const QMap<QString, QStringList>& entries = it.value();

        InstructionSet set;
        set.name = setName;

        InstructionChoice currentChoice;
        bool inChoice = false;

        for (auto keyIt = entries.begin(); keyIt != entries.end(); ++keyIt) {
            QString key = keyIt.key();
            const QStringList& values = keyIt.value();

            for (const QString& rawValue : values) {
                QString value = rawValue.trimmed();

                if (key == "Option") {
                    InstructionOption option;
                    option.text = value;
                    if (value == "*") option.skip = true;
                    else if (value.startsWith('%')) {
                        option.hidden = true;
                        option.text = value.mid(1).trimmed();
                    }
                    currentChoice.options.append(option);
                } else if (key == "OptionSet") {
                    if (!currentChoice.options.isEmpty()) {
                        currentChoice.options.last().optionSet = value;
                    }
                } else if (key == "Weight") {
                    bool ok = false;
                    int w = value.toInt(&ok);
                    if (ok) {
                        if (inChoice && !currentChoice.options.isEmpty()) {
                            currentChoice.options.last().weight = w;
                        } else {
                            set.weight = w;
                        }
                    }
                } else if (key == "Choice") {
                    if (!currentChoice.options.isEmpty()) {
                        set.choices.append(currentChoice);
                        currentChoice = InstructionChoice();
                    }
                    inChoice = true;

                    // legacy format
                    if (value.contains(',')) {
                        QStringList parts = value.split(',', Qt::SkipEmptyParts);
                        for (int i = 0; i < parts.size(); ++i) {
                            QString opt = parts[i].trimmed();
                            InstructionOption o;
                            bool isWeight = false;
                            int weight = opt.toInt(&isWeight);
                            if (isWeight && i > 0) {
                                currentChoice.options.last().weight = weight;
                                continue;
                            } else {
                                o.text = opt;
                                currentChoice.options.append(o);
                            }
                        }
                        set.choices.append(currentChoice);
                        currentChoice = InstructionChoice();
                        inChoice = false;
                    }
                } else if (key == "Set") {
                    set.includedSets.append(value);
                } else if (key == "Select") {
                    if (value.compare("first", Qt::CaseInsensitive) == 0)
                        set.selectMode = InstructionSelectMode::First;
                    else if (value.compare("random", Qt::CaseInsensitive) == 0)
                        set.selectMode = InstructionSelectMode::Random;
                    else
                        set.selectMode = InstructionSelectMode::All;
                }else if (key == "If") {
                    set.ifFlags.append(value.split(',', Qt::SkipEmptyParts));
                } else if (key == "NotIf") {
                    set.notIfFlags.append(value.split(',', Qt::SkipEmptyParts));
                } else if (key == "IfChosen") {
                    set.ifChosen.append(value.split(',', Qt::SkipEmptyParts));
                } else if (key == "IfNotChosen") {
                    set.ifNotChosen.append(value.split(',', Qt::SkipEmptyParts));
                } else if (key == "Flag")
                    set.setFlags.append(value);
                else if (key == "OptionFlag" && !currentChoice.options.isEmpty())
                    currentChoice.options.last().optionFlags.append(value);
                else if (key == "Check")
                    currentChoice.options.last().check.append(value);
                else if (key == "CheckOff")
                    currentChoice.options.last().checkOff.append(value);
            }
        }

        if (!currentChoice.options.isEmpty())
            set.choices.append(currentChoice);

        InstructionDefinition def;
        def.name = set.name;
        def.isClothing = false;
        def.askable = true;
        def.changeMode = InstructionChangeMode::Daily;
        def.selectMode = InstructionSelectMode::All;
        def.noneText = "";
        def.sets.append(set);

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

void ScriptParser::parseProcedureSections(const QMap<QString, QMap<QString, QStringList>>& sections) {
    for (auto it = sections.begin(); it != sections.end(); ++it) {
        QString sectionName = it.key();
        if (!sectionName.startsWith("procedure-"))
            continue;

        QString procName = sectionName.mid(QString("procedure-").length());
        const QMap<QString, QStringList>& entries = it.value();

        ProcedureDefinition proc;
        proc.name = procName;

        for (auto keyIt = entries.begin(); keyIt != entries.end(); ++keyIt) {
            QString key = keyIt.key();
            const QStringList& values = keyIt.value();

            for (const QString& rawValue : values) {
                QString value = rawValue.trimmed();

                if (key == "Title") {
                    proc.title = value;
                } else if (key == "Select") {
                    if (value.compare("first", Qt::CaseInsensitive) == 0)
                        proc.selectMode = ProcedureSelectMode::First;
                    else if (value.compare("random", Qt::CaseInsensitive) == 0)
                        proc.selectMode = ProcedureSelectMode::Random;
                    else
                        proc.selectMode = ProcedureSelectMode::All;
                } else if (key == "Procedure") {
                    ProcedureCall call;
                    if (value.contains(',')) {
                        QStringList parts = value.split(',', Qt::SkipEmptyParts);
                        call.name = parts[0].trimmed();
                        call.weight = parts.value(1, "1").toInt();
                    } else {
                        call.name = value;
                        call.weight = 1;
                    }
                    proc.calls.append(call);
                } else if (key == "If") {
                    proc.ifFlags += value.split(',', Qt::SkipEmptyParts);
                } else if (key == "NotIf") {
                    proc.notIfFlags += value.split(',', Qt::SkipEmptyParts);
                } else if (key == "PreStatus") {
                    proc.preStatuses += value.split(',', Qt::SkipEmptyParts);
                } else if (key == "NotBefore") {
                    proc.notBefore += value.split(',', Qt::SkipEmptyParts);
                } else if (key == "NotAfter") {
                    proc.notAfter += value.split(',', Qt::SkipEmptyParts);
                } else if (key == "NotBetween") {
                    QStringList parts = value.split(',', Qt::SkipEmptyParts);
                    if (parts.size() == 2)
                        proc.notBetween.append({ parts[0].trimmed(), parts[1].trimmed() });
                }
            }
        }

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
                        proc.messages.append(currentGroup);
                        currentGroup = MessageGroup();
                    }
                }
                else if (key == "Message") {
                    inMessageBlock = true;
                    currentGroup.messages.append(val.trimmed());
                }
                else if (key == "Text") {
                    proc.statusTexts.append(val.trimmed());
                }
            }
        }
        if (!currentGroup.messages.isEmpty()) {
            proc.messages.append(currentGroup);
        }

        if (entries.contains("Input"))
            proc.inputQuestions.append(entries["Input"]);

        if (entries.contains("NoInputProcedure"))
            proc.noInputProcedure = entries["NoInputProcedure"].value(0);

        if (entries.contains("Question"))
            proc.advancedQuestions.append(entries["Question"]);

        if (entries.contains("StartAutoAssign")) {
            QString value = entries["StartAutoAssign"].value(0);
            if (value.startsWith("time,", Qt::CaseInsensitive)) {
                proc.autoAssignMode = "time";
                proc.autoAssignValue = value.mid(QString("time,").length()).trimmed();
            } else if (value.startsWith("interval,", Qt::CaseInsensitive)) {
                proc.autoAssignMode = "interval";
                proc.autoAssignValue = value.mid(QString("interval,").length()).trimmed();
            } else if (value.startsWith("ask", Qt::CaseInsensitive)) {
                proc.autoAssignMode = "ask";
                int commaIndex = value.indexOf(',');
                if (commaIndex >= 0)
                    proc.autoAssignValue = value.mid(commaIndex + 1).trimmed();
                else
                    proc.autoAssignValue = "";
            }
        }

        if (entries.contains("StopAutoAssign"))
            proc.stopAutoAssign = true;

        if (entries.contains("Clothing"))
            proc.clothingInstruction = entries["Clothing"].value(0);

        if (entries.contains("ClearCheck"))
            proc.clearClothingCheck = entries["ClearCheck"].value(0).trimmed() == "1";

        if (entries.contains("MasterMail"))
            proc.masterMailSubject = entries["MasterMail"].value(0);

        if (entries.contains("MasterAttach")) {
            for (const QString& file : entries["MasterAttach"])
                proc.masterAttachments.append(file.trimmed());
        }

        if (entries.contains("SubMail")) {
            for (const QString& line : entries["SubMail"])
                proc.subMailLines.append(line.trimmed());
        }

        if (entries.contains("SetFlag"))
            proc.setFlags = entries["SetFlag"];

        if (entries.contains("ClearFlag"))
            proc.clearFlags = entries["ClearFlag"];

        if (entries.contains("Set$"))
            proc.setStringVars = entries["Set$"];

        if (entries.contains("Set#"))
            proc.setCounterVars = entries["Set#"];

        if (entries.contains("Set@"))
            proc.setTimeVars = entries["Set@"];

        if (entries.contains("Counter+"))
            proc.incrementCounters = entries["Counter+"];

        if (entries.contains("Counter-"))
            proc.decrementCounters = entries["Counter-"];

        if (entries.contains("Random#"))
            proc.randomCounters = entries["Random#"];

        if (entries.contains("Random$"))
            proc.randomStrings = entries["Random$"];

        if (entries.contains("Set$"))
            proc.setStrings = entries["Set$"];

        if (entries.contains("Input$"))
            proc.inputStrings = entries["Input$"];

        if (entries.contains("InputLong$"))
            proc.inputLongStrings = entries["InputLong$"];

        if (entries.contains("Drop$"))
            proc.dropStrings = entries["Drop$"];

        if (entries.contains("Set#"))
            proc.setCounters = entries["Set#"];

        if (entries.contains("Add#"))
            proc.addCounters = entries["Add#"];

        if (entries.contains("Subtract#"))
            proc.subtractCounters = entries["Subtract#"];

        if (entries.contains("Multiply#"))
            proc.multiplyCounters = entries["Multiply#"];

        if (entries.contains("Divide#"))
            proc.divideCounters = entries["Divide#"];

        if (entries.contains("Drop#"))
            proc.dropCounters = entries["Drop#"];

        if (entries.contains("Input#"))
            proc.inputCounters = entries["Input#"];

        if (entries.contains("InputNeg#"))
            proc.inputNegCounters = entries["InputNeg#"];

        if (entries.contains("Random#"))
            proc.randomCounters = entries["Random#"];

        if (entries.contains("Set!"))
            proc.setTimeVars = entries["Set!"];
        if (entries.contains("Add!"))
            proc.addTimeVars = entries["Add!"];
        if (entries.contains("Subtract!"))
            proc.subtractTimeVars = entries["Subtract!"];
        if (entries.contains("Multiply!"))
            proc.multiplyTimeVars = entries["Multiply!"];
        if (entries.contains("Divide!"))
            proc.divideTimeVars = entries["Divide!"];
        if (entries.contains("Round!"))
            proc.roundTimeVars = entries["Round!"];
        if (entries.contains("Drop!"))
            proc.dropTimeVars = entries["Drop!"];
        if (entries.contains("InputDate!"))
            proc.inputDateVars = entries["InputDate!"];
        if (entries.contains("InputDateDef!"))
            proc.inputDateDefVars = entries["InputDateDef!"];
        if (entries.contains("InputTime!"))
            proc.inputTimeVars = entries["InputTime!"];
        if (entries.contains("InputTimeDef!"))
            proc.inputTimeDefVars = entries["InputTimeDef!"];
        if (entries.contains("InputInterval!"))
            proc.inputIntervalVars = entries["InputInterval!"];
        if (entries.contains("Random!"))
            proc.randomTimeVars = entries["Random!"];
        if (entries.contains("AddDays!"))
            proc.addDaysVars = entries["AddDays!"];
        if (entries.contains("SubtractDays!"))
            proc.subtractDaysVars = entries["SubtractDays!"];
        if (entries.contains("Days#"))
            proc.extractToCounter += entries["Days#"];
        if (entries.contains("Hours#"))
            proc.extractToCounter += entries["Hours#"];
        if (entries.contains("Minutes#"))
            proc.extractToCounter += entries["Minutes#"];
        if (entries.contains("Seconds#"))
            proc.extractToCounter += entries["Seconds#"];
        if (entries.contains("Days!"))
            proc.convertFromCounter += entries["Days!"];
        if (entries.contains("Hours!"))
            proc.convertFromCounter += entries["Hours!"];
        if (entries.contains("Minutes!"))
            proc.convertFromCounter += entries["Minutes!"];
        if (entries.contains("Seconds!"))
            proc.convertFromCounter += entries["Seconds!"];

        QStringList lines;
        for (auto keyIt = entries.begin(); keyIt != entries.end(); ++keyIt) {
            for (const QString &val : keyIt.value())
                lines.append(val);
        }

        int index = 0;
        while (index < lines.size()) {
            if (lines[index].startsWith("Case=", Qt::CaseInsensitive)) {
                CaseBlock block = parseCaseBlock(lines, index);
                proc.cases.append(block);
            } else {
                ++index;
            }
        }

        parseTimeRestrictions(entries, proc.notBeforeTimes, proc.notAfterTimes, proc.notBetweenTimes);

        scriptData.procedures.insert(proc.name, proc);
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

void ScriptParser::parseTimerSections(const QMap<QString, QMap<QString, QStringList>>& sections) {
    for (auto it = sections.begin(); it != sections.end(); ++it) {
        QString sectionName = it.key();
        if (!sectionName.startsWith("timer-"))
            continue;

        QString name = sectionName.mid(QString("timer-").length());
        const QMap<QString, QStringList>& entries = it.value();

        TimerDefinition t;
        t.name = name;

        for (auto e = entries.begin(); e != entries.end(); ++e) {
            const QString& key = e.key();
            const QStringList& values = e.value();

            for (const QString& raw : values) {
                QString value = raw.trimmed();

                if (key == "Start") {
                    QStringList times = value.split(',', Qt::SkipEmptyParts);
                    t.startTimeMin = times.value(0);
                    t.startTimeMax = times.value(1, times.value(0));
                } else if (key == "End") {
                    t.endTime = value;
                } else if (key == "PreStatus") {
                    t.preStatuses += value.split(',', Qt::SkipEmptyParts);
                } else if (key == "If") {
                    t.ifFlags += value.split(',', Qt::SkipEmptyParts);
                } else if (key == "NotIf") {
                    t.notIfFlags += value.split(',', Qt::SkipEmptyParts);
                } else if (key == "NotBefore") {
                    t.notBefore += value.split(',', Qt::SkipEmptyParts);
                } else if (key == "NotAfter") {
                    t.notAfter += value.split(',', Qt::SkipEmptyParts);
                } else if (key == "NotBetween") {
                    QStringList parts = value.split(',', Qt::SkipEmptyParts);
                    if (parts.size() == 2)
                        t.notBetween.append({ parts[0].trimmed(), parts[1].trimmed() });
                }
            }
        }

        if (entries.contains("MasterMail"))
            t.masterMailSubject = entries["MasterMail"].value(0);

        if (entries.contains("MasterAttach")) {
            for (const QString& file : entries["MasterAttach"])
                t.masterAttachments.append(file.trimmed());
        }

        if (entries.contains("SubMail")) {
            for (const QString& line : entries["SubMail"])
                t.subMailLines.append(line.trimmed());
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
                t.cases.append(block);
            } else {
                ++index;
            }
        }

        parseDurationControl(entries, t.duration);
        parseTimeRestrictions(entries, t.notBeforeTimes, t.notAfterTimes, t.notBetweenTimes);

        scriptData.timers.insert(t.name, t);
    }
}

void ScriptParser::parseQuestionSections(const QMap<QString, QMap<QString, QStringList>>& sections) {
    for (auto it = sections.begin(); it != sections.end(); ++it) {
        QString sectionName = it.key();
        if (!sectionName.startsWith("question-"))
            continue;

        QString questionName = sectionName.mid(QString("question-").length());
        const QMap<QString, QStringList>& entries = it.value();

        QuestionDefinition question;
        question.name = questionName;

        QuestionAnswerBlock* currentAnswer = nullptr;

        for (auto e = entries.begin(); e != entries.end(); ++e) {
            QString key = e.key();
            const QStringList& values = e.value();

            for (const QString& val : values) {
                QString value = val.trimmed();

                if (key == "Phrase") {
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
                        currentAnswer->procedureName = value;
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
                }
            }
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

#include <QFile>
#include <QTextStream>
#include <QDebug>    // if you want to emit qWarning()/qDebug()

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
    return scriptData.general.subNames.isEmpty() ? QString() : scriptData.general.subNames.first();
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

QList<ClothTypeSection> ScriptParser::getClothTypeSections() const {
    return scriptData.clothingTypes.values();
}

QuestionSection ScriptParser::getQuestion(const QString &name) const {
    return scriptData.questions.value(name, QuestionDefinition());
}

void ScriptParser::setVariable(const QString &name, const QString &value) {
    scriptData.stringVariables[name] = value;
}
