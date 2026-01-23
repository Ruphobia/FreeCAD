#include "sketchview.h"
#include <QPainter>
#include <QtMath>
#include <QScrollBar>

SketchView::SketchView(QWidget* parent)
    : QGraphicsView(parent)
{
    m_scene = new QGraphicsScene(this);
    m_scene->setSceneRect(-2000, -2000, 4000, 4000);
    setScene(m_scene);

    setRenderHint(QPainter::Antialiasing);
    setDragMode(QGraphicsView::NoDrag);
    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
    setMouseTracking(true);

    // White sketch lines on dark background
    m_sketchPen = QPen(QColor(255, 255, 255), 2.0);
    m_sketchPen.setCosmetic(true);

    // Green dashed preview while drawing
    m_previewPen = QPen(QColor(0, 200, 0), 1.5, Qt::DashLine);
    m_previewPen.setCosmetic(true);

    setBackgroundBrush(QColor(50, 50, 50));

    // Center the view at origin
    centerOn(0, 0);
}

void SketchView::setTool(SketchTool tool)
{
    finishCurrentOperation();
    m_tool = tool;

    if (tool == SketchTool::None)
        setCursor(Qt::ArrowCursor);
    else
        setCursor(Qt::CrossCursor);
}

QPointF SketchView::snapToGrid(const QPointF& pt) const
{
    double x = qRound(pt.x() / m_gridSize) * m_gridSize;
    double y = qRound(pt.y() / m_gridSize) * m_gridSize;
    return QPointF(x, y);
}

void SketchView::finishCurrentOperation()
{
    m_drawing = false;
    m_clickCount = 0;
    m_polylinePoints.clear();
    m_polylinePath = QPainterPath();

    if (m_tempLine) { m_scene->removeItem(m_tempLine); delete m_tempLine; m_tempLine = nullptr; }
    if (m_tempCircle) { m_scene->removeItem(m_tempCircle); delete m_tempCircle; m_tempCircle = nullptr; }
    if (m_tempRect) { m_scene->removeItem(m_tempRect); delete m_tempRect; m_tempRect = nullptr; }
    if (m_tempPath) { m_scene->removeItem(m_tempPath); delete m_tempPath; m_tempPath = nullptr; }
    if (m_tempArcCircle) { m_scene->removeItem(m_tempArcCircle); delete m_tempArcCircle; m_tempArcCircle = nullptr; }
}

void SketchView::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::RightButton) {
        // Right click cancels current operation or finishes polyline
        if (m_tool == SketchTool::Polyline && m_polylinePoints.size() >= 2) {
            // Finish polyline
            auto* item = m_scene->addPath(m_polylinePath, m_sketchPen);
            item->setFlag(QGraphicsItem::ItemIsSelectable);
        }
        finishCurrentOperation();
        event->accept();
        return;
    }

    if (event->button() != Qt::LeftButton || m_tool == SketchTool::None) {
        QGraphicsView::mousePressEvent(event);
        return;
    }

    QPointF scenePos = snapToGrid(mapToScene(event->pos()));

    switch (m_tool) {
    case SketchTool::Line:
        if (!m_drawing) {
            m_startPoint = scenePos;
            m_drawing = true;
            m_tempLine = m_scene->addLine(QLineF(m_startPoint, m_startPoint), m_previewPen);
        } else {
            // Finalize line
            m_tempLine->setPen(m_sketchPen);
            m_tempLine->setLine(QLineF(m_startPoint, scenePos));
            m_tempLine->setFlag(QGraphicsItem::ItemIsSelectable);
            m_tempLine = nullptr;
            m_drawing = false;
        }
        break;

    case SketchTool::Circle:
        if (!m_drawing) {
            m_startPoint = scenePos;
            m_drawing = true;
            m_tempCircle = m_scene->addEllipse(
                m_startPoint.x(), m_startPoint.y(), 0, 0, m_previewPen);
        } else {
            double radius = QLineF(m_startPoint, scenePos).length();
            m_tempCircle->setRect(
                m_startPoint.x() - radius, m_startPoint.y() - radius,
                radius * 2, radius * 2);
            m_tempCircle->setPen(m_sketchPen);
            m_tempCircle->setFlag(QGraphicsItem::ItemIsSelectable);
            m_tempCircle = nullptr;
            m_drawing = false;
        }
        break;

    case SketchTool::Arc:
        m_clickCount++;
        if (m_clickCount == 1) {
            // First click: center
            m_arcCenter = scenePos;
            m_drawing = true;
            m_tempArcCircle = m_scene->addEllipse(
                m_arcCenter.x() - 1, m_arcCenter.y() - 1, 2, 2, m_previewPen);
        } else if (m_clickCount == 2) {
            // Second click: radius and start angle
            m_arcRadius = QLineF(m_arcCenter, scenePos).length();
            m_arcStartAngle = qAtan2(-(scenePos.y() - m_arcCenter.y()),
                                      scenePos.x() - m_arcCenter.x());
            // Remove the center marker
            if (m_tempArcCircle) {
                m_scene->removeItem(m_tempArcCircle);
                delete m_tempArcCircle;
                m_tempArcCircle = nullptr;
            }
            // Create temp arc path
            m_tempPath = m_scene->addPath(QPainterPath(), m_previewPen);
        } else if (m_clickCount == 3) {
            // Third click: end angle, finalize
            double endAngle = qAtan2(-(scenePos.y() - m_arcCenter.y()),
                                      scenePos.x() - m_arcCenter.x());
            double startDeg = qRadiansToDegrees(m_arcStartAngle);
            double endDeg = qRadiansToDegrees(endAngle);
            double spanDeg = endDeg - startDeg;
            if (spanDeg > 0) spanDeg -= 360;

            QPainterPath path;
            QRectF arcRect(m_arcCenter.x() - m_arcRadius, m_arcCenter.y() - m_arcRadius,
                           m_arcRadius * 2, m_arcRadius * 2);
            path.arcMoveTo(arcRect, startDeg);
            path.arcTo(arcRect, startDeg, spanDeg);

            if (m_tempPath) {
                m_scene->removeItem(m_tempPath);
                delete m_tempPath;
                m_tempPath = nullptr;
            }
            auto* item = m_scene->addPath(path, m_sketchPen);
            item->setFlag(QGraphicsItem::ItemIsSelectable);
            m_drawing = false;
            m_clickCount = 0;
        }
        break;

    case SketchTool::Rectangle:
        if (!m_drawing) {
            m_startPoint = scenePos;
            m_drawing = true;
            m_tempRect = m_scene->addRect(QRectF(m_startPoint, m_startPoint), m_previewPen);
        } else {
            QRectF rect(m_startPoint, scenePos);
            m_tempRect->setRect(rect.normalized());
            m_tempRect->setPen(m_sketchPen);
            m_tempRect->setFlag(QGraphicsItem::ItemIsSelectable);
            m_tempRect = nullptr;
            m_drawing = false;
        }
        break;

    case SketchTool::Polyline:
        if (!m_drawing) {
            m_polylinePoints.clear();
            m_polylinePath = QPainterPath();
            m_polylinePoints.push_back(scenePos);
            m_polylinePath.moveTo(scenePos);
            m_drawing = true;
            m_tempPath = m_scene->addPath(m_polylinePath, m_previewPen);
        } else {
            m_polylinePoints.push_back(scenePos);
            m_polylinePath.lineTo(scenePos);
            m_tempPath->setPath(m_polylinePath);
        }
        break;

    case SketchTool::Point: {
        double r = 3.0;
        auto* item = m_scene->addEllipse(
            scenePos.x() - r, scenePos.y() - r, r * 2, r * 2,
            m_sketchPen, QBrush(QColor(255, 255, 255)));
        item->setFlag(QGraphicsItem::ItemIsSelectable);
        break;
    }

    default:
        break;
    }

    event->accept();
}

void SketchView::mouseMoveEvent(QMouseEvent* event)
{
    QPointF scenePos = snapToGrid(mapToScene(event->pos()));

    if (m_drawing) {
        switch (m_tool) {
        case SketchTool::Line:
            if (m_tempLine)
                m_tempLine->setLine(QLineF(m_startPoint, scenePos));
            break;

        case SketchTool::Circle:
            if (m_tempCircle) {
                double radius = QLineF(m_startPoint, scenePos).length();
                m_tempCircle->setRect(
                    m_startPoint.x() - radius, m_startPoint.y() - radius,
                    radius * 2, radius * 2);
            }
            break;

        case SketchTool::Arc:
            if (m_clickCount == 2 && m_tempPath) {
                double endAngle = qAtan2(-(scenePos.y() - m_arcCenter.y()),
                                          scenePos.x() - m_arcCenter.x());
                double startDeg = qRadiansToDegrees(m_arcStartAngle);
                double endDeg = qRadiansToDegrees(endAngle);
                double spanDeg = endDeg - startDeg;
                if (spanDeg > 0) spanDeg -= 360;

                QPainterPath path;
                QRectF arcRect(m_arcCenter.x() - m_arcRadius, m_arcCenter.y() - m_arcRadius,
                               m_arcRadius * 2, m_arcRadius * 2);
                path.arcMoveTo(arcRect, startDeg);
                path.arcTo(arcRect, startDeg, spanDeg);
                m_tempPath->setPath(path);
            } else if (m_clickCount == 1 && m_tempArcCircle) {
                double radius = QLineF(m_arcCenter, scenePos).length();
                m_tempArcCircle->setRect(
                    m_arcCenter.x() - radius, m_arcCenter.y() - radius,
                    radius * 2, radius * 2);
            }
            break;

        case SketchTool::Rectangle:
            if (m_tempRect) {
                QRectF rect(m_startPoint, scenePos);
                m_tempRect->setRect(rect.normalized());
            }
            break;

        case SketchTool::Polyline:
            if (m_tempPath && !m_polylinePoints.empty()) {
                QPainterPath path = m_polylinePath;
                path.lineTo(scenePos);
                m_tempPath->setPath(path);
            }
            break;

        default:
            break;
        }
    }

    QGraphicsView::mouseMoveEvent(event);
}

void SketchView::mouseDoubleClickEvent(QMouseEvent* event)
{
    if (m_tool == SketchTool::Polyline && m_drawing && m_polylinePoints.size() >= 2) {
        // Double-click finishes polyline
        m_tempPath->setPath(m_polylinePath);
        m_tempPath->setPen(m_sketchPen);
        m_tempPath->setFlag(QGraphicsItem::ItemIsSelectable);
        m_tempPath = nullptr;
        m_drawing = false;
        m_polylinePoints.clear();
        m_polylinePath = QPainterPath();
        event->accept();
        return;
    }
    QGraphicsView::mouseDoubleClickEvent(event);
}

void SketchView::wheelEvent(QWheelEvent* event)
{
    double factor = event->angleDelta().y() > 0 ? 1.15 : 1.0 / 1.15;
    scale(factor, factor);
    event->accept();
}

void SketchView::drawBackground(QPainter* painter, const QRectF& rect)
{
    QGraphicsView::drawBackground(painter, rect);

    // Draw grid
    QPen gridPen(QColor(80, 80, 80), 0.5);
    gridPen.setCosmetic(true);
    painter->setPen(gridPen);

    double left = qFloor(rect.left() / m_gridSize) * m_gridSize;
    double top = qFloor(rect.top() / m_gridSize) * m_gridSize;

    for (double x = left; x < rect.right(); x += m_gridSize)
        painter->drawLine(QPointF(x, rect.top()), QPointF(x, rect.bottom()));
    for (double y = top; y < rect.bottom(); y += m_gridSize)
        painter->drawLine(QPointF(rect.left(), y), QPointF(rect.right(), y));

    // Draw axes
    QPen axisPen(QColor(120, 120, 120), 1.5);
    axisPen.setCosmetic(true);
    painter->setPen(axisPen);
    painter->drawLine(QPointF(rect.left(), 0), QPointF(rect.right(), 0));
    painter->drawLine(QPointF(0, rect.top()), QPointF(0, rect.bottom()));
}
