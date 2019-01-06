#include "xenia/ui/qt/widgets/card.h"
#include <QVBoxLayout>

namespace xe {
namespace ui {
namespace qt {

XCard::XCard(QWidget* parent) : Themeable<QWidget>("XCard", parent) {
  BuildCard();
}

XCard::XCard(const QString& title, QWidget* parent)
    : Themeable<QWidget>("XCard", parent), title_(title) {
  BuildCard();
}

void XCard::BuildCard() {
  QVBoxLayout* layout = new QVBoxLayout();
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(0);

  container_->setLayout(layout);

  // for now hardcode card size for testing purposes
  setFixedWidth(700);
  setFixedHeight(394);

  setStyleSheet("background: #2D2D2D;");
}

}  // namespace qt
}  // namespace ui
}  // namespace xe