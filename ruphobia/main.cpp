#include <QApplication>
#include <QFileDialog>
#include <QFileInfo>
#include <QIcon>
#include <QInputDialog>
#include <QMainWindow>
#include <QMenuBar>
#include <QMessageBox>
#include <QToolBar>
#include <QString>
#include <QGraphicsRectItem>
#include <QGraphicsEllipseItem>

#include <zip.h>

// OCCT
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepBuilderAPI_MakeWire.hxx>
#include <BRepPrimAPI_MakePrism.hxx>
#include <gp_Pnt.hxx>
#include <gp_Dir.hxx>
#include <gp_Vec.hxx>
#include <gp_Circ.hxx>
#include <TopoDS_Shape.hxx>
#include <TopoDS_Wire.hxx>
#include <TopoDS_Face.hxx>

#include "version.h"
#include "sketchview.h"
#include "occtview.h"

static QToolBar* contextToolbar = nullptr;
static SketchView* sketchCanvas = nullptr;
static OcctView* viewer3d = nullptr;
static QString currentAssemblyPath;

static bool writeFCStd(const QString& path, const QString& objectType, const QString& objectName)
{
    int errCode = 0;
    zip_t* archive = zip_open(path.toUtf8().constData(), ZIP_CREATE | ZIP_TRUNCATE, &errCode);
    if (!archive)
        return false;

    QByteArray xml;
    xml.append("<?xml version='1.0' encoding='utf-8'?>\n");
    xml.append("<Document SchemaVersion=\"4\" ProgramVersion=\"1.0.0\" FileVersion=\"1\">\n");
    xml.append("  <Properties Count=\"0\"/>\n");
    xml.append("  <Objects Count=\"1\">\n");
    xml.append("    <Object type=\"");
    xml.append(objectType.toUtf8());
    xml.append("\" name=\"");
    xml.append(objectName.toUtf8());
    xml.append("\" />\n");
    xml.append("  </Objects>\n");
    xml.append("  <ObjectData Count=\"1\">\n");
    xml.append("    <Object name=\"");
    xml.append(objectName.toUtf8());
    xml.append("\">\n");
    xml.append("      <Properties Count=\"1\">\n");
    xml.append("        <Property name=\"Label\" type=\"App::PropertyString\">\n");
    xml.append("          <String value=\"");
    xml.append(objectName.toUtf8());
    xml.append("\"/>\n");
    xml.append("        </Property>\n");
    xml.append("      </Properties>\n");
    xml.append("    </Object>\n");
    xml.append("  </ObjectData>\n");
    xml.append("</Document>\n");

    zip_source_t* source = zip_source_buffer(archive, xml.constData(), xml.size(), 0);
    if (!source) {
        zip_discard(archive);
        return false;
    }

    zip_int64_t idx = zip_file_add(archive, "Document.xml", source, ZIP_FL_OVERWRITE);
    if (idx < 0) {
        zip_source_free(source);
        zip_discard(archive);
        return false;
    }

    if (zip_close(archive) < 0) {
        zip_discard(archive);
        return false;
    }

    return true;
}

static void enterSketchMode(QMainWindow* window);
static void enterPartDesignMode(QMainWindow* window);
static void enterAssemblyMode(QMainWindow* window);

static void newPart(QMainWindow* window)
{
    QString path = QFileDialog::getSaveFileName(window, "New Part", QString(),
                                                "FreeCAD Files (*.FCStd)");
    if (path.isEmpty())
        return;

    if (!path.endsWith(".FCStd", Qt::CaseInsensitive))
        path.append(".FCStd");

    if (!writeFCStd(path, "PartDesign::Body", QFileInfo(path).baseName())) {
        QMessageBox::critical(window, "Error", "Failed to create part file.");
        return;
    }

    enterSketchMode(window);
}

static void enterSketchMode(QMainWindow* window)
{
    if (contextToolbar) {
        window->removeToolBar(contextToolbar);
        delete contextToolbar;
    }

    // Create sketch canvas if not already present
    bool firstTime = !sketchCanvas;
    if (firstTime) {
        sketchCanvas = new SketchView(window);
    }
    window->setCentralWidget(sketchCanvas);

    contextToolbar = new QToolBar("Sketcher", window);

    QAction* lineAction = contextToolbar->addAction(
        QIcon(":/icons/Sketcher_CreateLine.svg"), "");
    lineAction->setToolTip("Line");
    lineAction->setCheckable(true);

    QAction* circleAction = contextToolbar->addAction(
        QIcon(":/icons/Sketcher_CreateCircle.svg"), "");
    circleAction->setToolTip("Circle");
    circleAction->setCheckable(true);

    QAction* arcAction = contextToolbar->addAction(
        QIcon(":/icons/Sketcher_CreateArc.svg"), "");
    arcAction->setToolTip("Arc");
    arcAction->setCheckable(true);

    QAction* rectAction = contextToolbar->addAction(
        QIcon(":/icons/Sketcher_CreateRectangle.svg"), "");
    rectAction->setToolTip("Rectangle");
    rectAction->setCheckable(true);

    QAction* polylineAction = contextToolbar->addAction(
        QIcon(":/icons/Sketcher_CreatePolyline.svg"), "");
    polylineAction->setToolTip("Polyline");
    polylineAction->setCheckable(true);

    QAction* pointAction = contextToolbar->addAction(
        QIcon(":/icons/Sketcher_CreatePoint.svg"), "");
    pointAction->setToolTip("Point");
    pointAction->setCheckable(true);

    contextToolbar->addSeparator();

    QAction* dimAction = contextToolbar->addAction(
        QIcon(":/icons/Constraint_Dimension.svg"), "");
    dimAction->setToolTip("Dimension (D)");
    dimAction->setCheckable(true);

    // Make tools mutually exclusive
    auto setExclusive = [=](QAction* active, SketchTool tool) {
        for (auto* a : contextToolbar->actions())
            if (a != active) a->setChecked(false);
        if (active->isChecked())
            sketchCanvas->setTool(tool);
        else
            sketchCanvas->setTool(SketchTool::None);
    };

    QObject::connect(lineAction, &QAction::triggered, [=]() {
        setExclusive(lineAction, SketchTool::Line);
    });
    QObject::connect(circleAction, &QAction::triggered, [=]() {
        setExclusive(circleAction, SketchTool::Circle);
    });
    QObject::connect(arcAction, &QAction::triggered, [=]() {
        setExclusive(arcAction, SketchTool::Arc);
    });
    QObject::connect(rectAction, &QAction::triggered, [=]() {
        setExclusive(rectAction, SketchTool::Rectangle);
    });
    QObject::connect(polylineAction, &QAction::triggered, [=]() {
        setExclusive(polylineAction, SketchTool::Polyline);
    });
    QObject::connect(pointAction, &QAction::triggered, [=]() {
        setExclusive(pointAction, SketchTool::Point);
    });
    QObject::connect(dimAction, &QAction::triggered, [=]() {
        setExclusive(dimAction, SketchTool::Dimension);
    });

    // Sync toolbar buttons when tool is changed via keyboard shortcut
    QObject::connect(sketchCanvas, &SketchView::toolChangeRequested, [=](SketchTool tool) {
        for (auto* a : contextToolbar->actions())
            a->setChecked(false);
        if (tool == SketchTool::Dimension) dimAction->setChecked(true);
    });

    // S key exits sketch mode into Part Design mode (connect once)
    if (firstTime) {
        QObject::connect(sketchCanvas, &SketchView::exitSketchRequested, [window]() {
            enterPartDesignMode(window);
        });
    }

    window->addToolBar(contextToolbar);
    contextToolbar->show();
}

// Convert sketch geometry to an OCCT face for extrusion
static TopoDS_Face sketchToFace()
{
    TopoDS_Face face;
    if (!sketchCanvas)
        return face;

    QGraphicsScene* scene = sketchCanvas->scene();
    if (!scene)
        return face;

    // Find the first extrudable shape (rectangle or circle)
    for (auto* item : scene->items()) {
        if (auto* rectItem = dynamic_cast<QGraphicsRectItem*>(item)) {
            QRectF r = rectItem->rect();
            // Convert to OCCT: sketch Y is flipped (screen coords), use XY plane
            gp_Pnt p1(r.left(), -r.top(), 0);
            gp_Pnt p2(r.right(), -r.top(), 0);
            gp_Pnt p3(r.right(), -r.bottom(), 0);
            gp_Pnt p4(r.left(), -r.bottom(), 0);

            BRepBuilderAPI_MakeWire wireBuilder;
            wireBuilder.Add(BRepBuilderAPI_MakeEdge(p1, p2).Edge());
            wireBuilder.Add(BRepBuilderAPI_MakeEdge(p2, p3).Edge());
            wireBuilder.Add(BRepBuilderAPI_MakeEdge(p3, p4).Edge());
            wireBuilder.Add(BRepBuilderAPI_MakeEdge(p4, p1).Edge());

            if (wireBuilder.IsDone()) {
                BRepBuilderAPI_MakeFace faceBuilder(wireBuilder.Wire());
                if (faceBuilder.IsDone())
                    return faceBuilder.Face();
            }
        }
        if (auto* ellipseItem = dynamic_cast<QGraphicsEllipseItem*>(item)) {
            QRectF r = ellipseItem->rect();
            if (qAbs(r.width() - r.height()) < 0.01 && r.width() > 0.01) {
                // Circle
                double radius = r.width() / 2.0;
                gp_Pnt center(r.center().x(), -r.center().y(), 0);
                gp_Circ circ(gp_Ax2(center, gp_Dir(0, 0, 1)), radius);

                BRepBuilderAPI_MakeEdge edgeBuilder(circ);
                if (edgeBuilder.IsDone()) {
                    BRepBuilderAPI_MakeWire wireBuilder(edgeBuilder.Edge());
                    if (wireBuilder.IsDone()) {
                        BRepBuilderAPI_MakeFace faceBuilder(wireBuilder.Wire());
                        if (faceBuilder.IsDone())
                            return faceBuilder.Face();
                    }
                }
            }
        }
    }
    return face;
}

static void doPad(QMainWindow* window)
{
    TopoDS_Face face = sketchToFace();
    if (face.IsNull()) {
        QMessageBox::warning(window, "Pad", "No extrudable sketch geometry found.\nDraw a rectangle or circle first.");
        return;
    }

    bool ok = false;
    double depth = QInputDialog::getDouble(window, "Pad", "Extrusion depth:", 50.0,
                                           0.1, 10000.0, 1, &ok);
    if (!ok)
        return;

    // Extrude along Z axis
    gp_Vec direction(0, 0, depth);
    BRepPrimAPI_MakePrism prism(face, direction);
    if (!prism.IsDone()) {
        QMessageBox::critical(window, "Error", "Extrusion failed.");
        return;
    }

    TopoDS_Shape solid = prism.Shape();

    // Switch to 3D view and display
    if (!viewer3d) {
        viewer3d = new OcctView(window);
    }
    window->setCentralWidget(viewer3d);
    viewer3d->show();
    viewer3d->displayShape(solid);
}

static void enterPartDesignMode(QMainWindow* window)
{
    if (contextToolbar) {
        window->removeToolBar(contextToolbar);
        delete contextToolbar;
    }

    // Switch to 3D view
    if (!viewer3d) {
        viewer3d = new OcctView(window);
    }
    window->setCentralWidget(viewer3d);

    contextToolbar = new QToolBar("Part Design", window);

    QAction* padAction = contextToolbar->addAction(
        QIcon(":/icons/PartDesign_Pad.svg"), "");
    padAction->setToolTip("Pad (Extrude)");
    QObject::connect(padAction, &QAction::triggered, [window]() {
        doPad(window);
    });

    QAction* pocketAction = contextToolbar->addAction(
        QIcon(":/icons/PartDesign_Pocket.svg"), "");
    pocketAction->setToolTip("Pocket (Cut)");

    QAction* revolutionAction = contextToolbar->addAction(
        QIcon(":/icons/PartDesign_Revolution.svg"), "");
    revolutionAction->setToolTip("Revolution");

    QAction* loftAction = contextToolbar->addAction(
        QIcon(":/icons/PartDesign_AdditiveLoft.svg"), "");
    loftAction->setToolTip("Loft");

    contextToolbar->addSeparator();

    QAction* filletAction = contextToolbar->addAction(
        QIcon(":/icons/PartDesign_Fillet.svg"), "");
    filletAction->setToolTip("Fillet");

    QAction* chamferAction = contextToolbar->addAction(
        QIcon(":/icons/PartDesign_Chamfer.svg"), "");
    chamferAction->setToolTip("Chamfer");

    window->addToolBar(contextToolbar);
    contextToolbar->show();
}

static void enterAssemblyMode(QMainWindow* window)
{
    if (contextToolbar) {
        window->removeToolBar(contextToolbar);
        delete contextToolbar;
    }

    contextToolbar = new QToolBar("Assembly", window);
    QAction* newPartAction = contextToolbar->addAction(
        QIcon(":/icons/PartDesign_Body.svg"), "");
    newPartAction->setToolTip("New Part");
    QObject::connect(newPartAction, &QAction::triggered,
                     [window]() { newPart(window); });
    window->addToolBar(contextToolbar);
    contextToolbar->show();
}

static void newAssembly(QMainWindow* window)
{
    QString path = QFileDialog::getSaveFileName(window, "New Assembly", QString(),
                                                "FreeCAD Files (*.FCStd)");
    if (path.isEmpty())
        return;

    if (!path.endsWith(".FCStd", Qt::CaseInsensitive))
        path.append(".FCStd");

    if (!writeFCStd(path, "App::Part", QFileInfo(path).baseName())) {
        QMessageBox::critical(window, "Error", "Failed to create assembly file.");
        return;
    }

    currentAssemblyPath = path;

    window->setWindowTitle(QString("FreeCAD Ruphobia Addation - Build %1 - %2")
                               .arg(BUILD_NUMBER)
                               .arg(QFileInfo(path).fileName()));

    enterAssemblyMode(window);
}

int main(int argc, char** argv)
{
    QApplication app(argc, argv);

    QMainWindow window;
    window.setWindowTitle(QString("FreeCAD Ruphobia Addation - Build %1").arg(BUILD_NUMBER));
    window.resize(1024, 768);

    QMenu* fileMenu = window.menuBar()->addMenu("&File");
    QAction* newAction = fileMenu->addAction("New Assembly");
    QObject::connect(newAction, &QAction::triggered, [&window]() { newAssembly(&window); });

    window.show();

    return app.exec();
}
