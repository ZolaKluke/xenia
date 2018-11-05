#ifndef XENIA_APP_GAME_SCANNER_H_
#define XENIA_APP_GAME_SCANNER_H_

#include "xenia/app/library/nxe_scanner.h"
#include "xenia/app/library/scanner_utils.h"
#include "xenia/app/library/xex_scanner.h"

namespace xe {
namespace app {

struct GameInfo {
  XGameFormat format;
  std::wstring path;
  const NxeInfo* nxe_info = nullptr;
  const XexInfo* xex_info = nullptr;
};

class XGameScanner {
 public:
  static const GameInfo ScanGame(const std::wstring& path);

 private:
  
};

}  // namespace app
}  // namespace xe

#endif