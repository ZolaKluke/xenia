#include "xenia/ui/qt/widgets/slider.h"

namespace xe {
namespace ui {
namespace qt {

XSlider::XSlider(Qt::Orientation orientation, QWidget* parent)
    : Themeable<QSlider>("XSlider", orientation, parent){};

void XSlider::paintEvent(QPaintEvent*) {
  QColor bar_color = palette().foreground().color();
  QColor handle_color = bar_color;
  bar_color.setAlpha(196);

  QStyleOptionSlider option;
  initStyleOption(&option);

  // Grab measured rect from style
  QStyle* style(style());
  QRectF grove_rect = style->subControlRect(QStyle::CC_Slider, &option,
                                            QStyle::SC_SliderGroove);
  QRectF handle_rect = style->subControlRect(QStyle::CC_Slider, &option,
                                             QStyle::SC_SliderHandle);

  QPainter painter(this);
  painter.setRenderHint(QPainter::Antialiasing);

  // Paint Slider Grove
  painter.setPen(Qt::NoPen);
  painter.setBrush(QBrush(bar_color));
  grove_rect =
      QRectF(grove_rect.left(), grove_rect.center().y() - (bar_size_ / 2.0),
             grove_rect.right(), bar_size_);
  painter.drawRoundRect(grove_rect, bar_radius_, bar_radius_);
  painter.setBrush(QBrush(handle_color));
  // Paint Slider Handle
  painter.drawEllipse((QPointF)handle_rect.center(), slider_radius_,
                      slider_radius_);
}

}  // namespace qt
}  // namespace ui
}  // namespace xe