#===============================================================================
# LOAD LLVM CONFIGURATION
#===============================================================================
# Set this to a valid LLVM installation dir
set(CT_LLVM_INSTALL_DIR "" CACHE PATH "LLVM installation directory")

# Add the location of ClangConfig.cmake to CMake search paths (so that
# find_package can locate it)
list(APPEND CMAKE_PREFIX_PATH "${CT_LLVM_INSTALL_DIR}/lib/cmake/clang/")

find_package(Clang REQUIRED CONFIG)
find_package(LLVM REQUIRED CONFIG)

list(APPEND CMAKE_MODULE_PATH "${LLVM_CMAKE_DIR}")
include(AddLLVM)

add_definitions(${LLVM_DEFINITIONS})
include_directories(${LLVM_INCLUDE_DIRS})
include_directories(${CLANG_INCLUDE_DIRS})

# HelloWorld includes headers from LLVM and Clang - update the include paths
# accordingly
include_directories(SYSTEM "${LLVM_INCLUDE_DIRS};${CLANG_INCLUDE_DIRS}")


#===============================================================================
# ADD THE TARGET
#===============================================================================

add_executable(remove-comments RemoveComments.cc)
target_link_libraries(remove-comments
    clangTooling
    clangBasic
    clangAST
    clangLex
    clangRewrite
    clangFrontend
    clangDriver
    LLVMSupport
)

add_library(cpp2c SHARED
  ASTUtils.cc
  AlignmentMatchers.cc
  BoundingMatchers.cc
  Cpp2CAction.cc
  Cpp2CASTConsumer.cc
  DefinitionInfoCollector.cc
  DeclStmtTypeLoc.cc
  DeclCollectorMatchHandler.cc
  ExpansionMatchHandler.cc
  IncludeCollector.cc
  MacroForest.cc
  MacroExpansionArgument.cc
  MacroExpansionNode.cc
  StmtCollectorMatchHandler.cc
)

# Allow undefined symbols in shared objects on Darwin (this is the default
# behaviour on Linux)
target_link_libraries(cpp2c
  "$<$<PLATFORM_ID:Darwin>:-undefined dynamic_lookup>")
