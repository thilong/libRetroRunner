cmake_minimum_required(VERSION 3.4.1)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -Wall")

add_definitions("-DVFS_FRONTEND -DHAVE_STRL -DVK_NO_PROTOTYPES=1 -DVK_USE_PLATFORM_ANDROID_KHR")

set(OBOE_DIR oboe)
add_subdirectory(${OBOE_DIR} oboe)
include_directories(${OBOE_DIR}/include)
include_directories(${OBOE_DIR}/src)

include_directories("libretro-common/include")
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

set(LIBRETRO_COMMON
        libretro-common/vfs/vfs_implementation.c
        libretro-common/string/stdstring.c
        libretro-common/encodings/encoding_utf.c
        libretro-common/file/file_path.c
        libretro-common/time/rtime.c
)


add_library(RetroRunner SHARED

        retro_runner/types/object_ref.hpp
        retro_runner/types/semaphore_rr.cpp
        retro_runner/types/app_command.hpp
        retro_runner/types/frontend_notify.hpp

        retro_runner/runtime_contexts/core_context.cpp
        retro_runner/runtime_contexts/game_context.cpp

        retro_runner/core/core.cpp

        retro_runner/app/setting.cpp
        retro_runner/app/paths.cpp
        retro_runner/app/app_context.cpp
        retro_runner/app/environment.cpp
        retro_runner/app/speed_limiter.hpp

        retro_runner/video/video_context.cpp
        retro_runner/video/opengles/video_context_gles.cpp
        retro_runner/video/opengles/texture.cpp
        retro_runner/video/opengles/shader_pass.cpp
        retro_runner/video/opengles/frame_buffer_object.cpp


        retro_runner/video/vulkan/rr_vulkan.cpp
        retro_runner/video/vulkan/vk_read_write_buffer.cpp
        retro_runner/video/vulkan/vk_sampling_texture.cpp
        retro_runner/video/vulkan/rr_vulkan_instance.cpp
        retro_runner/video/vulkan/rr_vulkan_pipeline.cpp
        retro_runner/video/vulkan/rr_vulkan_swapchain.cpp
        retro_runner/video/vulkan/rr_draws.cpp
        retro_runner/video/vulkan/vulkan_wrapper.cpp
        retro_runner/video/vulkan/video_context_vulkan.cpp


        retro_runner/input/input_context.cpp
        retro_runner/input/empty_input.cpp
        retro_runner/input/software_input.cpp

        retro_runner/audio/audio_context.cpp
        retro_runner/audio/empty_audio_context.cpp
        retro_runner/audio/resampler/linear_resampler.cpp
        retro_runner/audio/resampler/sinc_resampler.cpp
        retro_runner/audio/oboe/oboe_audio_context.cpp

        retro_runner/vfs/vfs_context.cpp

        retro_runner/cheats/cheat_manager.cpp
        retro_runner/cheats/retro_cht_file.cpp

        retro_runner/utils/jnistring.cpp
        retro_runner/utils/utils.cpp

        retro_runner/android_jni.cpp

        ${LIBRETRO_COMMON}

)

# add lib dependencies
target_link_libraries(RetroRunner
        android
        log
        EGL
        oboe
        GLESv2
        jnigraphics
)

