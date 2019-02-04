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

  double label_indent() const { return label_indent_; }
  void set_label_indent(double indent) { label_indent_ = indent; }

  const QColor& border_color() const { return border_color_; }
  void set_border_color(const QColor& color) { border_color_ = color; }
  const QColor& checked_color() const { return checked_color_; }
  void set_checked_color(const QColor& color) { checked_color_ = color; }

 protected:
  void paintEvent(QPaintEvent* e) override;
  QSize sizeHint() const override;

 private:
  void Update();

  double label_indent_ = 8.0;
  QColor border_color_;
  QColor checked_color_;
};

}  // namespace qt
}  // namespace ui
}  // namespace xe

#endif