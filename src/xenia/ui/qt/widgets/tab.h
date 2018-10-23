#ifndef XENIA_UI_QT_TAB_H_
#define XENIA_UI_QT_TAB_H_

#include <QString>
#include <QWidget>

#include <QLabel>
#include <QVBoxLayout>

namespace xe {
namespace ui {
namespace qt {

class XTab : public QWidget {
  Q_OBJECT
 public:
  explicit XTab(const QString& tab_name) : tab_name_(tab_name) {
    //PLACEHOLDER
    layout_ = new QVBoxLayout();
    QLabel* placeholder = new QLabel(tab_name_);
    placeholder->setFont(QFont(":resources/fonts/segoeui.ttf"));
    placeholder->setStyleSheet("color: #ffffff; font-size: 48px;");
    layout_->addWidget(placeholder);
    setLayout(layout_);
  }

  const QString& tab_name() const { return tab_name_; }

 private:
  QString tab_name_;
  QVBoxLayout* layout_;
};

}  // namespace qt
}  // namespace ui
}  // namespace xe

#endif