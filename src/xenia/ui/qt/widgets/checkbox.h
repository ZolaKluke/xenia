#ifndef XENIA_UI_QT_CHECKBOX_H_
#define XENIA_UI_QT_CHECKBOX_H_

#include <QCheckBox>
#include "xenia/ui/qt/themeable_widget.h"

namespace xe {
namespace ui {
namespace qt {

class XCheckBox : public Themeable<QCheckBox> {
  Q_OBJECT
 public:
  explicit XCheckBox(QWidget* parent = nullptr);

 protected:
  void paintEvent(QPaintEvent* e) override;
  QSize sizeHint() const override;

 private:
};

}  // namespace qt
}  // namespace ui
}  // namespace xe

#endif