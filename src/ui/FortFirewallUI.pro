include(../global.pri)

include(FortFirewallUI-include.pri)

CONFIG -= debug_and_release
CONFIG += staticlib

TARGET = FortFirewallUILib
TEMPLATE = lib

SOURCES += \
    appinfo/appbasejob.cpp \
    appinfo/appiconjob.cpp \
    appinfo/appinfo.cpp \
    appinfo/appinfocache.cpp \
    appinfo/appinfojob.cpp \
    appinfo/appinfomanager.cpp \
    appinfo/appinfoutil.cpp \
    appinfo/appinfoworker.cpp \
    conf/addressgroup.cpp \
    conf/app.cpp \
    conf/appgroup.cpp \
    conf/confappmanager.cpp \
    conf/confmanager.cpp \
    conf/confzonemanager.cpp \
    conf/firewallconf.cpp \
    conf/inioptions.cpp \
    conf/rules/policy.cpp \
    conf/rules/policyset.cpp \
    conf/rules/rule.cpp \
    conf/zone.cpp \
    control/control.cpp \
    control/controlmanager.cpp \
    control/controlworker.cpp \
    driver/drivercommon.cpp \
    driver/drivermanager.cpp \
    driver/driverworker.cpp \
    form/basecontroller.cpp \
    form/controls/appinforow.cpp \
    form/controls/checkspincombo.cpp \
    form/controls/checktimeperiod.cpp \
    form/controls/clickablemenu.cpp \
    form/controls/combobox.cpp \
    form/controls/controlutil.cpp \
    form/controls/doublespinbox.cpp \
    form/controls/labelcolor.cpp \
    form/controls/labeldoublespin.cpp \
    form/controls/labelspin.cpp \
    form/controls/labelspincombo.cpp \
    form/controls/listview.cpp \
    form/controls/mainwindow.cpp \
    form/controls/menuwidget.cpp \
    form/controls/plaintextedit.cpp \
    form/controls/sidebutton.cpp \
    form/controls/spinbox.cpp \
    form/controls/spincombo.cpp \
    form/controls/tabbar.cpp \
    form/controls/tableview.cpp \
    form/controls/textarea2splitter.cpp \
    form/controls/textarea2splitterhandle.cpp \
    form/controls/zonesselector.cpp \
    form/dialog/dialogutil.cpp \
    form/dialog/passworddialog.cpp \
    form/graph/axistickerspeed.cpp \
    form/graph/graphplot.cpp \
    form/graph/graphwindow.cpp \
    form/home/homecontroller.cpp \
    form/home/homewindow.cpp \
    form/home/pages/aboutpage.cpp \
    form/home/pages/homebasepage.cpp \
    form/home/pages/homemainpage.cpp \
    form/home/pages/homepage.cpp \
    form/opt/optionscontroller.cpp \
    form/opt/optionswindow.cpp \
    form/opt/pages/addresses/addressescolumn.cpp \
    form/opt/pages/addressespage.cpp \
    form/opt/pages/applicationspage.cpp \
    form/opt/pages/apps/appscolumn.cpp \
    form/opt/pages/graphpage.cpp \
    form/opt/pages/optbasepage.cpp \
    form/opt/pages/optionspage.cpp \
    form/opt/pages/optmainpage.cpp \
    form/opt/pages/schedulepage.cpp \
    form/opt/pages/statisticspage.cpp \
    form/policy/policiescontroller.cpp \
    form/policy/policieswindow.cpp \
    form/policy/policyeditdialog.cpp \
    form/policy/policylistbox.cpp \
    form/prog/programalertwindow.cpp \
    form/prog/programeditdialog.cpp \
    form/prog/programscontroller.cpp \
    form/prog/programswindow.cpp \
    form/stat/pages/connectionspage.cpp \
    form/stat/pages/statbasepage.cpp \
    form/stat/pages/statmainpage.cpp \
    form/stat/pages/trafficpage.cpp \
    form/stat/statisticscontroller.cpp \
    form/stat/statisticswindow.cpp \
    form/svc/servicescontroller.cpp \
    form/svc/serviceswindow.cpp \
    form/tray/traycontroller.cpp \
    form/tray/trayicon.cpp \
    form/zone/zonescontroller.cpp \
    form/zone/zoneswindow.cpp \
    fortmanager.cpp \
    fortsettings.cpp \
    hostinfo/hostinfo.cpp \
    hostinfo/hostinfocache.cpp \
    hostinfo/hostinfojob.cpp \
    hostinfo/hostinfomanager.cpp \
    log/logbuffer.cpp \
    log/logentry.cpp \
    log/logentryblocked.cpp \
    log/logentryblockedip.cpp \
    log/logentryprocnew.cpp \
    log/logentrystattraf.cpp \
    log/logentrytime.cpp \
    log/logmanager.cpp \
    manager/dberrormanager.cpp \
    manager/drivelistmanager.cpp \
    manager/envmanager.cpp \
    manager/hotkeymanager.cpp \
    manager/logger.cpp \
    manager/nativeeventfilter.cpp \
    manager/serviceinfomanager.cpp \
    manager/servicemanager.cpp \
    manager/translationmanager.cpp \
    manager/windowmanager.cpp \
    model/applistmodel.cpp \
    model/appstatmodel.cpp \
    model/connblocklistmodel.cpp \
    model/policylistmodel.cpp \
    model/servicelistmodel.cpp \
    model/traflistmodel.cpp \
    model/zonelistmodel.cpp \
    model/zonesourcewrapper.cpp \
    model/zonetypewrapper.cpp \
    rpc/appinfomanagerrpc.cpp \
    rpc/askpendingmanagerrpc.cpp \
    rpc/confappmanagerrpc.cpp \
    rpc/confmanagerrpc.cpp \
    rpc/confzonemanagerrpc.cpp \
    rpc/drivermanagerrpc.cpp \
    rpc/logmanagerrpc.cpp \
    rpc/quotamanagerrpc.cpp \
    rpc/rpcmanager.cpp \
    rpc/serviceinfomanagerrpc.cpp \
    rpc/statblockmanagerrpc.cpp \
    rpc/statmanagerrpc.cpp \
    rpc/taskmanagerrpc.cpp \
    rpc/windowmanagerfake.cpp \
    stat/askpendingmanager.cpp \
    stat/deleteconnblockjob.cpp \
    stat/logblockedipjob.cpp \
    stat/quotamanager.cpp \
    stat/statblockbasejob.cpp \
    stat/statblockmanager.cpp \
    stat/statblockworker.cpp \
    stat/statmanager.cpp \
    stat/statsql.cpp \
    task/taskdownloader.cpp \
    task/taskeditinfo.cpp \
    task/taskinfo.cpp \
    task/taskinfoapppurger.cpp \
    task/taskinfoupdatechecker.cpp \
    task/taskinfozonedownloader.cpp \
    task/tasklistmodel.cpp \
    task/taskmanager.cpp \
    task/taskupdatechecker.cpp \
    task/taskworker.cpp \
    task/taskzonedownloader.cpp \
    user/iniuser.cpp \
    user/usersettings.cpp \
    util/conf/addressrange.cpp \
    util/conf/appparseoptions.cpp \
    util/conf/confutil.cpp \
    util/dateutil.cpp \
    util/device.cpp \
    util/fileutil.cpp \
    util/guiutil.cpp \
    util/iconcache.cpp \
    util/ini/mapsettings.cpp \
    util/ini/settings.cpp \
    util/ioc/ioccontainer.cpp \
    util/json/jsonutil.cpp \
    util/json/mapwrapper.cpp \
    util/model/stringlistmodel.cpp \
    util/model/tableitemmodel.cpp \
    util/model/tablesqlmodel.cpp \
    util/net/iprange.cpp \
    util/net/netdownloader.cpp \
    util/net/netutil.cpp \
    util/osutil.cpp \
    util/processinfo.cpp \
    util/regkey.cpp \
    util/service/serviceinfo.cpp \
    util/service/servicelistmonitor.cpp \
    util/service/servicemanageriface.cpp \
    util/service/servicemonitor.cpp \
    util/service/serviceworker.cpp \
    util/startuputil.cpp \
    util/stringutil.cpp \
    util/textareautil.cpp \
    util/triggertimer.cpp \
    util/variantutil.cpp \
    util/window/basewindowstatewatcher.cpp \
    util/window/widgetwindow.cpp \
    util/window/widgetwindowstatewatcher.cpp \
    util/window/windowstatewatcher.cpp \
    util/worker/workerjob.cpp \
    util/worker/workermanager.cpp \
    util/worker/workerobject.cpp

HEADERS += \
    appinfo/appbasejob.h \
    appinfo/appiconjob.h \
    appinfo/appinfo.h \
    appinfo/appinfocache.h \
    appinfo/appinfojob.h \
    appinfo/appinfomanager.h \
    appinfo/appinfoutil.h \
    appinfo/appinfoworker.h \
    conf/addressgroup.h \
    conf/app.h \
    conf/appgroup.h \
    conf/confappmanager.h \
    conf/confmanager.h \
    conf/confzonemanager.h \
    conf/firewallconf.h \
    conf/inioptions.h \
    conf/rules/policy.h \
    conf/rules/policyset.h \
    conf/rules/rule.h \
    conf/zone.h \
    control/control.h \
    control/controlmanager.h \
    control/controlworker.h \
    driver/drivercommon.h \
    driver/drivermanager.h \
    driver/driverworker.h \
    form/basecontroller.h \
    form/controls/appinforow.h \
    form/controls/checkspincombo.h \
    form/controls/checktimeperiod.h \
    form/controls/clickablemenu.h \
    form/controls/combobox.h \
    form/controls/controlutil.h \
    form/controls/doublespinbox.h \
    form/controls/labelcolor.h \
    form/controls/labeldoublespin.h \
    form/controls/labelspin.h \
    form/controls/labelspincombo.h \
    form/controls/listview.h \
    form/controls/mainwindow.h \
    form/controls/menuwidget.h \
    form/controls/plaintextedit.h \
    form/controls/sidebutton.h \
    form/controls/spinbox.h \
    form/controls/spincombo.h \
    form/controls/tabbar.h \
    form/controls/tableview.h \
    form/controls/textarea2splitter.h \
    form/controls/textarea2splitterhandle.h \
    form/controls/zonesselector.h \
    form/dialog/dialogutil.h \
    form/dialog/passworddialog.h \
    form/graph/axistickerspeed.h \
    form/graph/graphplot.h \
    form/graph/graphwindow.h \
    form/home/homecontroller.h \
    form/home/homewindow.h \
    form/home/pages/aboutpage.h \
    form/home/pages/homebasepage.h \
    form/home/pages/homemainpage.h \
    form/home/pages/homepage.h \
    form/opt/optionscontroller.h \
    form/opt/optionswindow.h \
    form/opt/pages/addresses/addressescolumn.h \
    form/opt/pages/addressespage.h \
    form/opt/pages/applicationspage.h \
    form/opt/pages/apps/appscolumn.h \
    form/opt/pages/graphpage.h \
    form/opt/pages/optbasepage.h \
    form/opt/pages/optionspage.h \
    form/opt/pages/optmainpage.h \
    form/opt/pages/schedulepage.h \
    form/opt/pages/statisticspage.h \
    form/policy/policiescontroller.h \
    form/policy/policieswindow.h \
    form/policy/policyeditdialog.h \
    form/policy/policylistbox.h \
    form/prog/programalertwindow.h \
    form/prog/programeditdialog.h \
    form/prog/programscontroller.h \
    form/prog/programswindow.h \
    form/stat/pages/connectionspage.h \
    form/stat/pages/statbasepage.h \
    form/stat/pages/statmainpage.h \
    form/stat/pages/trafficpage.h \
    form/stat/statisticscontroller.h \
    form/stat/statisticswindow.h \
    form/svc/servicescontroller.h \
    form/svc/serviceswindow.h \
    form/tray/traycontroller.h \
    form/tray/trayicon.h \
    form/windowtypes.h \
    form/zone/zonescontroller.h \
    form/zone/zoneswindow.h \
    fortcompat.h \
    fortmanager.h \
    fortsettings.h \
    hostinfo/hostinfo.h \
    hostinfo/hostinfocache.h \
    hostinfo/hostinfojob.h \
    hostinfo/hostinfomanager.h \
    log/logbuffer.h \
    log/logentry.h \
    log/logentryblocked.h \
    log/logentryblockedip.h \
    log/logentryprocnew.h \
    log/logentrystattraf.h \
    log/logentrytime.h \
    log/logmanager.h \
    manager/dberrormanager.h \
    manager/drivelistmanager.h \
    manager/envmanager.h \
    manager/hotkeymanager.h \
    manager/logger.h \
    manager/nativeeventfilter.h \
    manager/serviceinfomanager.h \
    manager/servicemanager.h \
    manager/translationmanager.h \
    manager/windowmanager.h \
    model/applistmodel.h \
    model/appstatmodel.h \
    model/connblocklistmodel.h \
    model/policylistmodel.h \
    model/servicelistmodel.h \
    model/traflistmodel.h \
    model/zonelistmodel.h \
    model/zonesourcewrapper.h \
    model/zonetypewrapper.h \
    rpc/appinfomanagerrpc.h \
    rpc/askpendingmanagerrpc.h \
    rpc/confappmanagerrpc.h \
    rpc/confmanagerrpc.h \
    rpc/confzonemanagerrpc.h \
    rpc/drivermanagerrpc.h \
    rpc/logmanagerrpc.h \
    rpc/quotamanagerrpc.h \
    rpc/rpcmanager.h \
    rpc/serviceinfomanagerrpc.h \
    rpc/statblockmanagerrpc.h \
    rpc/statmanagerrpc.h \
    rpc/taskmanagerrpc.h \
    rpc/windowmanagerfake.h \
    stat/askpendingmanager.h \
    stat/deleteconnblockjob.h \
    stat/logblockedipjob.h \
    stat/quotamanager.h \
    stat/statblockbasejob.h \
    stat/statblockmanager.h \
    stat/statblockworker.h \
    stat/statmanager.h \
    stat/statsql.h \
    task/taskdownloader.h \
    task/taskeditinfo.h \
    task/taskinfo.h \
    task/taskinfoapppurger.h \
    task/taskinfoupdatechecker.h \
    task/taskinfozonedownloader.h \
    task/tasklistmodel.h \
    task/taskmanager.h \
    task/taskupdatechecker.h \
    task/taskworker.h \
    task/taskzonedownloader.h \
    user/iniuser.h \
    user/usersettings.h \
    util/classhelpers.h \
    util/conf/addressrange.h \
    util/conf/appparseoptions.h \
    util/conf/confappswalker.h \
    util/conf/confutil.h \
    util/dateutil.h \
    util/device.h \
    util/fileutil.h \
    util/guiutil.h \
    util/iconcache.h \
    util/ini/mapsettings.h \
    util/ini/settings.h \
    util/ioc/ioccontainer.h \
    util/ioc/iocservice.h \
    util/json/jsonutil.h \
    util/json/mapwrapper.h \
    util/model/stringlistmodel.h \
    util/model/tableitemmodel.h \
    util/model/tablesqlmodel.h \
    util/net/iprange.h \
    util/net/netdownloader.h \
    util/net/netutil.h \
    util/osutil.h \
    util/processinfo.h \
    util/regkey.h \
    util/service/serviceinfo.h \
    util/service/servicelistmonitor.h \
    util/service/servicemanageriface.h \
    util/service/servicemonitor.h \
    util/service/serviceworker.h \
    util/startuputil.h \
    util/stringutil.h \
    util/textareautil.h \
    util/triggertimer.h \
    util/variantutil.h \
    util/window/basewindowstatewatcher.h \
    util/window/widgetwindow.h \
    util/window/widgetwindowstatewatcher.h \
    util/window/windowstatewatcher.h \
    util/worker/workerjob.h \
    util/worker/workermanager.h \
    util/worker/workerobject.h \
    util/worker/workertypes.h

# Icons, README.*
RESOURCES += \
    fort_icons.qrc \
    fort_readme.qrc

# Database Migrations
OTHER_FILES += \
    appinfo/migrations/*.sql \
    conf/migrations/*.sql \
    stat/migrations/block/*.sql \
    stat/migrations/conn/*.sql \
    stat/migrations/traf/*.sql

RESOURCES += \
    appinfo/appinfo_migrations.qrc \
    conf/conf_migrations.qrc \
    stat/stat_migrations.qrc

# Zone
OTHER_FILES += conf/zone/*.json

RESOURCES += \
    conf/conf_zone.qrc

# Driver integration
include(../driver/Driver.pri)

# 3rd party integrations
include(3rdparty/3rdparty.pri)
