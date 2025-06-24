RetroRunner library for android.

This project is mainly used to learn libretro and opengl es on android. 
It's a personal project.

# Thanks to following repo：
- LibretroDroid [BluetileEntertainment/LibretroDroid](https://github.com/BluetileEntertainment/LibretroDroid)
- RetroArch [libretro/RetroArch](https://github.com/libretro/RetroArch)


libPPSSPP:
    patch  libretro.cpp before test.

```
        void EmuThreadStart()
        {
            WARN_LOG(Log::Common, "EmuThread request start: %d", emuThreadState.load());


            bool wasPaused = emuThreadState == EmuThreadState::PAUSED;
            if (!wasPaused)
            {
                if(emuThreadState == EmuThreadState::START_REQUESTED){
                    WARN_LOG(Log::Common, "EmuThread is starting , ignored.");
                    return;
                }

                emuThreadState = EmuThreadState::START_REQUESTED;
                ctx->ThreadStart();
                emuThread = std::thread(&EmuThreadFunc);
                WARN_LOG(Log::Common, "EmuThread start");
            }else{
                emuThreadState = EmuThreadState::START_REQUESTED;
            }
        }
```

```
        void retro_reset(void)
        {
        PSP_Shutdown(true);

        if (BootState::Complete != PSP_Init(PSP_CoreParameter(), &g_bootErrorString))
        {
            ERROR_LOG(Log::Boot, "%s", g_bootErrorString.c_str());
            environ_cb(RETRO_ENVIRONMENT_SHUTDOWN, nullptr);
        }else{
            Core_UpdateState(CORE_RUNNING_CPU);
        }
        }
```