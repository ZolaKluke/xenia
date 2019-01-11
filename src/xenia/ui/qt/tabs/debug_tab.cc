#include "xenia/ui/qt/tabs/debug_tab.h"
#include <QHBoxLayout>
#include "xenia/ui/qt/widgets/slider.h"

#ifdef DEBUG

namespace xe {
namespace ui {
namespace qt {

DebugTab::DebugTab() : XTab("Debug", "DebugTab") { Build(); }

void DebugTab::Build() {
  layout_ = new QGridLayout();
  layout_->setContentsMargins(0, 0, 0, 0);
  layout_->setSpacing(0);
  setLayout(layout_);

  BuildCard();
}

void DebugTab::BuildCard() {
  // this is a placeholder widget to make sure the grid layout is correct
  // TODO: possibly move this to the card class?
  QWidget* card_container = new QWidget(this);

  card_ = new XCard("Debug", card_container);
  card_->AddWidget(CreateSliderGroup());

  layout_->addWidget(card_container, 0, 0, 10, 10);
  layout_->addWidget(card_, 1, 1, 9, 8);
}

QWidget* DebugTab::CreateSliderGroup() {
  QWidget* group = new QWidget();

  group->setStyleSheet("QLabel { color: white; }");

  QVBoxLayout* group_layout = new QVBoxLayout();
  group_layout->setContentsMargins(32, 32, 32, 32);
  group_layout->setSpacing(16);
  group->setLayout(group_layout);

  QLabel* title = new QLabel("Sliders");
  title->setFont(QFont("Segoe UI", 24));

  group_layout->addWidget(title);

  QHBoxLayout* control_layout = new QHBoxLayout();
  control_layout->setContentsMargins(0, 0, 0, 0);
  control_layout->setSpacing(20);

  group_layout->addLayout(control_layout);

  XSlider* horizontal_slider = new XSlider();
  horizontal_slider->setFixedWidth(120);

  QLabel* slider_label =
      new QLabel(QStringLiteral("Value: %1").arg(horizontal_slider->value()));

  connect(horizontal_slider, &XSlider::valueChanged, [=](int value) {
    slider_label->setText(QStringLiteral("Value: %1").arg(value));
  });

  XSlider* vertical_slider = new XSlider(Qt::Vertical);
  vertical_slider->setFixedHeight(120);

  control_layout->addWidget(horizontal_slider);
  control_layout->addWidget(slider_label);
  // TODO: XSlider is broken in vertical mode
  // control_layout->addWidget(vertical_slider);

  group_layout->addStretch();

  return group;
}

}  // namespace qt
}  // namespace ui
}  // namespace xe

#endif