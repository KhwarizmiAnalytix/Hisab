# ============================================================================= Quarisma Spell
# Checking Configuration Module

# This module configures codespell for automated spell checking and correction. WARNING: When
# enabled, automatically modifies source files to fix spelling errors.
#
# Usage in each module's CMakeLists.txt:
#
# option(XXX_ENABLE_SPELL "Enable spell checking with automatic corrections for XXX (WARNING:
# modifies source files)" OFF) mark_as_advanced(XXX_ENABLE_SPELL) ... if(XXX_ENABLE_SPELL)
# include(spell) endif()

# Skip spell checking for third-party libraries
get_filename_component(_spell_dir_name "${CMAKE_CURRENT_SOURCE_DIR}" NAME)
if(_spell_dir_name STREQUAL "ThirdParty"
   OR CMAKE_CURRENT_SOURCE_DIR MATCHES ".*/ThirdParty/.*"
   OR CMAKE_CURRENT_SOURCE_DIR MATCHES ".*/third_party/.*"
   OR CMAKE_CURRENT_SOURCE_DIR MATCHES ".*/3rdparty/.*"
)
  unset(_spell_dir_name)
  return()
endif()

# Find codespell executable (cached — find_program is a no-op on repeat calls)
find_program(
  CODESPELL_EXECUTABLE
  NAMES codespell
  PATHS "$ENV{HOME}/.local/bin" "/usr/local/bin" "/usr/bin"
        "$ENV{USERPROFILE}/AppData/Local/Programs/Python/Python*/Scripts"
        "$ENV{PROGRAMFILES}/Python*/Scripts"
  DOC "Path to codespell executable"
)

if(NOT CODESPELL_EXECUTABLE)
  message(
    FATAL_ERROR
      "Codespell requested but not found!

Please install codespell:

  - pip install codespell
  - conda install -c conda-forge codespell
  - Ubuntu/Debian: sudo apt-get install codespell
  - macOS: brew install codespell
  - Windows: pip install codespell

Or set ${_spell_dir_name}_ENABLE_SPELL=OFF to disable spell checking"
  )
else()
  message(STATUS "Found codespell: ${CODESPELL_EXECUTABLE}")

  # Check if .codespellrc configuration file exists
  set(CODESPELL_CONFIG_FILE "${CMAKE_CURRENT_SOURCE_DIR}/.codespellrc")
  set(_spell_args)

  if(EXISTS ${CODESPELL_CONFIG_FILE})
    message(STATUS "Using codespell configuration: ${CODESPELL_CONFIG_FILE}")
    # codespell automatically reads .codespellrc from the current directory
  else()
    # Default configuration if no .codespellrc exists
    list(APPEND _spell_args "--skip=.git,.augment,.github,.vscode,build,Build,Cmake,ThirdParty"
         "--ignore-words-list=ThirdParty" "--check-hidden=no"
    )
  endif()

  # Add write-changes flag for automatic corrections
  list(APPEND _spell_args "--write-changes")

  # Target names are unique per module directory to avoid conflicts
  string(TOLOWER "${_spell_dir_name}" _spell_dir_lower)

  add_custom_target(
    spell_check_${_spell_dir_lower}
    COMMAND ${CODESPELL_EXECUTABLE} ${_spell_args} ${CMAKE_CURRENT_SOURCE_DIR}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Running spell check for ${_spell_dir_name} with automatic corrections..."
    VERBATIM
  )

  add_custom_target(
    spell_check_build_${_spell_dir_lower} ALL
    COMMAND ${CODESPELL_EXECUTABLE} ${_spell_args} ${CMAKE_CURRENT_SOURCE_DIR}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Running spell check for ${_spell_dir_name} during build..."
    VERBATIM
  )

  message(
    WARNING
      "${_spell_dir_name}_ENABLE_SPELL is ON: codespell will modify source files directly to fix "
      "spelling errors. Ensure you have committed your changes before building."
  )
endif()

unset(_spell_dir_name)
unset(_spell_dir_lower)
unset(_spell_args)
