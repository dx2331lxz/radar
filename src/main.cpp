#include <QApplication>
#include <QWidget>
#include <QVBoxLayout>
#include <QScrollArea>
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
    window.setWindowTitle("雷达任务配置");

    auto *layout = new QVBoxLayout(&window);
    auto *status = new RadarStatusWidget();
    layout->addWidget(status);

    // 圆形雷达显示器
    auto *scope = new RadarScopeWidget();
    scope->setMinimumHeight(420);
    layout->addWidget(scope);

    auto *cfg = new RadarConfigWidget();
    // Put the config widget into a scroll area so the whole page can scroll
    auto *scroll = new QScrollArea();
    scroll->setWidgetResizable(true);
    scroll->setWidget(cfg);
    layout->addWidget(scroll);

    window.resize(640, 800);
    window.show();

    // Network manager: listen for local clients (6553) and connect to radar (6280)
    NetworkManager net;
    net.start();

    auto sendToRadar = [&net](const QJsonObject &obj)
    {
        QJsonDocument d(obj);
        net.sendToRadar(d.toJson(QJsonDocument::Compact));
    };

    QObject::connect(cfg, &RadarConfigWidget::sendInitRequested, cfg, sendToRadar);
    QObject::connect(cfg, &RadarConfigWidget::sendCalibrationRequested, cfg, sendToRadar);
    QObject::connect(cfg, &RadarConfigWidget::sendStandbyRequested, cfg, sendToRadar);
    QObject::connect(cfg, &RadarConfigWidget::sendSearchRequested, cfg, sendToRadar);
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
    QObject::connect(&net, &NetworkManager::radarDatagramReceived, &window, [scope](const QByteArray &data){
        RadarStatus s; if (RadarStatusParser::parseLittleEndian(data, s)) {
            if (s.detectRange > 0) scope->setMaxRangeMeters(float(s.detectRange));
        }
    });

    return app.exec();
}
