#ifndef XENIA_APP_GAME_SCANNER_H_
#define XENIA_APP_GAME_SCANNER_H_

#include "xenia/app/library/nxe_scanner.h"
#include "xenia/app/library/scanner_utils.h"
#include "xenia/app/library/xex_scanner.h"

#include <vector>

namespace xe {
namespace app {
using std::vector;

struct GameInfo {
  XGameFormat format;
  wstring path;
  const NxeInfo* nxe_info = nullptr;
  const XexInfo* xex_info = nullptr;
};

class GameScanner {
 public:
  static const bool IsGame(const wstring& path);
  static const vector<GameInfo> ScanPath(const wstring& path);
  static const GameInfo ScanGame(const wstring& path);

 private:
};

}  // namespace app
}  // namespace xe

#endif