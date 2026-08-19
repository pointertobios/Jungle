# Copyright (C) 2026 pointer-to-bios <pointer-to-bios@outlook.com>
# SPDX-License-Identifier: MIT

# 生成构建唯一标识符。
#
# 用法:
#   include(JungleBuildId)
#   jungle_generate_build_id(<out_var> [<git_source_dir>])
#
# 生成格式（时间戳为 UTC）:
#   无 git:                <时间戳>
#   有 git、工作树干净:     <commit>+<时间戳>
#   有 git、有修改:         <commit>+<工作树指纹>+<时间戳>
#   其中工作树指纹 = SHA256(未暂存+暂存差异 + 未跟踪文件列表) 的前 12 位。
function(jungle_generate_build_id out_var)
    set(_source_dir "${CMAKE_SOURCE_DIR}")
    if(ARGC GREATER 1)
        set(_source_dir "${ARGV1}")
    endif()

    find_package(Git QUIET)
    if(NOT Git_FOUND)
        find_program(GIT_EXECUTABLE git)
    endif()

    set(_git_commit "")
    set(_dirty_fingerprint "")

    if(GIT_EXECUTABLE)
        execute_process(
            COMMAND "${GIT_EXECUTABLE}" rev-parse --short HEAD
            WORKING_DIRECTORY "${_source_dir}"
            OUTPUT_VARIABLE _git_commit
            OUTPUT_STRIP_TRAILING_WHITESPACE
            RESULT_VARIABLE _git_result
            ERROR_QUIET
        )
        if(NOT _git_result EQUAL 0)
            set(_git_commit "")
        endif()
    endif()

    if(_git_commit)
        # 工作树指纹：未暂存 + 暂存的内容差异 + 未跟踪文件列表
        execute_process(
            COMMAND "${GIT_EXECUTABLE}" diff HEAD
            WORKING_DIRECTORY "${_source_dir}"
            OUTPUT_VARIABLE _git_diff
            RESULT_VARIABLE _diff_result
            ERROR_QUIET
        )
        execute_process(
            COMMAND "${GIT_EXECUTABLE}" status --porcelain=v1
            WORKING_DIRECTORY "${_source_dir}"
            OUTPUT_VARIABLE _git_status
            RESULT_VARIABLE _status_result
            ERROR_QUIET
        )
        if(_git_diff OR _git_status)
            string(SHA256 _dirty_fingerprint "${_git_diff}\n${_git_status}")
            string(SUBSTRING "${_dirty_fingerprint}" 0 12 _dirty_fingerprint)
        endif()
    endif()

    string(TIMESTAMP _build_timestamp "%Y%m%d%H%M%S" UTC)

    if(_git_commit)
        if(_dirty_fingerprint)
            set(_build_id "${_git_commit}+${_dirty_fingerprint}+${_build_timestamp}")
        else()
            set(_build_id "${_git_commit}+${_build_timestamp}")
        endif()
    else()
        set(_build_id "${_build_timestamp}")
    endif()

    set(${out_var} "${_build_id}" PARENT_SCOPE)
endfunction()
