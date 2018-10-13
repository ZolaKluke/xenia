/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2018 Ben Vanik. All rights reserved.                             *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 */

#ifndef XENIA_APP_GAME_LIBRARY_H_
#define XENIA_APP_GAME_LIBRARY_H_

#include <memory>
#include <vector>

#include "game_entry.h"

namespace xe {
namespace app {

class GameLibrary {
 public:
  GameLibrary(GameLibrary const&) = delete;
  GameLibrary& operator=(GameLibrary const&) = delete;

  static std::shared_ptr<GameLibrary> Instance(){
    static std::shared_ptr<GameLibrary> instance{new GameLibrary};
    return instance;
  }

  int AddEntry(GameEntry* entry);
  int RemoveEntry(GameEntry* entry);

  const std::vector<GameEntry*> games() const { return games_; }

 private:
  GameLibrary();

  std::vector<GameEntry*> games_;
};

}  // namespace app
}  // namespace xe

#endif