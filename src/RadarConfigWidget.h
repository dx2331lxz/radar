// RadarConfigWidget.h
#pragma once

#include <QWidget>
#include "RadarStatus.h"
#include <QGroupBox>
#include <QTreeWidget>
#include <QTimer>
#include <QLabel>
#include <QHash>
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
    // 当雷达盘通知目标被击毁时，移除右侧面板中的目标显示
    void removeTargetById(quint16 id);

private slots:
    void onApply();
    void onReset();
    void onPreview();
    // removed: onSendInit(), onSendCalibration()
    void onSendStandby();
    void onSendSearch();
    // removed: 跟踪/模拟/伺服发送槽
    // removed: onSendPower()
    void onSendDeploy();
    void onTargetTreeItemClicked(QTreeWidgetItem *item, int column);

private:
    // UI builders
    // removed: buildInitSection(), buildCalibSection()
    QGroupBox *buildStandbySection();
    QGroupBox *buildSearchSection();
    QGroupBox *buildHeaderSection();
    // removed: track/sim/servo sections
    // removed: buildPowerSection()
    QGroupBox *buildDeploySection();

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

    // removed: 跟踪/模拟任务成员

    // removed: 上下电任务

    // 展开撤收任务
    QComboBox *deployActionCombo{}; // 展开/撤收

    // removed: 伺服任务成员

    // Bottom actions
    QPushButton *applyBtn{};
    QPushButton *resetBtn{};
    QPushButton *previewBtn{};
    QTextEdit *previewEdit{};
    // Per-section send buttons
    // removed: sendInitBtn, sendCalibBtn
    QPushButton *sendStandbyBtn{};
    QPushButton *sendSearchBtn{};
    // removed: sendTrackBtn/sendSimBtn
    // removed: sendPowerBtn
    QPushButton *sendDeployBtn{};

    // 目标分组面板（右侧实时目标列表）
    QTreeWidget *targetTree{}; // 顶层有3个组：一级/二级/三级
    QTimer targetCleanupTimer; // 清理过期目标

    struct TargetInfo
    {
        quint16 id{0};
        float lastScore{0.0f};
        qint64 lastMs{0};
        float lastDistance{0.0f};
    };
    QHash<quint16, TargetInfo> m_targets; // keyed by track id
    // 当前已知雷达探测量程（由状态报文更新）
    float m_currentDetectRange = 5000.0f;
    // 当前选中目标（0表示无）
    quint16 m_selectedTargetId = 0;
    // 打击操作区控件
    QLabel *detailIdLabel{};
    QLabel *detailScoreLabel{};
    QLabel *detailDistLabel{};
    QLabel *detailTypeLabel{};
    QPushButton *lockBtn{};
    QPushButton *engageBtn{};

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
    // removed: track/simulation signals
    // removed: sendPowerRequested
    void sendDeployRequested(const QJsonObject &cfg);
    // 发送二进制展开/撤收任务包
    void sendDeployPacketRequested(const QByteArray &packet);
    // 当用户在右侧面板选择目标时发出 id
    void targetSelected(quint16 id);
    // 用户请求锁定或下达打击某目标（id）
    void targetLockRequested(quint16 id);
    void targetEngageRequested(quint16 id);
    // removed: servo signal

private:
    bool m_logIncoming = false; // 默认关闭接收报文日志
    bool m_isRetracted = false; // 最近一次状态是否处于撤收
};
