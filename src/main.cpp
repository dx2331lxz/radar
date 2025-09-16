#include <QApplication>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QScrollArea>
#include <QSplitter>
#include "RadarConfigWidget.h"
#include "RadarStatusWidget.h"
#include <QDebug>
#include "NetworkManager.h"
#include "RadarScopeWidget.h"
#include "RadarStatus.h"
#include "Protocol.h"
#include "MessageIds.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    QWidget window;
    window.setWindowTitle("雷达状态与任务配置");

    // 左：状态 + 圆盘；右：任务配置（滚动）
    auto *split = new QSplitter(Qt::Horizontal, &window);

    // 左侧：使用垂直分割器，上：状态(可滚动)，下：圆盘
    auto *leftSplit = new QSplitter(Qt::Vertical);
    // 状态区+滚动
    auto *status = new RadarStatusWidget();
    auto *statusScroll = new QScrollArea();
    statusScroll->setWidgetResizable(true);
    statusScroll->setWidget(status);
    statusScroll->setFrameShape(QFrame::NoFrame);
    leftSplit->addWidget(statusScroll);
    // 圆形雷达显示器
    auto *scope = new RadarScopeWidget();
    scope->setMinimumHeight(420);
    scope->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    leftSplit->addWidget(scope);
    leftSplit->setStretchFactor(0, 1); // 状态区
    leftSplit->setStretchFactor(1, 2); // 雷达盘更大

    // 右侧：配置 + 滚动条
    auto *cfg = new RadarConfigWidget();
    auto *scroll = new QScrollArea();
    scroll->setWidgetResizable(true);
    scroll->setWidget(cfg);
    scroll->setMinimumWidth(360);

    split->addWidget(leftSplit);
    split->addWidget(scroll);
    split->setStretchFactor(0, 2); // 左侧更宽
    split->setStretchFactor(1, 1);

    auto *root = new QHBoxLayout(&window);
    root->setContentsMargins(0, 0, 0, 0);
    root->addWidget(split);

    window.resize(1200, 800);
    window.show();

    // Network manager: listen for local clients (6553) and connect to radar (6280)
    NetworkManager net;
    net.start();
    net.setTarget(QHostAddress(QHostAddress::LocalHost), 6280);

    auto sendToRadar = [&net](const QJsonObject &obj)
    {
        QJsonDocument d(obj);
        net.sendToRadar(d.toJson(QJsonDocument::Compact));
    };

    // removed: init/calibration panel and signals
    QObject::connect(cfg, &RadarConfigWidget::sendStandbyRequested, cfg, sendToRadar);
    QObject::connect(cfg, &RadarConfigWidget::sendStandbyRequested, &window, [scope](const QJsonObject &)
                     { scope->setSearchActive(false); scope->clearTrails(); });
    // 二进制待机任务：发送并关闭扫描线
    QObject::connect(cfg, &RadarConfigWidget::sendStandbyPacketRequested, &net, &NetworkManager::sendToRadar);
    QObject::connect(cfg, &RadarConfigWidget::sendStandbyPacketRequested, &window, [scope](const QByteArray &)
                     { scope->setSearchActive(false); scope->clearTrails(); });
    QObject::connect(cfg, &RadarConfigWidget::sendSearchRequested, cfg, sendToRadar);
    QObject::connect(cfg, &RadarConfigWidget::sendSearchRequested, &window, [scope](const QJsonObject &)
                     { scope->setSearchActive(true); });
    QObject::connect(cfg, &RadarConfigWidget::sendSearchPacketRequested, &net, &NetworkManager::sendToRadar);
    QObject::connect(cfg, &RadarConfigWidget::sendSearchPacketRequested, &window, [scope](const QByteArray &)
                     { scope->setSearchActive(true); });
    QObject::connect(cfg, &RadarConfigWidget::targetAddressChanged, &window, [&net](const QString &ip, int port)
                     {
        Q_UNUSED(ip);
        Q_UNUSED(port);
    net.setTarget(QHostAddress(QHostAddress::LocalHost), 6280);
    qDebug() << "Target locked to" << QHostAddress(QHostAddress::LocalHost).toString() << 6280; });
    // removed: track/simulation/servo connections
    // removed: power panel and signal
    QObject::connect(cfg, &RadarConfigWidget::sendDeployRequested, cfg, sendToRadar);
    // 二进制展开/撤收任务：直接发送到雷达
    QObject::connect(cfg, &RadarConfigWidget::sendDeployPacketRequested, &net, &NetworkManager::sendToRadar);
    // 撤收后：待机、回零、关锁 => 我们在UI上先做待机和清轨迹（回零/锁由设备侧执行）
    QObject::connect(cfg, &RadarConfigWidget::sendDeployPacketRequested, &window, [scope](const QByteArray &packet)
                     {
        Q_UNUSED(packet);
        // 如果是撤收（任务类型0x00），停止扫描并清除轨迹
        scope->setSearchActive(false);
        scope->clearTrails(); });
    // removed: servo connection

    // log messages from network
    QObject::connect(&net, &NetworkManager::clientMessageReceived, [](const QByteArray &data)
                     { qDebug() << "Client->APP:" << data; });
    QObject::connect(&net, &NetworkManager::radarConnected, [](bool c)
                     { qDebug() << "Radar connected:" << c; });

    // forward radar UDP payloads into UI preview/log
    QObject::connect(&net, &NetworkManager::radarDatagramReceived, cfg, &RadarConfigWidget::onRadarDatagramReceived);
    QObject::connect(&net, &NetworkManager::radarDatagramReceived, status, &RadarStatusWidget::onRadarDatagram);
    QObject::connect(&net, &NetworkManager::radarDatagramReceived, scope, &RadarScopeWidget::onTrackDatagram);
    // 当右侧选择目标时，在雷达盘高亮
    QObject::connect(cfg, &RadarConfigWidget::targetSelected, scope, &RadarScopeWidget::highlightTarget);
    // 锁定/下达打击：目前仅打印，后续可以发送网络指令
    QObject::connect(cfg, &RadarConfigWidget::targetLockRequested, scope, &RadarScopeWidget::lockTarget);
    QObject::connect(cfg, &RadarConfigWidget::targetEngageRequested, scope, &RadarScopeWidget::engageTarget);
    // when scope reports a hit, remove from right panel
    QObject::connect(scope, &RadarScopeWidget::targetHit, cfg, &RadarConfigWidget::removeTargetById);
    // when scope reports a hit, also send HitReport (0x4444) packet via network
    QObject::connect(scope, &RadarScopeWidget::targetHit, &net, [&net](quint16 id)
                     {
        Protocol::HeaderConfig hc;
        hc.msgIdRadar = ProtocolIds::HitReport; // set message id to 0x4444
        hc.checkMethod = 1; // default to sum checksum
        QByteArray pkt = Protocol::buildHitPacket(hc, quint8(id & 0xFF));
        net.sendToRadar(pkt); });
    // 用状态报文动态更新量程
    QObject::connect(&net, &NetworkManager::radarDatagramReceived, &window, [scope, cfg](const QByteArray &data)
                     {
        RadarStatus s; if (RadarStatusParser::parseLittleEndian(data, s)) {
            if (s.detectRange > 0) scope->setMaxRangeMeters(float(s.detectRange));
            // 通知配置面板当前雷达是否为撤收状态
            cfg->onRadarStatusUpdated(s);
        } });

    return app.exec();
}
