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
  wstring filename;
  NxeInfo nxe_info;
  XexInfo xex_info;

  GameInfo() {}
  GameInfo(const GameInfo& other) = delete;
  GameInfo operator=(const GameInfo& other) = delete;
};

class XGameScanner {
 public:
  static const vector<GameInfo*> ScanPath(const wstring& path);
  static X_STATUS ScanGame(const wstring& path, GameInfo* out_info);

 private:
};

}  // namespace app
}  // namespace xe

#endif