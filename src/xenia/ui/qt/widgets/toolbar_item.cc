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

  connect(this, SIGNAL(triggered(QAction*)), SLOT(setDefaultAction(QAction*)));
}

void XToolBarItem::setIcon(const QString& icon_char) {
  icon_char_ = icon_char;

  // Create the icon from the specified font
  QFontMetrics measure(icon_font_);

  QRectF rect = measure.boundingRect(icon_char);
  auto width = rect.width();
  auto height = rect.height();
  auto descent = measure.descent();

  QPixmap icon_pixmap(width, height);
  icon_pixmap.fill(Qt::transparent);

  QPainter painter(&icon_pixmap);
  painter.setPen(Qt::NoPen);
  painter.setBrush(QBrush(icon_color_));
  painter.setRenderHint(QPainter::HighQualityAntialiasing);

  QPainterPath icon_path;
  QPointF baseline(0, height - descent);
  icon_path.addText(baseline, icon_font_, icon_char);
  painter.drawPath(icon_path);
  icon_pixmap_ = icon_pixmap;
}

void XToolBarItem::paintEvent(QPaintEvent*) {
  QPainter painter(this);
  painter.setPen(Qt::NoPen);
  painter.setRenderHint(QPainter::HighQualityAntialiasing);

  // Paint Icon
  int icon_width = icon().pixmap(iconSize()).width();
  QRect icon_rect(0, 0, icon_width, height());
  icon().paint(&painter, icon_rect);

  // Paint Text
  QFontMetrics measure(font());
  auto text_rect = measure.boundingRect(text());
  QPointF text_point(icon_rect.width() + spacing_,
                     (height() - (text_rect.height()) / 2.0));

  QPainterPath text_path;
  text_path.addText(text_point, font(), text());

  painter.setPen(Qt::NoPen);
  painter.setBrush(QBrush(palette().text().color()));
  painter.drawPath(text_path);
}

void XToolBarItem::enterEvent(QEvent*) { auto x = 1; }

void XToolBarItem::mousePressEvent(QMouseEvent*) {
  // Disable Button push-in
}

}  // namespace qt
}  // namespace ui
}  // namespace xe