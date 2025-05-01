#include "scriptparser.h"
#include "cyberdom.h"

#include <QFile>
#include <QTextStream>
#include <QFileInfo>
#include <QDir>
#include <QRegularExpression>

ScriptParser::ScriptParser(CyberDom *parent) : mainApp(parent) {

}

ScriptParser::~ScriptParser() {

}

bool ScriptParser::parseScript(const QString &filePath) {
    QFile file(filePath);
    if (!file.exists() || !file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "Error: Cannot open script file:" << filePath;
        return false;
    }

    // Clear any previous data
    rawSections.clear();
    statusSections.clear();
    permissionSections.clear();
    reportSections.clear();
    confessionSections.clear();
    jobSections.clear();
    punishmentSections.clear();
    flagSections.clear();
    procedureSections.clear();
    popupSections.clear();
    timerSections.clear();
    instructionSections.clear();
    clothingSections.clear();
    clothTypeSections.clear();
    generalSection.clear();
    initSection.clear();
    eventsSection.clear();
    statusGroups.clear();

    qDebug() << "\n[DEBUG] ============ PARSING SCRIPT FILE ============";
    qDebug() << "[DEBUG] Script file path: " << filePath;

    QTextStream in(&file);
    QString currentSection;
    QString currentType;
    int lineCount = 0;
    int statusSectionCount = 0;

    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        lineCount++;

        // Skip empty lines and comments
        if (line.isEmpty() || line.startsWith(";")) {
            continue;
        }

        // Check for includes
        if (line.startsWith("%include=")) {
            QString includePath = line.section('=', 1).trimmed();
            QFileInfo fileInfo(filePath);
            QString baseDir = fileInfo.absolutePath();
            parseIncludeFile(baseDir + QDir::separator() + includePath);
            continue;
        }

        if (line.startsWith("[clothtype-", Qt::CaseInsensitive)) {
            int start = line.indexOf('-') + 1;
            int end = line.indexOf(']');
            if (start > 0 && end > start) {
                QString typeName = line.mid(start, end - start).trimmed();
                if (!typeName.isEmpty() && !clothTypes.contains(typeName)) {
                    clothTypes.append(typeName);
                    qDebug() << "[DEBUG] Parsed cloth type:" << typeName;
                }
            }
        }

        // Check for section header
        if (line.startsWith("[") && line.endsWith("]")) {
            QString sectionHeader = line.mid(1, line.length() - 2);
            qDebug() << "[DEBUG] Line " << lineCount << ": Found section: " << sectionHeader;
            
            // Specifically log status sections
            if (sectionHeader.toLower().startsWith("status-") || 
                sectionHeader.toLower().contains("status")) {
                statusSectionCount++;
                qDebug() << "[DEBUG] Found status section: " << sectionHeader << " (total: " << statusSectionCount << ")";
            }
            
            parseSectionLine(line, currentSection, currentType);
            continue;
        }

        // Process key-value pairs
        if (!currentSection.isEmpty() && line.contains("=")) {
            parseKeyValueLine(line, currentSection, currentType);
        }
    }

    file.close();

    qDebug() << "[DEBUG] Finished parsing file, total lines: " << lineCount;
    qDebug() << "[DEBUG] Total raw sections found: " << rawSections.size();
    
    // Debug: Output all raw section names
    QStringList sectionNames = rawSections.keys();
    qDebug() << "[DEBUG] Raw section names: " << sectionNames.join(", ");
    
    // Debug: Check specifically for status sections in rawSections
    int rawStatusCount = 0;
    for (const QString &sectionName : sectionNames) {
        if (sectionName.toLower().startsWith("status-") || 
            sectionName.toLower() == "status" ||
            (sectionName.toLower().contains("status") && sectionName.contains("-"))) {
            rawStatusCount++;
            qDebug() << "[DEBUG] Raw status section found: " << sectionName;
            
            // Debug the actual content of each status section
            QMap<QString, QStringList> sectionData = rawSections[sectionName];
            qDebug() << "[DEBUG] Status section " << sectionName << " contains " << sectionData.size() << " keys:";
            for (auto it = sectionData.begin(); it != sectionData.end(); ++it) {
                qDebug() << "[DEBUG]   - Key: " << it.key() << ", Values: " << it.value().join(", ");
            }
        }
    }
    qDebug() << "[DEBUG] Total raw status sections: " << rawStatusCount;
    qDebug() << "[DEBUG] ============ END PARSING SCRIPT FILE ============\n";

    // Process the raw sections into structured data
    processGeneralSection();
    processStatusSections();
    processPermissionSections();
    processReportSections();
    processConfessionSections();
    processJobSections();
    processPunishmentSections();
    processFlagSections();
    processProcedureSections();
    processPopupSections();
    processTimerSections();
    processInstructionSections();
    processClothingSections();
    processClothTypeSections();

    // Build status groups mapping
    buildStatusGroups();

    // Check for include files
    parseIncludeFiles(filePath);

    return true;
}

void ScriptParser::parseSectionLine(const QString &line, QString &currentSection, QString &currentType) {
    // Extract section name (remove [ and ])
    QString sectionName = line.mid(1, line.length() - 2);

    // Skip commented-out sections
    if (sectionName.startsWith(";")) {
        currentSection = "";
        currentType = "";
        return;
    }

    // Add debug output to see what sections are being parsed
    qDebug() << "[DEBUG] Parsing section: " << sectionName;

    // To ensure we properly handle status sections, first extract the raw section name without any prefix
    QString rawSectionName = sectionName;
    
    // Normalize section name for comparison (lowercase)
    QString normalizedName = sectionName.toLower();

    // Determine section type
    if (normalizedName == "general") {
        currentSection = "general";
        currentType = "general";
    } else if (normalizedName == "init") {
        currentSection = "init";
        currentType = "init";
    } else if (normalizedName == "events") {
        currentSection = "events";
        currentType = "events";
    } else if (normalizedName.startsWith("status-")) {
        // This handles the exact format "Status-Normal" from your script
        currentSection = rawSectionName; // Keep the original casing
        currentType = "status";
        qDebug() << "[DEBUG] Found status section: " << sectionName;
    } else if (normalizedName.startsWith("permission-")) {
        currentSection = "permission";
        currentType = "permission";
    } else if (normalizedName.startsWith("report-")) {
        currentSection = "report";
        currentType = "report";
    } else if (normalizedName.startsWith("confession-")) {
        currentSection = "confession";
        currentType = "confession";
    } else if (normalizedName.startsWith("job-")) {
        currentSection = sectionName;
        currentType = "job";
        qDebug() << "[DEBUG] Found job section: " << sectionName;
    } else if (normalizedName.startsWith("punishment-")) {
        currentSection = "punishment";
        currentType = "punishment";
    } else if (normalizedName.startsWith("flag-")) {
        currentSection = "flag";
        currentType = "flag";
    } else if (normalizedName.startsWith("procedure-")) {
        currentSection = "procedure";
        currentType = "procedure";
    } else if (normalizedName.startsWith("popup-")) {
        currentSection = "popup";
        currentType = "popup";
    } else if (normalizedName.startsWith("timer-")) {
        currentSection = "timer";
        currentType = "timer";
    } else if (normalizedName.startsWith("instructions-")) {
        currentSection = "instruction";
        currentType = "instruction";
    } else if (normalizedName.startsWith("clothing-")) {
        currentSection = "clothing";
        currentType = "clothing";
    } else if (normalizedName.startsWith("clothtype-")) {
        currentSection = rawSectionName; // Keep the original section name with prefix
        currentType = "clothtype";
        qDebug() << "[DEBUG] Found cloth type section: " << sectionName;
    } else if (normalizedName.startsWith("set-")) {
        currentSection = "set";
        currentType = "set";
    } else if (normalizedName.startsWith("rule-")) {
        currentSection = "rule";
        currentType = "rule";
    } else if (normalizedName.startsWith("question-")) {
        currentSection = "question";
        currentType = "question";
    } else if (normalizedName.startsWith("popupgroup-")) {
        currentSection = "popupgroup";
        currentType = "popupgroup";
    } else if (normalizedName.startsWith("case-")) {
        currentSection = "case";
        currentType = "case";
    } else if (normalizedName.startsWith("ftp")) {
        currentSection = "ftp";
        currentType = "ftp";
    } else if (normalizedName.startsWith("font")) {
        currentSection = "font";
        currentType = "font";
    } else if (normalizedName.startsWith("language")) {
        currentSection = "language";
        currentType = "language";
    } else {
        qDebug() << "[Warning] Unknown section type:" << sectionName;
        currentSection = sectionName;
        currentType = "unknown";
    }

    // Initialize the section in our raw data if it doesn't exist
    if (!rawSections.contains(currentSection)) {
        rawSections[currentSection] = QMap<QString, QStringList>();
    }
}

void ScriptParser::parseKeyValueLine(const QString &line, const QString &currentSection, const QString &currentType) {
    int equalsPos = line.indexOf('=');
    if (equalsPos <= 0) return;

    QString key = line.left(equalsPos).trimmed();
    QString value = line.mid(equalsPos + 1) .trimmed();

    // Store normally
    if (!rawSections[currentSection].contains(key)) {
        rawSections[currentSection][key] = QStringList();
    }
    rawSections[currentSection][key].append(value);

    // --- New: store line as-is for cloth types ---
    if (currentType == "clothtype") {
        rawClothTypeLines[currentSection].append(line);
    }
}

void ScriptParser::processGeneralSection() {
    if (!rawSections.contains("general")) return;

    // Copy all key/values to generalSection (only keeping the first value for simplicity)
    QMapIterator<QString, QStringList> i(rawSections["general"]);
    while (i.hasNext()) {
        i.next();
        if (!i.value().isEmpty()) {
            generalSection[i.key().toLower()] = i.value().first();
        }
    }
}

void ScriptParser::processStatusSections() {
    qDebug() << "[DEBUG] Processing status sections. Total raw sections: " << rawSections.size();
    int count = 0;
    
    // Create a list to store status section keys for debugging
    QStringList statusSectionKeys;
    
    QMapIterator<QString, QMap<QString, QStringList>> i(rawSections);
    while (i.hasNext()) {
        i.next();
        
        // Check if this section is a status section by checking if it starts with "Status-" 
        // (case-insensitive for comparison but preserve original case for the name)
        if (i.key().toLower().startsWith("status-")) {
            QString statusName = i.key().mid(7); // Remove "Status-" prefix
            statusSectionKeys.append(i.key()); // Add to our debug list
            
            qDebug() << "[DEBUG] Processing status section: " << i.key() << " -> status name: " << statusName;
            count++;

            StatusSection status;
            status.name = statusName;
            status.type = "status";
            status.keyValues = i.value();

            // Process specific status attributes
            status.isSubStatus = (i.value().contains("substatus") && i.value()["substatus"].contains("1"));
            status.reportsOnly = (i.value().contains("reportsonly") && i.value()["reportsonly"].contains("1"));
            status.assignmentsAllowed = !(i.value().contains("assignments") && i.value()["assignments"].contains("0"));
            status.permissionsAllowed = !(i.value().contains("permissions") && i.value()["permissions"].contains("0"));
            status.confessionsAllowed = !(i.value().contains("confessions") && i.value()["confessions"].contains("0"));
            status.reportsAllowed = !(i.value().contains("reports") && i.value()["reports"].contains("0"));
            status.rulesAllowed = !(i.value().contains("rules") && i.value()["rules"].contains("0"));

            // Process status groups
            if (i.value().contains("group")) {
                status.groups = i.value()["group"];

                // Also build the status groups mapping
                for (const QString &group : status.groups) {
                    if (!statusGroups.contains(group)) {
                        statusGroups[group] = QStringList();
                    }
                    statusGroups[group].append(status.name);
                }
            }

            // Process quick reports
            if (i.value().contains("quickreport")) {
                status.quickReports = i.value()["quickreport"];
            }

            // Store the processed status
            statusSections[status.name] = status;
            qDebug() << "[DEBUG] Added status: " << status.name;
        }
    }
    
    qDebug() << "[DEBUG] Status Section Keys: " << statusSectionKeys.join(", ");
    qDebug() << "[DEBUG] Processed " << count << " status sections. Result: " << statusSections.size() << " status entries";
    
    // Debug all raw sections to see what's available
    QStringList allSections;
    QMapIterator<QString, QMap<QString, QStringList>> j(rawSections);
    while (j.hasNext()) {
        j.next();
        allSections.append(j.key());
    }
    qDebug() << "[DEBUG] All raw sections: " << allSections.join(", ");
}

void ScriptParser::processPermissionSections() {
    QMapIterator<QString, QMap<QString, QStringList>> i(rawSections);
    while (i.hasNext()) {
        i.next();
        if (!i.key().toLower().startsWith("permission-")) continue;

        PermissionSection permission;
        permission.name = i.key().mid(11); // Remove "permission-" prefix
        permission.type = "permission";
        permission.keyValues = i.value();

        // Process specific permission attributes
        if (i.value().contains("pct")) {
            bool ok;
            permission.percentChance = i.value()["pct"].first().toInt(&ok);
            if (!ok) permission.percentChance = 0;
        }

        if (i.value().contains("prestatus")) {
            QStringList allPreStatus;
            for (const QString &preStatusVal : i.value()["prestatus"]) {
                // Handle comma-separated prestatus values
                allPreStatus.append(preStatusVal.split(",", Qt::SkipEmptyParts));
            }
            permission.preStatus = allPreStatus;
        }

        if (i.value().contains("delay")) {
            permission.delayTime = i.value()["delay"].first();
        }

        if (i.value().contains("mininterval")) {
            permission.minInterval = i.value()["mininterval"].first();
        }

        if (i.value().contains("maxwait")) {
            permission.maxWait = i.value()["maxwait"].first();
        }

        if (i.value().contains("notify")) {
            bool ok;
            permission.notifyLevel = i.value()["notify"].first().toInt(&ok);
            if (!ok) permission.notifyLevel = 0;
        }

        if (i.value().contains("newstatus")) {
            permission.newStatus = i.value()["newstatus"].first();
        }

        if (i.value().contains("denymessage")) {
            permission.denyMessage = i.value()["denymessage"].first();
        }

        if (i.value().contains("permitmessage")) {
            permission.permitMessage = i.value()["permitmessage"].first();
        }

        if (i.value().contains("denyflag")) {
            for (const QString &flagVal : i.value()["denyflag"]) {
                // Handle comma-separated flag values
                permission.denyFlags.append(flagVal.split(",", Qt::SkipEmptyParts));
            }
        }

        if (i.value().contains("permitif")) {
            for (const QString &flagVal : i.value()["permitif"]) {
                // Handle comma-separated flag values
                permission.permitFlags.append(flagVal.split(",", Qt::SkipEmptyParts));
            }
        }

        if (i.value().contains("denybelow")) {
            bool ok;
            permission.denyBelow = i.value()["denybelow"].first().toInt(&ok);
            if (!ok) permission.denyBelow = 0;
        }

        if (i.value().contains("denyabove")) {
            bool ok;
            permission.denyAbove = i.value()["denyabove"].first().toInt(&ok);
            if (!ok) permission.denyAbove = INT_MAX;
        }

        // Store the processed permission
        permissionSections[permission.name] = permission;
    }
}

void ScriptParser::processReportSections() {
    QMapIterator<QString, QMap<QString, QStringList>> i(rawSections);
    while (i.hasNext()) {
        i.next();
        if (!i.key().toLower().startsWith("report-")) continue;

        ReportSection report;
        report.name = i.key().mid(7); // Remove "report-" prefix
        report.type = "report";
        report.keyValues = i.value();

        // Process specific report attributes
        report.onTop = (i.value().contains("ontop") && i.value()["ontop"].contains("1"));
        report.menuVisible = !(i.value().contains("menu") && i.value()["menu"].contains("0"));

        if (i.value().contains("prestatus")) {
            QStringList allPreStatus;
            for (const QString &preStatusVal : i.value()["prestatus"]) {
                // Handle comma-separated prestatus values
                allPreStatus.append(preStatusVal.split(",", Qt::SkipEmptyParts));
            }
            report.preStatus = allPreStatus;
        }

        if (i.value().contains("newstatus")) {
            report.newStatus = i.value()["newstatus"].first();
        }

        if (i.value().contains("endreport")) {
            report.endReport = i.value()["endreport"].first();
        }

        // Store the processed report
        reportSections[report.name] = report;
    }
}

void ScriptParser::processConfessionSections() {
    QMapIterator<QString, QMap<QString, QStringList>> i(rawSections);
    while (i.hasNext()) {
        i.next();
        if (!i.key().toLower().startsWith("confessions-")) continue;

        ConfessionSection confession;
        confession.name = i.key().mid(11); // Remove "confession-" prefix
        confession.type = "confession";
        confession.keyValues = i.value();

        // Process specific confession attributes
        confession.onTop = (i.value().contains("ontop") && i.value()["ontop"].contains("1"));

        if (i.value().contains("prestatus")) {
            QStringList allPreStatus;
            for (const QString &preStatusVal : i.value()["prestatus"]) {
                // Handle comma-separated prestatus values
                allPreStatus.append(preStatusVal.split(",", Qt::SkipEmptyParts));
            }
            confession.preStatus = allPreStatus;
        }

        if (i.value().contains("newstatus")) {
            confession.newStatus = i.value()["newstatus"].first();
        }

        if (i.value().contains("punish")) {
            bool ok;
            confession.punishSeverity = i.value()["punish"].first().toInt(&ok);
            if (!ok) confession.punishSeverity = 0;
        }

        // Store the processed confession
        confessionSections[confession.name] = confession;
    }
}

void ScriptParser::processJobSections() {
    QMapIterator<QString, QMap<QString, QStringList>> i(rawSections);
    int jobCount = 0;
    
    qDebug() << "[DEBUG] Starting to process job sections";
    
    while (i.hasNext()) {
        i.next();
        if (!i.key().toLower().startsWith("job-")) continue;

        JobSection job;
        job.name = i.key().mid(4); // Remove "job-" prefix
        job.type = "job";
        job.keyValues = i.value();

        // Process specific job attributes
        if (i.value().contains("interval")) {
            job.interval = i.value()["interval"].first();
        }

        if (i.value().contains("firstinterval")) {
            job.firstInterval = i.value()["firstinterval"].first();
        }

        if (i.value().contains("run")) {
            job.runDays = i.value()["run"];
        }

        if (i.value().contains("norun")) {
            job.noRunDays = i.value()["norun"];
        }

        if (i.value().contains("endtime")) {
            job.endTime = i.value()["endtime"].first();
        }

        if (i.value().contains("remindinterval")) {
            job.remindInterval = i.value()["remindinterval"].first();
        }

        if (i.value().contains("remindpenalty")) {
            bool ok;
            job.remindPenalty = i.value()["remindpenalty"].first().toInt(&ok);
            if (!ok) job.remindPenalty = 0;
        }

        if (i.value().contains("remindpenaltygroup")) {
            job.remindPenaltyGroup = i.value()["remindpenaltygroup"].first();
        }

        if (i.value().contains("latemerits")) {
            bool ok;
            job.lateMerits = i.value()["latemerits"].first().toInt(&ok);
            if (!ok) job.lateMerits = 0;
        }

        if (i.value().contains("expireafter")) {
            job.expireAfter = i.value()["expireafter"].first();
        }

        if (i.value().contains("expirepenalty")) {
            bool ok;
            job.expirePenalty = i.value()["expirepenalty"].first().toInt(&ok);
            if (!ok) job.expirePenalty = 0;
        }

        if (i.value().contains("expirepenaltygroup")) {
            job.expirePenaltyGroup = i.value()["expirepenaltygroup"].first();
        }

        if (i.value().contains("respite") || i.value().contains("respit")) {
            job.respite = i.value().value("respite", i.value().value("respit")).first();
        }

        if (i.value().contains("estimate")) {
            job.estimate = i.value()["estimate"].first();
        }

        job.oneTime = (i.value().contains("onetime") && i.value()["onetime"].contains("1"));
        job.announce = !(i.value().contains("announce") && i.value()["announce"].contains("0"));

        // Store the processed job
        jobSections[job.name] = job;
        jobCount++;
        
        qDebug() << "[DEBUG] Processed job section: " << job.name;
    }
    
    qDebug() << "[DEBUG] Finished processing job sections. Total jobs found: " << jobCount;
}

void ScriptParser::processPunishmentSections() {
    qDebug() << "\n[DEBUG] ============ PROCESSING PUNISHMENT SECTIONS ============";
    qDebug() << "[DEBUG] Looking for punishment sections in " << rawSections.size() << " raw sections";
    
    int punishmentCount = 0;
    QStringList punishmentSectionKeys;
    
    QMapIterator<QString, QMap<QString, QStringList>> i(rawSections);
    while (i.hasNext()) {
        i.next();
        
        if (i.key().toLower().startsWith("punishment-")) {
            punishmentSectionKeys.append(i.key()); // Store for debugging
            
            QString sectionName = i.key();
            QString punishmentName = i.key().mid(11); // Remove "punishment-" prefix
            qDebug() << "[DEBUG] Processing punishment section: " << sectionName << " -> punishment name: " << punishmentName;
            punishmentCount++;

            PunishmentSection punishment;
            punishment.name = punishmentName;
            punishment.type = "punishment";
            punishment.keyValues = i.value();

            // Process specific punishment attributes
            if (i.value().contains("value")) {
                bool ok;
                punishment.value = i.value()["value"].first().toDouble(&ok);
                if (!ok) punishment.value = 1.0;
            }

            if (i.value().contains("valueunit")) {
                punishment.valueUnit = i.value()["valueunit"].first();
            }

            if (i.value().contains("max")) {
                bool ok;
                punishment.max = i.value()["max"].first().toInt(&ok);
                if (!ok) punishment.max = 20;
            }

            if (i.value().contains("min")) {
                bool ok;
                punishment.min = i.value()["min"].first().toInt(&ok);
                if (!ok) punishment.min = 1;
            }

            if (i.value().contains("minseverity")) {
                bool ok;
                punishment.minSeverity = i.value()["minseverity"].first().toInt(&ok);
                if (!ok) punishment.minSeverity = 0;
            }

            if (i.value().contains("maxseverity")) {
                bool ok;
                punishment.maxSeverity = i.value()["maxseverity"].first().toInt(&ok);
                if (!ok) punishment.maxSeverity = INT_MAX;
            }

            if (i.value().contains("weight")) {
                bool ok;
                punishment.weight = i.value()["weight"].first().toInt(&ok);
                if (!ok) punishment.weight = 1;
            }

            if (i.value().contains("group")) {
                for (const QString &groupVal : i.value()["group"]) {
                    // Handle comma-separated group values
                    punishment.groups.append(groupVal.split(",", Qt::SkipEmptyParts));
                }
            }

            punishment.groupOnly = (i.value().contains("grouponly") && i.value()["grouponly"].contains("1"));
            punishment.longRunning = (i.value().contains("longrunning") && i.value()["longrunning"].contains("1"));
            punishment.mustStart = (i.value().contains("muststart") && i.value()["muststart"].contains("1"));
            punishment.accumulative = (i.value().contains("accumulative") && i.value()["accumulative"].contains("1"));

            if (i.value().contains("respite") || i.value().contains("respit")) {
                punishment.respite = i.value().value("respite", i.value().value("respit")).first();
            }

            if (i.value().contains("estimate")) {
                punishment.estimate = i.value()["estimate"].first();
            }

            if (i.value().contains("forbid")) {
                for (const QString &forbidVal : i.value()["forbid"]) {
                    punishment.forbidPermissions.append(forbidVal);
                }
            }

            // Store the processed punishment
            punishmentSections[punishment.name] = punishment;
            qDebug() << "[DEBUG] Added punishment: " << punishment.name;
        }
    }
    
    qDebug() << "[DEBUG] Punishment Section Keys: " << punishmentSectionKeys.join(", ");
    qDebug() << "[DEBUG] Processed " << punishmentCount << " punishment sections. Result: " << punishmentSections.size() << " punishment entries";
    
    // Debug all raw sections to see what's available
    QStringList allSections;
    QMapIterator<QString, QMap<QString, QStringList>> j(rawSections);
    while (j.hasNext()) {
        j.next();
        allSections.append(j.key());
    }
    qDebug() << "[DEBUG] All raw sections: " << allSections.join(", ");
    qDebug() << "[DEBUG] ============ END PROCESSING PUNISHMENT SECTIONS ============\n";
}

void ScriptParser::processFlagSections() {
    QMapIterator<QString, QMap<QString, QStringList>> i(rawSections);
    while (i.hasNext()) {
        i.next();
        if (!i.key().toLower().startsWith("flag-")) continue;

        FlagSection flag;
        flag.name = i.key().mid(5); // Remove "flag-" prefix
        flag.type = "flag";
        flag.keyValues = i.value();

        // Process specific flag attributes
        if (i.value().contains("group")) {
            for (const QString &groupVal : i.value()["group"]) {
                // Handle comma-separated group values
                flag.groups.append(groupVal.split(",", Qt::SkipEmptyParts));
            }
        }

        if (i.value().contains("duration")) {
            flag.duration = i.value()["duration"].first();
        }

        if (i.value().contains("expiremessage")) {
            flag.expireMessage = i.value()["expiremessage"].first();
        }

        if (i.value().contains("expireprocedure")) {
            flag.expireProcedure = i.value()["expireprocedure"].first();
        }

        if (i.value().contains("setprocedure")) {
            flag.setProcedure = i.value()["setprocedure"].first();
        }

        if (i.value().contains("removeprocedure")) {
            flag.removeProcedure = i.value()["removeprocedure"].first();
        }

        flag.reportFlag = !(i.value().contains("reportflag") && i.value()["reportflag"].contains("0"));

        // Store the processed flag
        flagSections[flag.name] = flag;
    }
}

void ScriptParser::processProcedureSections() {
    QMapIterator<QString, QMap<QString, QStringList>> i(rawSections);
    while (i.hasNext()) {
        i.next();
        if (!i.key().toLower().startsWith("procedure-")) continue;

        ProcedureSection procedure;
        procedure.name = i.key().mid(10); // Remove "procedure-" prefix
        procedure.type = "procedure";
        procedure.keyValues = i.value();

        // Process specific procedure attributes
        if (i.value().contains("prestatus")) {
            QStringList allPreStatus;
            for (const QString &preStatusVal : i.value()["prestatus"]) {
                // Handle comma-separated prestatus values
                allPreStatus.append(preStatusVal.split(",", Qt::SkipEmptyParts));
            }
            procedure.preStatus = allPreStatus;
        }

        // Store the processed procedure
        procedureSections[procedure.name] = procedure;
    }
}

void ScriptParser::processPopupSections() {
    QMapIterator<QString, QMap<QString, QStringList>> i(rawSections);
    while (i.hasNext()) {
        i.next();
        if (!i.key().toLower().startsWith("popup-")) continue;

        PopupSection popup;
        popup.name = i.key().mid(6); // Remove "popup-" prefix
        popup.type = "popup";
        popup.keyValues = i.value();

        // Process specific popup attributes
        if (i.value().contains("prestatus")) {
            QStringList allPreStatus;
            for (const QString &preStatusVal : i.value()["prestatus"]) {
                // Handle comma-separated prestatus values
                allPreStatus.append(preStatusVal.split(",", Qt::SkipEmptyParts));
            }
            popup.preStatus = allPreStatus;
        }

        if (i.value().contains("group")) {
            for (const QString &groupVal : i.value()["group"]) {
                // Handle comma-separated group values
                popup.groups.append(groupVal.split(",", Qt::SkipEmptyParts));
            }
        }

        popup.groupOnly = (i.value().contains("grouponly") && i.value()["grouponly"].contains("1"));

        if (i.value().contains("weight")) {
            bool ok;
            popup.weight = i.value()["weight"].first().toInt(&ok);
            if (!ok) popup.weight = 1;
        }

        // Store the processed popup
        popupSections[popup.name] = popup;
    }
}

void ScriptParser::processTimerSections() {
    QMapIterator<QString, QMap<QString, QStringList>> i(rawSections);
    while (i.hasNext()) {
        i.next();
        if (!i.key().toLower().startsWith("timer-")) continue;

        TimerSection timer;
        timer.name = i.key().mid(6); // Remove "timer-" prefix
        timer.type = "timer";
        timer.keyValues = i.value();

        // Process specific timer attributes
        if (i.value().contains("start")) {
            timer.startTime = i.value()["start"].first();
        }

        if (i.value().contains("end")) {
            timer.endTime = i.value()["end"].first();
        }

        if (i.value().contains("prestatus")) {
            QStringList allPreStatus;
            for (const QString &preStatusVal : i.value()["prestatus"]) {
                // Handle comma-separated prestatus values
                allPreStatus.append(preStatusVal.split(",", Qt::SkipEmptyParts));
            }
            timer.preStatus = allPreStatus;
        }

        // Store the processed timer
        timerSections[timer.name] = timer;

    }
}

void ScriptParser::processInstructionSections() {
    QMapIterator<QString, QMap<QString, QStringList>> i(rawSections);
    while (i.hasNext()) {
        i.next();
        if (!i.key().toLower().startsWith("instructions-")) continue;

        InstructionSection instruction;
        instruction.name = i.key().mid(13); // Remove "instructions-" prefix
        instruction.type = "instructions";
        instruction.keyValues = i.value();

        // Process specific instruction attributes
        instruction.askable = !(i.value().contains("askable") && i.value()["askable"].contains("0"));

        if (i.value().contains("change")) {
            instruction.changeFrequency = i.value()["change"].first().toLower();
        }

        if (i.value().contains("none")) {
            instruction.noneMessage = i.value()["none"].first();
        }

        // Store the processed instruction
        instructionSections[instruction.name] = instruction;
    }
}

void ScriptParser::processClothingSections() {
    QMapIterator<QString, QMap<QString, QStringList>> i(rawSections);
    while (i.hasNext()) {
        i.next();
        if (!i.key().toLower().startsWith("clothing-")) continue;

        InstructionSection clothing;
        clothing.name = i.key().mid(9); // Remove "clothing-" prefix
        clothing.type = "clothing";
        clothing.keyValues = i.value();

        // Process specific clothing attributes (same as instructions)
        clothing.askable = !(i.value().contains("askable") && i.value()["askable"].contains("0"));

        if (i.value().contains("change")) {
            clothing.changeFrequency = i.value()["change"].first().toLower();
        }

        if (i.value().contains("none")) {
            clothing.noneMessage = i.value()["none"].first();
        }

        // Store the processed clothing
        clothingSections[clothing.name] = clothing;
    }
}

void ScriptParser::processClothTypeSections() {
    qDebug() << "[DEBUG] Robust cloth type parsing...";
    int count = 0;

    for (const QString& section : rawClothTypeLines.keys()) {
        if (!section.toLower().startsWith("clothtype-")) continue;

        QString clothTypeName = section.mid(10); // remove 'clothtype-'
        ClothTypeSection clothType;
        clothType.name = clothTypeName;
        clothType.type = "clothtype";

        QString currentAttr;
        const QStringList& lines = rawClothTypeLines[section];
        for (const QString& rawLine : lines) {
            QString line = rawLine.trimmed();
            if (line.startsWith("attr=", Qt::CaseInsensitive)) {
                currentAttr = line.mid(5).trimmed();
                clothType.attributes[currentAttr] = QStringList();
                qDebug() << "[ATTR]" << currentAttr;
            } else if (line.startsWith("value=", Qt::CaseInsensitive)) {
                if (!currentAttr.isEmpty()) {
                    QString val = line.mid(6).trimmed();
                    clothType.attributes[currentAttr].append(val);
                    qDebug() << "    [VAL]" << val;
                }
            } else if (line.startsWith("check=", Qt::CaseInsensitive)) {
                clothType.checks["check"] = line.mid(6).trimmed();
            }
        }

        clothTypeSections[clothType.name] = clothType;
        qDebug() << "[DEBUG] Loaded cloth type:" << clothType.name << "with" << clothType.attributes.size() << "attributes";
        count++;
    }

    qDebug() << "[DEBUG] Fully parsed" << count << "cloth type sections.";
}

void ScriptParser::buildStatusGroups() {}

QString ScriptParser::expandVariables(const QString &text) {
    QString result = text;

    // Replace master and subname variables
    result.replace("{$zzMaster}", getMaster());
    result.replace("{$zzSubName}", getSubName());

    return result;
}

void ScriptParser::parseIncludeFiles(const QString &mainFilePath) {
    QFile file(mainFilePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "[ERROR] Unable to open file for include parsing:" << mainFilePath;
        return;
    }

    QTextStream in(&file);
    while (in.atEnd()) {
        QString line = in.readLine().trimmed();

        // Look for include statements
        if (line.startsWith("%include=")) {
            QString includePath = line.section('=', 1).trimmed();
            QFileInfo fileInfo(mainFilePath);
            QString baseDir = fileInfo.absolutePath();
            parseIncludeFile(baseDir + QDir::separator() + includePath);
        }
    }

    file.close();
}

void ScriptParser::parseIncludeFile(const QString &includePath) {
    QFile file(includePath);
    if (!file.exists() || !file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "[ERROR] Cannot open include file:" << includePath;
        return;
    }

    QTextStream in(&file);
    QString currentSection;
    QString currentType;

    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();

        // Skip empty lines and comments
        if (line.isEmpty() || line.startsWith(";")) {
            continue;
        }

        // Check for includes (nested includes)
        if (line.startsWith("%include=")) {
            QString nestedIncludePath = line.section('=', 1).trimmed();
            QFileInfo fileInfo(includePath);
            QString baseDir = fileInfo.absolutePath();
            parseIncludeFile(baseDir + QDir::separator() + nestedIncludePath);
            continue;
        }

        // Check for section header
        if (line.startsWith("[") && line.endsWith("]")) {
            parseSectionLine(line, currentSection, currentType);
            continue;
        }

        // Process key-value pairs
        if (!currentSection.isEmpty() && line.contains("=")) {
            parseKeyValueLine(line, currentSection, currentType);
        }
    }

    file.close();

    qDebug() << "Successfully parsed include file:" << includePath;
}

// Implement getter methods
QList<StatusSection> ScriptParser::getStatusSections() const {
    return statusSections.values();
}

QList<PermissionSection> ScriptParser::getPermissionSections() const {
    return permissionSections.values();
}

QList<ReportSection> ScriptParser::getReportSections() const {
    return reportSections.values();
}

QList<ConfessionSection> ScriptParser::getConfessionSections() const {
    return confessionSections.values();
}

QList<JobSection> ScriptParser::getJobSections() const {
    return jobSections.values();
}

QList<PunishmentSection> ScriptParser::getPunishmentSections() const {
    return punishmentSections.values();
}

QList<FlagSection> ScriptParser::getFlagSections() const {
    return flagSections.values();
}

QList<ProcedureSection> ScriptParser::getProcedureSections() const {
    return procedureSections.values();
}

QList<PopupSection> ScriptParser::getPopupSections() const {
    return popupSections.values();
}

QList<TimerSection> ScriptParser::getTimerSections() const {
    return timerSections.values();
}

QList<InstructionSection> ScriptParser::getInstructionSections() const {
    return instructionSections.values();
}

QList<InstructionSection> ScriptParser::getClothingSections() const {
    return clothingSections.values();
}

QList<ClothTypeSection> ScriptParser::getClothTypeSections() const {
    return clothTypeSections.values();
}

StatusSection ScriptParser::getStatus(const QString &name) const {
    return statusSections.value(name);
}

PermissionSection ScriptParser::getPermission(const QString &name) const {
    return permissionSections.value(name);
}

ReportSection ScriptParser::getReport(const QString &name) const {
    return reportSections.value(name);
}

ConfessionSection ScriptParser::getConfession(const QString &name) const {
    return confessionSections.value(name);
}

JobSection ScriptParser::getJob(const QString &name) const {
    return jobSections.value(name);
}

PunishmentSection ScriptParser::getPunishment(const QString &name) const {
    return punishmentSections.value(name);
}

FlagSection ScriptParser::getFlag(const QString &name) const {
    return flagSections.value(name);
}

ProcedureSection ScriptParser::getProcedure(const QString &name) const {
    return procedureSections.value(name);
}

PopupSection ScriptParser::getPopup(const QString &name) const {
    return popupSections.value(name);
}

TimerSection ScriptParser::getTimer(const QString &name) const {
    return timerSections.value(name);
}

InstructionSection ScriptParser::getInstruction(const QString &name) const {
    return instructionSections.value(name);
}

InstructionSection ScriptParser::getClothing(const QString &name) const {
    return clothingSections.value(name);
}

ClothTypeSection ScriptParser::getClothType(const QString &name) const {
    return clothTypeSections.value(name);
}

QString ScriptParser::getMaster() const {
    return generalSection.value("master", "Master");
}

QString ScriptParser::getSubName() const {
    return generalSection.value("subname", "slave");
}

QString ScriptParser::getIniValue(const QString &section, const QString &key, const QString &defaultValue) const {
    // If the section exists in generalSection (for General Section)
    if (section.toLower() == "general" && generalSection.contains(key.toLower())) {
        return generalSection.value(key.toLower(), defaultValue);
    }

    // If the section exists in initSection (for Init Section)
    if (section.toLower() == "init" && initSection.contains(key.toLower())) {
        return initSection.value(key.toLower(), defaultValue);
    }

    // For regular sections
    QString sectionKey;
    if (section.toLower() == "general") {
        sectionKey = "general";
    } else if (section.toLower() == "init") {
        sectionKey = "init";
    } else {
        sectionKey = section;
    }

    // Check if the section exists in raw sections
    if (rawSections.contains(sectionKey)) {
        QMap<QString, QStringList> sectionData = rawSections[sectionKey];
        if (sectionData.contains(key)) {
            if (!sectionData[key].isEmpty()) {
                return sectionData[key].first();
            }
        }
    }

    // Return default value if not found
    return defaultValue;
}

int ScriptParser::getMinMerits() const {
    bool ok;
    int value = generalSection.value("minmerits", "0").toInt(&ok);
    return ok ? value : 0;
}
int ScriptParser::getMaxMerits() const {
    bool ok;
    int value = generalSection.value("maxmerits", "1000").toInt(&ok);
    return ok ? value : 1000;
}

int ScriptParser::getYellowMerits() const {
    bool ok;
    int value = generalSection.value("yellow", "800").toInt(&ok);
    return ok ? value : 800;
}

int ScriptParser::getRedMerits() const {
    bool ok;
    int value = generalSection.value("red", "400").toInt(&ok);
    return ok ? value : 400;
}

bool ScriptParser::isTestMenuEnabled() const {
    QString testMenuStr = getIniValue("General", "TestMenu", "0");

    bool enabled = (testMenuStr == "1");

    qDebug() << "TestMenu value from INI:" << testMenuStr << ", Enabled:" << enabled;

    return enabled;
}

QStringList ScriptParser::getStatusesInGroup(const QString &groupName) const {
    return statusGroups.value(groupName, QStringList());
}

bool ScriptParser::isStatusInGroup(const QString &statusName, const QString &groupName) const {
    return statusGroups.value(groupName, QStringList()).contains(statusName);
}

QMap<QString, QStringList> ScriptParser::getRawSectionData(const QString &sectionName) const {
    return rawSections.value(sectionName, QMap<QString, QStringList>());
}

QStringList ScriptParser::getRawSectionNames() const {
    return rawSections.keys();
}
