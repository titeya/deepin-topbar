#include "mainpanel.h"
#include "../modules/account/accountplugin.h"
#include "../modules/indicator/indicatorplugin.h"
#include "../modules/network/networkplugin.h"
#include "../modules/power/powerplugin.h"
#include "../modules/sound/soundplugin.h"
#include "../modules/timewidget/datetimeplugin.h"
#include "../modules/system-tray/systemtrayplugin.h"
#include "../modules/systeminfo/systeminfoplugin.h"
#include "../modules/notify/notifyplugin.h"
#include "../modules/search/searchmodule.h"

#include "item/pluginsitem.h"
#include "utils/global.h"
#include "settings.h"

#include <QPainter>
#include <QPen>
#include <QKeyEvent>
#include <QEvent>
#include <DSettingsDialog>

DWIDGET_USE_NAMESPACE

using namespace dtb;

MainPanel::MainPanel(QWidget *parent)
    : QWidget(parent)
    , m_settings(&Settings::InStance())
{
    initUI();
    initConnect();

    reload();
}

void MainPanel::initUI()
{
    setWindowFlags(Qt::WindowDoesNotAcceptFocus);
    setAttribute(Qt::WA_TranslucentBackground);

    m_mainLayout = new QHBoxLayout;

    m_mainLayout->setMargin(0);
    m_mainLayout->setSpacing(3);
    m_mainLayout->setContentsMargins(5, 0, 5, 1);

    setLayout(m_mainLayout);
}

void MainPanel::initConnect()
{

}

void MainPanel::addItem(PluginsItemInterface * const module, const QString &itemKey)
{
    // check

    if (m_moduleMap.contains(module))
        if (m_moduleMap[module].contains(itemKey))
            return;

    PluginsItem *item = new PluginsItem(module, itemKey);

    m_moduleMap[module][itemKey] = item;

    m_mainLayout->addWidget(item);
}

void MainPanel::removeItem(PluginsItemInterface * const module, const QString &itemKey)
{
    if (!m_moduleMap.contains(module))
        return;

    PluginsItem * item = m_moduleMap[module][itemKey];

    if (!item)
        return;

    m_mainLayout->removeWidget(item);

    m_moduleMap[module].remove(itemKey);

    item->deleteLater();
}

bool MainPanel::saveConfig(const QString &itemKey, const QJsonObject &json)
{
    const QString &configFile = QStandardPaths::standardLocations(QStandardPaths::ConfigLocation).first() + "/deepin-topbar/" + itemKey + "/config.json";

    QFileInfo info(configFile);

    QDir dir(info.path());
    if (!dir.exists())
        dir.mkpath(info.path());

    QFile file(configFile);
    if (file.exists() && file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        file.write(QJsonDocument(json).toJson());
        file.close();
        return true;
    }

    file.setFileName(configFile);
    if (file.open(QIODevice::ReadWrite | QIODevice::Text)) {
        QTextStream out(&file);
        out << QJsonDocument(json).toJson();
        out.flush();
        file.close();
        return true;
    }

    return false;
}

const QJsonObject MainPanel::loadConfig(const QString &itemKey)
{
    const QString &configFile = QStandardPaths::standardLocations(QStandardPaths::ConfigLocation).first() + "/deepin-topbar/" + itemKey +"/config.json";

    QFile file(configFile);

    if (file.open(QIODevice::ReadOnly | QIODevice::Text))
        return QJsonDocument::fromJson(file.readAll()).object();
    else
        return QJsonObject();
}

void MainPanel::loadModules()
{
    // indicator is special
    loadModule(new indicator::IndicatorPlugin);

    //Stretch
    m_mainLayout->addStretch();

    loadModule(new systeminfo::SystemInfoPlugin);

    loadModule(new systemtray::SystemTrayPlugin);

    loadModule(new sound::SoundPlugin);

    loadModule(new power::PowerPlugin);

#ifdef QT_DEBUG
    loadModule(new network::NetworkPlugin);
#endif

    loadModule(new datetime::DateTimePlugin);

    loadModule(new search::SearchModule);

    loadModule(new notify::NotifyPlugin);
}

void MainPanel::loadModule(PluginsItemInterface * const module)
{
    if (!m_settings->settings()->value(QString("base.Module.%1").arg(module->pluginName())).toBool()) {
        delete module;
        return;
    }

    // init
    module->init(this);
}

void MainPanel::reload()
{
    for (int i = 0; i != m_moduleMap.size(); i++) {
        PluginsItemInterface *inter = m_moduleMap.keys().at(i);

        QMap<QString, PluginsItem*> map = m_moduleMap[inter];

        for (PluginsItem * item : map.values()) {
            item->deleteLater();
        }

        delete inter;
    }

    m_moduleMap.clear();

    QLayoutItem* item;
    while ( ( item = m_mainLayout->takeAt( 0 ) ) != NULL )
    {
        delete item->widget();
        delete item;
    }

    loadModules();
}

void MainPanel::paintEvent(QPaintEvent *event)
{
    QWidget::paintEvent(event);

    QPainter painter(this);
    QPen pen(painter.pen());
    pen.setBrush(QColor(0, 0, 0, .7 * 255));
    pen.setWidth(2);
    painter.setPen(pen);
    painter.drawLine(QPoint(0, TOPHEIGHT), QPoint(width(), TOPHEIGHT));
}

void MainPanel::setDefaultColor(const DefaultColor &defaultColor)
{
    m_defaultColor = defaultColor;

    for (PluginsItemInterface *inter : m_moduleMap.keys())
        inter->setDefaultColor(m_defaultColor);

    update();
}

void MainPanel::showSettingDialog()
{
    DSettingsDialog *dialog = new DSettingsDialog;

    dialog->updateSettings(m_settings->settings());

    dialog->exec();

    dialog->deleteLater();

    m_settings->settings()->sync();

    QTimer::singleShot(1, this, &MainPanel::reload);
}
