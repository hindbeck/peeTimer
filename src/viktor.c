#include <pebble.h>
#include <string.h>
#include <stdlib.h>

#define ANIMALS_KEY 0
#define TIME_KEY 1

#define ALARM_TIME 95
#define SOFT_ALARM_BUFFER 5
#define PRESS_LIMIT 3
#define PRESS_TIME_LIMIT 20
#define WAKEUP_REASON 0
#define WAKEUP_INTERVAL_SECONDS 10

static Window *window;
static TextLayer *text_layer;
static TextLayer *animal_layer;
static GFont s_animal_font;
static char text[20];
static int s_uptime = 0;
static int lastResetTime = 0;
static int lastPressTime = 0;
static int pressCounter = 0;

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  // Use a long-lived buffer
  static char s_uptime_buffer[32];

/*  int angle = (minute * 360) / 90;

  GRect bounds = layer_get_bounds(animal_layer);
  GRect frame = grect_inset(bounds, GEdgeInsets(12));
  graphics_context_set_fill_color(ctx, GColorLightGray);
  graphics_fill_radial(ctx, frame, GOvalScaleModeFitCircle, 20, 0, DEG_TO_TRIGANGLE(minute_angle));
https://github.com/pebble-examples/time-dots/blob/master/src/windows/main_window.c
*/
  s_uptime = difftime(time(NULL),lastResetTime);
  // Get time since last select-click
  int seconds = s_uptime % 60;
  int minutes = (s_uptime % 3600) / 60;
  int hours = s_uptime / 3600;

  snprintf(s_uptime_buffer, sizeof(s_uptime_buffer), "%d:%02d:%02d", hours, minutes,seconds);
  text_layer_set_text(text_layer, s_uptime_buffer);
  // Increment s_uptime
  if ((s_uptime / 60) >= ALARM_TIME && (seconds % 10) == 0) {
    vibes_long_pulse();
  } else if ((s_uptime / 60) >= (ALARM_TIME - SOFT_ALARM_BUFFER) && (seconds % 30) == 0) {
    vibes_short_pulse();
  }
}

static void addAnimal() {
  char randomletter[2];
  randomletter[0] = 'A' + (rand() % 25);
  randomletter[1] = '\0';
  strncat(text, randomletter,1);
  text_layer_set_text(animal_layer, text);

  if (strlen(text) > 16) {
    text[0] = '\0';
  }
  /*time_t future_time = time(NULL) + WAKEUP_INTERVAL_SECONDS;
  struct tm *tmp = localtime(&future_time);
  //Only schedule wakeup if between 7 and 19
  if (tmp->tm_hour > 7 && tmp->tm_hour < 22) {
    // Schedule wakeup event and keep the WakeupId
    wakeup_cancel_all();
    WakeupId s_wakeup_id = wakeup_schedule(future_time, WAKEUP_REASON, true);
  }*/
}

/*static void wakeup_handler(WakeupId id, int32_t reason) {
  time_t future_time = time(NULL) + WAKEUP_INTERVAL_SECONDS;
  struct tm *tmp = localtime(&future_time);
  //Only schedule wakeup if between 7 and 19
  if (tmp->tm_hour > 7 && tmp->tm_hour < 22) {
    // Schedule wakeup event and keep the WakeupId
    WakeupId s_wakeup_id = wakeup_schedule(future_time, WAKEUP_REASON, true);
  }
}*/

static void my_click_handler(ClickRecognizerRef recognizer, void *context) {
  if (((int) time(NULL) - lastPressTime) < PRESS_TIME_LIMIT) {
    pressCounter++;
  } else {
    lastPressTime = (int) time(NULL);
    pressCounter = 1;
  }

  if (pressCounter >= PRESS_LIMIT) {
    addAnimal();
    lastResetTime = (int) time(NULL);
  }
}

static void click_config_provider(void *context) {
  //window_single_click_subscribe(BUTTON_ID_SELECT, my_click_handler);
  //window_single_click_subscribe(BUTTON_ID_UP, my_click_handler);
  window_single_click_subscribe(BUTTON_ID_DOWN, my_click_handler);
}

static void window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  text_layer = text_layer_create(GRect(0, 0, bounds.size.w, 36));
  text_layer_set_text_alignment(text_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(text_layer));

  animal_layer = text_layer_create(GRect(0, 20, bounds.size.w, bounds.size.h-36));
  text_layer_set_text_alignment(animal_layer, GTextAlignmentLeft);

  layer_add_child(window_layer, text_layer_get_layer(animal_layer));
  s_animal_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_ANIMAL_FONT_32));
  text_layer_set_font(animal_layer, s_animal_font);
}

static void window_unload(Window *window) {
  persist_write_string(ANIMALS_KEY, text);
  persist_write_int(TIME_KEY, lastResetTime);
  fonts_unload_custom_font(s_animal_font);
  text_layer_destroy(text_layer);
  text_layer_destroy(animal_layer);
}

static void init(void) {
  srand(time(NULL));
  window = window_create();
  window_set_click_config_provider(window, click_config_provider);
  window_set_window_handlers(window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload,
  });
  const bool animated = false;
  text[0] = '\0';
  window_stack_push(window, animated);
  tick_timer_service_subscribe(SECOND_UNIT, tick_handler);
  persist_read_string(ANIMALS_KEY, text, sizeof(text));
  lastResetTime = persist_read_int(TIME_KEY);
  if (lastResetTime < 10)
    lastResetTime = (int) time(NULL);
  text_layer_set_text(animal_layer, text);

  // Subscribe to Wakeup API
  //wakeup_service_subscribe(wakeup_handler);
}

static void deinit(void) {
  window_destroy(window);
}

int main(void) {
  init();

  app_event_loop();
  deinit();
}
