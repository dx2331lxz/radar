#include "NetworkManager.h"
#include <QHostAddress>
#include <QDebug>

static constexpr quint16 LOCAL_UDP_PORT = 6553;                 // bind here for recv/send
static constexpr quint16 RADAR_PORT = 6280;                     // send to radar at this port
static const QHostAddress RADAR_ADDR = QHostAddress::Broadcast; // default send to broadcast, can be changed later

NetworkManager::NetworkManager(QObject *parent)
    : QObject(parent)
{
    connect(&m_reconnectTimer, &QTimer::timeout, this, [&]() {});
    m_reconnectTimer.setInterval(3000);
}

void NetworkManager::start()
{
    if (!m_udpSocket)
    {
        m_udpSocket = new QUdpSocket(this);
        if (!m_udpSocket->bind(QHostAddress::AnyIPv4, LOCAL_UDP_PORT, QUdpSocket::ShareAddress | QUdpSocket::ReuseAddressHint))
        {
            qWarning() << "Failed to bind UDP on" << LOCAL_UDP_PORT;
            return;
        }
        qDebug() << "Bound UDP recv on" << LOCAL_UDP_PORT;
        connect(m_udpSocket, &QUdpSocket::readyRead, this, &NetworkManager::onRadarDatagram);
    }
}

void NetworkManager::sendToRadar(const QByteArray &data)
{
    if (!m_udpSocket)
        return;
    qint64 written = m_udpSocket->writeDatagram(data, RADAR_ADDR, RADAR_PORT);
    if (written <= 0)
        qWarning() << "Failed to send UDP to radar";
}

void NetworkManager::onRadarDatagram()
{
    while (m_udpSocket && m_udpSocket->hasPendingDatagrams())
    {
        QByteArray buf;
        buf.resize(int(m_udpSocket->pendingDatagramSize()));
        QHostAddress sender;
        quint16 senderPort;
        m_udpSocket->readDatagram(buf.data(), buf.size(), &sender, &senderPort);
        // For binary frames, avoid printing raw payload fully
        qDebug() << "UDP recv from" << sender << senderPort << "len=" << buf.size();
        emit radarDatagramReceived(buf);
    }
}
