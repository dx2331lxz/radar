// RadarStatusWidget.cpp
#include "RadarStatusWidget.h"
#include <QGroupBox>
#include <QVBoxLayout>

RadarStatusWidget::RadarStatusWidget(QWidget *parent)
    : QWidget(parent)
{
    auto *root = new QGroupBox(tr("雷达状态"), this);
    auto *grid = new QGridLayout(root);

    auto addRow = [&](int r, const QString &name, QLabel *&val)
    {
        grid->addWidget(new QLabel(name), r, 0);
        val = new QLabel("-");
        val->setMinimumWidth(160);
        grid->addWidget(val, r, 1);
    };

    int r = 0;
    addRow(r++, tr("连接状态"), lblConn);
    addRow(r++, tr("硬件故障"), lblHwFault);
    addRow(r++, tr("软件故障"), lblSwFault);
    addRow(r++, tr("工作状态"), lblWorkState);
    addRow(r++, tr("探测范围(m)"), lblDetectRange);
    addRow(r++, tr("惯导有效"), lblINS);
    addRow(r++, tr("模拟开关"), lblSim);
    addRow(r++, tr("撤收状态"), lblRetract);
    addRow(r++, tr("行驶状态"), lblDrive);
    addRow(r++, tr("经度"), lblLon);
    addRow(r++, tr("纬度"), lblLat);
    addRow(r++, tr("海拔(m)"), lblAlt);
    addRow(r++, tr("偏航(°)"), lblYaw);
    addRow(r++, tr("俯仰(°)"), lblPitch);
    addRow(r++, tr("滚转(°)"), lblRoll);
    addRow(r++, tr("频点(GHz)"), lblFreq);
    addRow(r++, tr("天线上电"), lblAntPower);
    addRow(r++, tr("静默区(起-终 0.01°)"), lblSilent);

    auto *outer = new QVBoxLayout(this);
    outer->addWidget(root);

    // Set inactivity timer (6s). If no frame comes, mark as disconnected.
    m_inactiveTimer.setInterval(6000);
    m_inactiveTimer.setSingleShot(true);
    connect(&m_inactiveTimer, &QTimer::timeout, this, [this]
            { setText(lblConn, tr("未连接/超时")); });
}

void RadarStatusWidget::setText(QLabel *lab, const QString &text)
{
    if (lab)
        lab->setText(text);
}

void RadarStatusWidget::onRadarDatagram(const QByteArray &data)
{
    RadarStatus s;
    if (!RadarStatusParser::parseLittleEndian(data, s))
    {
        setText(lblConn, tr("未连接/无效报文"));
        return;
    }
    setStatus(s);
    m_inactiveTimer.start();
}

void RadarStatusWidget::setStatus(const RadarStatus &s)
{
    setText(lblConn, tr("已连接"));
    setText(lblHwFault, s.hwFaultText());
    setText(lblSwFault, s.swFaultText());
    setText(lblWorkState, s.workStateText());
    setText(lblDetectRange, QString::number(s.detectRange));
    setText(lblINS, s.insValid ? tr("有效") : tr("无效"));
    setText(lblSim, s.simOn ? tr("已开启") : tr("未开启"));
    setText(lblRetract, s.retracted ? tr("已撤收") : tr("未撤收"));
    setText(lblDrive, s.driving ? tr("动态行车") : tr("静态驻车"));
    setText(lblLon, QString::number(s.longitude, 'f', 6));
    setText(lblLat, QString::number(s.latitude, 'f', 6));
    setText(lblAlt, QString::number(s.altitude, 'f', 2));
    setText(lblYaw, QString::number(s.yaw, 'f', 2));
    setText(lblPitch, QString::number(s.pitch, 'f', 2));
    setText(lblRoll, QString::number(s.roll, 'f', 2));
    setText(lblFreq, QString::number(s.freqGHz, 'f', 2));
    setText(lblAntPower, s.antPowerModeText());
    setText(lblSilent, QString("%1 - %2").arg(s.silentStart).arg(s.silentEnd));
}
