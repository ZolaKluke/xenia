#include "xenia/ui/qt/widgets/slider.h"

namespace xe {
namespace ui {
namespace qt {

XSlider::XSlider(Qt::Orientation orientation, QWidget* parent)
    : Themeable<QSlider>("XSlider", orientation, parent){};

void XSlider::paintEvent(QPaintEvent*) {
  QPainter painter(this);
  painter.setRenderHint(QPainter::Antialiasing);

  // Paint Slider Bar
  painter.setPen(Qt::NoPen);
  painter.setBrush(palette().foreground());
  QRectF bar_rect;

  if (orientation() == Qt::Horizontal) {
    bar_rect = QRectF(0, (height() / 2.0) - bar_size_, width(), bar_size_);
    painter.drawRoundRect(bar_rect, bar_radius_, bar_radius_);
  } else {
    bar_rect = QRectF((width() / 2.0) - bar_size_, 0, bar_size_, height());
    painter.drawRoundRect(bar_rect, bar_radius_, bar_radius_);
  }

  // Paint Slider Knob
  double offset = (double)(value() - minimum()) / (maximum() - minimum());
  if(orientation() == Qt::Horizontal) {
    QPointF knob_position(
      bar_rect.left() + bar_rect.width() * offset,
      bar_rect.center().y()
      );

    painter.drawEllipse(knob_position, slider_radius_, slider_radius_);
  }
}

}  // namespace qt
}  // namespace ui
}  // namespace xe