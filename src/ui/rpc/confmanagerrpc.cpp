#include "confmanagerrpc.h"

#include <sqlite/sqlitedb.h>

#include "../conf/firewallconf.h"
#include "../fortmanager.h"
#include "../rpc/rpcmanager.h"

ConfManagerRpc::ConfManagerRpc(const QString &filePath, FortManager *fortManager, QObject *parent) :
    ConfManager(filePath, fortManager, parent, SqliteDb::OpenDefaultReadOnly)
{
}

RpcManager *ConfManagerRpc::rpcManager() const
{
    return fortManager()->rpcManager();
}

void ConfManagerRpc::onConfChanged(int confVersion, bool onlyFlags)
{
    if (this->confVersion() == confVersion)
        return;

    // TODO: reload conf

    setConfVersion(confVersion);

    emit confChanged(onlyFlags);
}

void ConfManagerRpc::setupAppEndTimer() { }

bool ConfManagerRpc::saveToDbIni(FirewallConf &newConf, bool onlyFlags)
{
    rpcManager()->invokeOnServer(Control::Rpc_ConfManager_save,
            { newConf.toVariant(onlyFlags), onlyFlags, confVersion() });
    // TODO: get result
    // showErrorMessage("Save Conf: Service error.");
    return true;
}
