#include <QApplication>
#include <QWidget>
#include <QVBoxLayout>
#include <QScrollArea>
#include "RadarConfigWidget.h"
#include "RadarStatusWidget.h"
#include <QDebug>
#include "NetworkManager.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    QWidget window;
    window.setWindowTitle("雷达任务配置");

    auto *layout = new QVBoxLayout(&window);
    auto *status = new RadarStatusWidget();
    layout->addWidget(status);

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

    return app.exec();
}
