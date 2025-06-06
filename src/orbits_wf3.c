#include <pebble.h>
#define HELLO
#define HELLO2

// define this line if building for sdk 4.x onward:
// #define timelineview 1

/*
 * >>> Need to send pod change info to the HTML settings page
 * >>> Need to change to a unique settings page for this app
 * >>> Need to implement "change my pod" on the settings page
 * 	including a 5 minute alarm, followed by updating the pod change time
 *
 * raj - 3/2014 - added bluetooth symbol
 * raj - 2/2014 - upgraded to SDK 2.0
 * raj - 8/2014
 * raj - 10/2014 - Changed to a background reminder app with no watchface
 * 
 * This watch face was created by starting with the "Gatekeeper" watch face by Tycho.
 * It has been somewhat modified and I tried to remove the layers and variables, etc.,
 * which aren't being used now.
 */

#include <pebble.h>

/* My version numbers */
#define MAJOR	0x1
#define MINOR	0x0

#define FONT_SIZE	30
    
/* Screen size info */
#if defined(PBL_RECT)
#define SCREEN_WIDTH 144
#define SCREEN_HEIGHT 168
#elif defined(PBL_ROUND)
#define SCREEN_WIDTH 180
#define SCREEN_HEIGHT 180
#endif

/*
 * These are not only used to index into the "my_date_layer" array
 * but they also have to match the name to number specifications in
 * the appinfo.json file, so that they can used as keys for
 * datebase lookups.
 */
#define DATE_MONTH	0
#define DATE_DAY	1
#define DATE_HOUR	2
#define DATE_MINUTE	3
#define BLUETOOTH	4
#define PODCHANGE	5
#define PCHOUR		6
#define CHANGEIT	7
#define TIMER		8
#define TIMER_START	9
#define ACK		10

#define ASTER_DISTANCE  72
#define COMET_DISTANCE	70
#define EARTH_DISTANCE	40

#if defined(PBL_RECT)
# define COMET_CENTER GPoint(72,72)
#else
# define COMET_CENTER GPoint(90,90)
#endif

#if defined(PBL_RECT)
#define BATTERY_HEIGHT	2		/* height of battery indicator */
#endif

static GFont date_font;
static GFont note_font;
static Window *my_window;
static Layer *my_window_layer;
static Window *timer_window;

static GBitmap *my_background_image_100;
static GBitmap *my_background_image_60;
static GBitmap *my_background_image_30;
static BitmapLayer *my_background_layer;
static GBitmap *my_hour_hand_image;
static RotBitmapLayer *my_hour_hand_layer;
static GBitmap *my_bluetooth_image;
static BitmapLayer *my_bt_layer;
static GBitmap *my_comet_image;
static RotBitmapLayer *my_comet_layer;
static GBitmap *my_small_comet_image;
static RotBitmapLayer *my_small_comet_layer;
static GBitmap *my_aster_image;
static RotBitmapLayer *my_aster_layer;
static TextLayer *timer_layer=NULL;
static GBitmap *my_arrow_image;
static GBitmap *my_double_arrow_image;
static RotBitmapLayer *my_arrow_layer;
static RotBitmapLayer *my_double_arrow_layer;
static TextLayer *bg_layer=NULL;

static TextLayer *my_date_layer[4];
bool display_dates[4];
static Layer *my_bat_layer;

static bool bluetooth_connected;
static bool pod_change_written;

time_t tap_display_time;
bool tap_display;

int	timer_interval;
time_t	timer_start;

status_t stat;

time_t pod_change;			/* pod change time value */
int changeit;                           /* change pod now */

static bool weekday_done=false;

#define SECONDS (1)
#define MINUTES (60 * SECONDS)
#define HOURS (60 * MINUTES)
#define DAYS (24 * HOURS)

static const uint32_t const alert_segments[]={
    500, 100, 200, 100, 500, 100, 200, 100, 500
};
VibePattern alert_pulse = {
    .durations = alert_segments,
    .num_segments = ARRAY_LENGTH(alert_segments),
};

/* Messages */
time_t now;

#ifdef timelineview
/* Place to save smaller window GRect info */
GRect small_window;

bool repos=false;
#endif

// Keep track of when the JS app on the phone is there
bool js_is_up = false;

/*
 * Return whether or not the hand has been repositioned
 * due to a notice.
 */
void
set_hand_angle(unsigned int hour_angle, int min_angle,
               RotBitmapLayer *layer, int length) 
{
  GRect rect;
  int32_t hour_x, hour_y;

  hour_angle = TRIG_MAX_ANGLE * hour_angle / 360;
  min_angle = TRIG_MAX_ANGLE * min_angle / 360;

  if (min_angle == 0)
      min_angle = 1;
    
  rot_bitmap_layer_set_angle(layer, min_angle);

  // (144 = screen width, 168 = screen height)
  // Calculate X and Y location wrt center

  hour_x = (int16_t)(sin_lookup(hour_angle)
		     * (int32_t)length / TRIG_MAX_RATIO) + (SCREEN_WIDTH/2);
  hour_y = (int16_t)(-cos_lookup(hour_angle)
		     * (int32_t)length / TRIG_MAX_RATIO) + (SCREEN_HEIGHT/2);

  app_log(APP_LOG_LEVEL_WARNING,
          __FILE__,
          __LINE__,
          "hour_y = %d",
          (int)hour_y);

  // Adjust for offset to center of screen
  rect = layer_get_frame((const Layer *)layer);
  rect.origin.x = hour_x - (rect.size.w/2);
  rect.origin.y = hour_y - (rect.size.h/2);

#ifdef timelineview
  /* Move the whole display up if needed */
  app_log(APP_LOG_LEVEL_WARNING,
          __FILE__,
          __LINE__,
          ">> small window height = %d, hand at y = %d",
          (int)small_window.size.h,
          (int)rect.origin.y);
  if ((small_window.size.h > 0 &&
       small_window.size.h < SCREEN_HEIGHT &&
       hour_y > (SCREEN_HEIGHT/2))) {
      repos = true;
      app_log(APP_LOG_LEVEL_WARNING,
              __FILE__,
              __LINE__,
              ">> repositioning *** ");
      rect.origin.y -= (SCREEN_HEIGHT - small_window.size.h);
  }
#endif
  
  layer_set_frame((Layer *)layer, rect);

  layer_mark_dirty((Layer *)layer);

  return;
}


int
time_until_pod_expire (time_t now) 
{
    int time_diff;
    int pod_expire;

    pod_expire = pod_change + (3 * DAYS);
    app_log(APP_LOG_LEVEL_WARNING,
	    __FILE__,
	    __LINE__,
	    "Pod expire time is %d",
	    (int)pod_expire);

    time_diff = pod_expire - now;

    return (time_diff);
}


void
update_comet (void)
{
    struct tm *tm;
    int pod_hour, pod_min;
    int time_diff;
    int hour_angle, min_angle;
    int distance = COMET_DISTANCE;

    app_log(APP_LOG_LEVEL_WARNING,
	    __FILE__,
	    __LINE__,
	    "update_comet: pod_change=%d",
	    (int)pod_change);

    if (!pod_change)
	return;
    
    tm = localtime(&pod_change);
    pod_hour = tm->tm_hour;
    pod_min = tm->tm_min;

    time_diff = time_until_pod_expire(now);
    
    app_log(APP_LOG_LEVEL_WARNING,
	    __FILE__,
	    __LINE__,
	    "Last pod change time is %d",
	    (int)pod_change);

    app_log(APP_LOG_LEVEL_DEBUG, __FILE__, __LINE__,
	    "Comet check - time diff is %d",
	    (int)time_diff);

/* raj- force no comet for now */
time_diff = 48 * HOURS;

    if (time_diff < 12 * HOURS && time_diff > 0) {
	app_log(APP_LOG_LEVEL_DEBUG, __FILE__, __LINE__,
		"Display comet, hour=%d, min=%d", pod_hour, pod_min);
	layer_set_hidden(bitmap_layer_get_layer((BitmapLayer *)my_small_comet_layer),
			 false);

/*
 * Now position it at the correct place
 * Spread the pod_hour number (0-11) across 360 degrees by multiplying by 30.
 * Add in the pod_min number (0-59) but spread across 30 degrees.
 *  (number of degrees between each hour marker)
 */
	hour_angle = ((pod_hour % 12) * 30) + (pod_min/2); 
	min_angle = hour_angle;

/*
 * If comet would be on left or right, then turn it a little so it displays better
 */
	if (hour_angle > 60 && hour_angle < 120) {
	    min_angle = 30;
	    distance = 60;
	}
	if (hour_angle > 240 && hour_angle < 300) {
	    min_angle = 210;
	    distance = 60;
	}
	app_log(APP_LOG_LEVEL_DEBUG, __FILE__, __LINE__,
		"distance=%d, min_angle=%d", distance, min_angle);
	set_hand_angle(hour_angle, min_angle, my_small_comet_layer, distance);
    } else {
	app_log(APP_LOG_LEVEL_DEBUG, __FILE__, __LINE__,
		"Hide comet");
	layer_set_hidden(bitmap_layer_get_layer((BitmapLayer *)my_small_comet_layer),
			 true);
    }
}

void
show_comet (struct Layer *layer, GContext *ctx) {
    int pod_hour, pod_min;
    int hour_angle;
    struct tm *tm;

    tm = localtime(&pod_change);
    pod_hour = tm->tm_hour;
    pod_min = tm->tm_min;

    app_log(APP_LOG_LEVEL_DEBUG, __FILE__, __LINE__,
            "comet hour is %d, min is %d", pod_hour, pod_min);
    hour_angle = ((pod_hour % 12) * 30) + (pod_min/2); 
    hour_angle = TRIG_MAX_ANGLE * hour_angle / 360;
    
    app_log(APP_LOG_LEVEL_DEBUG, __FILE__, __LINE__,
            "update_comet ******** hour angle=%d", hour_angle);
    
    graphics_context_set_compositing_mode(ctx, GCompOpSet);
    graphics_draw_rotated_bitmap(ctx, my_comet_image,
                                 COMET_CENTER,
                                 hour_angle,
                                 GPoint(SCREEN_WIDTH/2,SCREEN_HEIGHT/2));
//    rot_bitmap_layer_set_angle(my_comet_layer, hour_angle);
}


void
comet_update_proc (struct Layer *layer, GContext *ctx) 
{
    int time_diff;

    if (!pod_change)
	return;
    
    time_diff = time_until_pod_expire(now);
    
    app_log(APP_LOG_LEVEL_WARNING,
	    __FILE__,
	    __LINE__,
	    "Checking pod change time is %d",
	    (int)pod_change);

    app_log(APP_LOG_LEVEL_DEBUG, __FILE__, __LINE__,
	    "Large comet check - time diff is %d",
	    (int)time_diff);

    if (time_diff <= 0) {
        layer_set_hidden((Layer *)my_comet_layer, false);
        app_log(APP_LOG_LEVEL_WARNING,
                __FILE__,
                __LINE__,
                "Turned on comet layer");
        show_comet(layer, ctx);
    }
}


/* Arrow & BG related routines and data */
int arrow_angle = 90;
int bg_value = 0;
int bg_vibes = 0;
int bg_last = 0;

void
arrow_update_proc (struct Layer *layer, GContext *ctx) 
{
    int hour_angle;

    if (arrow_angle < 0 || arrow_angle > 180) {
        layer_set_hidden((Layer *)my_double_arrow_layer, false);
        app_log(APP_LOG_LEVEL_WARNING,
                __FILE__,
                __LINE__,
                "Arrow update hidden");
        return;
    }

    layer_set_hidden((Layer *)my_arrow_layer, false);
    hour_angle = TRIG_MAX_ANGLE * arrow_angle / 360;

    graphics_context_set_compositing_mode(ctx, GCompOpSet);
    graphics_draw_rotated_bitmap(ctx, my_arrow_image,
                                 COMET_CENTER,
                                 hour_angle,
                                 GPoint(SCREEN_WIDTH/2,SCREEN_HEIGHT/2));
    
}

void
double_arrow_update_proc (struct Layer *layer, GContext *ctx) 
{
    int hour_angle;

    app_log(APP_LOG_LEVEL_WARNING,
	    __FILE__,
	    __LINE__,
	    "Double Arrow update");

    if (arrow_angle >= 0 && arrow_angle <= 180) {
        layer_set_hidden((Layer *)my_double_arrow_layer, true);
        return;
    } else if (arrow_angle < 0) {
        hour_angle = 0;
    } else if (arrow_angle > 180) {
        hour_angle = 180;
    }

    layer_set_hidden((Layer *)my_double_arrow_layer, false);
    hour_angle = TRIG_MAX_ANGLE * hour_angle / 360;

    graphics_context_set_compositing_mode(ctx, GCompOpSet);
    graphics_draw_rotated_bitmap(ctx, my_double_arrow_image,
                                 COMET_CENTER,
                                 hour_angle,
                                 GPoint(SCREEN_WIDTH/2,SCREEN_HEIGHT/2));

}


void
update_weekday (int angle) 
{
    GRect rect;
    int32_t hour_x, hour_y;

    angle = TRIG_MAX_ANGLE * angle / 360;
    hour_x = (int16_t)(sin_lookup(angle)
                       * (int32_t)ASTER_DISTANCE / TRIG_MAX_RATIO) + (SCREEN_WIDTH/2);
    hour_y = (int16_t)(-cos_lookup(angle)
                       * (int32_t)ASTER_DISTANCE / TRIG_MAX_RATIO) + (SCREEN_HEIGHT/2);
    
    // Adjust for offset to center of screen
    rect = layer_get_frame((const Layer *)my_aster_layer);
    rect.origin.x = hour_x - (rect.size.w/2);
    rect.origin.y = hour_y - (rect.size.h/2);

#ifdef timelineview
    app_log(APP_LOG_LEVEL_DEBUG, __FILE__, __LINE__,
            "weekday - repos=%d", repos);
    if (repos) {
        rect.origin.y -= (SCREEN_HEIGHT - small_window.size.h);
    }
#endif

    layer_set_frame((Layer *)my_aster_layer, rect);
    layer_mark_dirty((Layer *)my_aster_layer);
}


/*
 * Return whether or not we are repositioning the
 * hands due to a notice.
 */
void
update_hand_positions(int hour, int min, int wday) {
    int hour_angle, min_angle, week_angle;
    int dif;
    int distance;

/*
 * Now position it at the correct place
 * Spread the hour number (0-11) across 360 degrees by multiplying by 30.
 * Add the min number (0-59) but spread across 30 degrees.
 *  (number of degrees between each hour marker)
 * Also calculate minutes spread across 360 degrees
 */
  hour_angle = ((hour % 12) * 30) + (min/2); 
  min_angle = min * 6;

  app_log(APP_LOG_LEVEL_WARNING,
          __FILE__,
          __LINE__,
          "hour_angle=%d, min_angle=%d",
          hour_angle, min_angle);

  distance = EARTH_DISTANCE;

  /*
   * See if we need to move it farther from the sun
   */
  dif = (min_angle - hour_angle);
  if (dif < 0) dif = 0 - dif;
  if (dif >= (180 - 45) && dif <= (180 + 45)) {
      app_log(APP_LOG_LEVEL_WARNING,
              __FILE__,
              __LINE__,
              "\n***** dif = %d\n", dif);
      distance += 8;
  }

  set_hand_angle(hour_angle, min_angle,
                 my_hour_hand_layer, distance);

  /* If we're at or after pod change day, include the comet */
  layer_mark_dirty((Layer *)my_small_comet_layer);
  update_comet();

  /* Throw up something to indicate day of the week */
  if (!weekday_done || (hour == 0 && min == 1)) {
      week_angle = ((wday * 360) / 7);
      update_weekday(week_angle);
      weekday_done = true;
  }

  return;
}


void
write_pod_change_info (char *log) 
{
    
    stat = persist_write_int(PODCHANGE, (uint32_t)pod_change);
    persist_write_int(MESSAGE_KEY_podchange, (uint32_t)pod_change);
    app_log(APP_LOG_LEVEL_WARNING,
	    __FILE__,
	    __LINE__,
	    "bluetooth connected is %d",
	    (int)bluetooth_connected);
    if (bluetooth_connected == true && stat == 4) {
	pod_change_written = true;
    } else {
	app_log(APP_LOG_LEVEL_WARNING,
		__FILE__,
		__LINE__,
		log,
		(int)stat);
	pod_change_written = false;
    }
}


void
update_date_fields(void) 
{
    static char date_month[]="12";
    static char date_day[]="12";
    static char date_hour[]="12";
    static char date_minute[]="12";
    struct tm *now_tick;

#ifdef timelineview
    GRect rect;
#endif

    now_tick = localtime(&now);
    
    strftime(date_month, sizeof(date_month), "%m", now_tick);
//    if (date_month[0] == '0') {
//	date_month[0] = date_month[1];
//	date_month[1] = ' ';
//    }
    text_layer_set_text(my_date_layer[DATE_MONTH], date_month);
    layer_mark_dirty(text_layer_get_layer(my_date_layer[DATE_MONTH]));
    
    strftime(date_day, sizeof(date_day), "%e", now_tick);
    text_layer_set_text(my_date_layer[DATE_DAY], date_day);
    layer_mark_dirty(text_layer_get_layer(my_date_layer[DATE_DAY]));
    
#ifdef timelineview
    /* Reposition these bottom fields? */
    rect = layer_get_frame((const Layer *)my_date_layer[DATE_HOUR]);
    app_log(APP_LOG_LEVEL_WARNING,
            __FILE__,
            __LINE__,
            "y = %d, adjustment = %d",
            rect.origin.y,
            (SCREEN_HEIGHT - small_window.size.h));
    if (small_window.size.h > 0 &&
        small_window.size.h < SCREEN_HEIGHT) {
        rect.origin.y = small_window.size.h - FONT_SIZE;
    } else {
        rect.origin.y = SCREEN_HEIGHT - FONT_SIZE;
    }
    layer_set_frame((Layer *)my_date_layer[DATE_HOUR], rect);
    
    rect = layer_get_frame((const Layer *)my_date_layer[DATE_MINUTE]);
    if (small_window.size.h > 0 &&
        small_window.size.h < SCREEN_HEIGHT) {
        rect.origin.y = small_window.size.h - FONT_SIZE;
    } else {
        rect.origin.y = SCREEN_HEIGHT - FONT_SIZE;
    }
    layer_set_frame((Layer *)my_date_layer[DATE_MINUTE], rect);
#endif

    strftime(date_hour, sizeof(date_hour), "%H", now_tick);
    text_layer_set_text(my_date_layer[DATE_HOUR], date_hour);
    layer_mark_dirty(text_layer_get_layer(my_date_layer[DATE_HOUR]));
    
    strftime(date_minute, sizeof(date_minute), "%M", now_tick);
    text_layer_set_text(my_date_layer[DATE_MINUTE], date_minute);
    layer_mark_dirty(text_layer_get_layer(my_date_layer[DATE_MINUTE]));
}


void
turn_off_dates (void *data) 
{
    int i;

    for (i = 0 ; i < 4 ; i++) {
        layer_set_hidden(text_layer_get_layer(my_date_layer[i]), !display_dates[i]);
    }
    tap_display = 0;
}
/*
 * Handle tap and wrist flick events to display date, time, etc.
 */
static void
handle_tap (AccelAxisType axis, int32_t direction) {
    int i;

    tap_display_time = time(0);
    tap_display = true;
    for (i = 0 ; i < 4 ; i++) {
	layer_set_hidden(text_layer_get_layer(my_date_layer[i]), false);
    }
    update_date_fields();

    /*
     * Set an alarm for 15 seconds
     */
    app_timer_register(10000, turn_off_dates, NULL);
}



void
init_date_layer(int layer, int a, int b, int c, int d) 
{
    
    my_date_layer[layer] = text_layer_create(GRect(a, b, c, d));
    text_layer_set_text_alignment(my_date_layer[layer], GTextAlignmentCenter);
    text_layer_set_text_color(my_date_layer[layer], GColorWhite);
    text_layer_set_background_color(my_date_layer[layer], GColorClear);
    text_layer_set_font(my_date_layer[layer], date_font);
    layer_add_child(my_window_layer,
		    text_layer_get_layer(my_date_layer[layer]));
}



void
handle_bluetooth(bool connected) 
{
    static bool last_state=true;
    
    if (connected == false) {
	app_log(APP_LOG_LEVEL_DEBUG, __FILE__, __LINE__,
		"Bluetooth lost");
	layer_set_hidden(bitmap_layer_get_layer(my_bt_layer), false);
	bluetooth_connected = false;
    } else {
	app_log(APP_LOG_LEVEL_DEBUG, __FILE__, __LINE__,
		"Bluetooth back");
	layer_set_hidden(bitmap_layer_get_layer(my_bt_layer), true);
	bluetooth_connected = true;
    }

    if (connected != last_state)
	vibes_double_pulse();

    last_state = connected;
}


/*
 * Change the star/background according to battery state
 */
void
update_battery () 
{
    BatteryChargeState bat = battery_state_service_peek();

    if (bat.charge_percent > 60) {
	app_log(APP_LOG_LEVEL_DEBUG, __FILE__, __LINE__,
		"Bitmap at 100");
        bitmap_layer_set_bitmap(my_background_layer, my_background_image_100);
    } else if (bat.charge_percent <= 60 && bat.charge_percent > 30) {
	app_log(APP_LOG_LEVEL_DEBUG, __FILE__, __LINE__,
		"Bitmap at 60");
        bitmap_layer_set_bitmap(my_background_layer, my_background_image_60);
    } else {
	app_log(APP_LOG_LEVEL_DEBUG, __FILE__, __LINE__,
		"Bitmap at 30");
        bitmap_layer_set_bitmap(my_background_layer, my_background_image_30);
    }
}


#ifdef notused
void
bat_update_proc(Layer *layer, GContext *ctx) 
{
#if defined(PBL_RECT)
    int i;
#elif defined(PBL_ROUND)
    GRect screen_rect;
#endif
    
    BatteryChargeState bat = battery_state_service_peek();

    graphics_context_set_stroke_color(ctx, GColorWhite);
    graphics_context_set_fill_color(ctx, GColorWhite);

    app_log(APP_LOG_LEVEL_WARNING,
	    __FILE__,
	    __LINE__,
	    "Battery at %d (width=%d)%%\n", (int)bat.charge_percent, SCREEN_WIDTH);

#if defined(PBL_RECT)
    for (i = 0 ; i < BATTERY_HEIGHT; i++) {
	graphics_draw_line(ctx, GPoint(0, i), GPoint(bat.charge_percent*SCREEN_WIDTH/100, i));
    }
#elif defined(PBL_ROUND)
    graphics_context_set_stroke_width(ctx, 10);
    screen_rect = layer_get_frame(my_window_layer);
    screen_rect.origin.x += 2;
    screen_rect.origin.y += 2;
    screen_rect.size.w -= 4;
    screen_rect.size.h -= 4;
    graphics_draw_arc(ctx, screen_rect, GOvalScaleModeFitCircle,
                      DEG_TO_TRIGANGLE((((100-bat.charge_percent) * (360))/100)),
                      DEG_TO_TRIGANGLE(360));
#ifdef notdef
    app_log(APP_LOG_LEVEL_WARNING,
            __FILE__,
            __LINE__,
            "Battery arc in rect %d,%d %d,%d from %d to 360 %%\n",
            screen_rect.origin.x,
            screen_rect.origin.y,
            screen_rect.size.w,
            screen_rect.size.h,
            (int)((((100-bat.charge_percent)*(360))/100)));
#endif
#endif
}
#endif


#ifdef HELLO
static void
write_hello (int key, int value) {
    DictionaryIterator *iter;

    app_message_outbox_begin(&iter);
    if (!iter) {
        // Error creating outbound message
        return;
    }
    
    dict_write_int32(iter, key, value);

    dict_write_end(iter);
    
    app_message_outbox_send();

    app_log(APP_LOG_LEVEL_WARNING,
            __FILE__,
            __LINE__,
            "Sent ack");
}

static int hello_interval=10; // trigger an arrow update after 1 min

#endif

bool screen_init=false;

void
rotate_asteroid (struct tm *tick_time) 
{
    int minute;
    int min_angle;

    minute = tick_time->tm_min;
    min_angle = minute * 6;
    min_angle = TRIG_MAX_ANGLE * min_angle / 360;
    rot_bitmap_layer_set_angle(my_aster_layer, min_angle);
}



void handle_minute_tick(struct tm *tick_time, TimeUnits units_changed) {
    int hour, minute, weekday;

#ifdef timelineview
    GRect rect;
#endif

    now = time(0);

    hour = tick_time->tm_hour;
    minute = tick_time->tm_min;
    weekday = tick_time->tm_wday;

    app_log(APP_LOG_LEVEL_DEBUG, __FILE__, __LINE__,
	    "Time is %d:%02d = %u", tick_time->tm_hour, tick_time->tm_min,
	    (unsigned int)now);

    {
        bool zone_set;
        char zonestr[TIMEZONE_NAME_LENGTH];

        zone_set = clock_is_timezone_set();
        
        app_log(APP_LOG_LEVEL_DEBUG, __FILE__, __LINE__,
                "Timezone is%sset", zone_set ? " " : " not ");

        clock_get_timezone(&zonestr[0], TIMEZONE_NAME_LENGTH);
        app_log(APP_LOG_LEVEL_DEBUG, __FILE__, __LINE__,
                "Timezone is %s", zonestr);
    }
    
#ifdef timelineview
    /* See if we need a smaller window */
    app_log(APP_LOG_LEVEL_DEBUG, __FILE__, __LINE__,
            ">> small_window size = %d", small_window.size.h);
#endif

    update_battery();

    update_hand_positions(hour, minute, weekday);
        
#ifdef timelineview
    rect = layer_get_frame((const Layer *)my_background_layer);
    if (repos) {
        rect.origin.y = 0 - (SCREEN_HEIGHT - small_window.size.h);
    } else {
        rect.origin.y = 0;
    }
    layer_set_frame((Layer *)my_background_layer, rect);
#endif

    update_date_fields();

    layer_mark_dirty((Layer *)my_hour_hand_layer);
    
    if (!pod_change_written) {
	write_pod_change_info("Updating pod change time failed - %d");
    }

/*
 * If mon, date, hour, min, are being displayed due to a screen tap,
 * and it's been at least 30 second, then reset them to their configured states.
 */
    if (tap_display &&
	(time(0) - tap_display_time) > (30 * SECONDS)) {
        turn_off_dates(NULL);
    }

/*
 * Trigger gathering Nightscout info
 */
#ifdef HELLO2
        app_log(APP_LOG_LEVEL_DEBUG, __FILE__, __LINE__,
                ">>>>>> is there an arrow update request?");
    if (++hello_interval >= 5 && js_is_up) {
        app_log(APP_LOG_LEVEL_DEBUG, __FILE__, __LINE__,
                ">>>>>> arrow update request");
        write_hello(MESSAGE_KEY_arrow, 0);
        hello_interval = 0;
    }

    if (bg_last && bg_last < (now - (20 * 60))) {
        layer_set_hidden((Layer *)bg_layer, true);
        bg_vibes = 2;
    }
    
#endif
    
    /* Rotate the asteroid */
    rotate_asteroid(tick_time);
    
//    check_comet();
    
    screen_init=true;
}

void handle_second_tick(struct tm *tick_time, TimeUnits units_changed) {
    int second;
    second = tick_time->tm_sec;
/*
    int sec_angle;

    sec_angle = second * 6;
    sec_angle = TRIG_MAX_ANGLE * sec_angle / 360;
    rot_bitmap_layer_set_angle(my_aster_layer, sec_angle);
*/

    if (bg_vibes > 0) {
        vibes_double_pulse();
        bg_vibes--;
    }

    if (second == 0 || !screen_init) {
        handle_minute_tick(tick_time, units_changed);
    }
}


void
handle_timer_tick(struct tm *tick_time, TimeUnits units_changed) {
    int min, sec;
    static char timer_text[40];          /* HH:MM */

    sec = timer_start + (timer_interval*60) - time(0L);
    app_log(APP_LOG_LEVEL_DEBUG, __FILE__, __LINE__,
            "Timer seconds is %d", sec);
    if (sec <= -5) {                     /* stop after 5 */
        /* Reset to normal minute tick handler */
//        tick_timer_service_subscribe(MINUTE_UNIT, handle_minute_tick);
        tick_timer_service_subscribe(SECOND_UNIT, handle_second_tick);
        
        window_stack_pop(true);             /* remove timer window */
        
        /* Clear the timer */
        timer_start = 0;
        persist_write_int(TIMER_START, (uint32_t)timer_start);
        return;                         /* we're done! */
    }
    
    if (sec <= 0) {
        /* vibes! */
        vibes_long_pulse();
        snprintf(timer_text, sizeof(timer_text), "\n\n%2d minutes done",
                 timer_interval);
        text_layer_set_text(timer_layer, timer_text);
        text_layer_set_text_alignment(timer_layer, GTextAlignmentCenter);
        return;
    }

    min = sec / 60;
    sec = sec % 60;
    
    snprintf(timer_text, sizeof(timer_text), "\n\n%02d:%02d", min, sec);
    text_layer_set_text(timer_layer, timer_text);
    text_layer_set_text_alignment(timer_layer, GTextAlignmentCenter);
}


void
start_timer(void) 
{
    
    timer_start = time(0L);

    persist_write_int(TIMER_START, (uint32_t)timer_start); /* save it */

    tick_timer_service_subscribe(SECOND_UNIT, handle_timer_tick);
    window_stack_push(timer_window, true);
    
    /* And set a wakeup timer as well */
    wakeup_schedule(timer_start + (timer_interval * 60), 0, false);
}


/*
 * Got a wakeup - redisplay current timer 
 */
void
handle_wakeup (uint32_t comet_time, int32_t cookie) 
{

    /* already handled by now */
    app_log(APP_LOG_LEVEL_DEBUG, __FILE__, __LINE__,
            "**** Update comet triggered");

    pod_change = (time_t)comet_time;            /* update internal state */
    write_pod_change_info("Failed updating comet - %d");
    update_comet();
//    layer_mark_dirty((Layer *)my_comet_layer);

    return;
}


void
update_configuration (void) 
{
    bool val;

    app_log(APP_LOG_LEVEL_DEBUG, __FILE__, __LINE__,
            "Update Configuration");

    if (persist_exists(DATE_MONTH)) {
        display_dates[DATE_MONTH] = persist_read_bool(DATE_MONTH);
        layer_set_hidden(text_layer_get_layer(my_date_layer[DATE_MONTH]),
                         !display_dates[DATE_MONTH]);
    }
    
    if (persist_exists(DATE_DAY)) {
        display_dates[DATE_DAY] = persist_read_bool(DATE_DAY);
        layer_set_hidden(text_layer_get_layer(my_date_layer[DATE_DAY]),
                         !display_dates[DATE_DAY]);
    }
    
    if (persist_exists(DATE_HOUR)) {
        display_dates[DATE_HOUR] = persist_read_bool(DATE_HOUR);
        layer_set_hidden(text_layer_get_layer(my_date_layer[DATE_HOUR]),
                         !display_dates[DATE_HOUR]);
    }
    
    if (persist_exists(DATE_MINUTE)) {
        display_dates[DATE_MINUTE] = persist_read_bool(DATE_MINUTE);
        layer_set_hidden(text_layer_get_layer(my_date_layer[DATE_MINUTE]),
                         !display_dates[DATE_MINUTE]);
    }
    
    if (persist_exists(BLUETOOTH)) {
        val = persist_read_bool(BLUETOOTH);
        if (val) {
            bluetooth_connection_service_subscribe(handle_bluetooth);
            /* Update immediate status */
            if (bluetooth_connection_service_peek()) {
                layer_set_hidden(bitmap_layer_get_layer(my_bt_layer), true);
            } else {
                layer_set_hidden(bitmap_layer_get_layer(my_bt_layer), false);
            }
            app_log(APP_LOG_LEVEL_DEBUG, __FILE__, __LINE__,
                    "Bluetooth subscribe, current state is %d",
                    bluetooth_connected);
        } else {
            bluetooth_connection_service_unsubscribe();
            app_log(APP_LOG_LEVEL_DEBUG, __FILE__, __LINE__,
                    "Bluetooth unsubscribe");
        }
    }
    
    if (bluetooth_connection_service_peek()) {
        layer_set_hidden(bitmap_layer_get_layer(my_bt_layer), true);
        bluetooth_connected = true;
    } else {
        layer_set_hidden(bitmap_layer_get_layer(my_bt_layer), false);
        bluetooth_connected = false;
    }
    
    if (persist_exists(PODCHANGE)) {
        uint32_t val;
	
        val = persist_read_int(PODCHANGE);
        if (val) {
            pod_change = (time_t)val;
            
            app_log(APP_LOG_LEVEL_WARNING,
                    __FILE__,
                    __LINE__,
                    "Pod change time read as %d",
                    (int)pod_change);
        }
    }
    persist_write_int(PODCHANGE, (uint32_t)pod_change);
    update_comet();
//  layer_mark_dirty((Layer *)my_comet_layer);

    stat = persist_write_int(CHANGEIT, (uint32_t)10); /* init */
    
    if (persist_exists(TIMER_START)) {
        uint32_t val;

        val = persist_read_int(TIMER_START);
        if (val) {
            timer_start = (time_t) val;

            app_log(APP_LOG_LEVEL_WARNING,
                    __FILE__,
                    __LINE__,
                    "Timer start time is: %u",
                    (uint)timer_start);
        }
    }

    if (persist_exists(TIMER)) {
        uint32_t val;

        val = persist_read_int(TIMER);
        if (val) {
            timer_interval = (time_t) val;

            app_log(APP_LOG_LEVEL_WARNING,
                    __FILE__,
                    __LINE__,
                    "Timer interval is: %u",
                    (uint)timer_interval);
        }
    }
    write_hello(MESSAGE_KEY_ack, pod_change);
}


void handle_msg_received (DictionaryIterator *received, void *context)
{

    Tuple *tuple;
    bool val;
    int pchour=0;
    time_t t;
    bool change_it = false;

    app_log(APP_LOG_LEVEL_WARNING,
            __FILE__,
            __LINE__,
            "msg received");

    tuple = dict_find(received, MESSAGE_KEY_arrow);
    if (tuple) {
        app_log(APP_LOG_LEVEL_WARNING,
                __FILE__,
                __LINE__,
                "ARROW %d",
                tuple->value->int16);
        arrow_angle = tuple->value->int16;
        return;                         /* that's it! */
    }

    tuple = dict_find(received, MESSAGE_KEY_sgv);
    if (tuple) {
        static char bg_string[4];
        
        app_log(APP_LOG_LEVEL_WARNING,
                __FILE__,
                __LINE__,
                "SGV");
        app_log(APP_LOG_LEVEL_WARNING,
                __FILE__,
                __LINE__,
                "%d", (int)tuple->value->int16);
        bg_value = tuple->value->int16;
        if (bg_value == 0) {
            /* got an error reading */
            layer_set_hidden((Layer *)bg_layer, true);
	    bg_vibes = 3;
        } else {

            bg_last = now;
	    if (bg_value > 200 || bg_value < 50) {
		bg_vibes = 3;
	    }

            snprintf(bg_string, sizeof(bg_string), "%3d", bg_value);
            if (arrow_angle >= 180) {
                layer_set_frame((Layer *)bg_layer, GRect((SCREEN_WIDTH/2)-(FONT_SIZE/1.8),
                                                         (SCREEN_HEIGHT)-(FONT_SIZE*2),
                                                         FONT_SIZE*2, FONT_SIZE));
            } else {
                layer_set_frame((Layer *)bg_layer, GRect((SCREEN_WIDTH/2)-(FONT_SIZE/1.8),
                                                         (SCREEN_HEIGHT)-(FONT_SIZE*1.2),
                                                         FONT_SIZE*2, FONT_SIZE));
            }
            text_layer_set_text(bg_layer, bg_string);
            layer_set_hidden((Layer *)bg_layer, false);
            layer_mark_dirty(text_layer_get_layer(bg_layer));
        }
    }

    tuple = dict_find(received, MESSAGE_KEY_ack);
    if (tuple) {
        app_log(APP_LOG_LEVEL_WARNING,
                __FILE__,
                __LINE__,
                "ACK");
        js_is_up = true;
        return;                         /* that's it! */
    }

    tuple = dict_find(received, MESSAGE_KEY_displayMonth);
    if (tuple) {
	val = true;
	if (tuple->value->int8 == 0) {
	    val = false;
	}
	app_log(APP_LOG_LEVEL_WARNING,
		__FILE__,
		__LINE__,
		"month = '%d'", (int)val);
	persist_write_bool(DATE_MONTH, val);
    }

    tuple = dict_find(received, MESSAGE_KEY_displayDate);
    if (tuple) {
	val = true;
	if (tuple->value->int8 == 0) {
	    val = false;
	}
	app_log(APP_LOG_LEVEL_WARNING,
		__FILE__,
		__LINE__,
		"date = '%d'", (int)val);
	persist_write_bool(DATE_DAY, val);
    }

    tuple = dict_find(received, MESSAGE_KEY_displayHour);
    if (tuple) {
	val = true;
	if (tuple->value->int8 == 0) {
	    val = false;
	}
	app_log(APP_LOG_LEVEL_WARNING,
		__FILE__,
		__LINE__,
		"hour = '%d'", (int)val);
	persist_write_bool(DATE_HOUR, val);
    }
    
    tuple = dict_find(received, MESSAGE_KEY_displayMinute);
    if (tuple) {
	val = true;
	if (tuple->value->int8 == 0) {
	    val = false;
	}
	app_log(APP_LOG_LEVEL_WARNING,
		__FILE__,
		__LINE__,
		"minute = '%d'", (int)val);
	persist_write_bool(DATE_MINUTE, val);
    }
    
    tuple = dict_find(received, MESSAGE_KEY_displayBluetooth);
    if (tuple) {
	val = true;
	if (tuple->value->int8 == 0) {
	 val = false;
	}
	app_log(APP_LOG_LEVEL_WARNING,
		__FILE__,
		__LINE__,
		"bluetooth = '%d'",
		tuple->value->int8);
	persist_write_bool(BLUETOOTH, val);
    }

    /* Get current time */
    t = time(0);
    app_log(APP_LOG_LEVEL_WARNING,
            __FILE__,
            __LINE__,
            "cur time = '%d'",
            (int)t);
	    

    /* If "ago" is there, modify by that */
    tuple = dict_find(received, MESSAGE_KEY_ago);
    if (tuple) {
	app_log(APP_LOG_LEVEL_WARNING,
		__FILE__,
		__LINE__,
		"ago = '%s'",
		tuple->value->cstring);
	pchour = atoi(tuple->value->cstring);

	if (pchour != 0) {
	    t -= pchour * HOURS;
	    app_log(APP_LOG_LEVEL_WARNING,
		    __FILE__,
		    __LINE__,
		    "new time = '%d'",
		    (int)t);
	}
    }

    tuple = dict_find(received, MESSAGE_KEY_changeit);
    if (tuple) {
	val = true;
	if (tuple->value->int8 == 0) {
	 val = false;
	}
        change_it = val;
	app_log(APP_LOG_LEVEL_WARNING,
		__FILE__,
		__LINE__,
		"Change pod now is %d",
		(int)val);
    }

    if (change_it || pchour > 0) {
        /* Set pod_change time */
        pod_change = t;
        app_log(APP_LOG_LEVEL_WARNING,
                __FILE__,
                __LINE__,
                "podchange = %u",
                (uint)pod_change);
        write_pod_change_info("Change_it pod change time - %d");
        write_hello(MESSAGE_KEY_ack, pod_change);              /* send timeline updates */
    }

    tuple = dict_find(received, MESSAGE_KEY_timer);
    if (tuple) {
	if (strcmp(tuple->value->cstring, "0") != 0) {
            app_log(APP_LOG_LEVEL_WARNING,
                    __FILE__,
                    __LINE__,
                    "Want a timer run = %s",
                    tuple->value->cstring);
            
/* If they said they want a timer run - start that window */
            timer_interval = atoi(tuple->value->cstring);
            persist_write_int(TIMER, (uint32_t)timer_interval);
            if (change_it)
                start_timer();
        } else if (strcmp(tuple->value->cstring, "0") && timer_start) {
            /* abort the timer now */
            timer_start = 0;
            persist_write_int(TIMER_START, (uint32_t)timer_start);
        }
    }

    update_configuration();

}


void handle_msg_dropped (AppMessageResult reason, void *ctx)
{

    app_log(APP_LOG_LEVEL_WARNING,
	    __FILE__,
	    __LINE__,
	    "Message dropped, reason code %d",
	    reason);
}


#ifdef timelineview
/*
 * Handle obstruction of the display
 */

static void
handle_obstruction_happening (GRect area, void *context) 
{

    app_log(APP_LOG_LEVEL_WARNING,
	    __FILE__,
	    __LINE__,	 
	    ">> Obstruction happening: %d, %d, %d, %d\n",
            area.origin.x, area.origin.y,
            area.size.w, area.size.h);

    small_window = area;                /* save for later */
    weekday_done = false;
}

static void
handle_obstruction_done (void *context) 
{

    app_log(APP_LOG_LEVEL_WARNING,
	    __FILE__,
	    __LINE__,	 
	    "Obstructed done: %d, %d, %d, %d\n",
            small_window.origin.x, small_window.origin.y,
            small_window.size.w, small_window.size.h);
    
    /* Force an update */
    now = time(0);
    handle_minute_tick(localtime(&now), MINUTE_UNIT);
}
#endif


void handle_init(void) {
    int dict_size;
  
    app_log(APP_LOG_LEVEL_WARNING,
	    __FILE__,
	    __LINE__,	 
	    "Init\n");

    app_message_register_inbox_received(handle_msg_received);
    app_message_register_inbox_dropped(handle_msg_dropped);

    /* Calculate size of buffer needed */
    dict_size = dict_calc_buffer_size(10,
                                      5, /* month */
                                      5, /* date */
                                      5, /* hour */
                                      5, /* minute */
                                      5, /* bluetooth */
                                      20, /* podchange */
                                      5, /* pchour */
                                      5, /* changeit */
                                      5, /* timer */
                                      10); /* timer_start */
    
    app_log(APP_LOG_LEVEL_WARNING,
	    __FILE__,
	    __LINE__,	 
	    "dict_size = %d\n", dict_size);

//    app_message_open(dict_size, dict_size);
    app_message_open(app_message_inbox_size_maximum(),
                     app_message_outbox_size_maximum());

    date_font = fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD);
    note_font = fonts_get_system_font(FONT_KEY_BITHAM_30_BLACK);

    my_window = window_create();
//	window_set_fullscreen(my_window, true);
    window_stack_push(my_window, true);
    
    my_window_layer  = window_get_root_layer(my_window);
    
    // Set up a layer for the static watch face background
#if defined(PBL_RECT)
    my_background_image_100 = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BACKGROUND_100);
    my_background_image_60 = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BACKGROUND_60);
    my_background_image_30 = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BACKGROUND_30);
#elif defined(PBL_ROUND)
    my_background_image_100 = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BACKGROUND_ROUND_100);
    my_background_image_60 = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BACKGROUND_ROUND_60);
    my_background_image_30 = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BACKGROUND_ROUND_30);
#endif
    my_background_layer = bitmap_layer_create(layer_get_frame(my_window_layer));
    bitmap_layer_set_bitmap(my_background_layer, my_background_image_100);
    layer_add_child(my_window_layer,
                    bitmap_layer_get_layer(my_background_layer));
    
    // Set up a layer for the hour hand
    my_hour_hand_image = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_HOUR_HAND);
    my_hour_hand_layer = rot_bitmap_layer_create(my_hour_hand_image);
    layer_add_child(my_window_layer,
                    bitmap_layer_get_layer((BitmapLayer *)my_hour_hand_layer));
    rot_bitmap_set_src_ic(my_hour_hand_layer, GPoint(13, 27));
    
    // Set up a layer for the pod change comet :)
#if defined(PBL_RECT)
    my_comet_image = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_COMET_SQUARE);
#else    
    my_comet_image = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_COMET);
#endif
    my_comet_layer = rot_bitmap_layer_create(my_comet_image);
    layer_add_child(my_window_layer,
                    bitmap_layer_get_layer((BitmapLayer *)my_comet_layer));
    rot_bitmap_set_src_ic(my_comet_layer, COMET_CENTER);
    layer_set_frame((Layer *)my_comet_layer, GRect(0,0,SCREEN_WIDTH,SCREEN_HEIGHT));
    bitmap_layer_set_compositing_mode((BitmapLayer *)my_comet_layer, GCompOpSet);
    layer_set_update_proc((struct Layer *)my_comet_layer, comet_update_proc);
    
    // Set up a layer for the small pod change comet :)
    my_small_comet_image = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_COMET_SMALL);
    my_small_comet_layer = rot_bitmap_layer_create(my_small_comet_image);
    layer_add_child(my_window_layer,
                    bitmap_layer_get_layer((BitmapLayer *)my_small_comet_layer));
    rot_bitmap_set_src_ic(my_small_comet_layer, GPoint(3, 14));
    
    my_aster_image = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_ASTER);
//    my_aster_layer = bitmap_layer_create(layer_get_frame(my_window_layer));
    my_aster_layer = rot_bitmap_layer_create(my_aster_image);
    layer_add_child(my_window_layer,
                    bitmap_layer_get_layer((BitmapLayer *)my_aster_layer));
    rot_bitmap_set_src_ic(my_aster_layer, GPoint(4, 4));

    // Set up a layer for the BG pointer
    my_arrow_image = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_ARROW);
    my_double_arrow_image = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_DOUBLE_ARROW);
    my_arrow_layer = rot_bitmap_layer_create(my_arrow_image);
    my_double_arrow_layer = rot_bitmap_layer_create(my_double_arrow_image);
    layer_add_child(my_window_layer,
                    bitmap_layer_get_layer((BitmapLayer *)my_arrow_layer));
    layer_add_child(my_window_layer,
                    bitmap_layer_get_layer((BitmapLayer *)my_double_arrow_layer));
    rot_bitmap_set_src_ic(my_arrow_layer, COMET_CENTER);
    rot_bitmap_set_src_ic(my_double_arrow_layer, COMET_CENTER);
    layer_set_frame((Layer *)my_arrow_layer, GRect(0,0,SCREEN_WIDTH,SCREEN_HEIGHT));
    layer_set_frame((Layer *)my_double_arrow_layer, GRect(0,0,SCREEN_WIDTH,SCREEN_HEIGHT));
    bitmap_layer_set_compositing_mode((BitmapLayer *)my_arrow_layer, GCompOpSet);
    bitmap_layer_set_compositing_mode((BitmapLayer *)my_double_arrow_layer, GCompOpSet);
    layer_set_update_proc((struct Layer *)my_arrow_layer, arrow_update_proc);
    layer_set_update_proc((struct Layer *)my_double_arrow_layer, double_arrow_update_proc);
    
/*
 * Create a text window for displaying BG values
 */
    bg_layer = text_layer_create(GRect((SCREEN_WIDTH/2)-(FONT_SIZE/1.8), (SCREEN_HEIGHT)-(FONT_SIZE*1.2),
                                          FONT_SIZE*2, FONT_SIZE));
//    text_layer_set_text_alignment(bg_layer, GTextAlignmentCenter);
    text_layer_set_text_color(bg_layer, GColorWhite);
    text_layer_set_background_color(bg_layer, GColorClear);
    text_layer_set_font(bg_layer, date_font);
    layer_add_child(my_window_layer, text_layer_get_layer(bg_layer));

#if defined(PBL_RECT)
    /* Month in upper-left */
    init_date_layer(DATE_MONTH, 0/*TL-x*/, 0/*TL-y*/, FONT_SIZE/*x*/, FONT_SIZE/*y*/);
    
    /* Day in upper-right */
    init_date_layer(DATE_DAY, SCREEN_WIDTH-FONT_SIZE/*TL-x*/, 0/*TL-y*/, FONT_SIZE/*x*/, FONT_SIZE/*y*/);
    
    /* Hour in lower-left */
    init_date_layer(DATE_HOUR, 0/*TL-x*/, SCREEN_HEIGHT-FONT_SIZE/*TL-y*/, FONT_SIZE/*x*/, FONT_SIZE/*y*/);
    
    /* Minute in upper-right */
    init_date_layer(DATE_MINUTE, SCREEN_WIDTH-FONT_SIZE/*TL-x*/, SCREEN_HEIGHT-FONT_SIZE/*TL-y*/, FONT_SIZE/*x*/,
                    FONT_SIZE/*y*/);
#elif defined(PBL_ROUND)
    /* Month in upper-left */
    init_date_layer(DATE_MONTH, 25/*TL-x*/, 25/*TL-y*/, FONT_SIZE/*x*/, FONT_SIZE/*y*/);
    
    /* Day in upper-right */
    init_date_layer(DATE_DAY, SCREEN_WIDTH-FONT_SIZE-25/*TL-x*/, 25/*TL-y*/,
                    FONT_SIZE/*x*/, FONT_SIZE/*y*/);
    
    /* Hour in lower-left */
    init_date_layer(DATE_HOUR, 27/*TL-x*/, SCREEN_HEIGHT-FONT_SIZE-27/*TL-y*/,
                    FONT_SIZE/*x*/, FONT_SIZE/*y*/);
    
    /* Minute in upper-right */
    init_date_layer(DATE_MINUTE, SCREEN_WIDTH-FONT_SIZE-27/*TL-x*/,
                    SCREEN_HEIGHT-FONT_SIZE-27/*TL-y*/,
                    FONT_SIZE/*x*/, FONT_SIZE/*y*/);
#endif    
    
/*
 * Setup our tap handler - to display the above information when tapped
 */
    accel_tap_service_subscribe(handle_tap);
    tap_display = false;		/* init. */
    
/*
 * Define a field for displaying when bluetooth connection is not there
 */
    my_bt_layer = bitmap_layer_create(GRect((SCREEN_WIDTH/2)-(FONT_SIZE/2),
                                            (SCREEN_HEIGHT/2)-(FONT_SIZE/2),
                                            FONT_SIZE, FONT_SIZE));
    my_bluetooth_image = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BLUETOOTH);
//    bitmap_layer_set_compositing_mode(my_bt_layer, GCompOpSet);
    bitmap_layer_set_bitmap(my_bt_layer, my_bluetooth_image);
    layer_add_child(my_window_layer, bitmap_layer_get_layer(my_bt_layer));
    layer_set_hidden(bitmap_layer_get_layer(my_bt_layer), true); /* hide it initially */
    
/*
 * Indicate battery life
 */
#if defined(PBL_RECT)
//    my_bat_layer = layer_create(GRect(0, SCREEN_HEIGHT-BATTERY_HEIGHT, SCREEN_WIDTH, BATTERY_HEIGHT));
#elif defined(PBL_ROUND)
//    my_bat_layer = layer_create(GRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT));
#endif    
//    layer_set_update_proc(my_bat_layer, bat_update_proc);
//    layer_add_child(my_window_layer, my_bat_layer);
    
/*
 * Create timer window for pod change timer
 */
    timer_window = window_create();
    timer_layer = text_layer_create(GRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT));
    text_layer_set_font(timer_layer, note_font);
    layer_add_child(window_get_root_layer(timer_window),
                    text_layer_get_layer(timer_layer));

//`    tick_timer_service_subscribe(MINUTE_UNIT, handle_minute_tick);
    tick_timer_service_subscribe(SECOND_UNIT, handle_second_tick);

#ifdef timelineview
    small_window = layer_get_unobstructed_bounds(my_window_layer);

    {
       UnobstructedAreaHandlers handlers = {
           .did_change = handle_obstruction_done,
           .will_change = handle_obstruction_happening,
        };
        
        unobstructed_area_service_subscribe(handlers, NULL);
    }
#endif

    app_log(APP_LOG_LEVEL_WARNING,
            __FILE__,
            __LINE__,	 
            "***** Updating configuration");

    update_configuration();
}


void handle_deinit(void) {

    tick_timer_service_unsubscribe();

    bluetooth_connection_service_unsubscribe();
    app_message_deregister_callbacks();

    gbitmap_destroy(my_background_image_100);
    gbitmap_destroy(my_background_image_60);
    gbitmap_destroy(my_background_image_30);
    bitmap_layer_destroy(my_background_layer);
    
    gbitmap_destroy(my_hour_hand_image);
    rot_bitmap_layer_destroy(my_hour_hand_layer);
    
    layer_destroy(my_bat_layer);

    gbitmap_destroy(my_bluetooth_image);
    bitmap_layer_destroy(my_bt_layer);

    for(int i = 0; i < 4; i++) {
	text_layer_destroy(my_date_layer[i]);
    }

    gbitmap_destroy(my_aster_image);
    rot_bitmap_layer_destroy(my_aster_layer);

    gbitmap_destroy(my_comet_image);
    rot_bitmap_layer_destroy(my_comet_layer);

    text_layer_destroy(timer_layer);
    window_destroy(timer_window);

    layer_destroy(my_window_layer);

//    window_destroy(my_window); /* don't destroy top window */

}




int main (void) {
    WakeupId id = 0;
    int32_t cookie;

    handle_init();

    wakeup_get_launch_event(&id, &cookie);
    app_log(APP_LOG_LEVEL_WARNING,
            __FILE__,
            __LINE__,	 
            "launch_reason = %u\n", (uint)launch_reason());
    if (launch_reason() == APP_LAUNCH_TIMELINE_ACTION) {
        uint32_t arg = launch_get_args();
        app_log(APP_LOG_LEVEL_WARNING,
                __FILE__,
                __LINE__,	 
                "launch_arg = %u\n", (uint)arg);
        
        handle_wakeup(arg, cookie);
    }

    app_event_loop();
    handle_deinit();
    /* end new */
}
