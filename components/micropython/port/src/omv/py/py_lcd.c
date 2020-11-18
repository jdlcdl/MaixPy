/*
 * This file is part of the OpenMV project.
 * Copyright (c) 2013/2014 Ibrahim Abdelkader <i.abdalkader@gmail.com>
 * This work is licensed under the MIT license, see the file LICENSE for details.
 *
 * LCD Python module.
 *
 */
#include <mp.h>
#include <objstr.h>
#include <spi.h>
#include "imlib.h"
#include "fb_alloc.h"
#include "vfs_wrapper.h"
#include "py_assert.h"
#include "py_helper.h"
#include "py_image.h"
#include "lcd.h"
#include "sleep.h"

#include "sysctl.h"
#include "global_config.h"
#include "boards.h"
#include "Maix_config.h"

// extern uint8_t g_lcd_buf[];

// extern mp_obj_t pyb_spi_send(mp_uint_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args);
// extern mp_obj_t pyb_spi_make_new(mp_obj_t type_in, mp_uint_t n_args, mp_uint_t n_kw, const mp_obj_t *args);
// extern mp_obj_t pyb_spi_deinit(mp_obj_t self_in);

// static mp_obj_t spi_port = NULL;
static uint16_t width_curr = 0, width_conf = 0;
static uint16_t height_curr = 0, height_conf = 0;
static enum {
    LCD_NONE,
    LCD_SHIELD,
    LCD_IPS_240X240,
    LCD_M5STICK,
    LCD_TWATCH,
} type = LCD_NONE;
static uint8_t rotation = 0;
static bool invert = false;
static uint16_t screen_dir = DIR_YX_RLDU;
// static bool backlight_init = false;

static enum
{
    LCD_TYPE_ST7789 = 0,
    LCD_TYPE_ILI9486 = 1,
} lcd_type_t;
typedef struct
{
    // Hardward Interface config
    uint8_t rst_pin;
    uint8_t dcx_pin;
    uint8_t cs_pin; // cs or nss
    uint8_t bl_pin;
    uint8_t clk_pin;
    uint32_t clk_rate; //


    // LCD parmater config
    uint16_t height;
    uint16_t width;
    uint16_t offset_h0;
    uint16_t offset_w0;
    uint16_t offset_h1;
    uint16_t offset_w1;

    uint8_t lcd_type;
    uint8_t dir;
    bool invert;

} py_lcd_config_t;

// Send out 8-bit data using the SPI object.
static void lcd_write_command_byte(uint8_t data_byte)
{
    tft_set_datawidth(8);
    tft_write_command(data_byte);
    return;
}

// Send out 8-bit data using the SPI object.
static void lcd_write_data_byte(uint8_t data_byte)
{
    uint8_t temp_type;
    temp_type = data_byte;
    tft_set_datawidth(8);
    tft_write_byte(&temp_type, 1);
    return;
}

// Send out 8-bit data using the SPI object.
static void lcd_write_command(uint8_t data_byte, uint32_t len, uint8_t *dat)
{
    tft_set_datawidth(8);
    tft_write_command(data_byte);
    tft_write_byte(dat, len);
    return;
}

// Send out 8-bit data using the SPI object.
static void lcd_write_data(uint32_t len, uint8_t *dat)
{
    tft_write_byte(dat, len);
    return;
}

static mp_obj_t py_lcd_deinit()
{
    switch (type)
    {
    case LCD_NONE:
        return mp_const_none;
    case LCD_SHIELD:
    case LCD_M5STICK:
    case LCD_TWATCH:
    case LCD_IPS_240X240:
        lcd_destroy();
        width_curr = 0;
        height_curr = 0;
        type = LCD_NONE;
        return mp_const_none;
    }
    return mp_const_none;
}

#define PY_LCD_CHECK_CONFIG(GOAL, val)                                                                        \
    {                                                                                                         \
        const char key[] = #GOAL;                                                                             \
        mp_map_elem_t *elem = mp_map_lookup(&self->map, mp_obj_new_str(key, sizeof(key) - 1), MP_MAP_LOOKUP); \
        if (elem != NULL)                                                                                     \
        {                                                                                                     \
            *(val) = mp_obj_get_int(elem->value);                                                             \
        }                                                                                                     \
    }

void py_lcd_load_config(py_lcd_config_t *lcd_cfg)
{
    const char cfg[] = "lcd";
    mp_obj_t tmp = maix_config_get_value(mp_obj_new_str(cfg, sizeof(cfg) - 1), mp_const_none);
    // mp_obj_print_helper(&mp_plat_print, tmp, PRINT_STR);
    // mp_print_str(&mp_plat_print, "\r\n");
    if (tmp != mp_const_none && mp_obj_is_type(tmp, &mp_type_dict))
    {
        mp_obj_dict_t *self = MP_OBJ_TO_PTR(tmp);

        PY_LCD_CHECK_CONFIG(rst, &lcd_cfg->rst_pin);
        PY_LCD_CHECK_CONFIG(dcx, &lcd_cfg->dcx_pin);
        PY_LCD_CHECK_CONFIG(ss, &lcd_cfg->cs_pin);
        PY_LCD_CHECK_CONFIG(clk, &lcd_cfg->clk_pin);

        PY_LCD_CHECK_CONFIG(height, &lcd_cfg->height);
        PY_LCD_CHECK_CONFIG(width, &lcd_cfg->width);
        PY_LCD_CHECK_CONFIG(invert, &lcd_cfg->invert);
        PY_LCD_CHECK_CONFIG(offset_w0, &lcd_cfg->offset_w0);
        PY_LCD_CHECK_CONFIG(offset_h0, &lcd_cfg->offset_h0);
        PY_LCD_CHECK_CONFIG(offset_w1, &lcd_cfg->offset_w1);
        PY_LCD_CHECK_CONFIG(offset_h1, &lcd_cfg->offset_h1);
        PY_LCD_CHECK_CONFIG(dir, &lcd_cfg->dir);
        PY_LCD_CHECK_CONFIG(lcd_type, &lcd_cfg->lcd_type);

        // mp_printf(&mp_plat_print, "[%s]: rst=%d, dcx=%d, ss=%d, clk=%d\r\n",
        //           __func__, lcd_cfg->rst_pin, lcd_cfg->dcx_pin, lcd_cfg->cs_pin, lcd_cfg->clk_pin);

    }
}

static mp_obj_t py_lcd_init(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args)
{
    int ret = 0;
    uint16_t color = BLACK;
    py_lcd_deinit();
    enum
    {
        ARG_type,
        ARG_freq,
        ARG_color,
        ARG_width,
        ARG_height,
        ARG_invert,
        ARG_offset_w0,
        ARG_offset_h0,
        ARG_offset_w1,
        ARG_offset_h1,
        ARG_rst,
        ARG_dcx,
        ARG_ss,
        ARG_clk,
    };
    static const mp_arg_t allowed_args[] = {
        {MP_QSTR_type, MP_ARG_INT, {.u_int = LCD_SHIELD}},
        {MP_QSTR_freq, MP_ARG_INT, {.u_int = CONFIG_LCD_DEFAULT_FREQ}},
        {MP_QSTR_color, MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL}},
        {MP_QSTR_width, MP_ARG_INT, {.u_int = CONFIG_LCD_DEFAULT_WIDTH}},
        {MP_QSTR_height, MP_ARG_INT, {.u_int = CONFIG_LCD_DEFAULT_HEIGHT}},
        {MP_QSTR_invert, MP_ARG_INT, {.u_int = 0}},
        {MP_QSTR_offset_w0, MP_ARG_INT, {.u_int = 0}},
        {MP_QSTR_offset_h0, MP_ARG_INT, {.u_int = 0}},
        {MP_QSTR_offset_w1, MP_ARG_INT, {.u_int = 0}},
        {MP_QSTR_offset_h1, MP_ARG_INT, {.u_int = 0}},
        {MP_QSTR_rst, MP_ARG_INT, {.u_int = 37}},
        {MP_QSTR_dcx, MP_ARG_INT, {.u_int = 38}},
        {MP_QSTR_ss, MP_ARG_INT, {.u_int = 36}},
        {MP_QSTR_clk, MP_ARG_INT, {.u_int = 39}},
    };
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);
    if (args[ARG_color].u_obj != MP_OBJ_NULL)
    {
        if (mp_obj_is_integer(args[ARG_color].u_obj))
        {
            color = mp_obj_get_int(args[ARG_color].u_obj);
        }
        else
        {
            mp_obj_t *arg_color;
            mp_obj_get_array_fixed_n(args[ARG_color].u_obj, 3, &arg_color);
            color = COLOR_R8_G8_B8_TO_RGB565(IM_MAX(IM_MIN(mp_obj_get_int(arg_color[0]), COLOR_R8_MAX), COLOR_R8_MIN),
                                             IM_MAX(IM_MIN(mp_obj_get_int(arg_color[1]), COLOR_G8_MAX), COLOR_G8_MIN),
                                             IM_MAX(IM_MIN(mp_obj_get_int(arg_color[2]), COLOR_B8_MAX), COLOR_B8_MIN));
        }
    }

    switch (args[ARG_type].u_int)
    {
    case LCD_NONE:
        return mp_const_none;
    case LCD_SHIELD:
    {
        type = LCD_SHIELD;

        py_lcd_config_t lcd_cfg;
        lcd_cfg.rst_pin = args[ARG_rst].u_int;
        lcd_cfg.dcx_pin = args[ARG_dcx].u_int;
        lcd_cfg.cs_pin = args[ARG_ss].u_int;
        lcd_cfg.clk_pin = args[ARG_clk].u_int;

        lcd_cfg.width = args[ARG_width].u_int;
        lcd_cfg.height = args[ARG_height].u_int;
        lcd_cfg.invert = args[ARG_invert].u_int;

        lcd_cfg.offset_w0 = args[ARG_offset_w0].u_int;
        lcd_cfg.offset_h0 = args[ARG_offset_h0].u_int;

        lcd_cfg.offset_w1 = args[ARG_offset_w1].u_int;
        lcd_cfg.offset_h1 = args[ARG_offset_h1].u_int;

        lcd_cfg.dir = 0;
        py_lcd_load_config(&lcd_cfg);

        width_conf = lcd_cfg.width;
        height_conf = lcd_cfg.height;
        width_curr = width_conf;
        height_curr = height_conf;

        invert = (lcd_cfg.invert == 1) ? true : false;

        // backlight_init = false;
        fpioa_set_function(lcd_cfg.rst_pin, FUNC_GPIOHS0 + RST_GPIONUM);
        fpioa_set_function(lcd_cfg.dcx_pin, FUNC_GPIOHS0 + DCX_GPIONUM);
        fpioa_set_function(lcd_cfg.cs_pin, FUNC_SPI0_SS0 + LCD_SPI_SLAVE_SELECT);
        fpioa_set_function(lcd_cfg.clk_pin, FUNC_SPI0_SCLK);

        // mp_printf(&mp_plat_print, "[%d]: lcd_cfg.offset_x1=%d, offset_y1=%d, offset_x2=%d, offset_y2=%d
        //     width_curr=%d, height_curr=%d, invert=%d, lcd_type=%d\r\n", __LINE__,
        //     lcd_cfg.offset_x1, lcd_cfg.offset_y1, lcd_cfg.offset_x2, lcd_cfg.offset_y2,
        //     width_curr, height_curr, invert, lcd_cfg.lcd_type);
        if (lcd_cfg.lcd_type == LCD_TYPE_ILI9486)
        {
            lcd_preinit_register_handler(&lcd_init_sequence_for_ili9486);
        }
        ret = lcd_init(args[ARG_freq].u_int, true, lcd_cfg.offset_w0, lcd_cfg.offset_h0,
                       lcd_cfg.offset_w1, lcd_cfg.offset_h1, invert, lcd_cfg.dir, width_curr, height_curr);
        if (0 != lcd_cfg.dir)
        {
            lcd_set_direction(lcd_cfg.dir);
        }
        lcd_clear(color);
        break;
    }
    case LCD_M5STICK:
    {
        width_conf = args[ARG_width].u_int;
        height_conf = args[ARG_height].u_int;
        width_curr = width_conf;
        height_curr = height_conf;
        type = LCD_M5STICK;
        fpioa_set_function(21, FUNC_GPIOHS0 + RST_GPIONUM);
        fpioa_set_function(20, FUNC_GPIOHS0 + DCX_GPIONUM);
        fpioa_set_function(22, FUNC_SPI0_SS0 + LCD_SPI_SLAVE_SELECT);
        fpioa_set_function(19, FUNC_SPI0_SCLK);
        fpioa_set_function(18, FUNC_SPI0_D0);
        ret = lcd_init(args[ARG_freq].u_int, false, 52, 40, 40, 52, true, 0, width_curr, height_curr);
        lcd_clear(color);
        break;
    }
    case LCD_TWATCH:
    {
        width_conf = args[ARG_width].u_int;
        height_conf = args[ARG_height].u_int;
        width_curr = width_conf;
        height_curr = height_conf;
        type = LCD_TWATCH;
        fpioa_set_function(37, FUNC_GPIOHS0 + RST_GPIONUM);
        fpioa_set_function(38, FUNC_GPIOHS0 + DCX_GPIONUM);
        fpioa_set_function(36, FUNC_SPI0_SS0 + LCD_SPI_SLAVE_SELECT);
        fpioa_set_function(39, FUNC_SPI0_SCLK);
        ret = lcd_init(args[ARG_freq].u_int, true, 0, 0, 0, 0, true, 0, width_curr, height_curr);
        lcd_clear(color);
        break;
    }
    case LCD_IPS_240X240:
    {
        uint16_t offset_w0 = 0, offset_h0 = 0, offset_w1 = 0, offset_h1 = 0;
        width_conf = 240;
        height_conf = 240;
        width_curr = width_conf;
        height_curr = height_conf;
        type = LCD_IPS_240X240;
        // backlight_init = false;
        fpioa_set_function(37, FUNC_GPIOHS0 + RST_GPIONUM);
        fpioa_set_function(38, FUNC_GPIOHS0 + DCX_GPIONUM);
        fpioa_set_function(36, FUNC_SPI0_SS0 + LCD_SPI_SLAVE_SELECT);
        fpioa_set_function(39, FUNC_SPI0_SCLK);
        if (args[ARG_offset_w0].u_int == -1)
            offset_w0 = 0;
        if (args[ARG_offset_h0].u_int == -1)
            offset_h0 = 0;
        if (args[ARG_offset_w1].u_int == -1)
            offset_w1 = 80;
        if (args[ARG_offset_h1].u_int == -1)
            offset_h1 = 0;
        ret = lcd_init(args[ARG_freq].u_int, true, offset_w0, offset_h0,
                       offset_w1, offset_h1, args[ARG_invert].u_int == 0 ? false : true, 0, width_curr, height_curr);
        lcd_clear(color);
        break;
    }
    default:
        mp_raise_ValueError("type error");
        break;
    }
    if (ret != 0)
    {
        width_conf = 0;
        height_conf = 0;
        width_curr = 0;
        height_curr = 0;
        mp_raise_OSError(ret);
    }
    rotation = 0;
    invert = 0;
    return mp_const_none;
}

static mp_obj_t py_lcd_direction(mp_obj_t dir_obj)
{
    int dir = mp_obj_get_int(dir_obj);
    lcd_set_direction(dir);
    return mp_const_none;
}

static mp_obj_t py_lcd_write_register(mp_obj_t addr_obj, mp_obj_t data_obj)
{
    uint8_t addr = mp_obj_get_int(addr_obj);
    tft_write_command(addr);
    if (mp_obj_is_integer(data_obj)) {
        uint8_t data = mp_obj_get_int(data_obj);
        tft_write_byte(&data, 1);
    }
    if(&mp_type_list == mp_obj_get_type(data_obj))
    {
        size_t len;
        mp_obj_t *items;
        mp_obj_list_get(data_obj, &len, &items);
        for (mp_int_t i = 0; i < len; i++) {
            mp_obj_t obj = items[i];
            uint8_t data = mp_obj_get_int(obj);
            tft_write_byte(&data, 1);
        }
    }
    return mp_const_none;
}

static mp_obj_t py_lcd_width()
{
    if (type == LCD_NONE)
        return mp_const_none;
    return mp_obj_new_int(width_curr);
}

static mp_obj_t py_lcd_height()
{
    if (type == LCD_NONE)
        return mp_const_none;
    return mp_obj_new_int(height_curr);
}

static mp_obj_t py_lcd_type()
{
    if (type == LCD_NONE)
        return mp_const_none;
    return mp_obj_new_int(type);
}

static mp_obj_t py_lcd_set_backlight(mp_obj_t state_obj)
{
    return mp_const_none;
}

static mp_obj_t py_lcd_get_backlight()
{
    return mp_const_none;
}

static mp_obj_t py_lcd_display(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args)
{
    image_t *arg_img = py_image_cobj(args[0]);
    PY_ASSERT_TRUE_MSG(IM_IS_MUTABLE(arg_img), "Image format is not supported.");

    rectangle_t rect;
    // uint16_t x,y;
    point_t oft;
    int is_cut;
    int l_pad = 0, r_pad = 0;
    int t_pad = 0, b_pad = 0;
    py_helper_keyword_rectangle_roi(arg_img, n_args, args, 1, kw_args, &rect);
    py_helper_keyword_xy(arg_img, n_args, args, 2, kw_args, MP_OBJ_NEW_QSTR(MP_QSTR_oft), &oft);

    // Fit X. bigger or smaller, cut or pad to center
    if (oft.x < 0 || oft.y < 0)
    {
        if (rect.w > width_curr)
        {
            int adjust = rect.w - width_curr;
            rect.w -= adjust;
            rect.x += adjust / 2;
        }
        else if (rect.w < width_curr)
        {
            int adjust = width_curr - rect.w;
            l_pad = adjust / 2;
            r_pad = (adjust + 1) / 2;
        }
        // Fit Y. bigger or smaller, cut or pad to center
        if (rect.h > height_curr)
        {
            int adjust = rect.h - height_curr;
            rect.h -= adjust;
            rect.y += adjust / 2;
        }
        else if (rect.h < height_curr)
        {
            int adjust = height_curr - rect.h;
            t_pad = adjust / 2;
            b_pad = (adjust + 1) / 2;
        }
    }
    else
    {
        l_pad = oft.x;
        t_pad = oft.y;
    }
    is_cut = ((rect.x != 0) || (rect.y != 0) ||
              (rect.w != arg_img->w) || (rect.h != arg_img->h));
    switch (type)
    {
    case LCD_NONE:
        return mp_const_none;
    case LCD_SHIELD:
    case LCD_M5STICK:
    case LCD_TWATCH:
    case LCD_IPS_240X240:
        //fill pad
        if (oft.x < 0 || oft.y < 0)
        {
            lcd_fill_rectangle(0, 0, width_curr, t_pad, BLACK);
            lcd_fill_rectangle(0, height_curr - b_pad, width_curr, height_curr, BLACK);
            lcd_fill_rectangle(0, t_pad, l_pad, height_curr - b_pad, BLACK);
            lcd_fill_rectangle(width_curr - r_pad, t_pad, width_curr, height_curr - b_pad, BLACK);
        }
        if (is_cut)
        {
            //cut from img
            if (IM_IS_GS(arg_img))
            {
                lcd_draw_pic_grayroi(l_pad, t_pad, arg_img->w, arg_img->h,
                                     rect.x, rect.y, rect.w, rect.h, (uint8_t *)(arg_img->pixels));
            }
            else
            {
                lcd_draw_pic_roi(l_pad, t_pad, arg_img->w, arg_img->h,
                                 rect.x, rect.y, rect.w, rect.h, (uint32_t *)(arg_img->pixels));
            }
        }
        else
        {
            //no cut
            if (IM_IS_GS(arg_img))
            {
                lcd_draw_pic_gray(l_pad, t_pad, rect.w, rect.h, (uint8_t *)(arg_img->pixels));
            }
            else
            {
                lcd_draw_picture(l_pad, t_pad, rect.w, rect.h, (uint32_t *)(arg_img->pixels));
            }
        }
        return mp_const_none;
    }
    return mp_const_none;
}

static mp_obj_t py_lcd_clear(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args)
{
    uint16_t color = BLACK;
    if (n_args >= 1)
    {
        if (mp_obj_is_integer(pos_args[0]))
        {
            color = mp_obj_get_int(pos_args[0]);
        }
        else
        {
            mp_obj_t *arg_color;
            mp_obj_get_array_fixed_n(pos_args[0], 3, &arg_color);
            color = COLOR_R8_G8_B8_TO_RGB565(IM_MAX(IM_MIN(mp_obj_get_int(arg_color[0]), COLOR_R8_MAX), COLOR_R8_MIN),
                                             IM_MAX(IM_MIN(mp_obj_get_int(arg_color[1]), COLOR_G8_MAX), COLOR_G8_MIN),
                                             IM_MAX(IM_MIN(mp_obj_get_int(arg_color[2]), COLOR_B8_MAX), COLOR_B8_MIN));
        }
    }
    switch (type)
    {
    case LCD_NONE:
        return mp_const_none;
    case LCD_SHIELD:
    case LCD_M5STICK:
    case LCD_TWATCH:
    case LCD_IPS_240X240:
        lcd_clear(color);
        return mp_const_none;
    }
    return mp_const_none;
}

STATIC bool check_rotation(mp_int_t r)
{
    if (r > 3 || r < 0)
        return false;
    return true;
}

STATIC bool check_invert(mp_int_t i)
{
    return (i == 0) || (i == 1);
}

void update_offset(uint8_t type, uint8_t rotation)
{
    if (type == LCD_IPS_240X240)
    {
        switch (rotation)
        {
        case 0:
            lcd_set_offset(80, 0);
            break;
        case 1:
            lcd_set_offset(0, 0);
            break;
        case 2:
            lcd_set_offset(0, 0);
            break;
        case 3:
            lcd_set_offset(0, 80);
            break;
        }
    }
}

STATIC uint16_t getValueByRotation(uint8_t rotation)
{
    uint16_t v = DIR_YX_RLDU;
    switch (rotation)
    {
    case 0:
        v = DIR_YX_RLDU;
        break;
    case 1:
        v = DIR_XY_RLUD;
        break;
    case 2:
        v = DIR_YX_LRUD;
        break;
    case 3:
        v = DIR_XY_LRDU;
        break;
    }
    return v;
}

STATIC void lcd_set_invert_helper()
{
    switch (rotation)
    {
    case 0:
        screen_dir = DIR_YX_RLUD;
        break;
    case 1:
        screen_dir = DIR_XY_LRUD;
        break;
    case 2:
        screen_dir = DIR_YX_LRDU;
        break;
    case 3:
        screen_dir = DIR_XY_RLDU;
        break;
    }
    lcd_set_direction((lcd_dir_t)screen_dir);
}

STATIC mp_obj_t py_lcd_rotation(size_t n_args, const mp_obj_t *args)
{
    if (n_args == 0)
        goto end;
    mp_int_t r = mp_obj_get_int(args[0]);
    if (r != rotation)
    {
        if (!check_rotation(r))
            mp_raise_ValueError("value:[0,3]");
        rotation = r;
        switch (rotation)
        {
        case 0:
        case 2:
            width_curr = width_conf;
            height_curr = height_conf;
            break;
        case 1:
        case 3:
            width_curr = height_conf;
            height_curr = width_conf;
            break;
        }
        if (invert)
            lcd_set_invert_helper();
        else
        {
            screen_dir = getValueByRotation(rotation);
            lcd_set_direction((lcd_dir_t)screen_dir);
        }
        update_offset(type, rotation);
    }
end:
    return mp_obj_new_int(rotation);
}

STATIC mp_obj_t py_lcd_invert(size_t n_args, const mp_obj_t *args)
{
    if (n_args == 0)
        goto end;
    mp_int_t i = mp_obj_get_int(args[0]);
    if (i != invert)
    {
        if (!check_invert(i))
            mp_raise_ValueError("True or False");
        invert = i ? true : false;
        if (invert)
        {
            lcd_set_invert_helper();
            update_offset(type, rotation);
        }
    }
end:
    return mp_obj_new_int(invert);
}

extern void imlib_draw_ascii_string(image_t *img, int x_off, int y_off, const char *str, int c, float scale, int x_spacing, int y_spacing, bool mono_space);
STATIC mp_obj_t py_lcd_draw_string(size_t n_args, const mp_obj_t *args)
{
    uint8_t* str_buf = NULL;
    char* str_cut = NULL;
    if (width_conf == 0 || height_conf == 0)
        mp_raise_msg(&mp_type_ValueError, "not init");
    str_buf = (uint8_t *)malloc(width_conf / 8 * 12 * 8 * 2);
    if (!str_buf)
        mp_raise_OSError(MP_ENOMEM);
    str_cut = (char *)malloc(width_conf / 8 + 1);
    if (!str_cut)
    {
        free(str_buf);
        mp_raise_OSError(MP_ENOMEM);
    }

    uint16_t x0 = mp_obj_get_int(args[0]);
    uint16_t y0 = mp_obj_get_int(args[1]);
    const char *str = mp_obj_str_get_str(args[2]);
    uint16_t fontc = RED;
    uint16_t bgc = BLACK;
    if (str == NULL)
        return mp_const_none;
    if (x0 >= width_conf || y0 > height_conf - 16)
        return mp_const_none;
    int len = strlen(str);
    int width, height;
    if (n_args >= 4)
        fontc = mp_obj_get_int(args[3]);
    if (n_args >= 5)
        bgc = mp_obj_get_int(args[4]);
    if (len > (width_conf - x0) / 8)
        len = (width_conf - x0) / 8;
    if (len <= 0)
        return mp_const_none;
    memcpy(str_cut, str, len);
    str_cut[len] = 0;
    width = len * 8;
    height = 12;
    image_t arg_img = {
        .bpp = IMAGE_BPP_RGB565,
        .w = width,
        .h = height,
        .pixels = str_buf
    };
    for(int i=0; i< width*height; ++i)
    {
        *(uint16_t*)(str_buf + i*2) = (uint16_t)bgc;
    }
    imlib_draw_ascii_string(&arg_img, 0, 0, str_cut,
                      fontc, 1, 0, 0,
                      true);
    lcd_draw_picture(x0, y0, width, height, (uint32_t*)str_buf);
    free(str_buf);
    free(str_cut);
    return mp_const_none;
}

STATIC mp_obj_t py_lcd_freq(size_t n_args, const mp_obj_t *pos_args)
{
    mp_int_t freq = lcd_get_freq();
    if (n_args >= 1)
    {
        freq = mp_obj_get_int(pos_args[0]);
        lcd_set_freq(freq);
    }
    return mp_obj_new_int(freq);
}

STATIC MP_DEFINE_CONST_FUN_OBJ_KW(py_lcd_init_obj, 0, py_lcd_init);
STATIC MP_DEFINE_CONST_FUN_OBJ_0(py_lcd_deinit_obj, py_lcd_deinit);
STATIC MP_DEFINE_CONST_FUN_OBJ_0(py_lcd_width_obj, py_lcd_width);
STATIC MP_DEFINE_CONST_FUN_OBJ_0(py_lcd_height_obj, py_lcd_height);
STATIC MP_DEFINE_CONST_FUN_OBJ_0(py_lcd_type_obj, py_lcd_type);
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(py_lcd_freq_obj, 0, 1, py_lcd_freq);
STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_lcd_set_backlight_obj, py_lcd_set_backlight);
STATIC MP_DEFINE_CONST_FUN_OBJ_0(py_lcd_get_backlight_obj, py_lcd_get_backlight);
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(py_lcd_display_obj, 1, py_lcd_display);
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(py_lcd_clear_obj, 0, py_lcd_clear);
STATIC MP_DEFINE_CONST_FUN_OBJ_1(py_lcd_direction_obj, py_lcd_direction);
STATIC MP_DEFINE_CONST_FUN_OBJ_2(py_lcd_write_register_obj, py_lcd_write_register);
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(py_lcd_rotation_obj, 0, 1, py_lcd_rotation);
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(py_lcd_invert_obj, 0, 1, py_lcd_invert);
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(py_lcd_draw_string_obj, 3, 5, py_lcd_draw_string);

static const mp_map_elem_t globals_dict_table[] = {
    {MP_OBJ_NEW_QSTR(MP_QSTR___name__), MP_OBJ_NEW_QSTR(MP_QSTR_lcd)},
    {MP_OBJ_NEW_QSTR(MP_QSTR_init), (mp_obj_t)&py_lcd_init_obj},
    {MP_OBJ_NEW_QSTR(MP_QSTR_deinit), (mp_obj_t)&py_lcd_deinit_obj},
    {MP_OBJ_NEW_QSTR(MP_QSTR_width), (mp_obj_t)&py_lcd_width_obj},
    {MP_OBJ_NEW_QSTR(MP_QSTR_height), (mp_obj_t)&py_lcd_height_obj},
    {MP_OBJ_NEW_QSTR(MP_QSTR_type), (mp_obj_t)&py_lcd_type_obj},
    {MP_OBJ_NEW_QSTR(MP_QSTR_freq), (mp_obj_t)&py_lcd_freq_obj},
    {MP_OBJ_NEW_QSTR(MP_QSTR_set_backlight), (mp_obj_t)&py_lcd_set_backlight_obj},
    {MP_OBJ_NEW_QSTR(MP_QSTR_get_backlight), (mp_obj_t)&py_lcd_get_backlight_obj},
    {MP_OBJ_NEW_QSTR(MP_QSTR_display), (mp_obj_t)&py_lcd_display_obj},
    {MP_OBJ_NEW_QSTR(MP_QSTR_clear), (mp_obj_t)&py_lcd_clear_obj},
    {MP_OBJ_NEW_QSTR(MP_QSTR_direction), (mp_obj_t)&py_lcd_direction_obj},
    {MP_OBJ_NEW_QSTR(MP_QSTR_rotation), (mp_obj_t)&py_lcd_rotation_obj},
    {MP_OBJ_NEW_QSTR(MP_QSTR_mirror), (mp_obj_t)&py_lcd_invert_obj},
    {MP_OBJ_NEW_QSTR(MP_QSTR_register), (mp_obj_t)&py_lcd_write_register_obj},
    {MP_OBJ_NEW_QSTR(MP_QSTR_draw_string), (mp_obj_t)&py_lcd_draw_string_obj},
    {MP_OBJ_NEW_QSTR(MP_QSTR_XY_RLUD), MP_OBJ_NEW_SMALL_INT(DIR_XY_RLUD)},
    {MP_OBJ_NEW_QSTR(MP_QSTR_YX_RLUD), MP_OBJ_NEW_SMALL_INT(DIR_YX_RLUD)},
    {MP_OBJ_NEW_QSTR(MP_QSTR_XY_LRUD), MP_OBJ_NEW_SMALL_INT(DIR_XY_LRUD)},
    {MP_OBJ_NEW_QSTR(MP_QSTR_YX_LRUD), MP_OBJ_NEW_SMALL_INT(DIR_YX_LRUD)},
    {MP_OBJ_NEW_QSTR(MP_QSTR_XY_RLDU), MP_OBJ_NEW_SMALL_INT(DIR_XY_RLDU)},
    {MP_OBJ_NEW_QSTR(MP_QSTR_YX_RLDU), MP_OBJ_NEW_SMALL_INT(DIR_YX_RLDU)},
    {MP_OBJ_NEW_QSTR(MP_QSTR_XY_LRDU), MP_OBJ_NEW_SMALL_INT(DIR_XY_LRDU)},
    {MP_OBJ_NEW_QSTR(MP_QSTR_YX_LRDU), MP_OBJ_NEW_SMALL_INT(DIR_YX_LRDU)},
    {MP_OBJ_NEW_QSTR(MP_QSTR_BLACK), MP_OBJ_NEW_SMALL_INT(BLACK)},
    {MP_OBJ_NEW_QSTR(MP_QSTR_NAVY), MP_OBJ_NEW_SMALL_INT(NAVY)},
    {MP_OBJ_NEW_QSTR(MP_QSTR_DARKGREEN), MP_OBJ_NEW_SMALL_INT(DARKGREEN)},
    {MP_OBJ_NEW_QSTR(MP_QSTR_DARKCYAN), MP_OBJ_NEW_SMALL_INT(DARKCYAN)},
    {MP_OBJ_NEW_QSTR(MP_QSTR_MAROON), MP_OBJ_NEW_SMALL_INT(MAROON)},
    {MP_OBJ_NEW_QSTR(MP_QSTR_PURPLE), MP_OBJ_NEW_SMALL_INT(PURPLE)},
    {MP_OBJ_NEW_QSTR(MP_QSTR_OLIVE), MP_OBJ_NEW_SMALL_INT(OLIVE)},
    {MP_OBJ_NEW_QSTR(MP_QSTR_LIGHTGREY), MP_OBJ_NEW_SMALL_INT(LIGHTGREY)},
    {MP_OBJ_NEW_QSTR(MP_QSTR_DARKGREY), MP_OBJ_NEW_SMALL_INT(DARKGREY)},
    {MP_OBJ_NEW_QSTR(MP_QSTR_BLUE), MP_OBJ_NEW_SMALL_INT(BLUE)},
    {MP_OBJ_NEW_QSTR(MP_QSTR_GREEN), MP_OBJ_NEW_SMALL_INT(GREEN)},
    {MP_OBJ_NEW_QSTR(MP_QSTR_CYAN), MP_OBJ_NEW_SMALL_INT(CYAN)},
    {MP_OBJ_NEW_QSTR(MP_QSTR_RED), MP_OBJ_NEW_SMALL_INT(RED)},
    {MP_OBJ_NEW_QSTR(MP_QSTR_MAGENTA), MP_OBJ_NEW_SMALL_INT(MAGENTA)},
    {MP_OBJ_NEW_QSTR(MP_QSTR_YELLOW), MP_OBJ_NEW_SMALL_INT(YELLOW)},
    {MP_OBJ_NEW_QSTR(MP_QSTR_WHITE), MP_OBJ_NEW_SMALL_INT(WHITE)},
    {MP_OBJ_NEW_QSTR(MP_QSTR_ORANGE), MP_OBJ_NEW_SMALL_INT(ORANGE)},
    {MP_OBJ_NEW_QSTR(MP_QSTR_GREENYELLOW), MP_OBJ_NEW_SMALL_INT(GREENYELLOW)},
    {MP_OBJ_NEW_QSTR(MP_QSTR_PINK), MP_OBJ_NEW_SMALL_INT(PINK)},

    {NULL, NULL},
};

STATIC MP_DEFINE_CONST_DICT(globals_dict, globals_dict_table);

const mp_obj_module_t lcd_module = {
    .base = {&mp_type_module},
    .globals = (mp_obj_t)&globals_dict,
};
