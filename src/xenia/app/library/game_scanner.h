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
  const NxeInfo* nxe_info = nullptr;
  const XexInfo* xex_info = nullptr;

  GameInfo() {}
  GameInfo(const GameInfo& other) = delete;
  GameInfo operator=(const GameInfo& other) = delete;
  ~GameInfo() {
    delete nxe_info;
    delete xex_info;
  }
};

class XGameScanner {
 public:
  static const vector<const GameInfo*> ScanPath(const wstring& path);
  static const GameInfo* ScanGame(const wstring& path);

 private:
};

}  // namespace app
}  // namespace xe

#endif