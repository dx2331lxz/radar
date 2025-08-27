// RadarStatusWidget.h
#pragma once

#include <QWidget>
#include <QLabel>
#include <QGridLayout>
#include <QTimer>
#include "RadarStatus.h"

class RadarStatusWidget : public QWidget
{
    Q_OBJECT
public:
    explicit RadarStatusWidget(QWidget *parent = nullptr);

public slots:
    void onRadarDatagram(const QByteArray &data);
    void setStatus(const RadarStatus &s);

private:
    void setText(QLabel *lab, const QString &text);
    QTimer m_inactiveTimer; // no report in 6s => disconnected

    QLabel *lblConn{};
    QLabel *lblHwFault{};
    QLabel *lblSwFault{};
    QLabel *lblWorkState{};
    QLabel *lblDetectRange{};
    QLabel *lblINS{};
    QLabel *lblSim{};
    QLabel *lblRetract{};
    QLabel *lblDrive{};
    QLabel *lblLon{};
    QLabel *lblLat{};
    QLabel *lblAlt{};
    QLabel *lblYaw{};
    QLabel *lblPitch{};
    QLabel *lblRoll{};
    QLabel *lblFreq{};
    QLabel *lblAntPower{};
    QLabel *lblSilent{};
};
