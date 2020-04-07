#pragma once

#include <SDL.h>
#include <omp.h>
#include <stdio.h>

#define TIME_ENABLE
#ifdef TIME_ENABLE
#define TIME_BEGIN before = SDL_GetTicks()
#define TIME_END_ delta = SDL_GetTicks() - before
#define TIME_END(s) TIME_END_; std::printf("[map] (tid=%d) " ## s ## " took %u ms\n", omp_get_thread_num(), delta)
#else
#define TIME_BEGIN {}
#define TIME_END {}
#endif