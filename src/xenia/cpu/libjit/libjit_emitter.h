/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2013 Ben Vanik. All rights reserved.                             *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 */

#ifndef XENIA_CPU_LIBJIT_LIBJIT_EMITTER_H_
#define XENIA_CPU_LIBJIT_LIBJIT_EMITTER_H_

#include <xenia/cpu/global_exports.h>
#include <xenia/cpu/sdb.h>
#include <xenia/cpu/ppc/instr.h>

#include <jit/jit.h>


namespace xe {
namespace cpu {
namespace libjit {


class LibjitEmitter {
public:
  LibjitEmitter(xe_memory_ref memory, jit_context_t context);
  ~LibjitEmitter();

  jit_context_t context();
  jit_type_t fn_signature();

  int PrepareFunction(sdb::FunctionSymbol* symbol);
  int MakeFunction(sdb::FunctionSymbol* symbol, jit_function_t fn);

  sdb::FunctionSymbol* fn();
  jit_function_t gen_fn();
  sdb::FunctionBlock* fn_block();

  void PushInsertPoint();
  void PopInsertPoint();

  void GenerateBasicBlocks();
//  llvm::BasicBlock* GetBasicBlock(uint32_t address);
//  llvm::BasicBlock* GetNextBasicBlock();
//  llvm::BasicBlock* GetReturnBasicBlock();

  int GenerateIndirectionBranch(uint32_t cia, jit_value_t target,
                                bool lk, bool likely_local);

  jit_value_t LoadStateValue(uint32_t offset, jit_type_t type,
                             const char* name = "");
  void StoreStateValue(uint32_t offset, jit_type_t type, jit_value_t value);

  jit_value_t cia_value();

  jit_value_t SetupLocal(jit_type_t type, const char* name);
  void FillRegisters();
  void SpillRegisters();

  jit_value_t xer_value();
  void update_xer_value(jit_value_t value);
  void update_xer_with_overflow(jit_value_t value);
  void update_xer_with_carry(jit_value_t value);
  void update_xer_with_overflow_and_carry(jit_value_t value);

  jit_value_t lr_value();
  void update_lr_value(jit_value_t value);

  jit_value_t ctr_value();
  void update_ctr_value(jit_value_t value);

  jit_value_t cr_value(uint32_t n);
  void update_cr_value(uint32_t n, jit_value_t value);
  void update_cr_with_cond(uint32_t n, jit_value_t lhs, jit_value_t rhs,
                           bool is_signed);

  jit_value_t gpr_value(uint32_t n);
  void update_gpr_value(uint32_t n, jit_value_t value);
  jit_value_t fpr_value(uint32_t n);
  void update_fpr_value(uint32_t n, jit_value_t value);

  jit_value_t GetMembase();
  jit_value_t GetMemoryAddress(uint32_t cia, jit_value_t addr);
  jit_value_t ReadMemory(
      uint32_t cia, jit_value_t addr, uint32_t size, bool acquire = false);
  void WriteMemory(
      uint32_t cia, jit_value_t addr, uint32_t size, jit_value_t value,
      bool release = false);

private:
  int MakeUserFunction();
  int MakePresentImportFunction();
  int MakeMissingImportFunction();

  void GenerateSharedBlocks();
  int PrepareBasicBlock(sdb::FunctionBlock* block);
  void GenerateBasicBlock(sdb::FunctionBlock* block);
  void SetupLocals();

  xe_memory_ref         memory_;
  jit_context_t         context_;
  GlobalExports         global_exports_;
  jit_type_t            fn_signature_;
  jit_type_t            global_export_signature_;

  sdb::FunctionSymbol*  fn_;
  jit_function_t        gen_fn_;
  sdb::FunctionBlock*   fn_block_;
  // llvm::BasicBlock*     return_block_;
  // llvm::BasicBlock*     internal_indirection_block_;
  // llvm::BasicBlock*     external_indirection_block_;
  // llvm::BasicBlock*     bb_;

  // std::vector<std::pair<llvm::BasicBlock*, llvm::BasicBlock::iterator> >
  //     insert_points_;

  //std::map<uint32_t, llvm::BasicBlock*> bbs_;

  // Address of the instruction being generated.
  uint32_t        cia_;

  ppc::InstrAccessBits access_bits_;
  struct {
    jit_value_t  indirection_target;
    jit_value_t  indirection_cia;

    jit_value_t  xer;
    jit_value_t  lr;
    jit_value_t  ctr;
    jit_value_t  cr[8];
    jit_value_t  gpr[32];
    jit_value_t  fpr[32];
  } locals_;
};


}  // namespace libjit
}  // namespace cpu
}  // namespace xe


#endif  // XENIA_CPU_LIBJIT_LIBJIT_EMITTER_H_
