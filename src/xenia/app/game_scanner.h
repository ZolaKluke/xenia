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
typedef struct {
  size_t size;
  vfs::File* file;
  XexHeader header;

  uint8_t* Read(size_t offset, size_t size) const {
    uint8_t* data = new uint8_t[size];
    size_t bytes_read;
    file->ReadSync(data, size, offset, &bytes_read);
    return *&data;
  }

  template <typename T>
  T Read(uint32_t offset) {
    uint8_t* data = Read(offset, sizeof(T));
    T swap = xe::load_and_swap<T>(data);
    delete[] data;
    return swap;
  }
} Xex;

typedef struct {
  size_t size;
  vfs::File* file;
} Nxe;

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
  static X_STATUS XexReadLoaderInfo(Xex* xex, uint32_t offset);
  static X_STATUS XexReadSectionInfo(Xex* xex, uint32_t offset);
  static X_STATUS XexDecryptHeaderKey(Xex* xex);
  static XexOptHeader XexReadOptionalHeader(Xex* xex, uint8_t* cursor);
  static XexOptHeader XexReadAlternateTitleIds(Xex* xex, uint32_t offset);
  static XexOptHeader XexReadDefaultFsCacheSize(Xex* xex, uint32_t value);
  static XexOptHeader XexReadDefaultHeapSize(Xex* xex, uint32_t value);
  static XexOptHeader XexReadDefaultStackSize(Xex* xex, uint32_t value);
  static XexOptHeader XexReadDeviceId(Xex* xex, uint8_t* data);
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
  static X_STATUS XexVerifyMagic(Xex* xex);
};

}  // namespace app
}  // namespace xe

#endif