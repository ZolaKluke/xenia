#include "xenia/ui/qt/tabs/debug_tab.h"
#include <QButtonGroup>
#include <QHBoxLayout>
#include "xenia/ui/qt/widgets/checkbox.h"
#include "xenia/ui/qt/widgets/radio_button.h"
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
  card_->AddWidget(CreateCheckboxGroup());
  card_->AddWidget(CreateRadioButtonGroup());
  card_->AddWidget(CreateSliderGroup());

  layout_->addWidget(card_container, 0, 0, 10, 10);
  layout_->addWidget(card_, 1, 1, 9, 8);
}

QWidget* DebugTab::CreateSliderGroup() {
  QWidget* group = new QWidget();

  group->setStyleSheet("QLabel { color: white; }");

  QVBoxLayout* group_layout = new QVBoxLayout();
  group_layout->setContentsMargins(32, 16, 32, 0);
  group_layout->setSpacing(16);
  group->setLayout(group_layout);

  QLabel* title = new QLabel("Sliders");
  title->setFont(QFont("Segoe UI", 24));

  group_layout->addWidget(title);

  QHBoxLayout* control_layout = new QHBoxLayout();
  control_layout->setContentsMargins(4, 4, 4, 4);
  control_layout->setSpacing(20);

  group_layout->addLayout(control_layout);

  // horizontal slider

  XSlider* horizontal_slider = new XSlider();
  horizontal_slider->setFixedWidth(120);

  QLabel* horizontal_label = new QLabel();

  connect(horizontal_slider, &XSlider::valueChanged, [=](int value) {
    QString text;
    horizontal_label->setText(text.sprintf("Value: %02d", value));
  });
  horizontal_slider->valueChanged(0);

  control_layout->addWidget(horizontal_slider);
  control_layout->addWidget(horizontal_label);

  control_layout->addSpacing(16);

  // vertical slider

  XSlider* vertical_slider = new XSlider(Qt::Vertical);
  vertical_slider->setFixedHeight(60);
  vertical_slider->setFixedWidth(20);

  QLabel* vertical_label = new QLabel();

  connect(vertical_slider, &XSlider::valueChanged, [=](int value) {
    QString text;
    vertical_label->setText(text.sprintf("Value: %02d", value));
  });
  vertical_slider->valueChanged(0);

  control_layout->addWidget(vertical_slider);
  control_layout->addWidget(vertical_label);

  control_layout->addStretch();

  group_layout->addStretch();

  return group;
}

QWidget* DebugTab::CreateCheckboxGroup() {
  QWidget* group = new QWidget();

  QVBoxLayout* group_layout = new QVBoxLayout();
  group_layout->setContentsMargins(32, 16, 32, 0);
  group_layout->setSpacing(16);
  group->setLayout(group_layout);

  QLabel* title = new QLabel("Checkboxes");
  title->setFont(QFont("Segoe UI", 24));

  group_layout->addWidget(title);

  QVBoxLayout* control_layout = new QVBoxLayout();
  control_layout->setContentsMargins(0, 0, 0, 0);
  control_layout->setSpacing(12);

  QHBoxLayout* layer_1_layout = new QHBoxLayout();
  layer_1_layout->setContentsMargins(12, 0, 12, 0);
  layer_1_layout->setSpacing(20);

  QHBoxLayout* layer_2_layout = new QHBoxLayout();
  layer_2_layout->setContentsMargins(12, 0, 12, 0);
  layer_2_layout->setSpacing(20);

  control_layout->addLayout(layer_1_layout);
  control_layout->addLayout(layer_2_layout);

  control_layout->addStretch();

  group_layout->addLayout(control_layout);

  XCheckBox* checkbox1 = new XCheckBox();
  checkbox1->setText("Test Checkbox");

  XCheckBox* checkbox2 = new XCheckBox();
  checkbox2->set_checked_color(QColor(255, 150, 100));
  checkbox2->setText("Alternate Color");

  layer_1_layout->addWidget(checkbox1);
  layer_1_layout->addWidget(checkbox2);

  layer_1_layout->addStretch();

  XCheckBox* checkbox3 = new XCheckBox();
  checkbox3->setText("Checkbox with really long text to test truncation");

  layer_2_layout->addWidget(checkbox3);

  group_layout->addStretch();

  return group;
}

QWidget* DebugTab::CreateRadioButtonGroup() {
  QWidget* group = new QWidget();

  QVBoxLayout* group_layout = new QVBoxLayout();
  group_layout->setContentsMargins(32, 16, 32, 0);
  group_layout->setSpacing(16);
  group->setLayout(group_layout);

  QLabel* title = new QLabel("Radio Buttons");
  title->setFont(QFont("Segoe UI", 24));

  group_layout->addWidget(title);

  QVBoxLayout* control_layout = new QVBoxLayout();
  control_layout->setContentsMargins(0, 0, 0, 0);
  control_layout->setSpacing(12);

  QHBoxLayout* layer_1_layout = new QHBoxLayout();
  layer_1_layout->setContentsMargins(12, 0, 12, 0);
  layer_1_layout->setSpacing(20);

  QHBoxLayout* layer_2_layout = new QHBoxLayout();
  layer_2_layout->setContentsMargins(12, 0, 12, 0);
  layer_2_layout->setSpacing(20);

  control_layout->addLayout(layer_1_layout);
  control_layout->addLayout(layer_2_layout);

  control_layout->addStretch();

  group_layout->addLayout(control_layout);

  XRadioButton* radio1 = new XRadioButton();
  radio1->setText("Test Radio Button 1");

  XRadioButton* radio2 = new XRadioButton();
  radio2->setText("Test Radio Button 2");

  layer_1_layout->addWidget(radio1);
  layer_1_layout->addWidget(radio2);

  layer_1_layout->addStretch();

  XRadioButton* radio3 = new XRadioButton();
  radio3->setText("Radio Button with really long text to test truncation");
  radio3->set_checked_color(QColor(255, 150, 100));

  XRadioButton* radio4 = new XRadioButton();
  radio4->setText("Error");
  radio4->set_checked_color(QColor(255, 0, 0));

  layer_2_layout->addWidget(radio3);
  layer_2_layout->addWidget(radio4);

  layer_2_layout->addStretch();

  // add radio buttons to their respective groups

  QButtonGroup* bg1 = new QButtonGroup();
  QButtonGroup* bg2 = new QButtonGroup();

  bg1->addButton(radio1);
  bg1->addButton(radio2);

  bg2->addButton(radio3);
  bg2->addButton(radio4);

  group_layout->addStretch();

  return group;
}

}  // namespace qt
}  // namespace ui
}  // namespace xe

#endif