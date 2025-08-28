// RadarScopeWidget.cpp
#include "RadarScopeWidget.h"
#include <QPainter>
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
    connect(&m_cleanupTimer, &QTimer::timeout, this, [this] {
        const qint64 now = QDateTime::currentMSecsSinceEpoch();
        const qint64 keepMs = 30 * 1000; // 保留30秒轨迹
        for (auto &t : m_trails) {
            while (!t.points.isEmpty() && now - t.points.front().ms > keepMs) {
                t.points.removeFirst();
            }
        }
        // 移除空轨迹
        m_trails.erase(std::remove_if(m_trails.begin(), m_trails.end(), [](const Trail &tr){ return tr.points.isEmpty(); }), m_trails.end());
        update();
    });
    m_cleanupTimer.start();
}

void RadarScopeWidget::setMaxRangeMeters(float r)
{
    m_maxRange = qMax(100.0f, r);
    update();
}

void RadarScopeWidget::onTrackDatagram(const QByteArray &data)
{
    TrackMessage msg;
    if (!TrackParser::parseLittleEndian(data, msg))
        return;

    // 将距离/方位转为平面点；以窗口中心为原点，北向上、东向右。
    const float d = msg.info.distance; // 已是直线距离（米）
    const float az = msg.info.azimuth; // 相对正北顺时针
    const QRectF circle(rect());
    QPointF p = polarToPoint(d, az, circle);

    // 找到/创建轨迹
    auto it = std::find_if(m_trails.begin(), m_trails.end(), [&](const Trail &t){ return t.id == msg.info.trackId; });
    if (it == m_trails.end()) {
        Trail t; t.id = msg.info.trackId; m_trails.push_back(t); it = m_trails.end() - 1;
    }
    it->points.push_back({p, QDateTime::currentMSecsSinceEpoch()});
    if (it->points.size() > 256) it->points.remove(0);
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

void RadarScopeWidget::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);

    const QRectF rc = rect();
    const QPointF c = rc.center();
    const float R = 0.48f * qMin(rc.width(), rc.height());
    const QRectF circle(c.x() - R, c.y() - R, 2*R, 2*R);

    // 背景雷达扇面
    p.setPen(QPen(QColor(40, 120, 40), 2));
    p.setBrush(QColor(20, 60, 20));
    p.drawEllipse(circle);

    // 圈层与刻度
    p.setPen(QPen(QColor(60, 180, 60), 1));
    for (int i=1;i<=4;++i) {
        p.drawEllipse(QRectF(c.x()-R*i/4.0, c.y()-R*i/4.0, 2*R*i/4.0, 2*R*i/4.0));
    }
    // 十字和方位刻度
    p.drawLine(QPointF(c.x()-R, c.y()), QPointF(c.x()+R, c.y()));
    p.drawLine(QPointF(c.x(), c.y()-R), QPointF(c.x(), c.y()+R));
    p.setPen(QPen(QColor(120, 220, 120), 1));
    for (int deg=0; deg<360; deg+=30) {
        const float theta = qDegreesToRadians(90.0f - float(deg));
        QPointF o = QPointF(c.x() + (R-10)*qCos(theta), c.y() - (R-10)*qSin(theta));
        QPointF i = QPointF(c.x() + R*qCos(theta),       c.y() - R*qSin(theta));
        p.drawLine(o, i);
        QString label = QString::number(deg);
        QPointF t = QPointF(c.x() + (R+12)*qCos(theta), c.y() - (R+12)*qSin(theta));
        p.drawText(QRectF(t.x()-12, t.y()-8, 24, 16), Qt::AlignCenter, label);
    }

    // 量程标签
    p.setPen(QColor(150, 240, 150));
    for (int i=1;i<=4;++i) {
        const float rr = R*i/4.0f;
        const float range = m_maxRange * (i/4.0f);
        p.drawText(QRectF(c.x()+rr-24, c.y()-12, 48, 16), Qt::AlignCenter, QString::number(int(range/1000)) + " km");
    }

    // 画轨迹
    p.setPen(QPen(QColor(50, 150, 255), 2));
    p.setBrush(QColor(50, 150, 255));
    for (const auto &t : m_trails) {
        // 折线
        for (int i=1;i<t.points.size();++i) {
            p.drawLine(t.points[i-1].pos, t.points[i].pos);
        }
        // 末端点
        if (!t.points.isEmpty()) {
            p.setBrush(QColor(80, 180, 255));
            p.drawEllipse(t.points.back().pos, 3, 3);
            p.drawText(QRectF(t.points.back().pos.x()+4, t.points.back().pos.y()-8, 40, 16), QString::number(t.id));
        }
    }
}
