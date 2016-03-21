#include <pebble.h>

static Window *s_main_window;
static Layer *s_canvas_layer;
static TextLayer *s_time_layer;
static TextLayer *s_cal_layer;
char s_cal_text[64];
static int kcalories_cached;

static const int KCALORIES_GOAL = 3000;
static const int KCALORIES_GOOD = 2500;

static int get_kcals() {
  int kcalories = 0;
  time_t start = time_start_of_today();
  time_t end = time(NULL);
  
  HealthMetric metric_active = HealthMetricActiveKCalories;
  HealthServiceAccessibilityMask mask_active = 
    health_service_metric_accessible(metric_active, start, end);
  bool any_data_available = mask_active & HealthServiceAccessibilityMaskAvailable;
  if (any_data_available) {
    kcalories += (int)health_service_sum_today(metric_active);
  }
  
  HealthMetric metric_resting = HealthMetricRestingKCalories;
  HealthServiceAccessibilityMask mask_resting = 
    health_service_metric_accessible(metric_resting, start, end);
  any_data_available = mask_resting & HealthServiceAccessibilityMaskAvailable;
  if (any_data_available) {
    kcalories += (int)health_service_sum_today(metric_resting);
  }
  
  APP_LOG(APP_LOG_LEVEL_INFO, "KCalories right now: %d", (int)kcalories);
  return kcalories;
}

static int calculate_kcal_deg() {
  return -((int)(((float)kcalories_cached/(float)KCALORIES_GOAL)*360));
}

static void canvas_update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  
  #ifdef PBL_ROUND
  GRect frame = grect_inset(bounds, GEdgeInsets(0));
  const int THICKNESS = 12;
  #else
  GRect frame = grect_inset(bounds, GEdgeInsets(4));
  const int THICKNESS = 8;
  #endif

  #ifdef PBL_COLOR
  graphics_context_set_fill_color(ctx, GColorYellow);
  #else
  graphics_context_set_fill_color(ctx, GColorLightGray);
  #endif
  
  
  graphics_fill_radial(ctx, frame, GOvalScaleModeFitCircle, THICKNESS,
                                  DEG_TO_TRIGANGLE(calculate_kcal_deg()), DEG_TO_TRIGANGLE(0));
  
}

static void display_calories() {
  // Cache the calories
  kcalories_cached = get_kcals();
  
  if (kcalories_cached >= KCALORIES_GOOD) {
    snprintf(s_cal_text, sizeof(s_cal_text), "%d cals \U0001F604", kcalories_cached);
  } else {
    snprintf(s_cal_text, sizeof(s_cal_text), "%d cals \U0001F61E", kcalories_cached);
  }
  text_layer_set_text(s_cal_layer, s_cal_text);
}

static void update_time() {
  // Get a tm structure
  time_t temp = time(NULL);
  struct tm *tick_time = localtime(&temp);

  // Write the current hours and minutes into a buffer
  static char s_buffer[8];
  strftime(s_buffer, sizeof(s_buffer), clock_is_24h_style() ?
                                          "%H:%M" : "%I:%M", tick_time);

  // Display this time on the TextLayer
  text_layer_set_text(s_time_layer, s_buffer);
  
  display_calories();
  
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  update_time();
}

static void main_window_load(Window *window) {
    // Get information about the Window
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
  
  // Set up canvas for drawing the arc
  s_canvas_layer = layer_create(bounds);
  
  // Assign the custom drawing procedure
  layer_set_update_proc(s_canvas_layer, canvas_update_proc);
  
  // Get the current calories
  kcalories_cached = get_kcals();

  // Create the TextLayer with specific bounds
  s_time_layer = text_layer_create(
      GRect(0, PBL_IF_ROUND_ELSE(58, 52) + 2, bounds.size.w, 50));

  text_layer_set_background_color(s_time_layer, GColorClear);
  text_layer_set_text_color(s_time_layer, GColorGreen);
  text_layer_set_text(s_time_layer, "00:00");
  text_layer_set_font(s_time_layer, fonts_get_system_font(FONT_KEY_LECO_36_BOLD_NUMBERS));
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);

  s_cal_layer = text_layer_create(
      GRect(0, PBL_IF_ROUND_ELSE(58, 52) + 40, bounds.size.w, 50));
  
  text_layer_set_background_color(s_cal_layer, GColorClear);
  #ifdef PBL_COLOR
  text_layer_set_text_color(s_cal_layer, GColorYellow);
  #else
  text_layer_set_text_color(s_cal_layer, GColorWhite);
  #endif
  text_layer_set_font(s_cal_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
  text_layer_set_text_alignment(s_cal_layer, GTextAlignmentCenter);
  display_calories();
  
  layer_add_child(window_layer, text_layer_get_layer(s_time_layer));
  layer_add_child(window_layer, text_layer_get_layer(s_cal_layer));
  layer_add_child(window_layer, s_canvas_layer);
  update_time();
  
}

static void main_window_unload(Window *window) {
  // Destroy TextLayer
  text_layer_destroy(s_time_layer);
}

static void init() {
  kcalories_cached = 0;
  // Create main Window element and assign to pointer
  s_main_window = window_create();
  window_set_background_color(s_main_window, GColorBlack);

  // Set handlers to manage the elements inside the Window
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload
  });

  // Show the Window on the watch, with animated=true
  window_stack_push(s_main_window, true);
  
  // Register with TickTimerService
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
  
}

static void deinit() {
  // Destroy Window
  window_destroy(s_main_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}