#ifndef XENIA_UI_QT_CARD_H_
#define XENIA_UI_QT_CARD_H_

#include <QLabel>
#include <QVBoxLayout>
#include "xenia/ui/qt/themeable_widget.h"

namespace xe {
namespace ui {
namespace qt {

class XCard : public Themeable<QWidget> {
  Q_OBJECT
 public:
  explicit XCard(QWidget* parent = nullptr);
  explicit XCard(const QString& title, QWidget* parent = nullptr);

  void add_widget(QWidget* widget);

  const QString& title() const { return title_; }
  void set_title(const QString& title) {
    title_ = title;
    Update();
  }

 private:
  void Build();
  void BuildTitle();

  void Update();

  QString title_;
  QVBoxLayout* layout_ = nullptr;
  QWidget* container_ = nullptr;
  QLabel* title_label_ = nullptr;
};

}  // namespace qt
}  // namespace ui
}  // namespace xe

#endif