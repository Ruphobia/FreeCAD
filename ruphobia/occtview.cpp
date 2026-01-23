#include "occtview.h"

#include <Aspect_DisplayConnection.hxx>
#include <Aspect_NeutralWindow.hxx>
#include <OpenGl_GraphicDriver.hxx>
#include <V3d_AmbientLight.hxx>
#include <V3d_DirectionalLight.hxx>
#include <AIS_Shape.hxx>
#include <Prs3d_ShadingAspect.hxx>
#include <Graphic3d_MaterialAspect.hxx>

OcctView::OcctView(QWidget* parent)
    : QOpenGLWidget(parent)
{
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);
}

void OcctView::initViewer()
{
    if (m_initialized)
        return;

    // Create graphic driver
    Handle(Aspect_DisplayConnection) displayConn = new Aspect_DisplayConnection();
    Handle(OpenGl_GraphicDriver) driver = new OpenGl_GraphicDriver(displayConn, false);

    // Create viewer
    m_viewer = new V3d_Viewer(driver);
    m_viewer->SetDefaultLights();
    m_viewer->SetLightOn();
    m_viewer->SetDefaultTypeOfView(V3d_PERSPECTIVE);

    // Create view
    m_view = m_viewer->CreateView();

    // Create window wrapper for the OpenGL widget
    Handle(Aspect_NeutralWindow) nativeWin = new Aspect_NeutralWindow();
    nativeWin->SetSize(width(), height());
    nativeWin->SetNativeHandle((Aspect_Drawable)winId());
    m_view->SetWindow(nativeWin);

    // Setup view parameters
    m_view->SetBackgroundColor(Quantity_NOC_GRAY30);
    m_view->MustBeResized();
    m_view->TriedronDisplay(Aspect_TOTP_LEFT_LOWER, Quantity_NOC_WHITE, 0.1);

    // Create interactive context
    m_context = new AIS_InteractiveContext(m_viewer);
    m_context->SetDisplayMode(AIS_Shaded, true);

    m_initialized = true;
}

void OcctView::initializeGL()
{
    initViewer();
}

void OcctView::paintGL()
{
    if (m_view.IsNull())
        return;
    m_view->Redraw();
}

void OcctView::resizeGL(int w, int h)
{
    if (!m_view.IsNull()) {
        m_view->Window()->DoResize();
        m_view->MustBeResized();
        m_view->Invalidate();
    }
}

void OcctView::displayShape(const TopoDS_Shape& shape)
{
    if (m_context.IsNull())
        return;

    Handle(AIS_Shape) aisShape = new AIS_Shape(shape);

    // Set material to look nice
    Handle(Prs3d_ShadingAspect) shadingAspect = new Prs3d_ShadingAspect();
    Graphic3d_MaterialAspect mat(Graphic3d_NameOfMaterial_Silver);
    shadingAspect->SetMaterial(mat);
    aisShape->Attributes()->SetShadingAspect(shadingAspect);

    m_context->Display(aisShape, AIS_Shaded, 0, true);
    fitAll();
}

void OcctView::fitAll()
{
    if (!m_view.IsNull()) {
        m_view->FitAll();
        m_view->ZFitAll();
        update();
    }
}

void OcctView::mousePressEvent(QMouseEvent* event)
{
    m_lastPos = event->pos();
    m_activeButton = event->button();

    if (event->button() == Qt::MiddleButton) {
        setCursor(Qt::ClosedHandCursor);
    }
}

void OcctView::mouseMoveEvent(QMouseEvent* event)
{
    if (m_view.IsNull())
        return;

    QPoint delta = event->pos() - m_lastPos;

    if (m_activeButton == Qt::MiddleButton) {
        if (event->modifiers() & Qt::ShiftModifier) {
            // Pan
            m_view->Pan(delta.x(), -delta.y());
        } else {
            // Orbit
            m_view->Rotation(event->pos().x(), event->pos().y());
        }
        update();
    } else if (m_activeButton == Qt::RightButton) {
        // Pan with right button
        m_view->Pan(delta.x(), -delta.y());
        update();
    }

    m_lastPos = event->pos();

    if (m_activeButton == Qt::MiddleButton && !(event->modifiers() & Qt::ShiftModifier)) {
        m_view->StartRotation(event->pos().x(), event->pos().y());
    }
}

void OcctView::mouseReleaseEvent(QMouseEvent* event)
{
    m_activeButton = Qt::NoButton;
    setCursor(Qt::ArrowCursor);
}

void OcctView::wheelEvent(QWheelEvent* event)
{
    if (m_view.IsNull())
        return;

    double delta = event->angleDelta().y();
    if (delta > 0)
        m_view->SetScale(m_view->Scale() * 1.1);
    else
        m_view->SetScale(m_view->Scale() / 1.1);
    update();
}
