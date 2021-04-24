#include "appinforow.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>

#include "../../appinfo/appinfocache.h"
#include "../../util/guiutil.h"
#include "../../util/osutil.h"
#include "controlutil.h"

AppInfoRow::AppInfoRow(QWidget *parent) : QWidget(parent)
{
    setupUi();
}

void AppInfoRow::retranslateUi()
{
    m_btAppCopyPath->setToolTip(tr("Copy Path"));
    m_btAppOpenFolder->setToolTip(tr("Open Folder"));
}

void AppInfoRow::setupUi()
{
    auto layout = new QHBoxLayout();
    layout->setContentsMargins(0, 0, 0, 0);

    m_btAppCopyPath = ControlUtil::createLinkButton(":/icons/copy.png");
    m_btAppOpenFolder = ControlUtil::createLinkButton(":/icons/folder-open.png");

    m_lineAppPath = ControlUtil::createLineLabel();

    m_labelAppProductName = ControlUtil::createLabel();
    m_labelAppProductName->setFont(ControlUtil::fontDemiBold());

    m_labelAppCompanyName = ControlUtil::createLabel();

    connect(m_btAppCopyPath, &QAbstractButton::clicked, this,
            [&] { GuiUtil::setClipboardData(m_lineAppPath->text()); });
    connect(m_btAppOpenFolder, &QAbstractButton::clicked, this,
            [&] { OsUtil::openFolder(m_lineAppPath->text()); });

    layout->addWidget(m_btAppCopyPath);
    layout->addWidget(m_btAppOpenFolder);
    layout->addWidget(m_lineAppPath, 1);
    layout->addWidget(m_labelAppProductName);
    layout->addWidget(m_labelAppCompanyName);

    setLayout(layout);
}

void AppInfoRow::refreshAppInfoVersion(const QString &appPath, AppInfoCache *appInfoCache)
{
    const auto appInfo = appInfoCache->appInfo(appPath);

    m_lineAppPath->setText(appPath);
    m_lineAppPath->setToolTip(appPath);

    m_labelAppProductName->setVisible(!appInfo.productName.isEmpty());
    m_labelAppProductName->setText(appInfo.productName + " v" + appInfo.productVersion);

    m_labelAppCompanyName->setVisible(!appInfo.companyName.isEmpty());
    m_labelAppCompanyName->setText(appInfo.companyName);
}
