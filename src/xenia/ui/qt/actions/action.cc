#include "xenia/ui/qt/actions/action.h"

#include <QPainter>

namespace xe {
namespace ui {
namespace qt {

XAction::XAction() {}

XAction::XAction(const QChar& icon, const QString& text) {
  setGlyphIcon(QFont("Segoe MDL2 Assets"), icon);
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
    QRectF icon_rect = measure.boundingRect(glyph_char_);
    double max = qMax(icon_rect.width(), icon_rect.height());

    // Create the Pixmap
    QPixmap pixmap(max, max);
    pixmap.fill(Qt::transparent);

    // Paint the Glyph
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::HighQualityAntialiasing);
    painter.setFont(glyph_font_);
    painter.setPen(QPen(Qt::white));

    painter.drawText(pixmap.rect(), Qt::AlignVCenter, glyph_char_);

    return pixmap;
  };

  QPixmap OFF = renderPixmap(glyph_palette_.color(QPalette::Foreground));
  QPixmap ON = renderPixmap(glyph_palette_.color(QPalette::Highlight));

  QIcon icon;
  icon.addPixmap(OFF, QIcon::Normal, QIcon::Off);
  icon.addPixmap(ON, QIcon::Normal, QIcon::On);

  setIcon(icon);
}

}  // namespace qt
}  // namespace ui
}  // namespace xe