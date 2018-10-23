#ifndef XENIA_UI_QT_TAB_H_
#define XENIA_UI_QT_TAB_H_

#include <QString>
#include <QWidget>

namespace xe {
namespace ui {
namespace qt {

class XTab : public QWidget {
  Q_OBJECT
 public:
  explicit XTab(const QString& tab_name) : tab_name_(tab_name) {}

  const QString& tab_name() const { return tab_name_; }

 private:
  QString tab_name_;
};

}  // namespace qt
}  // namespace ui
}  // namespace xe

#endif