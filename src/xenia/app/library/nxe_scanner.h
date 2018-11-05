#ifndef XENIA_APP_NXE_SCANNER_H_
#define XENIA_APP_NXE_SCANNER_H_

#include "xenia/vfs/file.h"

namespace xe {
namespace app {
using vfs::File;

struct NxeInfo {
  std::string game_title;
  uint8_t* icon;
  size_t icon_size;
  uint8_t* nxe_background_image;     // TODO
  size_t nxe_background_image_size;  // TODO
  uint8_t* nxe_slot_image;           // TODO
  size_t nxe_slot_image_size;        // TODO
};

class NxeScanner {
 public:
  static const NxeInfo* ScanNxe(File* file);
};

}  // namespace app
}  // namespace xe

#endif