//
// init tk context and handle events and other
// wrapper callback system of sdl3 callback system
//
// HACK:
// 1. while, use log in internal callback functions is strange...
//
// FIX:
// 1. should not use try catch in internal callback system!
//

#include "tk/tk.hpp"
#include "tk/Window.hpp"
#include "tk/GraphicsEngine/GraphicsEngine.hpp"
#include "tk/log.hpp"
#include "tk/ui/ui.hpp"
#include "ui/internal.hpp"

#define SDL_MAIN_USE_CALLBACKS
#include <SDL3/SDL_main.h>
#include <SDL3/SDL_timer.h>

#include <algorithm>
#include <numbers>

using namespace tk;
using namespace tk::graphics_engine;

////////////////////////////////////////////////////////////////////////////////
//                          tk Callback System
////////////////////////////////////////////////////////////////////////////////

extern void tk_init(int argc, char** argv);
extern void tk_iterate();
extern void tk_event(SDL_Event* event);
extern void tk_quit();

////////////////////////////////////////////////////////////////////////////////
//                           tk context
////////////////////////////////////////////////////////////////////////////////

struct tk_context
{
  void*          user_data = nullptr;

  Window         window;
  GraphicsEngine engine;

  bool           paused    = false;

  ~tk_context()
  {
    engine.destroy();
    window.destroy();
  }
};

static tk_context* tk_ctx;

void tk::init_tk_context(std::string_view title, uint32_t width, uint32_t height, void* user_data)
{
  tk_ctx = new tk_context();

  tk_ctx->user_data = user_data;
  tk_ctx->window.init(title, width, height);
  tk_ctx->engine.init(tk_ctx->window);
  ui::init(&tk_ctx->engine);
}

auto tk::get_user_data() -> void*
{
  return tk_ctx->user_data;
}

auto tk::get_main_window() -> Window*
{
  return &tk_ctx->window;
}


////////////////////////////////////////////////////////////////////////////////
//                                From imgui
////////////////////////////////////////////////////////////////////////////////

static double global_time = {};

// record framestate
static float framestates[60] = {};
static int   frame_index     = {};
static float frame_accum     = {};
static int   frame_count     = {};
static float framestate      = {};

static float dpi_scale = {};

static float   curve_tessellation_tolerance  = 1.25f;
static float   circle_tessellation_max_error = 0.30f;
static uint8_t circle_segment_counts[64]; // Precomputed segment count for given radius before we calculate it dynamically (to avoid calculation overhead)
constexpr uint32_t Circle_Max_Sample_Number = 48; // imgui think 48 is enough 'circle'
static float   arc_fast_radius_cutoff = {};

void update_dpi_scale()
{
  int count;
  auto displays = SDL_GetDisplays(&count);
  for (auto display : std::span(displays, count))
  {
    dpi_scale = SDL_GetDisplayContentScale(display);
    if (dpi_scale <= 0.0f)
      continue;
    else
      // FIXME: only single monitor for me
      break;
  }
  SDL_free(displays);
}

// Helper function to calculate a circle's segment count given its radius and a "maximum error" value.
// Estimation of number of circle segment based on error is derived using method described in https://stackoverflow.com/a/2244088/15194693
// Number of segments (N) is calculated using equation:
//   N = ceil ( pi / acos(1 - error / r) )     where r > 0, error <= r
// Our equation is significantly simpler that one in the post thanks for choosing segment that is
// perpendicular to X axis. Follow steps in the article from this starting condition and you will
// will get this result.
//
// Rendering circles with an odd number of segments, while mathematically correct will produce
// asymmetrical results on the raster grid. Therefore we're rounding N to next even number (7->8, 8->8, 9->10 etc.)
inline constexpr auto roundup_to_even(uint32_t num)
{
  return (num + 1) / 2 * 2;
}

inline constexpr auto get_circle_segment_count(float radius, float max_error)
{
  static constexpr uint32_t circle_max_segment_num = 512;
  static constexpr uint32_t circle_min_segment_num = 4;
  if (radius <= 0.f)
    return Circle_Max_Sample_Number;
  return std::clamp(roundup_to_even(std::ceil(std::numbers::pi / std::acos(1 - std::min(max_error, radius) / radius))), circle_min_segment_num, circle_max_segment_num);
}

inline constexpr auto get_circle_segment_count_radius(float num, float max_error)
{
  return max_error / (1 - std::cos(std::numbers::pi / std::max(num, std::numbers::pi_v<float>)));
}

void init_circle_tesslation_counts()
{
  for (auto i = 0; i < sizeof(circle_segment_counts); ++i)
    circle_segment_counts[i] = get_circle_segment_count(i, circle_tessellation_max_error);
  // FIXME: why arc_fast_radius_cutoff not equal sizeof(circle_segment_counts)
  arc_fast_radius_cutoff = get_circle_segment_count_radius(Circle_Max_Sample_Number, circle_tessellation_max_error);
}

////////////////////////////////////////////////////////////////////////////////
//                          SDL3 Callback System
////////////////////////////////////////////////////////////////////////////////

SDL_AppResult SDL_AppInit(void**, int argc, char** argv)
{
  try
  {
    init_circle_tesslation_counts();
    tk_init(argc, argv);
  }
  catch (const std::exception& e)
  {
    log::error(e.what());
    return SDL_APP_FAILURE;
  }
  return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void *appstate)
{
  try
  {
    //
    // record time
    //
    static uint64_t time = {};
    // Setup time step (we don't use SDL_GetTicks() because it is using millisecond resolution)
    // (Accept SDL_GetPerformanceCounter() not returning a monotonically increasing value. Happens in VMs and Emscripten, see #6189, #6114, #3644)
    static auto frequency = SDL_GetPerformanceFrequency();
    auto current_time = SDL_GetPerformanceCounter();
    if (current_time <= time)
        current_time = time + 1;
    static auto delta_time = time > 0 ? (float)((double)(current_time - time) / frequency) : (float)(1.0f / 60.0f);
    time = current_time;

    log::info("time: {}", time);

    //
    // record framestate
    //
    frame_accum += delta_time - framestates[frame_index];
    framestates[frame_index] = delta_time;
    frame_index = ++frame_index % sizeof(framestates);
    frame_count = std::min((uint64_t)frame_count + 1, sizeof(framestates));
    framestate = frame_accum > 0.f ? 1.f / (frame_accum / frame_count) : FLT_MAX;

    log::info("framestate: {}", framestate);

    //
    // update dpi
    //
    update_dpi_scale();

    tk_iterate();

    if (!tk_ctx->paused)
    {
      ui::render();
      global_time += delta_time;
    }
    else
      SDL_Delay(10);
  }
  catch (const std::exception& e)
  {
    log::error(e.what());
    return SDL_APP_FAILURE;
  }
  return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event)
{
  try
  {
    tk_event(event);

    switch (event->type)
    {
    case SDL_EVENT_QUIT:
      return SDL_APP_SUCCESS;
    case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
      if (event->window.windowID == SDL_GetWindowID(tk_ctx->window.get()))
        return SDL_APP_SUCCESS;
    case SDL_EVENT_WINDOW_MINIMIZED:
      tk_ctx->paused = true;
      break;
    case SDL_EVENT_WINDOW_MAXIMIZED:
      tk_ctx->paused = false;
      break;
    case SDL_EVENT_WINDOW_RESIZED:
      tk_ctx->engine.resize_swapchain();
      break;
    case SDL_EVENT_KEY_DOWN:
      if (event->key.key == SDLK_Q)
        return SDL_APP_SUCCESS;
      break;
    }

    ui::event_process(event);
  }
  catch (const std::exception& e)
  {
    log::error(e.what());
    return SDL_APP_FAILURE;
  }
  return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void *appstate, SDL_AppResult result)
{
  try
  {
    tk_quit();

    delete tk_ctx;
  }
  catch (const std::exception& e)
  {
    log::error(e.what());
    exit(EXIT_FAILURE);
  }

  if (result == SDL_APP_SUCCESS)
    exit(EXIT_SUCCESS);
  if (result == SDL_APP_FAILURE)
    exit(EXIT_FAILURE);
}
