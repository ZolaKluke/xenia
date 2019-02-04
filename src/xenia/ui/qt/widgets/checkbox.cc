#include "checkbox.h"
#include <QStyleOption>

namespace xe {
namespace ui {
namespace qt {

XCheckBox::XCheckBox(QWidget* parent)
    : Themeable<QCheckBox>("XCheckBox", parent) {}

void XCheckBox::paintEvent(QPaintEvent* e) {
  QStyleOptionButton option;
  initStyleOption(&option);

  QRect indicator_rect = style()->proxy()->subElementRect(
      QStyle::SE_CheckBoxIndicator, &option, this);

  QRect label_rect = style()->proxy()->subElementRect(
      QStyle::SE_CheckBoxContents, &option, this);

  label_rect.translate(8, 0);

  QRectF indicator_box = QRectF(1, 1, 16, 16);

  QPainter painter(this);
  painter.setRenderHints(QPainter::Antialiasing);

  // TODO: remove hardcoded colors

  QPen pen(QColor(196, 196, 196));
  pen.setJoinStyle(Qt::MiterJoin);
  painter.setPen(pen);

  painter.drawRect(indicator_box);

  painter.drawText(label_rect, text());

  if (isChecked()) {
    painter.setPen(Qt::transparent);
    QBrush checked_brush = QBrush(QColor(92, 228, 148));
    QRectF checked_rect = QRectF(3, 3, 12, 12);

    painter.setBrush(checked_brush);
    painter.drawRect(checked_rect);
  }
}

QSize XCheckBox::sizeHint() const {
  // Increase sizeHint by 8 to compensate for slightly larget indicator box and
  // translated label.
  // This is not exact, but to get it exact would require using
  // an algorithm with QFontMetrics.
  return QCheckBox::sizeHint() + QSize(8, 0);
}

}  // namespace qt
}  // namespace ui
}  // namespace xe