#include "xenia/ui/qt/widgets/slider.h"

namespace xe {
namespace ui {
namespace qt {

XSlider::XSlider(Qt::Orientation orientation, QWidget* parent)
    : Themeable<QSlider>("XSlider", orientation, parent) {}

}  // namespace qt
}  // namespace ui
}  // namespace xe