#include "xenia/ui/qt/widgets/status_bar.h"

namespace xe {
namespace ui {
namespace qt {

XStatusBar::XStatusBar(QWidget* parent)
    : Themeable<QStatusBar>("XStatusBar", parent) {
  // TODO: move to css
  setStyleSheet(
      "background: rgb(40, 40, 40);"
      "color: #c7c7c7;");

  setSizeGripEnabled(false);

  QGraphicsDropShadowEffect* effect = new QGraphicsDropShadowEffect;
  effect->setBlurRadius(16);
  effect->setXOffset(0);
  effect->setYOffset(-4);
  effect->setColor(QColor(0, 0, 0, 64));

  setGraphicsEffect(effect);
}

}  // namespace qt
}  // namespace ui
}  // namespace xe
