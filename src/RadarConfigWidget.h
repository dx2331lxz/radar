// RadarConfigWidget.h
#pragma once

#include <QWidget>
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

private slots:
    void onApply();
    void onReset();
    void onPreview();
    void onSendInit();
    void onSendCalibration();
    void onSendStandby();
    void onSendSearch();
    void onSendTrack();
    void onSendSimulation();
    void onSendPower();
    void onSendDeploy();
    void onSendServo();

private:
    // UI builders
    QGroupBox *buildInitSection();
    QGroupBox *buildCalibSection();
    QGroupBox *buildStandbySection();
    QGroupBox *buildSearchSection();
    QGroupBox *buildHeaderSection();
    QGroupBox *buildTrackSection();
    QGroupBox *buildSimSection();
    QGroupBox *buildPowerSection();
    QGroupBox *buildDeploySection();
    QGroupBox *buildServoSection();

    void setDefaults();
    QJsonObject gatherConfigJson() const;

    // Widgets per section
    // 初始化任务
    QComboBox *initModeCombo{};
    QSpinBox *initTimeoutSpin{};

    // 自校准任务
    QCheckBox *calibEnable{};
    QComboBox *calibTypeCombo{};

    // 待机任务
    QCheckBox *standbyEnable{};
    QComboBox *standbyPowerCombo{};
    QSpinBox *standbyDurationSpin{};

    // 搜索任务
    QCheckBox *searchEnable{};
    QDoubleSpinBox *searchBeamWidthSpin{};
    QSpinBox *searchRangeKmSpin{};
    QComboBox *searchWaveformCombo{};
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

    // 上下电任务
    QComboBox *powerActionCombo{}; // 上电/下电

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
    QPushButton *sendInitBtn{};
    QPushButton *sendCalibBtn{};
    QPushButton *sendStandbyBtn{};
    QPushButton *sendSearchBtn{};
    QPushButton *sendTrackBtn{};
    QPushButton *sendSimBtn{};
    QPushButton *sendPowerBtn{};
    QPushButton *sendDeployBtn{};
    QPushButton *sendServoBtn{};

signals:
    void sendInitRequested(const QJsonObject &cfg);
    void sendCalibrationRequested(const QJsonObject &cfg);
    void sendStandbyRequested(const QJsonObject &cfg);
    void sendSearchRequested(const QJsonObject &cfg);
    // 发送二进制搜索任务包
    void sendSearchPacketRequested(const QByteArray &packet);
    // 通知更新目标地址
    void targetAddressChanged(const QString &ip, int port);
    void sendTrackRequested(const QJsonObject &cfg);
    void sendSimulationRequested(const QJsonObject &cfg);
    void sendPowerRequested(const QJsonObject &cfg);
    void sendDeployRequested(const QJsonObject &cfg);
    void sendServoRequested(const QJsonObject &cfg);

private:
    bool m_logIncoming = false; // 默认关闭接收报文日志
};
