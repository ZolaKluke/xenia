/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2018 Ben Vanik. All rights reserved.                             *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 */

#ifndef XENIA_APP_GAME_ENTRY_H_
#define XENIA_APP_GAME_ENTRY_H_

namespace xe {
namespace app {
  
  struct GameEntry {
    char* path;

    char* title_id;
    char* media_id;
  };

}
}  // namespace xe

#endif