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
    void setTarget(const QHostAddress &addr, quint16 port);

signals:
    void radarConnected(bool connected);
    void clientMessageReceived(const QByteArray &data);
    void radarDatagramReceived(const QByteArray &data);

private slots:
    void onRadarDatagram();

private:
    QUdpSocket *m_udpSocket{nullptr};
    QTimer m_reconnectTimer; // kept for potential periodic tasks
    QHostAddress m_targetAddr{QHostAddress::LocalHost};
    quint16 m_targetPort{6280};
};
