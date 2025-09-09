// RadarScopeWidget.cpp
#include "RadarScopeWidget.h"
#include <QPainter>
#include <QPainterPath>
#include <QConicalGradient>
#include <QtMath>
#include <QDateTime>

RadarScopeWidget::RadarScopeWidget(QWidget *parent)
    : QWidget(parent)
{
    setAutoFillBackground(true);
    QPalette pal = palette();
    pal.setColor(QPalette::Window, QColor(10, 20, 10));
    setPalette(pal);

    m_cleanupTimer.setInterval(1000);
    connect(&m_cleanupTimer, &QTimer::timeout, this, [this]
            {
        const qint64 now = QDateTime::currentMSecsSinceEpoch();
    const qint64 keepMs = m_trailKeepMs; // 可配置的轨迹保留时长
        for (auto &t : m_trails) {
            while (!t.points.isEmpty() && now - t.points.front().ms > keepMs) {
                t.points.removeFirst();
            }
        }
        // 移除空轨迹
        m_trails.erase(std::remove_if(m_trails.begin(), m_trails.end(), [](const Trail &tr){ return tr.points.isEmpty(); }), m_trails.end());
        update(); });
    m_cleanupTimer.start();

    // 扫描线动画：每16ms更新一次角度
    m_sweepTimer.setInterval(16);
    connect(&m_sweepTimer, &QTimer::timeout, this, [this]
            {
        if (!m_sweepOn) return;
        // 每tick转动角度
        m_sweepAngle += m_sweepSpeed * (m_sweepTimer.interval() / 1000.0f);
        while (m_sweepAngle >= 360.0f) m_sweepAngle -= 360.0f;
        update(); });
}

void RadarScopeWidget::setMaxRangeMeters(float r)
{
    m_maxRange = qMax(100.0f, r);
    update();
}

void RadarScopeWidget::onTrackDatagram(const QByteArray &data)
{
    // 轻量过滤 + 解析
    if (!TrackParser::hasReadableMagic(data))
        return;

    TrackMessage msg;
    if (!TrackParser::parseLittleEndian(data, msg))
        return;

    // 将距离/方位转为平面点；以窗口中心为原点，北向上、东向右。
    const float d = msg.info.distance; // 已是直线距离（米）
    const float az = msg.info.azimuth; // 相对正北顺时针
    const QRectF circle(rect());
    QPointF p = polarToPoint(d, az, circle);

    // 找到/创建轨迹
    auto it = std::find_if(m_trails.begin(), m_trails.end(), [&](const Trail &t)
                           { return t.id == msg.info.trackId; });
    if (it == m_trails.end())
    {
        Trail t;
        t.id = msg.info.trackId;
        m_trails.push_back(t);
        it = m_trails.end() - 1;
        if (m_showNotices)
        {
            m_notices.push_back({tr("发现新目标 #%1").arg(t.id), QDateTime::currentMSecsSinceEpoch()});
        }
    }
    // 更新目标类型/尺寸/速度/距离/身份信息
    it->targetType = msg.info.targetType;
    it->targetSize = msg.info.targetSize;
    it->lastSpeed = qAbs(msg.info.speed);
    it->lastDistance = msg.info.distance;
    // 假设身份由 targetType==0 表示未知，否则视为已知
    it->identityKnown = (msg.info.targetType != 0);
    it->points.push_back({p, QDateTime::currentMSecsSinceEpoch()});
    if (it->points.size() > m_maxTrailPoints)
        it->points.remove(0);
    update();
}

QPointF RadarScopeWidget::polarToPoint(float distance_m, float azimuth_deg, const QRectF &circle) const
{
    const float r = qBound(0.0f, distance_m, m_maxRange);
    const float ratio = r / m_maxRange; // 0..1
    // 绘图半径取正方形中最小边
    const float radius = 0.48f * qMin(circle.width(), circle.height());
    // 转屏幕坐标：北(0°)向上 => 屏幕y减；顺时针为正 => 屏幕角度 = -azimuth + 90? 以北为0度，转换到数学坐标x轴正右、逆时针为正：theta = 90° - azimuth
    const float theta = qDegreesToRadians(90.0f - azimuth_deg);
    const float rr = ratio * radius;
    const QPointF center = circle.center();
    const float x = center.x() + rr * qCos(theta);
    const float y = center.y() - rr * qSin(theta); // y向下为正，故减
    return {x, y};
}

QString RadarScopeWidget::typeSizeLabel(int type, int size)
{
    QString t;
    switch (type)
    {
    case 0:
        t = QStringLiteral("未知");
        break;
    case 1:
        t = QStringLiteral("旋翼");
        break;
    case 2:
        t = QStringLiteral("固定翼");
        break;
    case 3:
        t = QStringLiteral("直升机");
        break;
    case 4:
        t = QStringLiteral("民航");
        break;
    case 5:
        t = QStringLiteral("车辆");
        break;
    default:
        t = QStringLiteral("?%1").arg(type);
        break;
    }
    QString s;
    switch (size)
    {
    case 0:
        s = QStringLiteral("小");
        break;
    case 1:
        s = QStringLiteral("中");
        break;
    case 2:
        s = QStringLiteral("大");
        break;
    case 3:
        s = QStringLiteral("特大");
        break;
    default:
        s = QStringLiteral("?%1").arg(size);
        break;
    }
    return t + QStringLiteral("·") + s;
}

float RadarScopeWidget::computeThreatScore(const RadarScopeWidget::Trail &t) const
{
    // 规范化距离：越近 -> 越高威胁。这里用简单的线性归一：d_norm = 1 - min(d / maxRange, 1)
    float dnorm = 1.0f - qBound(0.0f, t.lastDistance / m_maxRange, 1.0f);
    // 速度归一化：速度越高 -> 越高威胁
    float snorm = qBound(0.0f, t.lastSpeed / m_maxSpeed, 1.0f);
    // 类型风险因子：按要求映射（未知=1, 旋翼=0.3, 固定翼=0.3, 直升机=0.6, 民航=0.7, 车=0.6）
    float typeFactor = 1.0f;
    switch (t.targetType)
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
    // 将 typeFactor 归一到 0..1(较高值表示更危险)。已知 mapping: unknown=1(high), others lower.
    // We invert so that higher normalized value = more dangerous: tnorm = 1 - (typeFactor/maxPossible)
    const float maxTypeVal = 1.0f; // since unknown uses 1.0
    float tnorm = 1.0f - qBound(0.0f, typeFactor / maxTypeVal, 1.0f);

    // 身份：未知=1（危险），已知=0（安全） -> identityNorm in [0..1]
    float idnorm = t.identityKnown ? 0.0f : 1.0f;

    // 加权合成（注意权重之和为1）
    float score = m_weightDistance * dnorm + m_weightSpeed * snorm + m_weightType * tnorm + m_weightIdentity * idnorm;
    // clamp
    return qBound(0.0f, score, 1.0f);
}

void RadarScopeWidget::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);

    const QRectF rc = rect();
    const QPointF c = rc.center();
    const float R = 0.48f * qMin(rc.width(), rc.height());
    const QRectF circle(c.x() - R, c.y() - R, 2 * R, 2 * R);

    // 背景雷达扇面
    p.setPen(QPen(QColor(40, 120, 40), 2));
    p.setBrush(QColor(20, 60, 20));
    p.drawEllipse(circle);

    // 圈层与刻度
    p.setPen(QPen(QColor(60, 180, 60), 1));
    for (int i = 1; i <= 4; ++i)
    {
        p.drawEllipse(QRectF(c.x() - R * i / 4.0, c.y() - R * i / 4.0, 2 * R * i / 4.0, 2 * R * i / 4.0));
    }
    // 十字和方位刻度
    p.drawLine(QPointF(c.x() - R, c.y()), QPointF(c.x() + R, c.y()));
    p.drawLine(QPointF(c.x(), c.y() - R), QPointF(c.x(), c.y() + R));
    p.setPen(QPen(QColor(120, 220, 120), 1));
    for (int deg = 0; deg < 360; deg += 30)
    {
        const float theta = qDegreesToRadians(90.0f - float(deg));
        QPointF o = QPointF(c.x() + (R - 10) * qCos(theta), c.y() - (R - 10) * qSin(theta));
        QPointF i = QPointF(c.x() + R * qCos(theta), c.y() - R * qSin(theta));
        p.drawLine(o, i);
        QString label = QString::number(deg);
        QPointF t = QPointF(c.x() + (R + 12) * qCos(theta), c.y() - (R + 12) * qSin(theta));
        p.drawText(QRectF(t.x() - 12, t.y() - 8, 24, 16), Qt::AlignCenter, label);
    }

    // 量程标签
    p.setPen(QColor(150, 240, 150));
    for (int i = 1; i <= 4; ++i)
    {
        const float rr = R * i / 4.0f;
        const float range = m_maxRange * (i / 4.0f);
        p.drawText(QRectF(c.x() + rr - 24, c.y() - 12, 48, 16), Qt::AlignCenter, QString::number(int(range / 1000)) + " km");
    }

    // 画轨迹（根据点的时间做轻微衰减）
    for (const auto &t : m_trails)
    {
        if (t.points.size() < 2)
            continue;
        const qint64 now = QDateTime::currentMSecsSinceEpoch();
        for (int i = 1; i < t.points.size(); ++i)
        {
            const qint64 age = now - t.points[i].ms;
            const float alpha = qBound(30.0f, 255.0f * (1.0f - float(age) / float(qMax<qint64>(1, m_trailKeepMs))), 255.0f);
            QPen pen(QColor(50, 150, 255, int(alpha)));
            pen.setWidth(2);
            p.setPen(pen);
            p.drawLine(t.points[i - 1].pos, t.points[i].pos);
        }
        // 末端点
        const auto &last = t.points.back();
        // 根据威胁得分计算颜色（蓝->红）
        const float score = computeThreatScore(t); // 0..1
        QColor col;
        // interpolate blue (0,128,255) -> red (255,50,50)
        col.setRed(int(80 + 175 * score));
        col.setGreen(int(180 - 130 * score));
        col.setBlue(int(255 - 205 * score));
        p.setPen(Qt::NoPen);
        p.setBrush(col);
        p.drawEllipse(last.pos, 4, 4);
        // 绘制类型/尺寸标签
        QString label = RadarScopeWidget::typeSizeLabel(t.targetType, t.targetSize);
        if (!label.isEmpty())
        {
            QFont f = p.font();
            f.setPointSize(8);
            p.setFont(f);
            p.setPen(QColor(230, 230, 230));
            QRectF tr(last.pos.x() + 8, last.pos.y() - 10, 120, 14);
            // 显示 类型·尺寸 以及威胁分 (0.00~1.00)
            p.drawText(tr, Qt::AlignLeft | Qt::AlignVCenter, label + QStringLiteral(" ") + QString::number(score, 'f', 2));
        }
        // 如果有目标类型/尺寸信息（存在于 Trail structure? we need to map by id）
        // 在当前实现中 Trail only contains points; try to extract type/size from notices or external source.
        // For now, no per-trail metadata stored; label drawing will be skipped unless we have info.
    }

    // 左上角提示（只显示关键信息）
    if (m_showNotices && !m_notices.isEmpty())
    {
        const qint64 now = QDateTime::currentMSecsSinceEpoch();
        // 清理过期
        m_notices.erase(std::remove_if(m_notices.begin(), m_notices.end(), [&](const Notice &n)
                                       { return now - n.ms > m_noticeKeepMs; }),
                        m_notices.end());
        // 绘制剩余
        int y = 8;
        for (const auto &n : m_notices)
        {
            const qint64 age = now - n.ms;
            float a = 1.0f - float(age) / float(m_noticeKeepMs);
            a = qBound(0.0f, a, 1.0f);
            QColor fg(230, 250, 230, int(255 * a));
            QColor bg(0, 0, 0, int(120 * a));
            p.setPen(Qt::NoPen);
            p.setBrush(bg);
            QFont f = p.font();
            f.setBold(true);
            p.setFont(f);
            const QString text = n.text;
            const QFontMetrics fm(p.font());
            const int w = fm.horizontalAdvance(text) + 12;
            const int h = fm.height() + 8;
            QRect r(8, y, w, h);
            p.drawRoundedRect(r, 6, 6);
            p.setPen(fg);
            p.drawText(r.adjusted(6, 4, -6, -4), Qt::AlignVCenter | Qt::AlignLeft, text);
            y += h + 6;
        }
    }

    // 扫描线与余辉
    if (m_sweepOn)
    {
        // 计算几何
        const float R = circle.width() / 2.0f;
        const QPointF c = circle.center();
        // 扫描方向：从中心向外一条亮线 + 线后短扇形余辉
        const float theta = qDegreesToRadians(90.0f - m_sweepAngle);
        QPointF tip(c.x() + R * qCos(theta), c.y() - R * qSin(theta));

        // 主扫描线
        QPen pen(QColor(80, 220, 120), 2);
        p.setPen(pen);
        p.drawLine(c, tip);

        // 余辉扇形（约6度宽，线性渐隐）
        p.setPen(Qt::NoPen);
        QConicalGradient grad(c, m_sweepAngle);
        QColor head = QColor(100, 255, 140, 120);
        QColor tail = QColor(0, 0, 0, 0);
        grad.setColorAt(0.00, head);
        grad.setColorAt(0.25, QColor(60, 180, 100, 60));
        grad.setColorAt(1.00, tail);
        p.setBrush(grad);

        // 使用path绘制窄扇形
        QPainterPath path;
        path.moveTo(c);
        // 扇形角宽（度）
        const float span = 8.0f;
        QRectF arcRect(circle);
        // Qt的drawPie以3点钟方向为0度，逆时针为正，因此做一个角度换算：
        float startAngle = -(m_sweepAngle + span / 2.0f) + 90.0f; // 转到Qt角度系
        // 转为1/16度
        int start16 = int(startAngle * 16.0f);
        int span16 = int(span * 16.0f);
        p.drawPie(arcRect, start16, span16);
    }
}

void RadarScopeWidget::setSearchActive(bool on)
{
    if (m_sweepOn == on)
        return;
    m_sweepOn = on;
    if (m_sweepOn)
    {
        m_sweepAngle = 0.0f;
        if (!m_sweepTimer.isActive())
            m_sweepTimer.start();
    }
    else
    {
        m_sweepTimer.stop();
    }
    update();
}

void RadarScopeWidget::clearTrails()
{
    m_trails.clear();
    m_notices.clear();
    update();
}
