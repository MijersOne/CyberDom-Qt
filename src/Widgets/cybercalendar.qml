import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root
    width: 800
    height: 600
    color: "#1e1e2e" 

    property int gameYear: new Date().getFullYear()
    property int gameMonth: new Date().getMonth()
    property int gameDay: new Date().getDate()
    
    property int currentYear: gameYear
    property int currentMonth: gameMonth
    
    property var eventList: []
    
    property var selectedDate: new Date(gameYear, gameMonth, gameDay)

    RowLayout {
        anchors.fill: parent
        anchors.margins: 20
        spacing: 20

        // ==========================================
        // LEFT SIDE: THE CALENDAR GRID
        // ==========================================
        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 10

            // --- TOP HEADER ---
            RowLayout {
                Layout.fillWidth: true
                Button {
                    text: "<"
                    onClicked: {
                        if (root.currentMonth === 0) { root.currentMonth = 11; root.currentYear-- }
                        else { root.currentMonth-- }
                    }
                }
                Text {
                    Layout.fillWidth: true
                    horizontalAlignment: Text.AlignHCenter
                    text: ["January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December"][root.currentMonth] + " " + root.currentYear
                    color: "#cdd6f4"
                    font.pixelSize: 24
                    font.bold: true
                }
                Button {
                    text: ">"
                    onClicked: {
                        if (root.currentMonth === 11) { root.currentMonth = 0; root.currentYear++ }
                        else { root.currentMonth++ }
                    }
                }
            }

            // --- DAYS OF WEEK ---
            DayOfWeekRow {
                Layout.fillWidth: true
                locale: Qt.locale("en_US")
                delegate: Text {
                    text: model.shortName
                    color: "#a6adc8"
                    font.bold: true
                    horizontalAlignment: Text.AlignHCenter
                }
            }

            // --- CALENDAR GRID ---
            MonthGrid {
                id: grid
                Layout.fillWidth: true
                Layout.fillHeight: true
                month: root.currentMonth
                year: root.currentYear
                locale: Qt.locale("en_US")

                delegate: Rectangle {
                    id: cellRect
                    
                    property int cellYear: model.year
                    property int cellMonth: model.month
                    property int cellDay: model.day
                    
                    property bool isGameToday: (cellYear === root.gameYear && cellMonth === root.gameMonth && cellDay === root.gameDay)
                    property bool isSelected: (cellYear === root.selectedDate.getFullYear() && cellMonth === root.selectedDate.getMonth() && cellDay === root.selectedDate.getDate())
                    
                    color: isGameToday ? "#3498db" : (cellMonth === grid.month ? "#313244" : "#181825")
                    radius: 8 
                    
                    border.color: isSelected ? "white" : "#45475a"
                    border.width: isSelected ? 2 : 1
                    clip: true 

                    MouseArea {
                        anchors.fill: parent
                        onClicked: {
                            root.selectedDate = new Date(cellRect.cellYear, cellRect.cellMonth, cellRect.cellDay)
                        }
                    }

                    Text {
                        id: dayText 
                        anchors.top: parent.top
                        anchors.horizontalCenter: parent.horizontalCenter
                        anchors.topMargin: 5
                        text: cellRect.cellDay
                        color: isGameToday ? "white" : (cellRect.cellMonth === grid.month ? "#cdd6f4" : "#6c7086")
                        font.bold: isGameToday
                        font.pixelSize: 16
                    }

                    // --- REAL EVENT BARS ---
                    Column {
                        id: eventColumn
                        anchors.top: dayText.bottom 
                        anchors.topMargin: 2
                        anchors.bottom: parent.bottom
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.margins: 4
                        spacing: 2
                        clip: true 
                        
                        Repeater {
                            model: root.eventList.filter(function(e) {
                                var cellTime = new Date(cellRect.cellYear, cellRect.cellMonth, cellRect.cellDay).getTime();
                                var startTime = new Date(e.startYear, e.startMonth, e.startDay).getTime();
                                var endTime = new Date(e.endYear, e.endMonth, e.endDay).getTime();
                                return cellTime >= startTime && cellTime <= endTime;
                            })
                            
                            delegate: Rectangle {
                                width: parent.width
                                height: 18
                                radius: 4
                                color: {
                                    if (modelData.eventType === "Punishment") return "#e74c3c"
                                    if (modelData.eventType === "Job") return '#109030'
                                    if (modelData.eventType === "Holiday") return '#2591c7'
                                    if (modelData.eventType === "Birthday") return "#9b59b6"
                                    return "#f1c40f" 
                                }
                                Text {
                                    anchors.centerIn: parent
                                    // FIXED: Safer text fallback check
                                    text: modelData.eventTitle ? modelData.eventTitle : "Unnamed " + modelData.eventType
                                    color: "white"
                                    font.pixelSize: 10
                                    font.bold: true
                                    elide: Text.ElideRight 
                                    width: parent.width - 4 
                                    horizontalAlignment: Text.AlignHCenter
                                }
                            }
                        }
                    }
                }
            }
        }

        // ==========================================
        // RIGHT SIDE: THE EVENTS VIEWER
        // ==========================================
        Rectangle {
            Layout.preferredWidth: 260
            Layout.fillHeight: true
            color: "#181825"
            radius: 10
            border.color: "#313244"
            
            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 15
                spacing: 15
                
                Text {
                    text: Qt.formatDate(root.selectedDate, "dddd, MMM d")
                    color: "#cdd6f4"
                    font.pixelSize: 20
                    font.bold: true
                    Layout.fillWidth: true
                }
                
                Rectangle {
                    Layout.fillWidth: true
                    height: 1
                    color: "#45475a"
                }
                
                ScrollView {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    
                    ColumnLayout {
                        width: parent.width
                        spacing: 10
                        
                        Repeater {
                            id: viewerRepeater
                            model: root.eventList.filter(function(e) {
                                var st = new Date(e.startYear, e.startMonth, e.startDay).getTime();
                                var et = new Date(e.endYear, e.endMonth, e.endDay).getTime();
                                var clickedTime = root.selectedDate.getTime();
                                return clickedTime >= st && clickedTime <= et;
                            })
                            
                            delegate: Rectangle {
                                Layout.fillWidth: true
                                height: 60
                                color: "#313244"
                                radius: 6
                                
                                Rectangle {
                                    width: 6
                                    anchors.left: parent.left
                                    anchors.top: parent.top
                                    anchors.bottom: parent.bottom
                                    radius: 4
                                    color: {
                                        if (modelData.eventType === "Punishment") return "#e74c3c"
                                        if (modelData.eventType === "Job") return '#109030'
                                        if (modelData.eventType === "Holiday") return '#2591c7'
                                        if (modelData.eventType === "Birthday") return "#9b59b6"
                                        return "#f1c40f"
                                    }
                                }
                                
                                Column {
                                    anchors.fill: parent
                                    anchors.margins: 10
                                    anchors.leftMargin: 15 
                                    
                                    Text {
                                        // FIXED: Safer text fallback check
                                        text: modelData.eventTitle ? modelData.eventTitle : "Unnamed " + modelData.eventType
                                        color: "white"
                                        font.bold: true
                                        font.pixelSize: 14
                                        elide: Text.ElideRight
                                        width: parent.width
                                    }
                                    Text {
                                        // FIXED: Point to eventType instead of type
                                        text: modelData.eventTitle ? modelData.eventTitle : "Unknown Event"
                                        color: "#a6adc8"
                                        font.pixelSize: 12
                                    }
                                }
                            }
                        }
                        
                        Text {
                            visible: viewerRepeater.count === 0
                            text: "No events scheduled."
                            color: "#6c7086"
                            font.pixelSize: 14
                            font.italic: true
                            Layout.alignment: Qt.AlignHCenter
                            Layout.topMargin: 20
                        }
                    }
                }
            }
        }
    }
}