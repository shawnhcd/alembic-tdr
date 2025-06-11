if(NOT ZSTD_DIR)
    message(FATAL_ERROR "ZSTD_DIR is not specified. Please set it to the path of the Zstandard installation.")
endif()

set(ZSTD_INCLUDE_DIR "${ZSTD_DIR}/include")
set(ZSTD_LIBRARY "${ZSTD_DIR}/lib64/libzstd.a")  # 或者是 .so/.dll 根据平台

# 检查文件是否存在
if(EXISTS "${ZSTD_INCLUDE_DIR}/zstd.h" AND EXISTS "${ZSTD_LIBRARY}")
    message(STATUS "Found Zstandard: ${ZSTD_LIBRARY}")
else()
    message(FATAL_ERROR "Zstandard not found at specified path.")
endif()
