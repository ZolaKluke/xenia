#include "xenia/ui/qt/actions/action.h"

#include <QPainter>

namespace xe {
namespace ui {
namespace qt {

XAction::XAction() {}

XAction::XAction(const QChar& icon, const QString& text) {
  setGlyphIcon(QFont("Segoe MDL2 Assets", 64), icon);
  setIconText(text);
}

void XAction::setGlyphIcon(const QFont& font, const QChar& glyph_char) {
  glyph_char_ = glyph_char;
  glyph_font_ = font;

  rebuildGlyphIcons();
}

void XAction::rebuildGlyphIcons() {
  auto renderPixmap = [=](QColor color) {
    // Measure the Glyph
    QFontMetrics measure(glyph_font_);
    QRect icon_rect = measure.boundingRect(glyph_char_);
    double max = qMax(icon_rect.width(), icon_rect.height());

    // Create the Pixmap
    // boundingRect can be inaccurate so add a 4px padding to be safe
    QPixmap pixmap(max + 2, max + 2);
    pixmap.fill(Qt::transparent);

    // Paint the Glyph
    QPainter painter(&pixmap);
    painter.setFont(glyph_font_);
    painter.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing);
    painter.setPen(QPen(color));

    painter.drawText(pixmap.rect(), Qt::AlignVCenter, glyph_char_);

    return pixmap;
  };

  // TODO: load colors from CSS
  QPixmap OFF = renderPixmap(Qt::white);
  QPixmap ON = renderPixmap(Qt::white);

  QIcon icon;
  icon.addPixmap(OFF, QIcon::Normal, QIcon::Off);
  icon.addPixmap(ON, QIcon::Normal, QIcon::On);

  setIcon(icon);
}

}  // namespace qt
}  // namespace ui
}  // namespace xe