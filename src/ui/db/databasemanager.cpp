#include "databasemanager.h"

#include <QDateTime>

#include "../util/fileutil.h"
#include "databasesql.h"
#include "sqlite/sqliteengine.h"
#include "sqlite/sqlitedb.h"
#include "sqlite/sqlitestmt.h"

DatabaseManager::DatabaseManager(const QString &filePath,
                                 QObject *parent) :
    QObject(parent),
    m_lastTrafHour(0),
    m_lastTrafDay(0),
    m_lastTrafMonth(0),
    m_filePath(filePath),
    m_sqliteDb(new SqliteDb())
{
    SqliteEngine::initialize();
}

DatabaseManager::~DatabaseManager()
{
    qDeleteAll(m_sqliteStmts.values());

    delete m_sqliteDb;

    SqliteEngine::shutdown();
}

bool DatabaseManager::initialize()
{
    const bool fileExists = FileUtil::fileExists(m_filePath);

    if (!m_sqliteDb->open(m_filePath))
        return false;

    m_sqliteDb->execute(DatabaseSql::sqlPragmas);

    return fileExists || createTables();
}

void DatabaseManager::addApp(const QString &appPath, bool &isNew)
{
    const qint64 appId = getAppId(appPath, isNew);

    m_appIds.append(appId);
}

void DatabaseManager::addTraffic(quint16 procCount, const quint8 *procBits,
                                 const quint32 *trafBytes)
{
    QVector<quint16> delProcIndexes;

    const qint64 trafTime = QDateTime::currentSecsSinceEpoch();

    const qint32 trafHour = qint32(trafTime / 3600);
    const bool isNewHour = (trafHour != m_lastTrafHour);

    const qint32 trafDay = isNewHour ? getUnixDay(trafTime)
                                     : m_lastTrafDay;
    const bool isNewDay = (trafDay != m_lastTrafDay);

    const qint32 trafMonth = isNewDay ? getUnixMonth(trafTime)
                                      : m_lastTrafMonth;
    const bool isNewMonth = (trafMonth != m_lastTrafMonth);

    SqliteStmt *stmtInsertAppHour = nullptr;
    SqliteStmt *stmtInsertAppDay = nullptr;
    SqliteStmt *stmtInsertAppMonth = nullptr;

    SqliteStmt *stmtInsertHour = nullptr;
    SqliteStmt *stmtInsertDay = nullptr;
    SqliteStmt *stmtInsertMonth = nullptr;

    if (isNewHour) {
        m_lastTrafHour = trafHour;

        stmtInsertAppHour = getSqliteStmt(DatabaseSql::sqlInsertTrafficAppHour);
        stmtInsertHour = getSqliteStmt(DatabaseSql::sqlInsertTrafficHour);

        stmtInsertAppHour->bindInt(1, trafHour);
        stmtInsertHour->bindInt(1, trafHour);

        if (isNewDay) {
            m_lastTrafDay = trafDay;

            stmtInsertAppDay = getSqliteStmt(DatabaseSql::sqlInsertTrafficAppDay);
            stmtInsertDay = getSqliteStmt(DatabaseSql::sqlInsertTrafficDay);

            stmtInsertAppDay->bindInt(1, trafDay);
            stmtInsertDay->bindInt(1, trafDay);

            if (isNewMonth) {
                m_lastTrafMonth = trafMonth;

                stmtInsertAppMonth = getSqliteStmt(DatabaseSql::sqlInsertTrafficAppMonth);
                stmtInsertMonth = getSqliteStmt(DatabaseSql::sqlInsertTrafficMonth);

                stmtInsertAppMonth->bindInt(1, trafMonth);
                stmtInsertMonth->bindInt(1, trafMonth);
            }
        }
    }

    SqliteStmt *stmtUpdateAppHour = getSqliteStmt(DatabaseSql::sqlUpdateTrafficAppHour);
    SqliteStmt *stmtUpdateAppDay = getSqliteStmt(DatabaseSql::sqlUpdateTrafficAppDay);
    SqliteStmt *stmtUpdateAppMonth = getSqliteStmt(DatabaseSql::sqlUpdateTrafficAppMonth);

    SqliteStmt *stmtUpdateHour = getSqliteStmt(DatabaseSql::sqlUpdateTrafficHour);
    SqliteStmt *stmtUpdateDay = getSqliteStmt(DatabaseSql::sqlUpdateTrafficDay);
    SqliteStmt *stmtUpdateMonth = getSqliteStmt(DatabaseSql::sqlUpdateTrafficMonth);

    SqliteStmt *stmtUpdateAppTotal = getSqliteStmt(DatabaseSql::sqlUpdateTrafficAppTotal);

    stmtUpdateAppHour->bindInt(1, trafHour);
    stmtUpdateAppDay->bindInt(1, trafDay);
    stmtUpdateAppMonth->bindInt(1, trafMonth);

    stmtUpdateHour->bindInt(1, trafHour);
    stmtUpdateDay->bindInt(1, trafDay);
    stmtUpdateMonth->bindInt(1, trafMonth);

    m_sqliteDb->beginTransaction();

    for (quint16 procIndex = 0; procIndex < procCount; ++procIndex) {
        const bool active = procBits[procIndex / 8] & (1 << (procIndex & 7));
        if (!active) {
            delProcIndexes.append(procIndex);
        }

        const quint32 *procTrafBytes = &trafBytes[procIndex * 2];
        const quint32 inBytes = procTrafBytes[0];
        const quint32 outBytes = procTrafBytes[1];

        if (!(isNewHour || inBytes || outBytes))
            continue;

        const qint64 appId = m_appIds.at(procIndex);

        // Insert zero bytes
        if (isNewHour) {
            insertTraffic(stmtInsertAppHour, appId);
            insertTraffic(stmtInsertHour);

            if (isNewDay) {
                insertTraffic(stmtInsertAppDay, appId);
                insertTraffic(stmtInsertDay);

                if (isNewMonth) {
                    insertTraffic(stmtInsertAppMonth, appId);
                    insertTraffic(stmtInsertMonth);
                }
            }
        }

        // Update bytes
        updateTraffic(stmtUpdateAppHour, inBytes, outBytes, appId);
        updateTraffic(stmtUpdateAppDay, inBytes, outBytes, appId);
        updateTraffic(stmtUpdateAppMonth, inBytes, outBytes, appId);

        updateTraffic(stmtUpdateHour, inBytes, outBytes);
        updateTraffic(stmtUpdateDay, inBytes, outBytes);
        updateTraffic(stmtUpdateMonth, inBytes, outBytes);

        // Update total bytes
        stmtUpdateAppTotal->bindInt64(1, appId);
        updateTraffic(stmtUpdateAppTotal, inBytes, outBytes);
    }

    m_sqliteDb->commitTransaction();

    // Delete inactive processes
    {
        int i = delProcIndexes.size();
        while (--i >= 0) {
            const quint16 procIndex = delProcIndexes.at(i);
            m_appIds.removeAt(procIndex);
        }
    }
}

bool DatabaseManager::createTables()
{
    m_sqliteDb->beginTransaction();

    const bool res = m_sqliteDb->execute(DatabaseSql::sqlCreateTables);

    m_sqliteDb->commitTransaction();

    return res;
}

qint64 DatabaseManager::getAppId(const QString &appPath, bool &isNew)
{
    qint64 appId = 0;

    // Check existing
    {
        SqliteStmt *stmt = getSqliteStmt(DatabaseSql::sqlSelectAppId);

        stmt->bindText(1, appPath);
        if (stmt->step() == SqliteStmt::StepRow) {
            appId = stmt->columnInt64();
        }
        stmt->reset();
    }

    // Create new one
    if (!appId) {
        SqliteStmt *stmt = getSqliteStmt(DatabaseSql::sqlInsertAppId);
        const qint64 unixTime = QDateTime::currentSecsSinceEpoch();

        stmt->bindText(1, appPath);
        stmt->bindInt64(2, unixTime);
        stmt->bindInt64(3, unixTime);

        if (stmt->step() == SqliteStmt::StepDone) {
            appId = m_sqliteDb->lastInsertRowid();
            isNew = true;
        }
        stmt->reset();
    }

    return appId;
}

void DatabaseManager::getAppList(QStringList &list)
{
    SqliteStmt *stmt = getSqliteStmt(DatabaseSql::sqlSelectAppPaths);

    while (stmt->step() == SqliteStmt::StepRow) {
        list.append(stmt->columnText());
    }
    stmt->reset();
}

SqliteStmt *DatabaseManager::getSqliteStmt(const char *sql)
{
    SqliteStmt *stmt = m_sqliteStmts.value(sql);

    if (stmt == nullptr) {
        stmt = new SqliteStmt();
        stmt->prepare(m_sqliteDb->db(), sql, SqliteStmt::PreparePersistent);

        m_sqliteStmts.insert(sql, stmt);
    }

    return stmt;
}

void DatabaseManager::insertTraffic(SqliteStmt *stmt, qint64 appId)
{
    if (appId != 0) {
        stmt->bindInt64(2, appId);
    }

    stmt->step();
    stmt->reset();
}

void DatabaseManager::updateTraffic(SqliteStmt *stmt, quint32 inBytes,
                                    quint32 outBytes, qint64 appId)
{
    stmt->bindInt64(2, inBytes);
    stmt->bindInt64(3, outBytes);

    if (appId != 0) {
        stmt->bindInt64(4, appId);
    }

    stmt->step();
    stmt->reset();
}

qint32 DatabaseManager::getUnixDay(qint64 unixTime)
{
    const QDate date = QDateTime::fromSecsSinceEpoch(unixTime).date();

    return qint32(QDateTime(date).toSecsSinceEpoch() / 3600);
}

qint32 DatabaseManager::getUnixMonth(qint64 unixTime)
{
    const QDate date = QDateTime::fromSecsSinceEpoch(unixTime).date();

    return qint32(QDateTime(QDate(date.year(), date.month(), 1))
                  .toSecsSinceEpoch() / 3600);
}
