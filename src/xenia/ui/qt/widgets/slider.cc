#include "xenia/ui/qt/widgets/slider.h"

namespace xe {
namespace ui {
namespace qt {

XSlider::XSlider(Qt::Orientation orientation, QWidget* parent)
    : Themeable<QSlider>("XSlider", orientation, parent){};

void XSlider::paintEvent(QPaintEvent*) {
  QStyleOptionSlider option;
  initStyleOption(&option);

  // Grab measured rect from style
  QStyle* style(style());
  QRect grove_rect = style->subControlRect(QStyle::CC_Slider, &option,
                                           QStyle::SC_SliderGroove);
  QRect handle_rect = style->subControlRect(QStyle::CC_Slider, &option,
                                            QStyle::SC_SliderHandle);

  QPainter painter(this);
  painter.setRenderHint(QPainter::Antialiasing);

  // Paint Slider Grove
  painter.setPen(Qt::NoPen);
  painter.setBrush(palette().foreground());
  grove_rect = QRect(grove_rect.left(), (grove_rect.height() / 2.0) - bar_size_,
                     grove_rect.right(), bar_size_);
  painter.drawRoundRect(grove_rect, bar_radius_, bar_radius_);

  // Paint Slider Handle
  painter.drawEllipse(handle_rect.center(), slider_radius_, slider_radius_);
}

}  // namespace qt
}  // namespace ui
}  // namespace xe