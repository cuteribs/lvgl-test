#include <lvgl.h>
#include <TFT_eSPI.h>
#include <Adafruit_FT6206.h>
#include <Wire.h>

#define TFT_WIDTH 320
#define TFT_HEIGHT 480

/*Change to your screen resolution*/
static const uint8_t rotation = 3;
static const uint16_t screenWidth = (rotation == 1 || rotation == 3) ? TFT_HEIGHT : TFT_WIDTH;
static const uint16_t screenHeight = (rotation == 1 || rotation == 3) ? TFT_WIDTH : TFT_HEIGHT;

static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf[screenWidth * 10];

TFT_eSPI tft = TFT_eSPI(); /* TFT instance */
Adafruit_FT6206 ts = Adafruit_FT6206();

void setup_hardware()
{
	Serial.begin(115200);

	pinMode(TFT_BL, OUTPUT);
	analogWrite(TFT_BL, 0xff * 0.3); /* set brightness to 30% */

	Wire.begin(18, 19);

	if (!ts.begin(40))
	{
		Serial.println("Unable to start touchscreen.");
		return;
	}

	tft.begin();							 /* TFT init */
	tft.setRotation(rotation); /* Landscape orientation, flipped */
}

/* Display flushing */
void my_lcd_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p)
{
	uint32_t w = (area->x2 - area->x1 + 1);
	uint32_t h = (area->y2 - area->y1 + 1);

	tft.startWrite();
	tft.setAddrWindow(area->x1, area->y1, w, h);
	tft.pushColors((uint16_t *)&color_p->full, w * h, true);
	tft.endWrite();

	lv_disp_flush_ready(disp);
}

/* initialize LCD */
lv_disp_t *register_lcd()
{
	lv_disp_draw_buf_init(&draw_buf, buf, NULL, screenWidth * 10);

	/*Initialize the display*/

	static lv_disp_drv_t disp_drv;
	lv_disp_drv_init(&disp_drv);
	disp_drv.hor_res = screenWidth;
	disp_drv.ver_res = screenHeight;
	disp_drv.flush_cb = my_lcd_flush;
	disp_drv.draw_buf = &draw_buf;
	lv_disp_t *disp = lv_disp_drv_register(&disp_drv);
	return disp;
}

/* read touch screen */
void my_touch_read(lv_indev_drv_t *drv, lv_indev_data_t *data)
{
	static uint16_t lastx = 0;
	static uint16_t lasty = 0;

	if (!ts.touched())
	{
		data->state = LV_INDEV_STATE_REL;
		data->point.x = lastx;
		data->point.y = lasty;
		return;
	}

	TS_Point touchPos = ts.getPoint();
	data->state = LV_INDEV_STATE_PR;
	auto xpos = touchPos.x;
	auto ypos = touchPos.y;

	switch (rotation)
	{
	case 1:
		xpos = touchPos.y;
		ypos = TFT_WIDTH - touchPos.x;
		break;
	case 2:
		xpos = TFT_WIDTH - touchPos.x;
		ypos = TFT_HEIGHT - touchPos.y;
		break;
	case 3:
		xpos = TFT_HEIGHT - touchPos.y;
		ypos = touchPos.x;
		break;
	}

	data->point.x = xpos;
	data->point.y = ypos;
	lastx = xpos;
	lasty = ypos;

	Serial.printf("touched: %d,%d (rotation: %d)", xpos, ypos, rotation);
	Serial.println();
	return;
}

/* initialize touch screen */
void register_touchscreen()
{
	Serial.println("Start touchscreen.");
	static lv_indev_drv_t indev_drv;
	lv_indev_drv_init(&indev_drv);
	indev_drv.type = LV_INDEV_TYPE_POINTER;
	indev_drv.read_cb = my_touch_read;
	lv_indev_drv_register(&indev_drv);
}

static void btn_onclick(lv_event_t *e)
{
	Serial.println("Button clicked!");
}

void setup()
{
	setup_hardware();

	String LVGL_Arduino = "Hello Arduino! ";
	LVGL_Arduino += String('V') + lv_version_major() + "." + lv_version_minor() + "." + lv_version_patch();

	Serial.println(LVGL_Arduino);
	Serial.println("I am LVGL_Arduino");

	lv_init();
	lv_disp_t *disp = register_lcd();
	register_touchscreen();

	Serial.println("Setup done");

	// set theme
	// lv_theme_t *theme = lv_theme_default_init(
	// 		disp,
	// 		lv_color_hex3(0xfff), lv_color_hex3(0x000),
	// 		true,
	// 		&lv_font_montserrat_14);
	// lv_disp_set_theme(disp, theme);

	lv_obj_t *btn = lv_btn_create(lv_scr_act());
	lv_obj_align(btn, LV_ALIGN_TOP_LEFT, 10, 10);
	lv_obj_add_event_cb(btn, btn_onclick, LV_EVENT_CLICKED, NULL);
	lv_obj_t *lbl = lv_label_create(btn);
	lv_label_set_text(lbl, "Click Here");
	lv_obj_center(lbl);
}

void loop()
{
	lv_timer_handler(); /* let the GUI do its work */
	delay(1);
}