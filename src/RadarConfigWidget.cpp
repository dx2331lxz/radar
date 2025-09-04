// RadarConfigWidget.cpp
#include "RadarConfigWidget.h"
#include "Protocol.h"
#include "RadarStatus.h"
#include "MessageIds.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QDateTime>

static QGroupBox *wrapWithTitle(const QString &title, QWidget *inner)
{
    auto *box = new QGroupBox(title);
    auto *v = new QVBoxLayout(box);
    v->addWidget(inner);
    return box;
}

RadarConfigWidget::RadarConfigWidget(QWidget *parent)
    : QWidget(parent)
{
    auto *root = new QVBoxLayout(this);

    // Build sections in user flow order (removed: Power/Init/Calibration per request):
    // Header -> Deploy -> Standby -> Search -> Track -> Simulation -> Servo
    root->addWidget(buildHeaderSection());
    // removed: root->addWidget(buildPowerSection());
    // removed: root->addWidget(buildInitSection());
    // removed: root->addWidget(buildCalibSection());
    root->addWidget(buildDeploySection());
    root->addWidget(buildStandbySection());
    root->addWidget(buildSearchSection());
    root->addWidget(buildTrackSection());
    root->addWidget(buildSimSection());
    root->addWidget(buildServoSection());

    // Preview + actions
    auto *btnRow = new QHBoxLayout();
    applyBtn = new QPushButton(tr("应用配置"));
    resetBtn = new QPushButton(tr("恢复默认"));
    previewBtn = new QPushButton(tr("预览配置"));
    btnRow->addWidget(applyBtn);
    btnRow->addWidget(resetBtn);
    btnRow->addWidget(previewBtn);

    previewEdit = new QTextEdit();
    previewEdit->setReadOnly(true);
    previewEdit->setMinimumHeight(140);

    root->addLayout(btnRow);
    root->addWidget(previewEdit, 1);

    // Connections
    connect(previewBtn, &QPushButton::clicked, this, &RadarConfigWidget::onPreview);
    connect(applyBtn, &QPushButton::clicked, this, &RadarConfigWidget::onApply);
    connect(resetBtn, &QPushButton::clicked, this, &RadarConfigWidget::onReset);

    setDefaults();
}
QGroupBox *RadarConfigWidget::buildHeaderSection()
{
    auto *w = new QWidget();
    auto *form = new QFormLayout(w);
    hdrDeviceModel = new QSpinBox();
    hdrDeviceModel->setRange(0, 65535);
    hdrDeviceModel->setValue(6000);
    hdrDevIdRadar = new QSpinBox();
    hdrDevIdRadar->setRange(0, 65535);
    hdrDevIdRadar->setValue(0);
    hdrDevIdExternal = new QSpinBox();
    hdrDevIdExternal->setRange(0, 65535);
    hdrDevIdExternal->setValue(0);
    hdrCheckMethod = new QComboBox();
    hdrCheckMethod->addItem(tr("不校验 (0)"), 0);
    hdrCheckMethod->addItem(tr("和校验 (1)"), 1);
    hdrCheckMethod->addItem(tr("CRC-16 (2)"), 2);
    hdrCheckMethod->setCurrentIndex(2);

    // 目标IP与端口
    targetIpEdit = new QLineEdit();
    targetIpEdit->setPlaceholderText("127.0.0.1");
    targetIpEdit->setText("127.0.0.1");
    targetIpEdit->setReadOnly(true);
    targetPortSpin = new QSpinBox();
    targetPortSpin->setRange(1, 65535);
    targetPortSpin->setValue(6280);

    hdrMsgIdRadar = new QSpinBox();
    hdrMsgIdRadar->setRange(0, 65535);
    hdrMsgIdRadar->setValue(ProtocolIds::CmdSearch); // 仿真器搜索控制ID
    hdrMsgIdExternal = new QSpinBox();
    hdrMsgIdExternal->setRange(0, 65535);
    hdrMsgIdExternal->setValue(0);

    form->addRow(tr("设备型号"), hdrDeviceModel);
    form->addRow(tr("报文ID(雷达)"), hdrMsgIdRadar);
    form->addRow(tr("报文ID(外部)"), hdrMsgIdExternal);
    form->addRow(tr("设备ID(雷达)"), hdrDevIdRadar);
    form->addRow(tr("设备ID(外部)"), hdrDevIdExternal);
    form->addRow(tr("校验方式"), hdrCheckMethod);
    form->addRow(tr("目标IP"), targetIpEdit);
    form->addRow(tr("目标端口"), targetPortSpin);

    return wrapWithTitle(tr("协议帧头配置"), w);
}

QGroupBox *RadarConfigWidget::buildStandbySection()
{
    auto *w = new QWidget();
    auto *form = new QFormLayout(w);
    // 精简：仅提供一个按钮用于触发待机控制包发送
    sendStandbyBtn = new QPushButton(tr("发送 待机"));
    form->addRow(sendStandbyBtn);
    connect(sendStandbyBtn, &QPushButton::clicked, this, &RadarConfigWidget::onSendStandby);
    return wrapWithTitle(tr("雷达待机任务"), w);
}

QGroupBox *RadarConfigWidget::buildSearchSection()
{
    auto *w = new QWidget();
    auto *form = new QFormLayout(w);
    // 精简：仅提供一个按钮用于触发搜索控制包发送
    sendSearchBtn = new QPushButton(tr("发送 搜索"));
    form->addRow(sendSearchBtn);
    connect(sendSearchBtn, &QPushButton::clicked, this, &RadarConfigWidget::onSendSearch);
    return wrapWithTitle(tr("雷达搜索任务"), w);
}

QGroupBox *RadarConfigWidget::buildTrackSection()
{
    auto *w = new QWidget();
    auto *form = new QFormLayout(w);
    trackEnable = new QCheckBox(tr("启用跟踪"));
    trackTargetIdEdit = new QLineEdit();
    trackTargetIdEdit->setPlaceholderText(tr("目标 ID (可选)"));
    trackRateSpin = new QSpinBox();
    trackRateSpin->setRange(1, 100);
    trackRateSpin->setSuffix(tr(" Hz"));
    form->addRow(trackEnable);
    form->addRow(tr("目标ID"), trackTargetIdEdit);
    form->addRow(tr("更新频率"), trackRateSpin);
    sendTrackBtn = new QPushButton(tr("发送 跟踪"));
    form->addRow(sendTrackBtn);
    connect(sendTrackBtn, &QPushButton::clicked, this, &RadarConfigWidget::onSendTrack);
    return wrapWithTitle(tr("雷达跟踪任务"), w);
}

QGroupBox *RadarConfigWidget::buildSimSection()
{
    auto *w = new QWidget();
    auto *form = new QFormLayout(w);
    simEnable = new QCheckBox(tr("启用模拟"));
    simScenarioCombo = new QComboBox();
    simScenarioCombo->addItems({tr("单目标"), tr("多目标"), tr("杂波场景")});
    simTargetCountSpin = new QSpinBox();
    simTargetCountSpin->setRange(0, 256);
    form->addRow(simEnable);
    form->addRow(tr("场景"), simScenarioCombo);
    form->addRow(tr("目标数量"), simTargetCountSpin);
    sendSimBtn = new QPushButton(tr("发送 模拟"));
    form->addRow(sendSimBtn);
    connect(sendSimBtn, &QPushButton::clicked, this, &RadarConfigWidget::onSendSimulation);
    return wrapWithTitle(tr("雷达模拟任务"), w);
}

QGroupBox *RadarConfigWidget::buildDeploySection()
{
    auto *w = new QWidget();
    auto *form = new QFormLayout(w);
    deployActionCombo = new QComboBox();
    deployActionCombo->addItems({tr("展开"), tr("撤收")});
    form->addRow(tr("动作"), deployActionCombo);
    sendDeployBtn = new QPushButton(tr("发送 展开/撤收"));
    form->addRow(sendDeployBtn);
    connect(sendDeployBtn, &QPushButton::clicked, this, &RadarConfigWidget::onSendDeploy);
    return wrapWithTitle(tr("雷达展开/撤收任务"), w);
}

QGroupBox *RadarConfigWidget::buildServoSection()
{
    auto *w = new QWidget();
    auto *form = new QFormLayout(w);
    servoActionCombo = new QComboBox();
    servoActionCombo->addItems({tr("转动"), tr("停止")});
    servoSpeedSpin = new QDoubleSpinBox();
    servoSpeedSpin->setRange(0.0, 60.0);
    servoSpeedSpin->setSingleStep(0.1);
    servoSpeedSpin->setSuffix(tr(" °/s"));
    form->addRow(tr("动作"), servoActionCombo);
    form->addRow(tr("角速度"), servoSpeedSpin);
    sendServoBtn = new QPushButton(tr("发送 伺服"));
    form->addRow(sendServoBtn);
    connect(sendServoBtn, &QPushButton::clicked, this, &RadarConfigWidget::onSendServo);
    return wrapWithTitle(tr("伺服转停任务"), w);
}

void RadarConfigWidget::setDefaults()
{
    // 待机/搜索任务无可配置项，保持默认

    trackEnable->setChecked(false);
    trackTargetIdEdit->clear();
    trackRateSpin->setValue(10);

    simEnable->setChecked(false);
    simScenarioCombo->setCurrentIndex(0);
    simTargetCountSpin->setValue(10);

    deployActionCombo->setCurrentIndex(0);

    servoActionCombo->setCurrentIndex(0);
    servoSpeedSpin->setValue(10.0);

    // header defaults
    if (hdrDeviceModel)
        hdrDeviceModel->setValue(6000);
    if (hdrMsgIdRadar)
        hdrMsgIdRadar->setValue(ProtocolIds::CmdSearch);
    if (hdrMsgIdExternal)
        hdrMsgIdExternal->setValue(0);
    if (hdrDevIdRadar)
        hdrDevIdRadar->setValue(0);
    if (hdrDevIdExternal)
        hdrDevIdExternal->setValue(0);
    if (hdrCheckMethod)
        hdrCheckMethod->setCurrentIndex(2); // CRC-16
    if (targetIpEdit)
        targetIpEdit->setText("127.0.0.1");
    if (targetPortSpin)
        targetPortSpin->setValue(6280);
}

QJsonObject RadarConfigWidget::gatherConfigJson() const
{
    QJsonObject j;
    // Removed from panel: init/calibration/power
    j["standby"] = QJsonObject{{"action", QStringLiteral("standby")}};
    // 精简：只标注搜索操作，无具体参数
    j["search"] = QJsonObject{{"action", QStringLiteral("search")}};
    j["track"] = QJsonObject{
        {"enabled", trackEnable->isChecked()},
        {"target_id", trackTargetIdEdit->text()},
        {"rate_hz", trackRateSpin->value()},
    };
    j["simulation"] = QJsonObject{
        {"enabled", simEnable->isChecked()},
        {"scenario", simScenarioCombo->currentText()},
        {"target_count", simTargetCountSpin->value()},
    };
    // j["power"] omitted
    j["deploy"] = QJsonObject{{"action", deployActionCombo->currentText()}};
    j["servo"] = QJsonObject{
        {"action", servoActionCombo->currentText()},
        {"speed_deg_s", servoSpeedSpin->value()},
    };
    return j;
}

void RadarConfigWidget::onApply()
{
    // 保留发送逻辑，当前仅预览
    onPreview();
}

void RadarConfigWidget::onReset()
{
    setDefaults();
    onPreview();
}

void RadarConfigWidget::onPreview()
{
    auto j = gatherConfigJson();
    QJsonDocument doc(j);
    previewEdit->setPlainText(QString::fromUtf8(doc.toJson(QJsonDocument::Indented)));
}

void RadarConfigWidget::onRadarDatagramReceived(const QByteArray &data)
{
    if (!m_logIncoming)
        return; // 默认不显示接收到的报文
    // Append received UDP payload to the preview/log area with a small header
    QString prev = previewEdit->toPlainText();
    QString msg = QString("[RADAR->APP] %1\n(len=%2 bytes)\n").arg(QDateTime::currentDateTime().toString(Qt::ISODate)).arg(data.size());
    if (!prev.isEmpty())
        prev += "\n" + msg;
    else
        prev = msg;
    previewEdit->setPlainText(prev);
}

void RadarConfigWidget::onRadarStatusUpdated(const RadarStatus &s)
{
    m_isRetracted = s.retracted;
}

// Per-section send handlers: build that section's JSON, emit signal and show in preview

void RadarConfigWidget::onSendStandby()
{
    // 预览仅展示动作
    QJsonObject j{{"action", QStringLiteral("standby")}};
    previewEdit->setPlainText(QString::fromUtf8(QJsonDocument(j).toJson(QJsonDocument::Indented)));

    // 构造并发送二进制“待机任务”数据包（任务类型0x00）
    Protocol::HeaderConfig hc;
    hc.deviceModel = quint16(hdrDeviceModel ? hdrDeviceModel->value() : 6000);
    hc.msgIdRadar = ProtocolIds::CmdStandby; // 待机任务ID
    hc.msgIdExternal = quint16(hdrMsgIdExternal ? hdrMsgIdExternal->value() : 0);
    hc.deviceIdRadar = quint16(hdrDevIdRadar ? hdrDevIdRadar->value() : 0);
    hc.deviceIdExternal = quint16(hdrDevIdExternal ? hdrDevIdExternal->value() : 0);
    hc.checkMethod = quint8(hdrCheckMethod ? hdrCheckMethod->currentData().toInt() : 1);

    if (targetIpEdit && targetPortSpin)
        emit targetAddressChanged(targetIpEdit->text(), targetPortSpin->value());
    QByteArray packet = Protocol::buildStandbyTaskPacket(hc, 0x00);
    emit sendStandbyPacketRequested(packet);
}

void RadarConfigWidget::onSendSearch()
{
    // 如果雷达处于撤收状态，阻止发送并提示
    if (m_isRetracted)
    {
        previewEdit->setPlainText(tr("雷达当前处于撤收状态，无法进入搜索。请先展开雷达。"));
        return;
    }

    // 1) JSON预览仅显示操作类型
    QJsonObject j{{"action", QStringLiteral("search")}};
    previewEdit->setPlainText(QString::fromUtf8(QJsonDocument(j).toJson(QJsonDocument::Indented)));

    // 2) 构造并发送二进制“搜索任务”数据包（任务类型固定0x01）
    Protocol::HeaderConfig hc;
    hc.deviceModel = quint16(hdrDeviceModel ? hdrDeviceModel->value() : 6000);
    hc.msgIdRadar = ProtocolIds::CmdSearch; // 固定为仿真器识别的SEARCH控制
    hc.msgIdExternal = quint16(hdrMsgIdExternal ? hdrMsgIdExternal->value() : 0);
    hc.deviceIdRadar = quint16(hdrDevIdRadar ? hdrDevIdRadar->value() : 0);
    hc.deviceIdExternal = quint16(hdrDevIdExternal ? hdrDevIdExternal->value() : 0);
    hc.checkMethod = quint8(hdrCheckMethod ? hdrCheckMethod->currentData().toInt() : 1);

    // 先更新目标地址
    if (targetIpEdit && targetPortSpin)
        emit targetAddressChanged(targetIpEdit->text(), targetPortSpin->value());
    QByteArray packet = Protocol::buildSearchTaskPacket(hc, 0x01);
    emit sendSearchPacketRequested(packet);
}

void RadarConfigWidget::onSendTrack()
{
    QJsonObject j{{"enabled", trackEnable->isChecked()}, {"target_id", trackTargetIdEdit->text()}, {"rate_hz", trackRateSpin->value()}};
    emit sendTrackRequested(j);
    previewEdit->setPlainText(QString::fromUtf8(QJsonDocument(j).toJson(QJsonDocument::Indented)));
}

void RadarConfigWidget::onSendSimulation()
{
    QJsonObject j{{"enabled", simEnable->isChecked()}, {"scenario", simScenarioCombo->currentText()}, {"target_count", simTargetCountSpin->value()}};
    emit sendSimulationRequested(j);
    previewEdit->setPlainText(QString::fromUtf8(QJsonDocument(j).toJson(QJsonDocument::Indented)));
}

void RadarConfigWidget::onSendDeploy()
{
    const bool isDeploy = (deployActionCombo && deployActionCombo->currentIndex() == 0); // 0: 展开, 1: 撤收
    QJsonObject j{{"action", isDeploy ? QStringLiteral("deploy") : QStringLiteral("retract")}};
    emit sendDeployRequested(j);
    previewEdit->setPlainText(QString::fromUtf8(QJsonDocument(j).toJson(QJsonDocument::Indented)));

    // 构造并发送二进制“展开/撤收任务”数据包（任务类型：0x01 展开，0x00 撤收）
    Protocol::HeaderConfig hc;
    hc.deviceModel = quint16(hdrDeviceModel ? hdrDeviceModel->value() : 6000);
    hc.msgIdRadar = ProtocolIds::CmdDeploy; // 展开/撤收任务ID
    hc.msgIdExternal = quint16(hdrMsgIdExternal ? hdrMsgIdExternal->value() : 0);
    hc.deviceIdRadar = quint16(hdrDevIdRadar ? hdrDevIdRadar->value() : 0);
    hc.deviceIdExternal = quint16(hdrDevIdExternal ? hdrDevIdExternal->value() : 0);
    hc.checkMethod = quint8(hdrCheckMethod ? hdrCheckMethod->currentData().toInt() : 1);

    if (targetIpEdit && targetPortSpin)
        emit targetAddressChanged(targetIpEdit->text(), targetPortSpin->value());

    const quint8 taskType = isDeploy ? 0x01 : 0x00;
    QByteArray packet = Protocol::buildDeployTaskPacket(hc, taskType);
    emit sendDeployPacketRequested(packet);
}

void RadarConfigWidget::onSendServo()
{
    QJsonObject j{{"action", servoActionCombo->currentText()}, {"speed_deg_s", servoSpeedSpin->value()}};
    emit sendServoRequested(j);
    previewEdit->setPlainText(QString::fromUtf8(QJsonDocument(j).toJson(QJsonDocument::Indented)));
}
