#include "NetworkManager.h"
#include <QHostAddress>
#include <QDebug>

namespace
{
    QString hexDump(const QByteArray &data, int bytesPerLine = 16)
    {
        QString out;
        const int n = data.size();
        for (int i = 0; i < n; i += bytesPerLine)
        {
            // 偏移
            out += QString("%1: ").arg(i, 6, 16, QLatin1Char('0')).toUpper();
            // HEX
            for (int j = 0; j < bytesPerLine; ++j)
            {
                if (i + j < n)
                    out += QString("%1 ").arg(quint8(data[i + j]), 2, 16, QLatin1Char('0')).toUpper();
                else
                    out += "   ";
                if (j == bytesPerLine / 2 - 1)
                    out += " "; // 中间再加一个空格
            }
            // ASCII
            out += " |";
            for (int j = 0; j < bytesPerLine && i + j < n; ++j)
            {
                uchar c = uchar(data[i + j]);
                out += (c >= 32 && c <= 126) ? QChar(c) : QChar('.');
            }
            out += "|\n";
        }
        return out;
    }
} // namespace

static constexpr quint16 LOCAL_UDP_PORT = 6553; // bind here for recv/send

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
    qint64 written = m_udpSocket->writeDatagram(data, m_targetAddr, m_targetPort);
    if (written <= 0)
        qWarning() << "Failed to send UDP to radar" << m_targetAddr.toString() << m_targetPort << "err=" << m_udpSocket->errorString();
    else
    {
        qDebug() << "UDP sent to" << m_targetAddr << m_targetPort << "len=" << written;
        qDebug().noquote() << hexDump(data);
    }
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
        emit clientMessageReceived(buf);
    }
}

void NetworkManager::setTarget(const QHostAddress &addr, quint16 port)
{
    m_targetAddr = addr;
    m_targetPort = port;
}
