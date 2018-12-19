#ifndef XENIA_APP_XEX_SCANNER_H_
#define XENIA_APP_XEX_SCANNER_H_

#include "xenia/kernel/util/xdbf_utils.h"
#include "xenia/kernel/util/xex2_info.h"
#include "xenia/vfs/file.h"

#include <string>

namespace xe {
namespace app {
using kernel::util::XdbfGameData;
using vfs::File;

typedef xe_xex2_header_t XexHeader;
typedef xe_xex2_opt_header_t XexOptHeader;
typedef xe_xex2_game_ratings_t XGameRatings;
typedef xe_xex2_region_flags XGameRegions;
typedef xe_xex2_version_t XGameVersion;

struct XexInfo {
  std::string game_title;
  XexHeader* header = nullptr;
  uint8_t* icon = nullptr;
  size_t icon_size;

  XexInfo() {}
  XexInfo(const XexInfo& other) = delete;
  XexInfo& operator=(const XexInfo& other) = delete;
  ~XexInfo() {
    delete header;
    delete[] icon;
  }
};

class XexScanner {
 public:
  static const XexInfo* ScanXex(File* xex_file);

 private:
  static XexHeader* ReadXexHeader(File* file);
  static X_STATUS ReadXexImage(File* file, XexHeader* header, uint8_t*& image,
                               size_t& image_size);
  static X_STATUS ReadXexResources(File* file, XexInfo* info);
};

}  // namespace app
}  // namespace xe

#endif