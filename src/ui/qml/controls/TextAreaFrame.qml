import QtQuick 2.13
import QtQuick.Controls 2.13

Frame {
    id: frame

    signal textChanged()  // Workaround for QTBUG-59908

    readonly property alias textArea: textArea

    padding: 0

    ScrollView {
        anchors.fill: parent

        TextArea {
            id: textArea
            clip: true  // to clip placeholder text
            persistentSelection: true
            selectByMouse: true
            hoverEnabled: false
            onTextChanged: frame.textChanged()
            onReleased: textContextMenu.show(event, textArea)
        }
    }
}
