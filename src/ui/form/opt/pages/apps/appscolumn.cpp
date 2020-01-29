#include "appscolumn.h"

#include <QLabel>
#include <QVBoxLayout>

#include "../../../controls/controlutil.h"
#include "../../../controls/plaintextedit.h"

AppsColumn::AppsColumn(QWidget *parent) :
    QWidget(parent)
{
    setupUi();
}

void AppsColumn::setupUi()
{
    auto layout = new QVBoxLayout();
    layout->setMargin(0);

    // Header
    auto headerLayout = new QHBoxLayout();
    layout->addLayout(headerLayout);

    m_icon = ControlUtil::createLabel();
    m_labelTitle = ControlUtil::createLabel();
    m_labelTitle->setFont(ControlUtil::fontDemiBold());

    headerLayout->addWidget(m_icon);
    headerLayout->addWidget(m_labelTitle, 1);

    // Text Area
    m_editText = new PlainTextEdit();
    m_editText->setTabChangesFocus(true);
    layout->addWidget(m_editText);

    this->setLayout(layout);
}
