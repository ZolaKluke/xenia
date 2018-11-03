#include "xenia/ui/qt/widgets/toolbar.h"

#include <QLayout>

namespace xe {
namespace ui {
namespace qt {

XToolBar::XToolBar(QWidget* parent) : Themeable<QToolBar>("XToolBar", parent) {
  const char* css =
      "QToolBar#XToolBar {"
      "background-color:#232323;"
      "color: #ffffff;"
      "min-height: 44px;"
      "qproperty-iconSize: 18px;"
      "qproperty-spacing: 32;"
      "}"
      "QToolBar::separator::horizontal {"
      "background: #626262;"
      "width: 1px;"
      "margin: 8px 0;"
      "}"
      "QToolButton {"
      "color: #ffffff;"
      "font-size: 12px;"
      "}";

  setStyleSheet(css);
  setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
};


XToolBarItem* XToolBar::addAction(XAction* action) {
  addSpacing(spacing_);
  XToolBarItem* item = new XToolBarItem(action, this);
  addWidget(item);
  return item;
}

void XToolBar::addSeparator() {
  addSpacing();
  QToolBar::addSeparator();
}

QWidget* XToolBar::addSpacing(int size) {
  if (size == 0) {
    size = spacing_;
  }

  QWidget* spacer = new QWidget(this);

  spacers_.push_back(spacer);
  addWidget(spacer);
  return spacer;
}

void XToolBar::setSpacing(const int& spacing) {
  spacing_ = spacing;
  for (QWidget* spacer : spacers_) {
    if (orientation() == Qt::Horizontal) {
      spacer->setMinimumWidth(spacing);
    } else {
      spacer->setMinimumHeight(spacing);
    }
  }
  update();
}

}  // namespace qt
}  // namespace ui
}  // namespace xe