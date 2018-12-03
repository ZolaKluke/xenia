#include "xenia/ui/qt/widgets/sidebar.h"

namespace xe {
namespace ui {
namespace qt {

XSideBar::XSideBar() : Themeable<QToolBar>("XSideBar") {}

XSideBarButton* XSideBar::addAction(const QString& text) {
  auto button = new XSideBarButton(text);
  // TODO: move to css
  button->setFixedHeight(60);
  button->setFixedWidth(300);

  buttons_.append(button);

  QToolBar::addWidget(button);

  return button;
}

XSideBarButton* XSideBar::addAction(QChar glyph, const QString& text) {
  auto button = new XSideBarButton(glyph, text);
  // TODO: move to css
  button->setFixedHeight(60);
  button->setFixedWidth(300);

  buttons_.append(button);

  QToolBar::addWidget(button);

  return button;
}

}  // namespace qt
}  // namespace ui
}  // namespace xe
