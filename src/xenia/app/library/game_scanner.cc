#include "xenia/app/library/game_scanner.h"
#include "xenia/app/library/scanner_utils.h"

namespace xe {
namespace app {

const GameInfo XGameScanner::ScanGame(const std::wstring& path) {
  GameInfo info;
  info.path = path;
  info.format = ResolveFormat(path);

  auto device = CreateDevice(path);
  if (device == nullptr || !device->Initialize()) {
    throw 0x0;  // TODO
  }

  // Read XEX
  auto xex_entry = device->ResolvePath("default.xex");
  if (xex_entry) {
    File* xex_file = nullptr;
    auto status = xex_entry->Open(vfs::FileAccess::kFileReadData, &xex_file);
    if (XSUCCEEDED(status)) {
      info.xex_info = XexScanner::ScanXex(xex_file);
    }

    xex_file->Destroy();
  }

  // Read NXE
  auto nxe_entry = device->ResolvePath("nxeart");
  if (nxe_entry) {
    File* nxe_file = nullptr;
    auto status = nxe_entry->Open(vfs::FileAccess::kFileReadData, &nxe_file);
    if (XSUCCEEDED(status)) {
      info.nxe_info = NxeScanner::ScanNxe(nxe_file);
    }

    nxe_file->Destroy();
  }

  delete device;
  return info;
}

// XEX Scanning ///////////////////////////////////////////////////////////////

}  // namespace app
}  // namespace xe