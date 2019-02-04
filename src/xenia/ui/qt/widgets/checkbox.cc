#include "checkbox.h"
#include <QStyleOption>
#include "xenia/ui/qt/theme_manager.h"

namespace xe {
namespace ui {
namespace qt {

XCheckBox::XCheckBox(QWidget* parent)
    : Themeable<QCheckBox>("XCheckBox", parent) {
  Update();
}

void XCheckBox::Update() {
  // Load colors from the current theme file.
  // Loading colors from the theme directly is not an ideal situation as the
  // component will ignore CSS theming.
  // This should also be updated in paintEvent() as a theme change would not be
  // updated for this component otherwise.
  const Theme& theme = ThemeManager::Instance().current_theme();

  if (!border_color_.isValid()) {
    border_color_ = theme.ColorForKey("light2");
  }
  if (!checked_color_.isValid()) {
    checked_color_ = theme.ColorForKey("secondary");
  }
}

void XCheckBox::paintEvent(QPaintEvent* e) {
  Update();

  QStyleOptionButton option;
  initStyleOption(&option);

  // get original rect for checkbox label

  QRect label_rect = style()->proxy()->subElementRect(
      QStyle::SE_CheckBoxContents, &option, this);
  label_rect.translate(label_indent_, 0);

  // create rect for indicator box
  // rect must start at 1 as the painter draws either side of start offset so
  // starting at (0,0) would leave 2 sides cut off
  QRectF indicator_box = QRectF(1, 1, 16, 16);

  QPainter painter(this);
  painter.setRenderHints(QPainter::Antialiasing);

  QPen pen(border_color_);
  pen.setJoinStyle(Qt::MiterJoin);
  painter.setPen(pen);

  painter.drawRect(indicator_box);

  painter.drawText(label_rect, text());

  // paint checked inner box if checkbox is checked
  if (isChecked()) {
    painter.setPen(Qt::transparent);
    QBrush checked_brush = QBrush(checked_color_);
    QRectF checked_rect = QRectF(3, 3, 12, 12);

    painter.setBrush(checked_brush);
    painter.drawRect(checked_rect);
  }
}

QSize XCheckBox::sizeHint() const {
  // Increase sizeHint by indent amount to compensate for slightly larget
  // indicator box and translated label.
  // This is not exact, but to get it exact would require using an algorithm
  // with QFontMetrics.
  return QCheckBox::sizeHint() + QSize(label_indent_, 0);
}

}  // namespace qt
}  // namespace ui
}  // namespace xe