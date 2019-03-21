/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2013 Ben Vanik. All rights reserved.                             *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 */

#include "xenia/kernel/xam/xam_module.h"

#include <vector>

#include "xenia/base/math.h"
#include "xenia/kernel/kernel_state.h"
#include "xenia/kernel/xam/xam_private.h"

namespace xe {
namespace kernel {
namespace xam {

typedef struct _FU_LAUNCH_DATA {
  xe::be<uint32_t> Source;           // 0x0 sz:0x4
  xe::be<uint32_t> Reason;           // 0x4 sz:0x4
  xe::be<uint32_t> LiveEnabled;      // 0x8 sz:0x4
  char Padding[0x1F0];               // 0xC sz:0x1F0
  char XBE[0x200];                   // 0x1FC sz:0x200
} FU_LAUNCH_DATA, *PFU_LAUNCH_DATA;  // size 1020
 
XamModule::XamModule(Emulator* emulator, KernelState* kernel_state)
    : KernelModule(kernel_state, "xe:\\xam.xex"), loader_data_() {
  RegisterExportTable(export_resolver_);

// Register all exported functions.
  RegisterAvatarExports(export_resolver_, kernel_state_);
  RegisterContentExports(export_resolver_, kernel_state_);
  RegisterInfoExports(export_resolver_, kernel_state_);
  RegisterInputExports(export_resolver_, kernel_state_);
  RegisterMsgExports(export_resolver_, kernel_state_);
  RegisterNetExports(export_resolver_, kernel_state_);
  RegisterNotifyExports(export_resolver_, kernel_state_);
  RegisterNuiExports(export_resolver_, kernel_state_);
  RegisterUIExports(export_resolver_, kernel_state_);
  RegisterUserExports(export_resolver_, kernel_state_);
  RegisterVideoExports(export_resolver_, kernel_state_);
  RegisterVoiceExports(export_resolver_, kernel_state_);

  loader_data_.launch_data.resize(sizeof(FU_LAUNCH_DATA));
  FU_LAUNCH_DATA* data = (FU_LAUNCH_DATA*)loader_data_.launch_data.data();
  data->Source = 1;
  data->Reason = 0;
  data->LiveEnabled = 0;
  memset(data->Padding, 0, 0x1F0);
  strcpy_s(data->XBE, "\\Device\\Harddisk0\\Partition1\\default.xbe");
  loader_data_.launch_data_present = true;
}

std::vector<xe::cpu::Export*> xam_exports(4096);

xe::cpu::Export* RegisterExport_xam(xe::cpu::Export* export_entry) {
  assert_true(export_entry->ordinal < xam_exports.size());
  xam_exports[export_entry->ordinal] = export_entry;
  return export_entry;
}

void XamModule::RegisterExportTable(xe::cpu::ExportResolver* export_resolver) {
  assert_not_null(export_resolver);

// Build the export table used for resolution.
#include "xenia/kernel/util/export_table_pre.inc"
  static xe::cpu::Export xam_export_table[] = {
#include "xenia/kernel/xam/xam_table.inc"
  };
#include "xenia/kernel/util/export_table_post.inc"
  for (size_t i = 0; i < xe::countof(xam_export_table); ++i) {
    auto& export_entry = xam_export_table[i];
    assert_true(export_entry.ordinal < xam_exports.size());
    if (!xam_exports[export_entry.ordinal]) {
      xam_exports[export_entry.ordinal] = &export_entry;
    }
  }
  export_resolver->RegisterTable("xam.xex", &xam_exports);
}

XamModule::~XamModule() {}

}  // namespace xam
}  // namespace kernel
}  // namespace xe
