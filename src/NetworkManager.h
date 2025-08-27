// NetworkManager.h
#pragma once

#include <QObject>
#include <QUdpSocket>
#include <QTimer>

class NetworkManager : public QObject
{
    Q_OBJECT
public:
    explicit NetworkManager(QObject *parent = nullptr);
    void start();
    bool isRadarConnected() const;
    void sendToRadar(const QByteArray &data);

signals:
    void radarConnected(bool connected);
    void clientMessageReceived(const QByteArray &data);
    void radarDatagramReceived(const QByteArray &data);

private slots:
    void onRadarDatagram();

private:
    QUdpSocket *m_udpSocket{nullptr};
    QTimer m_reconnectTimer; // kept for potential periodic tasks
};
