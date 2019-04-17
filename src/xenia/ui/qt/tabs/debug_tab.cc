#include "xenia/ui/qt/tabs/debug_tab.h"

#include <QButtonGroup>
#include <QGraphicsEffect>
#include <QHBoxLayout>

#include "xenia/ui/qt/widgets/checkbox.h"
#include "xenia/ui/qt/widgets/groupbox.h"
#include "xenia/ui/qt/widgets/radio_button.h"
#include "xenia/ui/qt/widgets/separator.h"
#include "xenia/ui/qt/widgets/slider.h"

#ifdef DEBUG

namespace xe {
namespace ui {
namespace qt {

// sidebar_->addAction(0xE90F, "Components");
// sidebar_->addAction(0xE700, "Navigation");
// sidebar_->addAction(0xE790, "Theme");
// sidebar_->addAction(0xE8F1, "Library");

DebugTab::DebugTab() : XTab("Debug", "DebugTab") {
  sidebar_items_ =
      QList<SidebarItem>{{0xE90F, "Components", CreateComponentsTab()},
                         {0xE700, "Navigation", CreateNavigationTab()},
                         {0xE790, "Theme", CreateThemeTab()},
                         {0xE8F1, "Library", CreateLibraryTab()}};

  Build();
}

void DebugTab::Build() {
  layout_ = new QHBoxLayout();
  layout_->setContentsMargins(0, 0, 0, 0);
  layout_->setSpacing(0);
  setLayout(layout_);

  content_layout_ = new QStackedLayout();
  for (const SidebarItem& item : sidebar_items_) {
    content_layout_->addWidget(item.widget);
  }

  BuildSidebar();
  layout_->addLayout(content_layout_);
}

void DebugTab::BuildSidebar() {
  sidebar_container_ = new QWidget(this);
  sidebar_container_->setObjectName("sidebarContainer");
  sidebar_container_->setStyleSheet(
      "background: #232323; min-width: 300px; max-width: 300px;");
  QVBoxLayout* sidebar_layout = new QVBoxLayout;
  sidebar_layout->setMargin(0);
  sidebar_layout->setSpacing(0);

  sidebar_container_->setLayout(sidebar_layout);

  // Add drop shadow to sidebar widget
  QGraphicsDropShadowEffect* effect = new QGraphicsDropShadowEffect;
  effect->setBlurRadius(16);
  effect->setXOffset(4);
  effect->setYOffset(0);
  effect->setColor(QColor(0, 0, 0, 64));

  sidebar_container_->setGraphicsEffect(effect);

  // Create sidebar
  sidebar_ = new XSideBar;
  sidebar_->setOrientation(Qt::Vertical);
  sidebar_->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
  sidebar_->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Fixed);

  // Create sidebar title
  QWidget* sidebar_title = new QWidget;
  sidebar_title->setObjectName("sidebarTitle");

  QVBoxLayout* title_layout = new QVBoxLayout;
  title_layout->setMargin(0);
  title_layout->setContentsMargins(0, 40, 0, 0);
  title_layout->setSpacing(0);

  sidebar_title->setLayout(title_layout);

  // Title labels
  QLabel* xenia_title = new QLabel("Debug");
  xenia_title->setObjectName("sidebarTitleLabel");
  xenia_title->setStyleSheet(
      "color: #FFF;"
      "font-size: 32px; "
      "font-weight: normal;");
  xenia_title->setAlignment(Qt::AlignCenter);
  title_layout->addWidget(xenia_title, 0, Qt::AlignCenter);

  // Title separator
  auto separator = new XSeparator;
  separator->setStyleSheet("background: #505050");
  title_layout->addSpacing(32);
  title_layout->addWidget(separator, 0, Qt::AlignCenter);

  // Setup Sidebar toolbar
  sidebar_->addWidget(sidebar_title);

  sidebar_->addSpacing(20);

  for (const SidebarItem& item : sidebar_items_) {
    auto btn = sidebar_->addAction(item.glyph, item.name);
    connect(btn, &XSideBarButton::clicked,
            [&]() { content_layout_->setCurrentWidget(item.widget); });
  }

  sidebar_layout->addWidget(sidebar_, 0, Qt::AlignHCenter | Qt::AlignTop);
  sidebar_layout->addStretch(1);

  // Add sidebar to tab widget
  layout_->addWidget(sidebar_container_, 0, Qt::AlignLeft);
}

QWidget* DebugTab::CreateComponentsTab() {
  QWidget* w = new QWidget();
  QVBoxLayout* layout = new QVBoxLayout();
  w->setLayout(layout);

  layout->setSpacing(16);
  layout->setContentsMargins(0, 16, 0, 0);

  layout->addWidget(CreateSliderGroup());
  layout->addWidget(CreateCheckboxGroup());
  layout->addWidget(CreateRadioButtonGroup());

  layout->addStretch();

  return w;
}
QWidget* DebugTab::CreateNavigationTab() {
  QWidget* w = new QWidget();
  w->setStyleSheet("background: blue;");
  return w;
}
QWidget* DebugTab::CreateThemeTab() {
  QWidget* w = new QWidget();
  w->setStyleSheet("background: green;");
  return w;
}
QWidget* DebugTab::CreateLibraryTab() {
  QWidget* w = new QWidget();
  w->setStyleSheet("background: yellow;");
  return w;
}

QWidget* DebugTab::CreateSliderGroup() {
  QWidget* group = new QWidget();
  group->setStyleSheet("QLabel { color: white }");

  QVBoxLayout* group_layout = new QVBoxLayout();
  group_layout->setContentsMargins(32, 0, 32, 0);
  group_layout->setSpacing(16);
  group->setLayout(group_layout);

  XGroupBox* groupbox = new XGroupBox("Sliders");

  QHBoxLayout* groupbox_layout = new QHBoxLayout();
  groupbox_layout->setContentsMargins(16, 16, 16, 16);
  groupbox->setLayout(groupbox_layout);

  group_layout->addWidget(groupbox);

  // horizontal slider

  XSlider* horizontal_slider = new XSlider();
  horizontal_slider->setFixedWidth(120);

  QLabel* horizontal_label = new QLabel();
  horizontal_label->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Ignored);
  connect(horizontal_slider, &XSlider::valueChanged, [=](int value) {
    QString text;
    horizontal_label->setText(text.sprintf("Value: %02d", value));
  });
  horizontal_slider->valueChanged(0);

  groupbox_layout->addWidget(horizontal_slider);
  groupbox_layout->addWidget(horizontal_label);

  groupbox_layout->addSpacing(16);

  // vertical slider

  XSlider* vertical_slider = new XSlider(Qt::Vertical);
  vertical_slider->setFixedSize(20, 60);
  // vertical slider causes issues in a vertical orientation right now
  // TODO: fix this. for now just ignore its vertical size
  vertical_slider->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Ignored);

  QLabel* vertical_label = new QLabel();
  connect(vertical_slider, &XSlider::valueChanged, [=](int value) {
    QString text;
    vertical_label->setText(text.sprintf("Value: %02d", value));
  });
  vertical_slider->valueChanged(0);

  groupbox_layout->addWidget(vertical_slider);
  groupbox_layout->addWidget(vertical_label);

  groupbox_layout->addStretch();

  return group;
}

QWidget* DebugTab::CreateCheckboxGroup() {
  QWidget* group = new QWidget();

  QVBoxLayout* group_layout = new QVBoxLayout();
  group_layout->setContentsMargins(32, 0, 32, 0);
  group_layout->setSpacing(16);
  group->setLayout(group_layout);

  XGroupBox* groupbox = new XGroupBox("Checkboxes");

  QVBoxLayout* groupbox_layout = new QVBoxLayout();
  groupbox_layout->setContentsMargins(16, 16, 16, 16);
  groupbox->setLayout(groupbox_layout);

  group_layout->addWidget(groupbox);

  QHBoxLayout* layer_1_layout = new QHBoxLayout();
  layer_1_layout->setContentsMargins(0, 0, 0, 0);
  layer_1_layout->setSpacing(20);

  QHBoxLayout* layer_2_layout = new QHBoxLayout();
  layer_2_layout->setContentsMargins(0, 0, 0, 0);
  layer_2_layout->setSpacing(20);

  groupbox_layout->addLayout(layer_1_layout);
  groupbox_layout->addLayout(layer_2_layout);

  group_layout->addLayout(groupbox_layout);

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

  return group;
}

QWidget* DebugTab::CreateRadioButtonGroup() {
  QWidget* group = new QWidget();

  QVBoxLayout* group_layout = new QVBoxLayout();
  group_layout->setContentsMargins(32, 0, 32, 0);
  group_layout->setSpacing(0);
  group->setLayout(group_layout);

  XGroupBox* groupbox = new XGroupBox("Radio Buttons");

  QVBoxLayout* groupbox_layout = new QVBoxLayout();
  groupbox_layout->setContentsMargins(16, 16, 16, 16);
  groupbox->setLayout(groupbox_layout);

  group_layout->addWidget(groupbox);

  QHBoxLayout* layer_1_layout = new QHBoxLayout();
  layer_1_layout->setContentsMargins(0, 0, 0, 0);
  layer_1_layout->setSpacing(20);

  QHBoxLayout* layer_2_layout = new QHBoxLayout();
  layer_2_layout->setContentsMargins(0, 0, 0, 0);
  layer_2_layout->setSpacing(20);

  groupbox_layout->addLayout(layer_1_layout);
  groupbox_layout->addLayout(layer_2_layout);

  group_layout->addLayout(groupbox_layout);

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

  return group;
}

}  // namespace qt
}  // namespace ui
}  // namespace xe

#endif