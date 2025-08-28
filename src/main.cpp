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

    QObject::connect(cfg, &RadarConfigWidget::sendInitRequested, cfg, sendToRadar);
    QObject::connect(cfg, &RadarConfigWidget::sendCalibrationRequested, cfg, sendToRadar);
    QObject::connect(cfg, &RadarConfigWidget::sendStandbyRequested, cfg, sendToRadar);
    QObject::connect(cfg, &RadarConfigWidget::sendSearchRequested, cfg, sendToRadar);
    QObject::connect(cfg, &RadarConfigWidget::sendSearchPacketRequested, &net, &NetworkManager::sendToRadar);
    QObject::connect(cfg, &RadarConfigWidget::targetAddressChanged, &window, [&net](const QString &ip, int port)
                     {
        Q_UNUSED(ip);
        Q_UNUSED(port);
    net.setTarget(QHostAddress(QHostAddress::LocalHost), 6280);
    qDebug() << "Target locked to" << QHostAddress(QHostAddress::LocalHost).toString() << 6280; });
    QObject::connect(cfg, &RadarConfigWidget::sendTrackRequested, cfg, sendToRadar);
    QObject::connect(cfg, &RadarConfigWidget::sendSimulationRequested, cfg, sendToRadar);
    QObject::connect(cfg, &RadarConfigWidget::sendPowerRequested, cfg, sendToRadar);
    QObject::connect(cfg, &RadarConfigWidget::sendDeployRequested, cfg, sendToRadar);
    QObject::connect(cfg, &RadarConfigWidget::sendServoRequested, cfg, sendToRadar);

    // log messages from network
    QObject::connect(&net, &NetworkManager::clientMessageReceived, [](const QByteArray &data)
                     { qDebug() << "Client->APP:" << data; });
    QObject::connect(&net, &NetworkManager::radarConnected, [](bool c)
                     { qDebug() << "Radar connected:" << c; });

    // forward radar UDP payloads into UI preview/log
    QObject::connect(&net, &NetworkManager::radarDatagramReceived, cfg, &RadarConfigWidget::onRadarDatagramReceived);
    QObject::connect(&net, &NetworkManager::radarDatagramReceived, status, &RadarStatusWidget::onRadarDatagram);
    QObject::connect(&net, &NetworkManager::radarDatagramReceived, scope, &RadarScopeWidget::onTrackDatagram);
    // 用状态报文动态更新量程
    QObject::connect(&net, &NetworkManager::radarDatagramReceived, &window, [scope](const QByteArray &data)
                     {
        RadarStatus s; if (RadarStatusParser::parseLittleEndian(data, s)) {
            if (s.detectRange > 0) scope->setMaxRangeMeters(float(s.detectRange));
        } });

    return app.exec();
}
