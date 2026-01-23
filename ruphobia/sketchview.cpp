#include "sketchview.h"
#include "dimensionitem.h"
#include <QPainter>
#include <QtMath>
#include <QScrollBar>
#include <limits>

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
    setFocusPolicy(Qt::StrongFocus);

    // White sketch lines on dark background
    m_sketchPen = QPen(QColor(255, 255, 255), 2.0);
    m_sketchPen.setCosmetic(true);

    // Green dashed preview while drawing
    m_previewPen = QPen(QColor(0, 200, 0), 1.5, Qt::DashLine);
    m_previewPen.setCosmetic(true);

    // Red dimension annotations
    m_dimensionPen = QPen(QColor(255, 80, 80), 1.5);
    m_dimensionPen.setCosmetic(true);

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
    if (m_tempDimLine) { m_scene->removeItem(m_tempDimLine); delete m_tempDimLine; m_tempDimLine = nullptr; }
}

static void addDimensionAnnotation(QGraphicsScene* scene, const QPointF& p1, const QPointF& p2,
                                   const QPen& pen, QGraphicsItem* sourceItem,
                                   DimensionItem::GeomType geomType, int edgeIndex = 0)
{
    double distance = QLineF(p1, p2).length();
    if (distance < 0.01)
        return;

    auto* dim = new DimensionItem(p1, p2, pen, sourceItem, geomType, edgeIndex);
    scene->addItem(dim);
}

void SketchView::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::RightButton) {
        if (m_tool == SketchTool::Polyline && m_polylinePoints.size() >= 2) {
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
            m_arcCenter = scenePos;
            m_drawing = true;
            m_tempArcCircle = m_scene->addEllipse(
                m_arcCenter.x() - 1, m_arcCenter.y() - 1, 2, 2, m_previewPen);
        } else if (m_clickCount == 2) {
            m_arcRadius = QLineF(m_arcCenter, scenePos).length();
            m_arcStartAngle = qAtan2(-(scenePos.y() - m_arcCenter.y()),
                                      scenePos.x() - m_arcCenter.x());
            if (m_tempArcCircle) {
                m_scene->removeItem(m_tempArcCircle);
                delete m_tempArcCircle;
                m_tempArcCircle = nullptr;
            }
            m_tempPath = m_scene->addPath(QPainterPath(), m_previewPen);
        } else if (m_clickCount == 3) {
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

    case SketchTool::Dimension: {
        // Click on existing geometry to dimension it (with tolerance)
        double tol = 5.0;
        QRectF hitArea(scenePos.x() - tol, scenePos.y() - tol, tol * 2, tol * 2);
        QList<QGraphicsItem*> hitItems = m_scene->items(
            hitArea, Qt::IntersectsItemShape, Qt::DescendingOrder,
            QTransform());

        // Find the nearest edge and dimension it
        for (auto* item : hitItems) {
            // Skip dimension items themselves
            if (dynamic_cast<DimensionItem*>(item))
                continue;

            if (auto* lineItem = dynamic_cast<QGraphicsLineItem*>(item)) {
                QLineF line = lineItem->line();
                addDimensionAnnotation(m_scene, line.p1(), line.p2(), m_dimensionPen,
                                       lineItem, DimensionItem::GeomType::Line);
                break;
            }
            if (auto* rectItem = dynamic_cast<QGraphicsRectItem*>(item)) {
                QRectF r = rectItem->rect();
                QLineF edges[4] = {
                    QLineF(r.topLeft(), r.topRight()),       // top
                    QLineF(r.bottomLeft(), r.bottomRight()), // bottom
                    QLineF(r.topLeft(), r.bottomLeft()),     // left
                    QLineF(r.topRight(), r.bottomRight())    // right
                };
                double minDist = std::numeric_limits<double>::max();
                int closest = 0;
                for (int i = 0; i < 4; i++) {
                    QPointF a = edges[i].p1();
                    QPointF b = edges[i].p2();
                    QPointF ap = scenePos - a;
                    QPointF ab = b - a;
                    double abLen2 = ab.x() * ab.x() + ab.y() * ab.y();
                    double t = qBound(0.0, (ap.x() * ab.x() + ap.y() * ab.y()) / abLen2, 1.0);
                    QPointF proj = a + ab * t;
                    double dist = QLineF(scenePos, proj).length();
                    if (dist < minDist) {
                        minDist = dist;
                        closest = i;
                    }
                }
                addDimensionAnnotation(m_scene, edges[closest].p1(),
                                       edges[closest].p2(), m_dimensionPen,
                                       rectItem, DimensionItem::GeomType::RectEdge, closest);
                break;
            }
            if (auto* ellipseItem = dynamic_cast<QGraphicsEllipseItem*>(item)) {
                QRectF r = ellipseItem->rect();
                double radius = r.width() / 2.0;
                if (radius > 0.01) {
                    QPointF center = r.center();
                    QPointF edgePoint(center.x() + radius, center.y());
                    addDimensionAnnotation(m_scene, center, edgePoint, m_dimensionPen,
                                           ellipseItem, DimensionItem::GeomType::Circle);
                }
                break;
            }
            if (auto* pathItem = dynamic_cast<QGraphicsPathItem*>(item)) {
                QPainterPath path = pathItem->path();
                if (path.elementCount() >= 2) {
                    QPointF start(path.elementAt(0).x, path.elementAt(0).y);
                    QPointF end(path.elementAt(path.elementCount() - 1).x,
                                path.elementAt(path.elementCount() - 1).y);
                    addDimensionAnnotation(m_scene, start, end, m_dimensionPen,
                                           pathItem, DimensionItem::GeomType::Line);
                }
                break;
            }
        }
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

void SketchView::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_D && !event->isAutoRepeat()) {
        setTool(SketchTool::Dimension);
        emit toolChangeRequested(SketchTool::Dimension);
        event->accept();
        return;
    }
    if (event->key() == Qt::Key_S && !event->isAutoRepeat()) {
        finishCurrentOperation();
        setTool(SketchTool::None);
        emit exitSketchRequested();
        event->accept();
        return;
    }
    if (event->key() == Qt::Key_Escape) {
        finishCurrentOperation();
        setTool(SketchTool::None);
        emit toolChangeRequested(SketchTool::None);
        event->accept();
        return;
    }
    QGraphicsView::keyPressEvent(event);
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
