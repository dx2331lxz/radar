// RadarConfigWidget.cpp
#include "RadarConfigWidget.h"
#include "Protocol.h"
#include "RadarStatus.h"
#include "MessageIds.h"
#include "TrackMessage.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QByteArray>
#include <QTextStream>
#include <QTextDocument>
#include <QTextCursor>
#include <QTextBlock>
#include <QDateTime>
#include <QTreeWidget>
#include <QTimer>
#include <QHeaderView>
#include <QLabel>

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
    // removed: track/sim/servo sections

    // Operation log (replaces preview/actions)
    operationLog = new QPlainTextEdit();
    operationLog->setReadOnly(true);
    operationLog->setMinimumHeight(140);
    clearLogBtn = new QPushButton(tr("清空日志"));
    auto *logRow = new QHBoxLayout();
    logRow->addStretch();
    logRow->addWidget(clearLogBtn);
    root->addLayout(logRow);
    root->addWidget(operationLog, 1);

    // 目标分组视图：按威胁等级分为3组
    targetTree = new QTreeWidget();
    targetTree->setHeaderLabels({tr("目标ID"), tr("威胁分")});
    targetTree->setColumnCount(2);
    targetTree->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    targetTree->header()->setSectionResizeMode(1, QHeaderView::Stretch);
    // 添加三组顶级项
    auto *lvl1 = new QTreeWidgetItem(targetTree, QStringList{tr("一级 威胁 0.00-0.30")});
    auto *lvl2 = new QTreeWidgetItem(targetTree, QStringList{tr("二级 威胁 0.30-0.70")});
    auto *lvl3 = new QTreeWidgetItem(targetTree, QStringList{tr("三级 威胁 0.70-1.00")});
    lvl1->setData(0, Qt::UserRole, QVariant::fromValue(1));
    lvl2->setData(0, Qt::UserRole, QVariant::fromValue(2));
    lvl3->setData(0, Qt::UserRole, QVariant::fromValue(3));
    targetTree->addTopLevelItem(lvl1);
    targetTree->addTopLevelItem(lvl2);
    targetTree->addTopLevelItem(lvl3);
    targetTree->expandAll();
    root->addWidget(wrapWithTitle(tr("目标威胁分组"), targetTree));

    // 目标详情与打击操作区
    auto *detailW = new QWidget();
    auto *detailForm = new QFormLayout(detailW);
    detailIdLabel = new QLabel("-");
    detailScoreLabel = new QLabel("-");
    detailDistLabel = new QLabel("-");
    detailTypeLabel = new QLabel("-");
    lockBtn = new QPushButton(tr("锁定目标"));
    engageBtn = new QPushButton(tr("下达打击"));
    detailForm->addRow(tr("目标ID"), detailIdLabel);
    detailForm->addRow(tr("威胁分"), detailScoreLabel);
    detailForm->addRow(tr("距离(m)"), detailDistLabel);
    detailForm->addRow(tr("类型"), detailTypeLabel);
    auto *h = new QHBoxLayout();
    h->addWidget(lockBtn);
    h->addWidget(engageBtn);
    detailForm->addRow(h);
    root->addWidget(wrapWithTitle(tr("打击目标"), detailW));

    connect(lockBtn, &QPushButton::clicked, this, [this]()
            {
        if (m_selectedTargetId) {
            emit targetLockRequested(m_selectedTargetId);
            appendLog(QString("请求锁定目标: %1").arg(m_selectedTargetId));
        } });
    connect(engageBtn, &QPushButton::clicked, this, [this]()
            {
        if (m_selectedTargetId) {
            emit targetEngageRequested(m_selectedTargetId);
            appendLog(QString("请求下达打击: %1").arg(m_selectedTargetId));
        } });

    // 点击选择信号
    connect(targetTree, &QTreeWidget::itemClicked, this, &RadarConfigWidget::onTargetTreeItemClicked);

    // 清理过期目标（30s）
    targetCleanupTimer.setInterval(30000);
    connect(&targetCleanupTimer, &QTimer::timeout, this, [this]
            {
        const qint64 now = QDateTime::currentMSecsSinceEpoch();
        const qint64 keepMs = 60000; // 1min
        QList<quint16> toRemove;
        for (auto it = m_targets.begin(); it != m_targets.end(); ++it) {
            if (now - it.value().lastMs > keepMs) toRemove.append(it.key());
        }
        if (!toRemove.isEmpty()) {
            // remove from tree
            auto root = targetTree->invisibleRootItem();
            for (quint16 id : toRemove) {
                // search all children
                for (int gi = 0; gi < root->childCount(); ++gi) {
                    auto *g = root->child(gi);
                    for (int i = 0; i < g->childCount(); ++i) {
                        auto *it = g->child(i);
                        if (it->data(0, Qt::UserRole).toUInt() == id) {
                            g->removeChild(it);
                            break;
                        }
                    }
                }
                m_targets.remove(id);
            }
        } });
    targetCleanupTimer.start();

    // Connections
    connect(clearLogBtn, &QPushButton::clicked, this, [this]()
            { operationLog->clear(); });

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

// removed: buildTrackSection/buildServoSection implementations

void RadarConfigWidget::setDefaults()
{
    // 待机/搜索任务无可配置项，保持默认

    deployActionCombo->setCurrentIndex(0);
    // removed: track/sim/servo defaults

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
    // removed: track/simulation configurations from panel
    // j["power"] omitted
    j["deploy"] = QJsonObject{{"action", deployActionCombo->currentText()}};
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
    // show JSON snapshot in operation log
    appendLog(QString("配置预览:\n%1").arg(QString::fromUtf8(doc.toJson(QJsonDocument::Indented))));
}

void RadarConfigWidget::onRadarDatagramReceived(const QByteArray &data)
{
    // If logging incoming raw frames is enabled, append payload
    if (m_logIncoming)
    {
        QString msg = QString("[RADAR->APP] %1 (len=%2 bytes)").arg(QDateTime::currentDateTime().toString(Qt::ISODate)).arg(data.size());
        appendLog(msg);
    }

    // Try parse track message and update target grouping
    if (!TrackParser::hasReadableMagic(data))
        return;
    TrackMessage msg;
    if (!TrackParser::parseLittleEndian(data, msg))
        return;

    // (不再在日志中记录轨迹解析摘要)

    // Compute threat score using simplified logic from RadarScopeWidget
    // distance normalization (closer => higher)
    float maxRange = 5000.0f; // assume default if not known from status
    float dnorm = 1.0f - qBound(0.0f, msg.info.distance / maxRange, 1.0f);
    // speed normalization (higher => higher)
    float maxSpeed = 100.0f;
    float snorm = qBound(0.0f, qAbs(msg.info.speed) / maxSpeed, 1.0f);
    // type factor mapping (unknown=1, rotor=0.3, fixed=0.3, heli=0.6, civil=0.7, vehicle=0.6)
    float typeFactor = 1.0f;
    switch (msg.info.targetType)
    {
    case 0:
        typeFactor = 1.0f;
        break;
    case 1:
        typeFactor = 0.3f;
        break;
    case 2:
        typeFactor = 0.3f;
        break;
    case 3:
        typeFactor = 0.6f;
        break;
    case 4:
        typeFactor = 0.7f;
        break;
    case 5:
        typeFactor = 0.6f;
        break;
    default:
        typeFactor = 1.0f;
        break;
    }
    float tnorm = 1.0f - qBound(0.0f, typeFactor / 1.0f, 1.0f);
    float idnorm = (msg.info.targetType != 0) ? 0.0f : 1.0f;
    // weights: distance 0.3 speed 0.3 type 0.2 identity 0.2
    float score = 0.3f * dnorm + 0.3f * snorm + 0.2f * tnorm + 0.2f * idnorm;
    score = qBound(0.0f, score, 1.0f);

    // update target store
    const quint16 tid = msg.info.trackId;
    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    TargetInfo ti;
    ti.id = tid;
    ti.lastScore = score;
    ti.lastDistance = msg.info.distance;
    ti.lastMs = now;
    // If out of current detect range, remove if exists and skip adding
    if (msg.info.distance > m_currentDetectRange)
    {
        m_targets.remove(tid);
        // remove from tree if present
        auto root = targetTree->invisibleRootItem();
        for (int gi = 0; gi < root->childCount(); ++gi)
        {
            auto *g = root->child(gi);
            for (int i = g->childCount() - 1; i >= 0; --i)
            {
                auto *it = g->child(i);
                if (it->data(0, Qt::UserRole).toUInt() == tid)
                {
                    g->removeChild(it);
                }
            }
        }
        // update counts
        // (we'll update group labels below after insertion/removal)
        return;
    }
    m_targets[tid] = ti;

    // find and/or move tree item
    auto root = targetTree->invisibleRootItem();
    QTreeWidgetItem *foundItem = nullptr;
    QTreeWidgetItem *foundGroup = nullptr;
    for (int gi = 0; gi < root->childCount(); ++gi)
    {
        auto *g = root->child(gi);
        for (int i = 0; i < g->childCount(); ++i)
        {
            auto *it = g->child(i);
            if (it->data(0, Qt::UserRole).toUInt() == tid)
            {
                foundItem = it;
                foundGroup = g;
                break;
            }
        }
        if (foundItem)
            break;
    }

    // determine target group
    int groupIndex = 0;
    if (score >= 0.7f)
        groupIndex = 2; // third
    else if (score >= 0.3f)
        groupIndex = 1; // second
    else
        groupIndex = 0; // first
    auto *targetGroup = root->child(groupIndex);

    // if found and group changed, remove from old
    if (foundItem && foundGroup && foundGroup != targetGroup)
    {
        foundGroup->removeChild(foundItem);
        foundItem = nullptr;
    }

    if (!foundItem)
    {
        auto *it = new QTreeWidgetItem();
        it->setText(0, QString::number(tid));
        it->setText(1, QString::number(score, 'f', 2));
        it->setData(0, Qt::UserRole, QVariant::fromValue<uint>(tid));
        // set color by group
        QColor fg;
        if (groupIndex == 2)
            fg = QColor(220, 80, 80); // red-ish
        else if (groupIndex == 1)
            fg = QColor(220, 180, 60); // orange-ish
        else
            fg = QColor(120, 200, 255); // blue-ish
        it->setForeground(0, fg);
        it->setForeground(1, fg);
        targetGroup->addChild(it);
    }
    else
    {
        foundItem->setText(1, QString::number(score, 'f', 2));
    }
    // after any change: sort children of each group by score desc and update group titles with counts
    auto root2 = targetTree->invisibleRootItem();
    for (int gi = 0; gi < root2->childCount(); ++gi)
    {
        auto *g = root2->child(gi);
        // collect children with scores
        QVector<QPair<float, QTreeWidgetItem *>> arr;
        for (int i = 0; i < g->childCount(); ++i)
        {
            auto *c = g->child(i);
            float sc = c->text(1).toFloat();
            arr.append({sc, c});
        }
        std::sort(arr.begin(), arr.end(), [](const QPair<float, QTreeWidgetItem *> &a, const QPair<float, QTreeWidgetItem *> &b)
                  { return a.first > b.first; });
        g->takeChildren();
        for (auto &p : arr)
            g->addChild(p.second);
        // update group label with count
        int count = g->childCount();
        QString baseLabel;
        if (gi == 0)
            baseLabel = tr("一级 威胁 0.00-0.30");
        else if (gi == 1)
            baseLabel = tr("二级 威胁 0.30-0.70");
        else
            baseLabel = tr("三级 威胁 0.70-1.00");
        g->setText(0, QString("%1 (%2)").arg(baseLabel).arg(count));
    }
    targetTree->expandAll();
}

void RadarConfigWidget::onRadarStatusUpdated(const RadarStatus &s)
{
    m_isRetracted = s.retracted;
    // update current detect range and remove targets beyond it
    if (s.detectRange > 0)
        m_currentDetectRange = float(s.detectRange);
    // remove any stored targets beyond new range
    QList<quint16> toRemove;
    for (auto it = m_targets.begin(); it != m_targets.end(); ++it)
    {
        if (it.value().lastDistance > m_currentDetectRange)
            toRemove.append(it.key());
    }
    if (!toRemove.isEmpty())
    {
        auto root = targetTree->invisibleRootItem();
        for (quint16 id : toRemove)
        {
            for (int gi = 0; gi < root->childCount(); ++gi)
            {
                auto *g = root->child(gi);
                for (int i = g->childCount() - 1; i >= 0; --i)
                {
                    auto *it = g->child(i);
                    if (it->data(0, Qt::UserRole).toUInt() == id)
                    {
                        g->removeChild(it);
                    }
                }
            }
            m_targets.remove(id);
        }
        // update group labels counts
        for (int gi = 0; gi < targetTree->invisibleRootItem()->childCount(); ++gi)
        {
            auto *g = targetTree->invisibleRootItem()->child(gi);
            int count = g->childCount();
            QString baseLabel;
            if (gi == 0)
                baseLabel = tr("一级 威胁 0.00-0.30");
            else if (gi == 1)
                baseLabel = tr("二级 威胁 0.30-0.70");
            else
                baseLabel = tr("三级 威胁 0.70-1.00");
            g->setText(0, QString("%1 (%2)").arg(baseLabel).arg(count));
        }
    }
}

// Per-section send handlers: build that section's JSON, emit signal and show in preview

void RadarConfigWidget::onSendStandby()
{
    // 预览仅展示动作
    QJsonObject j{{"action", QStringLiteral("standby")}};
    appendLog(QString("发送 待机 任务:\n%1").arg(QString::fromUtf8(QJsonDocument(j).toJson(QJsonDocument::Indented))));

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
    // log hex dump of sent binary
    appendLog(QString("已发送 待机 包 len=%1\n%2").arg(packet.size()).arg(QString::fromLatin1(packet.toHex(' ').toUpper())));
}

void RadarConfigWidget::onSendSearch()
{
    // 如果雷达处于撤收状态，阻止发送并提示
    if (m_isRetracted)
    {
        appendLog(tr("雷达当前处于撤收状态，无法进入搜索。请先展开雷达。"));
        return;
    }

    // 1) JSON预览仅显示操作类型
    QJsonObject j{{"action", QStringLiteral("search")}};
    appendLog(QString("发送 搜索 任务:\n%1").arg(QString::fromUtf8(QJsonDocument(j).toJson(QJsonDocument::Indented))));

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
    appendLog(QString("已发送 搜索 包 len=%1\n%2").arg(packet.size()).arg(QString::fromLatin1(packet.toHex(' ').toUpper())));
}

// removed: track/simulation handlers

void RadarConfigWidget::onSendDeploy()
{
    const bool isDeploy = (deployActionCombo && deployActionCombo->currentIndex() == 0); // 0: 展开, 1: 撤收
    QJsonObject j{{"action", isDeploy ? QStringLiteral("deploy") : QStringLiteral("retract")}};
    emit sendDeployRequested(j);
    appendLog(QString("发送 展开/撤收 任务:\n%1").arg(QString::fromUtf8(QJsonDocument(j).toJson(QJsonDocument::Indented))));

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
    appendLog(QString("已发送 展开/撤收 包 len=%1\n%2").arg(packet.size()).arg(QString::fromLatin1(packet.toHex(' ').toUpper())));
}

void RadarConfigWidget::onTargetTreeItemClicked(QTreeWidgetItem *item, int column)
{
    Q_UNUSED(column);
    if (!item)
        return;
    // top-level groups have no associated target id
    quint32 id = item->data(0, Qt::UserRole).toUInt();
    if (id == 0)
        return; // groups stored with no id
    m_selectedTargetId = quint16(id);
    // populate detail labels from m_targets if possible
    if (m_targets.contains(m_selectedTargetId))
    {
        auto t = m_targets.value(m_selectedTargetId);
        detailIdLabel->setText(QString::number(t.id));
        detailScoreLabel->setText(QString::number(t.lastScore, 'f', 2));
        detailDistLabel->setText(QString::number(t.lastDistance, 'f', 1));
        // type not stored here; leave as '-' or enhance to store
        detailTypeLabel->setText("-");
    }
    else
    {
        detailIdLabel->setText(QString::number(m_selectedTargetId));
        detailScoreLabel->setText("-");
        detailDistLabel->setText("-");
        detailTypeLabel->setText("-");
    }
    emit targetSelected(quint16(id));
}

void RadarConfigWidget::removeTargetById(quint16 id)
{
    if (m_targets.contains(id))
        m_targets.remove(id);
    // remove from tree
    auto root = targetTree->invisibleRootItem();
    for (int gi = 0; gi < root->childCount(); ++gi)
    {
        auto *g = root->child(gi);
        for (int i = g->childCount() - 1; i >= 0; --i)
        {
            auto *it = g->child(i);
            if (it->data(0, Qt::UserRole).toUInt() == id)
            {
                g->removeChild(it);
            }
        }
        // update group label counts
        int count = g->childCount();
        QString baseLabel;
        if (gi == 0)
            baseLabel = tr("一 级 威胁 0.00-0.30");
        else if (gi == 1)
            baseLabel = tr("二 级 威胁 0.30-0.70");
        else
            baseLabel = tr("三 级 威胁 0.70-1.00");
        g->setText(0, QString("%1 (%2)").arg(baseLabel).arg(count));
    }
}

void RadarConfigWidget::appendLog(const QString &msg)
{
    if (!operationLog)
        return;
    const QString line = QString("[%1] %2").arg(QDateTime::currentDateTime().toString(Qt::ISODate)).arg(msg);
    operationLog->appendPlainText(line);
    // trim if exceeding max lines
    if (m_maxLogLines > 0)
    {
        int blocks = operationLog->blockCount();
        if (blocks > m_maxLogLines)
        {
            int removeCount = blocks - m_maxLogLines;
            QTextDocument *doc = operationLog->document();
            QTextCursor cur(doc);
            // move to start
            cur.setPosition(0);
            // advance removeCount blocks
            for (int i = 0; i < removeCount; ++i)
            {
                cur.movePosition(QTextCursor::NextBlock, QTextCursor::KeepAnchor);
            }
            // remove the selected leading blocks
            cur.removeSelectedText();
            // remove leading newline if present
            if (doc->firstBlock().text().isEmpty())
            {
                QTextCursor c2(doc);
                c2.setPosition(doc->firstBlock().position());
                c2.movePosition(QTextCursor::NextBlock, QTextCursor::KeepAnchor);
                c2.removeSelectedText();
            }
        }
    }
}

// removed: servo handler
