#include "pebble.h"

static const uint32_t WEATHER_ICONS[] = {
  RESOURCE_ID_CLEAR_DAY,
  RESOURCE_ID_CLEAR_NIGHT,
  RESOURCE_ID_WINDY,
  RESOURCE_ID_COLD,
  RESOURCE_ID_PARTLY_CLOUDY_DAY,
  RESOURCE_ID_PARTLY_CLOUDY_NIGHT,
  RESOURCE_ID_HAZE,
  RESOURCE_ID_CLOUD,
  RESOURCE_ID_RAIN,
  RESOURCE_ID_SNOW,
  RESOURCE_ID_HAIL,
  RESOURCE_ID_CLOUDY,
  RESOURCE_ID_STORM,
  RESOURCE_ID_NA,
};

enum WeatherKey {
  WEATHER_ICON_KEY = 0x0,         // TUPLE_INT
  WEATHER_TEMPERATURE_KEY = 0x1,  // TUPLE_CSTRING
  INVERT_COLOR_KEY = 0x2,  // TUPLE_CSTRING
};

GColor background_color;
GColor foreground_color;

Window *window;

BitmapLayer *icon_layer;
GBitmap *icon_bitmap = NULL;
TextLayer *temp_layer;

TextLayer *text_day_layer;
GRect day_layer_hidden_rect = {{8, 50}, {144-8, 25}};
GRect day_layer_visible_rect = {{8, 0}, {144-8, 25}};

TextLayer *text_date_layer;
GRect date_layer_hidden_rect = {{8, 71}, {144-8, 25}};
GRect date_layer_visible_rect = {{8, 21}, {144-8, 25}};

Layer *line_layer;

TextLayer *text_time_layer;
GRect time_layer_hidden_rect = {{7, -56}, {144-7, 49}};
GRect time_layer_visible_rect = {{7, -7}, {144-7, 49}};

TextLayer *battery_text_layer;

static AppSync sync;
static uint8_t sync_buffer[64];

void set_invert_color(bool invert) {
  if (invert) {
    background_color = GColorWhite;
    foreground_color = GColorBlack;
  } else {
    background_color = GColorBlack;
    foreground_color = GColorWhite;
  }

  window_set_background_color(window, background_color);
  text_layer_set_text_color(temp_layer, foreground_color);
  text_layer_set_text_color(text_day_layer, foreground_color);
  text_layer_set_text_color(text_date_layer, foreground_color);
  text_layer_set_text_color(text_time_layer, foreground_color);
  text_layer_set_text_color(battery_text_layer, foreground_color);

  GBitmapFormat format = gbitmap_get_format(icon_bitmap);
  if (format ==  GBitmapFormat1BitPalette) {
    GColor *palette = gbitmap_get_palette(icon_bitmap);
    palette[0] = background_color;
    palette[1] = foreground_color;
  }

  Layer *window_layer = window_get_root_layer(window);
  layer_mark_dirty(window_layer);
}

static void sync_tuple_changed_callback(const uint32_t key,
                                        const Tuple* new_tuple,
                                        const Tuple* old_tuple,
                                        void* context) {
  bool invert;

  // App Sync keeps new_tuple in sync_buffer, so we may use it directly
  switch (key) {
    case WEATHER_ICON_KEY:
      if (icon_bitmap) {
        gbitmap_destroy(icon_bitmap);
      }

      icon_bitmap = gbitmap_create_with_resource(
          WEATHER_ICONS[new_tuple->value->uint8]);
      bitmap_layer_set_bitmap(icon_layer, icon_bitmap);
      break;

    case WEATHER_TEMPERATURE_KEY:
      text_layer_set_text(temp_layer, new_tuple->value->cstring);
      break;

    case INVERT_COLOR_KEY:
      invert = new_tuple->value->uint8 != 0;
      persist_write_bool(INVERT_COLOR_KEY, invert);
      set_invert_color(invert);
      break;
  }
}

// Redraw line between date and time
void line_layer_update_callback(Layer *layer, GContext* ctx) {
  graphics_context_set_fill_color(ctx, foreground_color);
  graphics_fill_rect(ctx, layer_get_bounds(layer), 0, GCornerNone);
}

void bluetooth_connection_changed(bool connected) {
  static bool _connected = true;

  // This seemed to get called twice on disconnect
  if (!connected && _connected) {
    vibes_short_pulse();

    if (icon_bitmap) {
      gbitmap_destroy(icon_bitmap);
    }

    icon_bitmap = gbitmap_create_with_resource(RESOURCE_ID_NO_BT);
    bitmap_layer_set_bitmap(icon_layer, icon_bitmap);
  }
  _connected = connected;
}

void handle_minute_tick(struct tm *tick_time, TimeUnits units_changed) {
  // Need to be static because they're used by the system later.
  static char day_text[] = "xxxxxxxxx";
  static char date_text[] = "Xxxxxxxxx 00";
  static char time_text[] = "00:00";
  static int yday = -1;

  char *time_format;

  // Only update the date when it has changed.
  if (yday != tick_time->tm_yday) {
    strftime(day_text, sizeof(day_text), "%A", tick_time);
    text_layer_set_text(text_day_layer, day_text);

    strftime(date_text, sizeof(date_text), "%B %e", tick_time);
    text_layer_set_text(text_date_layer, date_text);
  }

  if (clock_is_24h_style()) {
    time_format = "%R";
  } else {
    time_format = "%I:%M";
  }

  strftime(time_text, sizeof(time_text), time_format, tick_time);

  // Handle lack of non-padded hour format string for twelve hour clock.
  if (!clock_is_24h_style() && (time_text[0] == '0')) {
    text_layer_set_text(text_time_layer, time_text + 1);
  } else {
    text_layer_set_text(text_time_layer, time_text);
  }

  // On the first update of the date and time animate the text fields into view
  static bool first_update = true;
  if (first_update) {
    first_update = false;

    PropertyAnimation *animation = property_animation_create_layer_frame(
        text_layer_get_layer(text_day_layer),
        &day_layer_hidden_rect, &day_layer_visible_rect);
    animation_schedule(property_animation_get_animation(animation));

    animation = property_animation_create_layer_frame(
        text_layer_get_layer(text_date_layer),
        &date_layer_hidden_rect, &date_layer_visible_rect);
    animation_schedule(property_animation_get_animation(animation));

    animation = property_animation_create_layer_frame(
        text_layer_get_layer(text_time_layer),
        &time_layer_hidden_rect, &time_layer_visible_rect);
    animation_schedule(property_animation_get_animation(animation));
  }
}

void update_battery_state(BatteryChargeState battery_state) {
  static char battery_text[] = "100%";
  snprintf(battery_text, sizeof(battery_text), "%d%%",
      battery_state.charge_percent);
  text_layer_set_text(battery_text_layer, battery_text);
}

void handle_init(void) {
  bool invert_color = persist_read_bool(INVERT_COLOR_KEY);

  if (invert_color) {
    background_color = GColorWhite;
    foreground_color = GColorBlack;
  } else {
    background_color = GColorBlack;
    foreground_color = GColorWhite;
  }

  window = window_create();
  window_stack_push(window, true /* Animated */);
  window_set_background_color(window, background_color);

  Layer *window_layer = window_get_root_layer(window);

  // Setup weather bar
  Layer *weather_holder = layer_create(GRect(0, 0, 144, 50));
  layer_add_child(window_layer, weather_holder);

  icon_layer = bitmap_layer_create(GRect(0, 0, 40, 40));
  layer_add_child(weather_holder, bitmap_layer_get_layer(icon_layer));

  temp_layer = text_layer_create(GRect(40, 3, 144 - 40, 28));
  text_layer_set_text_color(temp_layer, foreground_color);
  text_layer_set_background_color(temp_layer, GColorClear);
  text_layer_set_font(temp_layer,
      fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
  text_layer_set_text_alignment(temp_layer, GTextAlignmentRight);
  layer_add_child(weather_holder, text_layer_get_layer(temp_layer));

  // Initialize date & time text
  Layer *main_holder = layer_create(GRect(0, 52, 144, 94));
  layer_add_child(window_layer, main_holder);

  Layer *date_holder = layer_create(GRect(0, 0, 144, 50));
  layer_add_child(main_holder, date_holder);

  ResHandle roboto_21 = resource_get_handle(RESOURCE_ID_FONT_ROBOTO_CONDENSED_21);
  text_day_layer = text_layer_create(day_layer_hidden_rect);
  text_layer_set_text_color(text_day_layer, foreground_color);
  text_layer_set_background_color(text_day_layer, GColorClear);
  text_layer_set_font(text_day_layer, fonts_load_custom_font(roboto_21));
  layer_add_child(date_holder, text_layer_get_layer(text_day_layer));

  text_date_layer = text_layer_create(date_layer_hidden_rect);
  text_layer_set_text_color(text_date_layer, foreground_color);
  text_layer_set_background_color(text_date_layer, GColorClear);
  text_layer_set_font(text_date_layer, fonts_load_custom_font(roboto_21));
  layer_add_child(date_holder, text_layer_get_layer(text_date_layer));

  line_layer = layer_create(GRect(8, 51, 144-16, 2));
  layer_set_update_proc(line_layer, line_layer_update_callback);
  layer_add_child(main_holder, line_layer);

  Layer *time_holder = layer_create(GRect(0, 52, 144, 49));
  layer_add_child(main_holder, time_holder);

  ResHandle roboto_49 = resource_get_handle(RESOURCE_ID_FONT_ROBOTO_BOLD_SUBSET_49);
  text_time_layer = text_layer_create(time_layer_hidden_rect);
  text_layer_set_text_color(text_time_layer, foreground_color);
  text_layer_set_background_color(text_time_layer, GColorClear);
  text_layer_set_font(text_time_layer, fonts_load_custom_font(roboto_49));
  layer_add_child(time_holder, text_layer_get_layer(text_time_layer));

  // Setup messaging
  const int inbound_size = 64;
  const int outbound_size = 64;
  app_message_open(inbound_size, outbound_size);

  Tuplet initial_values[] = {
    TupletInteger(WEATHER_ICON_KEY, (uint8_t) 13),
    TupletCString(WEATHER_TEMPERATURE_KEY, ""),
    TupletInteger(INVERT_COLOR_KEY, invert_color),
  };

  app_sync_init(&sync, sync_buffer, sizeof(sync_buffer), initial_values,
                ARRAY_LENGTH(initial_values), sync_tuple_changed_callback,
                NULL, NULL);

  battery_text_layer = text_layer_create(GRect(0, 168 - 18, 144, 168));
  text_layer_set_text_color(battery_text_layer, foreground_color);
  text_layer_set_background_color(battery_text_layer, GColorClear);
  text_layer_set_font(battery_text_layer,
                      fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
  text_layer_set_text_alignment(battery_text_layer, GTextAlignmentRight);
  layer_add_child(window_layer, text_layer_get_layer(battery_text_layer));

  // Subscribe to notifications
  bluetooth_connection_service_subscribe(bluetooth_connection_changed);
  tick_timer_service_subscribe(MINUTE_UNIT, handle_minute_tick);
  battery_state_service_subscribe(update_battery_state);

  // Update the battery on launch
  update_battery_state(battery_state_service_peek());

  // TODO: Update display here to avoid blank display on launch?
}

void handle_deinit(void) {
  bluetooth_connection_service_unsubscribe();
  tick_timer_service_unsubscribe();
  battery_state_service_unsubscribe();
}

int main(void) {
  handle_init();

  app_event_loop();

  handle_deinit();
}
