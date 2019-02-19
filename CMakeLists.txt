
add_llvm_loadable_module( LLVMshmemheatpass
  shmemheatpass.cpp

  DEPENDS
  intrinsics_gen
  PLUGIN_TOOL
  opt
  )


