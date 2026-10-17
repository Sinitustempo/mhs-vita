/* Monster Hunter Stories: extra / overriding imports (see dynlib_extra.h). */
{ "__google_potentially_blocking_region_begin", (uintptr_t)&mhs_noop },
{ "__google_potentially_blocking_region_end",   (uintptr_t)&mhs_noop },
{ "__android_log_assert",                       (uintptr_t)&mhs_android_log_assert },
{ "SL_IID_EFFECTSEND",                          (uintptr_t)&mhs_SL_IID_EFFECTSEND },
{ "SL_IID_ENVIRONMENTALREVERB",                 (uintptr_t)&mhs_SL_IID_ENVIRONMENTALREVERB },
