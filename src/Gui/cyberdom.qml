import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

ApplicationWindow {
    visible: true
    width: 800
    height: 600
    title: qsTr("CyberDom")

    // --- Menu Bar ---
    menuBar: MenuBar {
        Menu {
            title: qsTr("File")
            MenuItem { text: qsTr("Make A New Report File") }
            MenuItem { text: qsTr("View Report File") }
            MenuItem { text: qsTr("Reset Application Data") }
            MenuSeparator {}
            MenuItem { text: qsTr("Options") }
            MenuItem { text: qsTr("Exit"); onTriggered: Qt.quit() }
        }
        Menu {
            title: qsTr("Communication")
            Menu { title: qsTr("Report") }
            Menu { title: qsTr("Confess") }
            Menu { title: qsTr("Ask Permission") }
            MenuItem { text: qsTr("Report Clothing") }
            MenuItem { text: qsTr("Ask for Clothing Instructions") }
            MenuItem { text: qsTr("Ask for Other Instructions") }
        }
        Menu {
            title: qsTr("Assignments")
            MenuItem { text: qsTr("Assignments") }
            MenuItem { text: qsTr("Calendar") }
        }
        Menu {
            title: qsTr("Rules")
            MenuItem { text: qsTr("Permission") }
            MenuItem { text: qsTr("Reports") }
            MenuItem { text: qsTr("Confessions") }
            MenuItem { text: qsTr("Instructions") }
            MenuItem { text: qsTr("Other Rules") }
        }
        Menu {
            title: qsTr("Test")
            MenuItem { text: qsTr("Time Add") }
            MenuItem { text: qsTr("Randomize") }
        }
        Menu {
            title: qsTr("Help")
            MenuItem { text: qsTr("About") }
        }
    }

    // --- Main Layout ---
    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 20
        spacing: 15

        // Status Header
        Label {
            text: qsTr("Status:")
            font.pointSize: 14
            font.bold: true
            color: palette.text
        }

        // Scrollable Status Area
        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true

            background: Rectangle {
                color: palette.base
                border.color: palette.mid
                radius: 4
            }

            TextArea {
                id: statusLabel
                text: qsTr("TextLabel")
                readOnly: true
                wrapMode: Text.WordWrap
                selectByMouse: true
                padding: 10
            }
        }

        // --- Info Section (Merits & Time)
        RowLayout {
            Layout.fillWidth: true
            spacing: 20

            Label {
                text: qsTr("Merits:")
                font.pointSize: 14
                font.bold: true
            }

            Item {
                Layout.fillWidth: true
            }

            GridLayout {
                columns: 2
                rowSpacing: 5
                columnSpacing: 10

                Label {
                    text: qsTr("Clock:")
                    Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
                }
                Label {
                    id: clockLabel
                    text: qsTr("00:00:00")
                    font.family: "Monospace"
                    Layout.minimumWidth: 80
                    Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
                }

                Label {
                    text: qsTr("Timer:")
                    Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
                }
                Label {
                    id: timerLabel
                    text: qsTr("00:00:00")
                    font.family: "Monospace"
                    Layout.minimumWidth: 80
                    Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
                }
            }
        }

        // --- Controls Section ---
        RowLayout {
            Layout.fillWidth: true
            spacing: 15

            ProgressBar {
                id: progressBar
                Layout.fillWidth: true
                value: 0.0
            }

            Button {
                id: resetTimer
                text: qsTr("Reset")
                Layout.preferredWidth: 100
            }
        }
    }
}
