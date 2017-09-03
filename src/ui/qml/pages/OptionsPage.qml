import QtQuick 2.9
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.2
import com.fortfirewall 1.0

BasePage {

    function onSaved() {  // overload
        fortManager.startWithWindows = cbStart.checked;
    }

    Column {
        anchors.fill: parent

        CheckBox {
            id: cbStart
            text: QT_TRANSLATE_NOOP("qml", "Start with Windows")
            checked: fortManager.startWithWindows
        }
        CheckBox {
            id: cbFilter
            text: QT_TRANSLATE_NOOP("qml", "Filtering enabled")
            checked: firewallConf.filterEnabled
            onToggled: {
                firewallConf.filterEnabled = checked;
            }
        }
    }
}
