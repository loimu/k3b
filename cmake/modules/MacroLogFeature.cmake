# This file defines the Feature Logging macros.
#
# MACRO_LOG_FEATURE(VAR FEATURE DESCRIPTION URL [REQUIRED [MIN_VERSION [COMMENTS]]])
#   Logs the information so that it can be displayed at the end
#   of the configure run
#   VAR : TRUE or FALSE, indicating whether the feature is supported
#   FEATURE: name of the feature, e.g. "libjpeg"
#   DESCRIPTION: description what this feature provides
#   URL: home page
#   REQUIRED: TRUE or FALSE, indicating whether the featue is required
#   MIN_VERSION: minimum version number. empty string if unneeded
#   COMMENTS: More info you may want to provide.  empty string if unnecessary
#
# MACRO_DISPLAY_FEATURE_LOG()
#   Call this to display the collected results.
#   Exits CMake with a FATAL error message if a required feature is missing
#
# Example:
#
# INCLUDE(MacroLogFeature)
#
# FIND_PACKAGE(JPEG)
# MACRO_LOG_FEATURE(JPEG_FOUND "libjpeg" "Support JPEG images" "http://www.ijg.org" TRUE "3.2a" "")
# ...
# MACRO_DISPLAY_FEATURE_LOG()

# Copyright (c) 2006, Alexander Neundorf, <neundorf@kde.org>
# Copyright (c) 2006, Allen Winter, <winter@kde.org>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.

IF (NOT _macroLogFeatureAlreadyIncluded)
   SET(_file ${CMAKE_BINARY_DIR}/MissingRequirements.txt)
   IF (EXISTS ${_file})
      FILE(REMOVE ${_file})
   ENDIF (EXISTS ${_file})

   SET(_file ${CMAKE_BINARY_DIR}/EnabledFeatures.txt)
   IF (EXISTS ${_file})
      FILE(REMOVE ${_file})
   ENDIF (EXISTS ${_file})

   SET(_file ${CMAKE_BINARY_DIR}/DisabledFeatures.txt)
   IF (EXISTS ${_file})
      FILE(REMOVE ${_file})
  ENDIF (EXISTS ${_file})

  SET(_macroLogFeatureAlreadyIncluded TRUE)
ENDIF (NOT _macroLogFeatureAlreadyIncluded)


MACRO(MACRO_LOG_FEATURE _var _package _description _url ) # _required _minvers _comments)

   SET(_required "${ARGV4}")
   SET(_minvers "${ARGV5}")
   SET(_comments "${ARGV6}")

   IF (${_var})
     SET(_LOGFILENAME ${CMAKE_BINARY_DIR}/EnabledFeatures.txt)
   ELSE (${_var})
     IF (${_required} MATCHES "[Tt][Rr][Uu][Ee]")
       SET(_LOGFILENAME ${CMAKE_BINARY_DIR}/MissingRequirements.txt)
     ELSE (${_required} MATCHES "[Tt][Rr][Uu][Ee]")
       SET(_LOGFILENAME ${CMAKE_BINARY_DIR}/DisabledFeatures.txt)
     ENDIF (${_required} MATCHES "[Tt][Rr][Uu][Ee]")
   ENDIF (${_var})

   SET(_logtext "   * ${_package}")

   IF (NOT ${_var})
      IF (${_minvers} MATCHES ".*")
        SET(_logtext "${_logtext} (${_minvers} or higher)")
      ENDIF (${_minvers} MATCHES ".*")
      SET(_logtext "${_logtext}  <${_url}>")
   ENDIF (NOT ${_var})

   SET(_logtext "${_logtext}\n     ${_description}")

   IF (NOT ${_var})
      IF (${_comments} MATCHES ".*")
        SET(_logtext "${_logtext}\n     ${_comments}")
      ENDIF (${_comments} MATCHES ".*")
#      SET(_logtext "${_logtext}\n") #double-space missing features?
   ENDIF (NOT ${_var})

   FILE(APPEND "${_LOGFILENAME}" "${_logtext}\n")
 
ENDMACRO(MACRO_LOG_FEATURE)


MACRO(MACRO_DISPLAY_FEATURE_LOG)

   SET(_missingFile ${CMAKE_BINARY_DIR}/MissingRequirements.txt)
   SET(_enabledFile ${CMAKE_BINARY_DIR}/EnabledFeatures.txt)
   SET(_disabledFile ${CMAKE_BINARY_DIR}/DisabledFeatures.txt)

   IF (EXISTS ${_missingFile} OR EXISTS ${_enabledFile} OR EXISTS ${_disabledFile})
     SET(_printSummary TRUE)
   ENDIF (EXISTS ${_missingFile} OR EXISTS ${_enabledFile} OR EXISTS ${_disabledFile})

   IF(_printSummary)
     IF (EXISTS ${_missingFile})
       FILE(READ ${_missingFile} _requirements)
       SET(_summary "\n-----------------------------------------------------------------------------\n-- The following REQUIRED dependencies could NOT be found.\n-----------------------------------------------------------------------------\n${_requirements}")
       FILE(REMOVE ${_missingFile})
       SET(_haveMissingReq 1)
     ENDIF (EXISTS ${_missingFile})

     SET(_elist 0)
     IF (EXISTS ${_enabledFile})
       SET(_elist 1)
       FILE(READ ${_enabledFile} _enabled)
       FILE(REMOVE ${_enabledFile})
       SET(_summary "${_summary}\n-----------------------------------------------------------------------------\n-- The following dependencies were found.\n-----------------------------------------------------------------------------\n${_enabled}")
     ENDIF (EXISTS ${_enabledFile})

     SET(_dlist 0)
     IF (EXISTS ${_disabledFile})
       SET(_dlist 1)
       FILE(READ ${_disabledFile} _disabled)
       FILE(REMOVE ${_disabledFile})
       SET(_summary "${_summary}\n-----------------------------------------------------------------------------\n-- The following OPTIONAL dependencies could NOT be found.\n-----------------------------------------------------------------------------\n${_disabled}")
     ELSE (EXISTS ${_disabledFile})
       IF (${_elist})
         SET(_summary "${_summary}\n-----------------------------------------------------------------------------\n-- Congratulations! All external packages have been found.")
       ENDIF (${_elist})
     ENDIF (EXISTS ${_disabledFile})

     MESSAGE(${_summary})
     MESSAGE("-----------------------------------------------------------------------------\n")

     IF(_haveMissingReq)
       MESSAGE(FATAL_ERROR "Exiting: Missing Requirements")
     ENDIF(_haveMissingReq)
   ENDIF(_printSummary)

ENDMACRO(MACRO_DISPLAY_FEATURE_LOG)