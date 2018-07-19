# Copyright (C) 2018  Christian Berger
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

###########################################################################
# Find libopenh264.
FIND_PATH(OPENH264_INCLUDE_DIR
          NAMES wels/codec_api.h
          PATHS /usr/local/include/
                /usr/include/)
MARK_AS_ADVANCED(OPENH264_INCLUDE_DIR)
FIND_LIBRARY(OPENH264_LIBRARY
             NAMES openh264
             PATHS ${LIBOPENH264DIR}/lib/
                    /usr/lib/arm-linux-gnueabihf/
                    /usr/lib/arm-linux-gnueabi/
                    /usr/lib/x86_64-linux-gnu/
                    /usr/local/lib64/
                    /usr/lib64/
                    /usr/lib/)
MARK_AS_ADVANCED(OPENH264_LIBRARY)

###########################################################################
IF (OPENH264_INCLUDE_DIR
    AND OPENH264_LIBRARY)
    SET(OPENH264_FOUND 1)
    SET(OPENH264_LIBRARIES ${OPENH264_LIBRARY})
    SET(OPENH264_INCLUDE_DIRS ${OPENH264_INCLUDE_DIR})
ENDIF()

MARK_AS_ADVANCED(OPENH264_LIBRARIES)
MARK_AS_ADVANCED(OPENH264_INCLUDE_DIRS)

IF (OPENH264_FOUND)
    MESSAGE(STATUS "Found openh264: ${OPENH264_INCLUDE_DIRS}, ${OPENH264_LIBRARIES}")
ELSE ()
    MESSAGE(STATUS "Could not find openh264")
ENDIF()
