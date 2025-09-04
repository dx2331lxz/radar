// RadarConfigWidget.h
#pragma once

#include <QWidget>
#include "RadarStatus.h"
#include <QGroupBox>
#include <QCheckBox>
#include <QComboBox>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QLineEdit>
#include <QPushButton>
#include <QTextEdit>
#include <QJsonObject>
#include <QGroupBox>

class RadarConfigWidget : public QWidget
{
    Q_OBJECT
public:
    explicit RadarConfigWidget(QWidget *parent = nullptr);

    // 是否在右侧面板显示接收到的雷达报文（默认：不显示）
    void setLogIncoming(bool on) { m_logIncoming = on; }
    bool logIncoming() const { return m_logIncoming; }

public slots:
    void onRadarDatagramReceived(const QByteArray &data);
    // 从外部更新解析后的雷达状态（用于决定是否允许搜索）
    void onRadarStatusUpdated(const RadarStatus &s);

private slots:
    void onApply();
    void onReset();
    void onPreview();
    // removed: onSendInit(), onSendCalibration()
    void onSendStandby();
    void onSendSearch();
    void onSendTrack();
    void onSendSimulation();
    // removed: onSendPower()
    void onSendDeploy();
    void onSendServo();

private:
    // UI builders
    // removed: buildInitSection(), buildCalibSection()
    QGroupBox *buildStandbySection();
    QGroupBox *buildSearchSection();
    QGroupBox *buildHeaderSection();
    QGroupBox *buildTrackSection();
    QGroupBox *buildSimSection();
    // removed: buildPowerSection()
    QGroupBox *buildDeploySection();
    QGroupBox *buildServoSection();

    void setDefaults();
    QJsonObject gatherConfigJson() const;

    // Widgets per section
    // removed: 初始化任务 / 自校准任务

    // 待机任务：精简为仅按钮

    // 搜索任务
    // 精简：只保留一个“发送 搜索”按钮，不再提供搜索相关的可配置项
    // 帧头参数
    QSpinBox *hdrDeviceModel{};
    QSpinBox *hdrDevIdRadar{};
    QSpinBox *hdrDevIdExternal{};
    QComboBox *hdrCheckMethod{};
    QSpinBox *hdrMsgIdRadar{};
    QSpinBox *hdrMsgIdExternal{};
    QLineEdit *targetIpEdit{};
    QSpinBox *targetPortSpin{};

    // 跟踪任务
    QCheckBox *trackEnable{};
    QLineEdit *trackTargetIdEdit{};
    QSpinBox *trackRateSpin{};

    // 模拟任务
    QCheckBox *simEnable{};
    QComboBox *simScenarioCombo{};
    QSpinBox *simTargetCountSpin{};

    // removed: 上下电任务

    // 展开撤收任务
    QComboBox *deployActionCombo{}; // 展开/撤收

    // 伺服转停任务
    QComboBox *servoActionCombo{};    // 转动/停止
    QDoubleSpinBox *servoSpeedSpin{}; // 角速度 (deg/s)

    // Bottom actions
    QPushButton *applyBtn{};
    QPushButton *resetBtn{};
    QPushButton *previewBtn{};
    QTextEdit *previewEdit{};
    // Per-section send buttons
    // removed: sendInitBtn, sendCalibBtn
    QPushButton *sendStandbyBtn{};
    QPushButton *sendSearchBtn{};
    QPushButton *sendTrackBtn{};
    QPushButton *sendSimBtn{};
    // removed: sendPowerBtn
    QPushButton *sendDeployBtn{};
    QPushButton *sendServoBtn{};

signals:
    // removed: sendInitRequested, sendCalibrationRequested
    void sendStandbyRequested(const QJsonObject &cfg);
    void sendSearchRequested(const QJsonObject &cfg);
    // 发送二进制搜索任务包
    void sendSearchPacketRequested(const QByteArray &packet);
    // 发送二进制待机任务包
    void sendStandbyPacketRequested(const QByteArray &packet);
    // 通知更新目标地址
    void targetAddressChanged(const QString &ip, int port);
    void sendTrackRequested(const QJsonObject &cfg);
    void sendSimulationRequested(const QJsonObject &cfg);
    // removed: sendPowerRequested
    void sendDeployRequested(const QJsonObject &cfg);
    // 发送二进制展开/撤收任务包
    void sendDeployPacketRequested(const QByteArray &packet);
    void sendServoRequested(const QJsonObject &cfg);

private:
    bool m_logIncoming = false; // 默认关闭接收报文日志
    bool m_isRetracted = false; // 最近一次状态是否处于撤收
};
