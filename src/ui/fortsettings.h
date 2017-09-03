#ifndef FORTSETTINGS_H
#define FORTSETTINGS_H

#include <QObject>
#include <QSettings>

class FirewallConf;

class FortSettings : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool debug READ debug WRITE setDebug NOTIFY iniChanged)
    Q_PROPERTY(QString language READ language WRITE setLanguage NOTIFY iniChanged)
    Q_PROPERTY(QString updatesUrl READ updatesUrl WRITE setUpdatesUrl NOTIFY iniChanged)
    Q_PROPERTY(bool startWithWindows READ startWithWindows WRITE setStartWithWindows NOTIFY startWithWindowsChanged)
    Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY errorMessageChanged)

public:
    explicit FortSettings(const QStringList &args,
                          QObject *parent = nullptr);

    bool boot() const { return m_boot; }

    bool debug() const { return iniBool("base/debug"); }
    void setDebug(bool on) { setIniValue("base/debug", on); }

    QString language() const { return iniText("base/language", "en"); }
    void setLanguage(const QString &v) { setIniValue("base/language", v, "en"); }

    QString updatesUrl() const { return iniText("base/updatesUrl"); }
    void setUpdatesUrl(const QString &v) { setIniValue("base/updatesUrl", v); }

    bool startWithWindows() const;
    void setStartWithWindows(bool start);

    QString errorMessage() const { return m_errorMessage; }

signals:
    void iniChanged();
    void startWithWindowsChanged();
    void errorMessageChanged();

public slots:
    QString confFilePath() const;
    QString confBackupFilePath() const;

    bool readConf(FirewallConf &conf);
    bool writeConf(const FirewallConf &conf);

    bool readConfFlags(FirewallConf &conf) const;
    bool writeConfFlags(const FirewallConf &conf);

private:
    void processArguments(const QStringList &args);
    void setupIni();

    void setErrorMessage(const QString &errorMessage);

    bool tryToReadConf(FirewallConf &conf, const QString &filePath);
    bool tryToWriteConf(const FirewallConf &conf, const QString &filePath);

    bool iniBool(const QString &key, bool defaultValue = false) const;
    int iniInt(const QString &key, int defaultValue = 0) const;
    int iniUInt(const QString &key, int defaultValue = 0) const;
    int iniReal(const QString &key, qreal defaultValue = 0) const;
    QString iniText(const QString &key, const QString &defaultValue = QString()) const;
    QStringList iniList(const QString &key) const;

    QVariant iniValue(const QString &key,
                      const QVariant &defaultValue = QVariant()) const;
    void setIniValue(const QString &key, const QVariant &value,
                     const QVariant &defaultValue = QVariant());

    static QString startupShortcutPath();

private:
    uint m_boot     : 1;

    QString m_profilePath;

    QString m_errorMessage;

    QSettings *m_ini;
};

#endif // FORTSETTINGS_H
