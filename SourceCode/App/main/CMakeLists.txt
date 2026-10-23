# SPDX-FileCopyrightText: 2026 CMY Labs PTY LTD
# SPDX-License-Identifier: LicenseRef-NCL-1.0 AND Apache-2.0
# 
# CMY Labs PTY LTD's original copyrightable additions are NCL-1.0 ONLY.
# Retained upstream expression remains under Apache-2.0.
# The licenses apply to their respective contributions, not as alternatives.
# Retained upstream license identifiers below describe upstream material.
# Modified in this project; upstream notices are preserved.
# Upstream copyright: 2015-2022 Espressif Systems; Apache-2.0.
# Terms: repository-root LICENSE. Contribution scopes: licensing/MANIFEST.json.

idf_component_register(SRCS "miniz.c" "station.c" "cdc_acm_vcp.cpp" "esp_rest_main.c" 
                            "rest_server.c" "decode_image.c" "cartmain.c"
                           
                            "sd_card_main.c"
                    INCLUDE_DIRS ".")

if(CONFIG_WEB_DEPLOY_SF)
    set(WEB_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../front/phantom-app")
    if(EXISTS ${WEB_SRC_DIR}/dist)
        spiffs_create_partition_image(www ${WEB_SRC_DIR}/dist FLASH_IN_PROJECT)
    else()
        message(FATAL_ERROR "${WEB_SRC_DIR}/dist doesn't exit. Please run 'npm run build' in ${WEB_SRC_DIR}")
    endif()
endif()

set_property(DIRECTORY "${COMPONENT_DIR}" APPEND PROPERTY
     ADDITIONAL_MAKE_CLEAN_FILES)


