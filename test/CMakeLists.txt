configure_lit_site_cfg(
  ${CMAKE_CURRENT_SOURCE_DIR}/lit.site.cfg.py.in
  ${CMAKE_CURRENT_BINARY_DIR}/lit.site.cfg.py
  MAIN_CONFIG
  ${CMAKE_CURRENT_SOURCE_DIR}/lit.cfg.py
  )

# Set the depends list as a variable so that it can grow conditionally.
# NOTE: Sync the substitutions in test/lit.cfg when adding to this list.
set(LLVM_TEST_DEPENDS
          FileCheck
          not
          # topt
        )


add_lit_testsuite(check-topt "Running the topt regression tests"
  ${CMAKE_CURRENT_BINARY_DIR}
  DEPENDS ${LLVM_TEST_DEPENDS}
  )

add_lit_testsuites(TOPT ${CMAKE_CURRENT_SOURCE_DIR}
  DEPENDS ${LLVM_TEST_DEPENDS}
  )

set_target_properties(check-topt PROPERTIES FOLDER "Tests")
