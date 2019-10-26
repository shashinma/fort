#include "confmanager.h"

#include <QLoggingCategory>

#include <sqlite/sqlitedb.h>
#include <sqlite/sqlitestmt.h>

#include "../fortcommon.h"
#include "../fortsettings.h"
#include "../util/dateutil.h"
#include "../util/fileutil.h"
#include "../util/net/netutil.h"
#include "../util/osutil.h"
#include "addressgroup.h"
#include "appgroup.h"
#include "firewallconf.h"

Q_DECLARE_LOGGING_CATEGORY(CLOG_CONF_MANAGER)
Q_LOGGING_CATEGORY(CLOG_CONF_MANAGER, "fort.confManager")

#define logWarning() qCWarning(CLOG_CONF_MANAGER,)
#define logCritical() qCCritical(CLOG_CONF_MANAGER,)

#define DATABASE_USER_VERSION   1

namespace {

const char * const sqlPragmas =
        "PRAGMA journal_mode = WAL;"
        "PRAGMA locking_mode = EXCLUSIVE;"
        "PRAGMA synchronous = NORMAL;"
        ;

const char * const sqlSelectAddressGroups =
        "SELECT addr_group_id, include_all, exclude_all,"
        "    include_text, exclude_text"
        "  FROM address_group"
        "  ORDER BY order_index;"
        ;

const char * const sqlSelectAppGroups =
        "SELECT app_group_id, enabled,"
        "    fragment_packet, period_enabled,"
        "    limit_in_enabled, limit_out_enabled,"
        "    speed_limit_in, speed_limit_out,"
        "    name, block_text, allow_text,"
        "    period_from, period_to"
        "  FROM app_group"
        "  ORDER BY order_index;"
        ;

const char * const sqlInsertAddressGroup =
        "INSERT INTO address_group(addr_group_id, order_index,"
        "    include_all, exclude_all, include_text, exclude_text)"
        "  VALUES(?1, ?2, ?3, ?4, ?5, ?6);"
        ;

const char * const sqlUpdateAddressGroup =
        "UPDATE address_group"
        "  SET order_index = ?2,"
        "    include_all = ?3, exclude_all = ?4,"
        "    include_text = ?5, exclude_text = ?6"
        "  WHERE addr_group_id = ?1;"
        ;

const char * const sqlInsertAppGroup =
        "INSERT INTO app_group(app_group_id, order_index, enabled,"
        "    fragment_packet, period_enabled,"
        "    limit_in_enabled, limit_out_enabled,"
        "    speed_limit_in, speed_limit_out,"
        "    name, block_text, allow_text,"
        "    period_from, period_to)"
        "  VALUES(?1, ?2, ?3, ?4, ?5, ?6, ?7, ?8, ?9, ?10, ?11, ?12, ?13, ?14);"
        ;

const char * const sqlUpdateAppGroup =
        "UPDATE app_group"
        "  SET order_index = ?2, enabled = ?3,"
        "    fragment_packet = ?4, period_enabled = ?5,"
        "    limit_in_enabled = ?6, limit_out_enabled = ?7,"
        "    speed_limit_in = ?8, speed_limit_out = ?9,"
        "    name = ?10, block_text = ?11, allow_text = ?12,"
        "    period_from = ?13, period_to = ?14"
        "  WHERE app_group_id = ?1;"
        ;

}

ConfManager::ConfManager(FortSettings *fortSettings,
                         QObject *parent) :
    QObject(parent),
    m_fortSettings(fortSettings),
    m_sqliteDb(new SqliteDb(fortSettings->confFilePath()))
{
}

ConfManager::~ConfManager()
{
    delete m_sqliteDb;
}

void ConfManager::setErrorMessage(const QString &errorMessage)
{
    if (m_errorMessage != errorMessage) {
        m_errorMessage = errorMessage;
        emit errorMessageChanged();
    }
}

bool ConfManager::initialize()
{
    if (!m_sqliteDb->open()) {
        logCritical() << "File open error:"
                      << m_sqliteDb->filePath()
                      << m_sqliteDb->errorMessage();
        return false;
    }

    m_sqliteDb->execute(sqlPragmas);

    if (!m_sqliteDb->migrate(":/conf/migrations", DATABASE_USER_VERSION)) {
        logCritical() << "Migration error"
                      << m_sqliteDb->filePath();
        return false;
    }

    return true;
}

FirewallConf *ConfManager::cloneConf(const FirewallConf &conf,
                                     QObject *parent)
{
    auto newConf = new FirewallConf(parent);

    newConf->copy(conf);

    return newConf;
}

void ConfManager::setupDefault(FirewallConf &conf) const
{
    AddressGroup *inetGroup = conf.inetAddressGroup();
    inetGroup->setExcludeText(NetUtil::localIpv4Networks().join('\n'));

    auto appGroup = new AppGroup();
    appGroup->setName("Main");
    appGroup->setAllowText(FileUtil::appBinLocation() + '/');
    conf.addAppGroup(appGroup);
}

bool ConfManager::load(FirewallConf &conf)
{
    bool isNewConf = true;

    if (!loadFromDb(conf, isNewConf)) {
        setErrorMessage(m_sqliteDb->errorMessage());
        return false;
    }

    if (isNewConf && !m_fortSettings->readConf(conf, isNewConf)) {
        setErrorMessage(m_fortSettings->errorMessage());
        return false;
    }

    if (isNewConf) {
        setupDefault(conf);
    }

    return true;
}

bool ConfManager::save(const FirewallConf &conf, bool onlyFlags)
{
    if (!onlyFlags && !saveToDb(conf))
        return false;

    if (!m_fortSettings->writeConfIni(conf)) {
        setErrorMessage(m_fortSettings->errorMessage());
        return false;
    }

    // Remove old JSON config.
    FileUtil::removeFile(m_fortSettings->confOldFilePath());
    FileUtil::removeFile(m_fortSettings->confBackupFilePath());

    return true;
}

bool ConfManager::loadFromDb(FirewallConf &conf, bool &isNew)
{
    // Load Address Groups
    {
        SqliteStmt stmt;
        if (!stmt.prepare(m_sqliteDb->db(), sqlSelectAddressGroups))
            return false;

        int index = 0;
        while (stmt.step() == SqliteStmt::StepRow) {
            auto addrGroup = conf.addressGroupsList().at(index);
            Q_ASSERT(addrGroup != nullptr);

            addrGroup->setId(stmt.columnInt64(0));
            addrGroup->setIncludeAll(stmt.columnInt(1));
            addrGroup->setExcludeAll(stmt.columnInt(2));
            addrGroup->setIncludeText(stmt.columnText(3));
            addrGroup->setExcludeText(stmt.columnText(4));

            if (++index > 1)
                break;
        }

        if (index == 0) {
            isNew = true;
            return true;
        }
        isNew = false;
    }

    // Load App Groups
    {
        SqliteStmt stmt;
        if (!stmt.prepare(m_sqliteDb->db(), sqlSelectAppGroups))
            return false;

        while (stmt.step() == SqliteStmt::StepRow) {
            auto appGroup = new AppGroup();

            appGroup->setId(stmt.columnInt64(0));
            appGroup->setEnabled(stmt.columnInt(1));
            appGroup->setFragmentPacket(stmt.columnInt(2));
            appGroup->setPeriodEnabled(stmt.columnInt(3));
            appGroup->setLimitInEnabled(stmt.columnInt(4));
            appGroup->setLimitOutEnabled(stmt.columnInt(5));
            appGroup->setSpeedLimitIn(quint32(stmt.columnInt(6)));
            appGroup->setSpeedLimitOut(quint32(stmt.columnInt(7)));
            appGroup->setName(stmt.columnText(8));
            appGroup->setBlockText(stmt.columnText(9));
            appGroup->setAllowText(stmt.columnText(10));
            appGroup->setPeriodFrom(stmt.columnText(11));
            appGroup->setPeriodTo(stmt.columnText(12));

            conf.addAppGroup(appGroup);
        }
    }

    return true;
}

bool ConfManager::saveToDb(const FirewallConf &conf)
{
    bool ok = true;

    m_sqliteDb->beginTransaction();

    // Save Address Groups
    int orderIndex = 0;
    for (AddressGroup *addrGroup : conf.addressGroupsList()) {
        const bool rowExists = (addrGroup->id() != 0);
        if (!addrGroup->edited() && rowExists)
            continue;

        const QVariantList vars = QVariantList()
                << (rowExists ? addrGroup->id() : QVariant())
                << orderIndex++
                << addrGroup->includeAll()
                << addrGroup->excludeAll()
                << addrGroup->includeText()
                << addrGroup->excludeText()
                   ;

        const char *sql = rowExists ? sqlUpdateAddressGroup : sqlInsertAddressGroup;

        m_sqliteDb->executeEx(sql, vars, 0, &ok);
        if (!ok) goto end;

        if (!rowExists) {
            addrGroup->setId(m_sqliteDb->lastInsertRowid());
        }
        addrGroup->setEdited(false);
    }

    // Save App Groups
    orderIndex = 0;
    for (AppGroup *appGroup : conf.appGroupsList()) {
        const bool rowExists = (appGroup->id() != 0);
        if (!appGroup->edited() && rowExists)
            continue;

        const QVariantList vars = QVariantList()
                << (rowExists ? appGroup->id() : QVariant())
                << orderIndex++
                << appGroup->enabled()
                << appGroup->fragmentPacket()
                << appGroup->periodEnabled()
                << appGroup->limitInEnabled()
                << appGroup->limitOutEnabled()
                << appGroup->speedLimitIn()
                << appGroup->speedLimitOut()
                << appGroup->name()
                << appGroup->blockText()
                << appGroup->allowText()
                << appGroup->periodFrom()
                << appGroup->periodTo()
                   ;

        const char *sql = rowExists ? sqlUpdateAppGroup : sqlInsertAppGroup;

        m_sqliteDb->executeEx(sql, vars, 0, &ok);
        if (!ok) goto end;

        if (!rowExists) {
            appGroup->setId(m_sqliteDb->lastInsertRowid());
        }
        appGroup->setEdited(false);
    }

 end:
    if (!ok) {
        setErrorMessage(m_sqliteDb->errorMessage());
    }

    m_sqliteDb->endTransaction(ok);

    return ok;
}
