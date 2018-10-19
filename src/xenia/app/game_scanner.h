#ifndef XENIA_APP_GAME_SCANNER_H_
#define XENIA_APP_GAME_SCANNER_H_

#include "xenia/app/game_entry.h"
#include "xenia/kernel/util/xex2_info.h"
#include "xenia/vfs/file.h"

#include <QString>
#include <vector>

namespace xe {
namespace app {
using std::vector;

typedef xe_xex2_header_t XexHeader;
typedef xe_xex2_opt_header_t XexOptHeader;
typedef xe_xex2_loader_info_t XexSecurityInfo;
typedef struct {
  size_t size;
  char* data;
  XexHeader header;
  XexSecurityInfo security_info;
} Xex;

class GameScanner {
 public:
  static vector<QString> ScanPath(const QString& path);
  static vector<QString> ScanPaths(const vector<QString>& paths);
  static GameEntry* ScanFile(const QString& filepath);
  static vector<GameEntry*> ScanFiles(const vector<QString>& filepaths);

 private:
  static GameEntry* ScanIso(const QString& isopath);
  static GameEntry* ScanXex(const QString& xexpath);

  static Xex* ReadXex(vfs::File* xex);
  static X_STATUS XexReadHeader(Xex* xex);
  static XexOptHeader XexReadOptionalHeader(Xex* xex, char* cursor);
  static XexOptHeader XexReadAlternateTitleIds(Xex* xex, uint32_t offset);
  static XexOptHeader XexReadDeviceId(Xex* xex, char* data);
  static XexOptHeader XexReadExecutionInfo(Xex* xex, uint32_t offset);
  static XexOptHeader XexReadFileFormatInfo(Xex* xex, uint32_t offset);
  static XexOptHeader XexReadGameRatings(Xex* xex, uint32_t offset);
  static XexOptHeader XexReadImageBaseAddress(Xex* xex, uint32_t value);
  static XexOptHeader XexReadLanKey(Xex* xex, uint32_t offset);
  static XexOptHeader XexReadMultiDiscMediaIds(Xex* xex, uint32_t offset);
  static XexOptHeader XexReadOriginalBaseAddress(Xex* xex, uint32_t value);
  static XexOptHeader XexReadOriginalPeName(Xex* xex, uint32_t offset);
  static XexOptHeader XexReadResourceInfo(Xex* xex, uint32_t offset);
  static XexOptHeader XexReadSystemFlags(Xex* xex, uint32_t offset);
  static X_STATUS XexReadSecurityInfo(Xex* xex, uint32_t offset);
  static bool VerifyXexMagic(vfs::File* xex);
};

}  // namespace app
}  // namespace xe

#endif