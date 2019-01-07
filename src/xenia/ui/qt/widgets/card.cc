#include "xenia/ui/qt/widgets/card.h"
#include <QGraphicsEffect>
#include <QLabel>
#include <QVBoxLayout>

namespace xe {
namespace ui {
namespace qt {

XCard::XCard(QWidget* parent) : Themeable<QWidget>("XCard", parent) { Build(); }

XCard::XCard(const QString& title, QWidget* parent)
    : Themeable<QWidget>("XCard", parent), title_(title) {
  Build();

  Update();
}

void XCard::Build() {
  layout_ = new QVBoxLayout();
  layout_->setContentsMargins(0, 0, 0, 0);
  layout_->setSpacing(0);

  setLayout(layout_);

  setStyleSheet(
      "QLabel{color:white} QWidget#XCard{background: #2D2D2D; "
      "border-top-right-radius: 2px; border-top-left-radius: 2px;}");
  QGraphicsDropShadowEffect* effect = new QGraphicsDropShadowEffect;
  effect->setBlurRadius(16);
  effect->setXOffset(0);
  effect->setYOffset(0);
  effect->setColor(QColor(0, 0, 0, 64));
  setGraphicsEffect(effect);

  if (!title_.isEmpty()) {
    BuildTitle();
  }
}

void XCard::BuildTitle() {
  QWidget* title_container = new QWidget;
  QVBoxLayout* title_container_layout = new QVBoxLayout;
  title_container_layout->setContentsMargins(64, 32, 0, 0);
  title_container_layout->setSpacing(0);
  title_container->setLayout(title_container_layout);

  title_label_ = new QLabel(title_);
  title_label_->setFont(QFont("Segoe UI Semibold", 36));

  title_container_layout->addWidget(title_label_);

  layout_->addWidget(title_container, 0, Qt::AlignTop);
}

void XCard::Update() {
  if (!title_.isEmpty()) {
    if (!title_label_) {
      BuildTitle();
    }
    title_label_->setText(title_);
  }
}

}  // namespace qt
}  // namespace ui
}  // namespace xe