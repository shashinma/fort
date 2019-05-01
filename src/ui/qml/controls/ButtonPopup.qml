import QtQuick 2.13
import QtQuick.Controls 2.13

Button {
    id: bt

    checkable: true

    default property alias content: popup.contentData

    Popup {
        id: popup
        y: bt.height
        visible: bt.checked

        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
                     | Popup.CloseOnPressOutsideParent

        onClosed: {
            bt.checked = false;
        }
    }
}
