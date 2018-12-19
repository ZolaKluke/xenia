#ifndef XENIA_APP_NXE_SCANNER_H_
#define XENIA_APP_NXE_SCANNER_H_

#include "xenia/vfs/file.h"

namespace xe {
namespace app {
using vfs::File;

struct NxeInfo {
  std::string game_title;
  uint8_t* icon = nullptr;
  size_t icon_size;
  uint8_t* nxe_background_image = nullptr;  // TODO
  size_t nxe_background_image_size;         // TODO
  uint8_t* nxe_slot_image = nullptr;        // TODO
  size_t nxe_slot_image_size;               // TODO

  ~NxeInfo() {
    if (icon != nullptr) delete[] icon;
    if (nxe_background_image != nullptr) delete[] nxe_background_image;
    if (nxe_slot_image != nullptr) delete[] nxe_slot_image;
  }
};

class NxeScanner {
 public:
  static const NxeInfo* ScanNxe(File* file);
};

}  // namespace app
}  // namespace xe

#endif