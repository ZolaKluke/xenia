#include "xenia/ui/qt/widgets/toolbar_item.h"

#include <QApplication>
#include <QFontDatabase>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QParallelAnimationGroup>
#include <QPropertyAnimation>

namespace xe {
namespace ui {
namespace qt {

XToolBarItem::XToolBarItem(XAction* action, QWidget* parent)
    : Themeable<QToolButton>("XToolBarItem", parent) {
  setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
  setDefaultAction(action);
  setContentsMargins(0, 0, 0, 0);
  setCheckable(false);
  setCursor(Qt::PointingHandCursor);

  connect(this, SIGNAL(triggered(QAction*)), SLOT(setDefaultAction(QAction*)));
}

void XToolBarItem::enterEvent(QEvent*) { auto x = 1; }

void XToolBarItem::mousePressEvent(QMouseEvent*) {
  // Disable Button push-in
}

}  // namespace qt
}  // namespace ui
}  // namespace xe