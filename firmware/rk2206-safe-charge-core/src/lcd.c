

#include "iot_errno.h"
#include "iot_gpio.h"
#include "iot_spi.h"
#include "lcd.h"
#include "lcd_font.h"

#define LCD_ENABLE_SPI      1
#define LCD_SPI_BUS         ESPI0_M1

#define LCD_PIN_CS          GPIO0_PC0
#define LCD_PIN_CLK         GPIO0_PC1
#define LCD_PIN_MOSI        GPIO0_PC2
#define LCD_PIN_RES         GPIO0_PC3
#define LCD_PIN_DC          GPIO0_PA4

#define LCD_CS_Clr()        IoTGpioSetOutputVal(LCD_PIN_CS, IOT_GPIO_VALUE0)
#define LCD_CS_Set()        IoTGpioSetOutputVal(LCD_PIN_CS, IOT_GPIO_VALUE1)

#define LCD_CLK_Clr()       IoTGpioSetOutputVal(LCD_PIN_CLK, IOT_GPIO_VALUE0)
#define LCD_CLK_Set()       IoTGpioSetOutputVal(LCD_PIN_CLK, IOT_GPIO_VALUE1)

#define LCD_MOSI_Clr()      IoTGpioSetOutputVal(LCD_PIN_MOSI, IOT_GPIO_VALUE0)
#define LCD_MOSI_Set()      IoTGpioSetOutputVal(LCD_PIN_MOSI, IOT_GPIO_VALUE1)

#define LCD_RES_Clr()       IoTGpioSetOutputVal(LCD_PIN_RES, IOT_GPIO_VALUE0)
#define LCD_RES_Set()       IoTGpioSetOutputVal(LCD_PIN_RES, IOT_GPIO_VALUE1)

#define LCD_DC_Clr()        IoTGpioSetOutputVal(LCD_PIN_DC, IOT_GPIO_VALUE0)
#define LCD_DC_Set()        IoTGpioSetOutputVal(LCD_PIN_DC, IOT_GPIO_VALUE1)

#if LCD_ENABLE_SPI
IoT_SPI_InitTypeDef iot_spi = {
    .Mode = SPI_MODE_MASTER,
    .Direction = SPI_DIRECTION_1LINE_TX,
    .DataSize = SPI_DATASIZE_8BIT,
    .CLKPolarity = SPI_POLARITY_HIGH,
    .CLKPhase = SPI_PHASE_2EDGE,
    .BaudRatePrescaler = SPI_BAUDRATEPRESCALER_1,
    .FirstBit = SPI_FIRSTBIT_MSB,
};
#endif

static void lcd_write_bus(uint8_t dat)
{
#if LCD_ENABLE_SPI
    IoTSpiWrite(LCD_SPI_BUS, &dat, 1);
#else
    uint8_t i;

    LCD_CS_Clr();
    for (i=0; i<8; i++)
    {
        LCD_CLK_Clr();
        if (dat & 0x80)
        {
            LCD_MOSI_Set();
        }
        else
        {
            LCD_MOSI_Clr();
        }
        LCD_CLK_Set();
        dat<<=1;
    }
    LCD_CS_Set();
#endif
}

static void lcd_wr_data8(uint8_t dat)
{
    lcd_write_bus(dat);
}

static void lcd_wr_data(uint16_t dat)
{
    lcd_write_bus(dat >> 8);
    lcd_write_bus(dat);
}

static void lcd_wr_reg(uint8_t dat)
{
    LCD_DC_Clr();
    lcd_write_bus(dat);
    LCD_DC_Set();
}

static void lcd_address_set(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2)
{

    lcd_wr_reg(0x2a);
    lcd_wr_data(x1);
    lcd_wr_data(x2);

    lcd_wr_reg(0x2b);
    lcd_wr_data(y1);
    lcd_wr_data(y2);

    lcd_wr_reg(0x2c);
}

static uint32_t mypow(uint8_t m, uint8_t n)
{
    uint32_t result = 1;

    while (n--)
    {
        result *= m;
    }

    return result;
}

static void lcd_show_chinese_12x12(uint16_t x, uint16_t y, uint8_t *s, uint16_t fc, uint16_t bc, uint8_t sizey, uint8_t mode)
{
    uint8_t i, j, m=0;
    uint16_t k;
    uint16_t HZnum;
    uint16_t TypefaceNum;
    uint16_t x0 = x;

    TypefaceNum = (sizey/8+((sizey%8)?1:0)) * sizey;


    HZnum = sizeof(tfont12) / sizeof(typFNT_GB12);

    for (k = 0; k < HZnum; k++)
    {
        if ((tfont12[k].Index[0] == *(s)) && (tfont12[k].Index[1] == *(s+1))
            && (tfont12[k].Index[2] == *(s+2)))
        {
            lcd_address_set(x, y, x+sizey-1, y+sizey-1);
            for (i = 0; i < TypefaceNum; i++)
            {
                for (j = 0; j < 8; j++)
                {
                    if (!mode)
                    {
                        if (tfont12[k].Msk[i] & (0x01<<j))
                        {
                            lcd_wr_data(fc);
                        }
                        else
                        {
                            lcd_wr_data(bc);
                        }

                        m++;
                        if ((m % sizey) == 0)
                        {
                            m = 0;
                            break;
                        }
                    }
                    else
                    {
                        if (tfont12[k].Msk[i] & (0x01<<j))
                        {

                            lcd_draw_point(x,y,fc);
                        }

                        x++;
                        if ((x - x0) == sizey)
                        {
                            x = x0;
                            y++;
                            break;
                        }
                    }
                }
            }

            break;
        }
    }
}

static void lcd_show_chinese_16x16(uint16_t x, uint16_t y, uint8_t *s, uint16_t fc, uint16_t bc, uint8_t sizey, uint8_t mode)
{
    uint8_t i, j, m = 0;
    uint16_t k;
    uint16_t HZnum;
    uint16_t TypefaceNum;
    uint16_t x0 = x;

    TypefaceNum = (sizey/8 + ((sizey%8)?1:0)) * sizey;

    HZnum=sizeof(tfont16) / sizeof(typFNT_GB16);

    for (k = 0; k < HZnum; k++)
    {
        if ((tfont16[k].Index[0] == *(s)) && (tfont16[k].Index[1] == *(s+1))
            && (tfont16[k].Index[2] == *(s+2)))
        {
            lcd_address_set(x, y, x+sizey-1, y+sizey-1);
            for (i = 0; i < TypefaceNum; i++)
            {
                for (j = 0; j < 8; j++)
                {
                    if (!mode)
                    {
                        if (tfont16[k].Msk[i] & (0x01 << j))
                        {
                            lcd_wr_data(fc);
                        }
                        else
                        {
                            lcd_wr_data(bc);
                        }

                        m++;
                        if (m%sizey == 0)
                        {
                            m = 0;
                            break;
                        }
                    }
                    else
                    {
                        if (tfont16[k].Msk[i] & (0x01<<j))
                        {

                            lcd_draw_point(x,y,fc);
                        }

                        x++;
                        if ((x - x0) == sizey)
                        {
                            x = x0;
                            y++;
                            break;
                        }
                    }
                }
            }

            break;
        }
    }
}

static void lcd_show_chinese_24x24(uint16_t x, uint16_t y, uint8_t *s, uint16_t fc, uint16_t bc, uint8_t sizey, uint8_t mode)
{
    uint8_t i,j,m = 0;
    uint16_t k;
    uint16_t HZnum;
    uint16_t TypefaceNum;
    uint16_t x0 = x;

    TypefaceNum = (sizey/8 + ((sizey%8)?1:0)) * sizey;

    HZnum = sizeof(tfont24) / sizeof(typFNT_GB24);

    for (k = 0; k < HZnum; k++)
    {
        if ((tfont24[k].Index[0] == *(s)) && (tfont24[k].Index[1] == *(s+1))
            && (tfont24[k].Index[2] == *(s+2)))
        {
            lcd_address_set(x, y, x+sizey-1, y+sizey-1);
            for (i = 0; i < TypefaceNum; i++)
            {
                for (j = 0; j < 8; j++)
                {
                    if (!mode)
                    {
                        if (tfont24[k].Msk[i] & (0x01<<j))
                        {
                            lcd_wr_data(fc);
                        }
                        else
                        {
                            lcd_wr_data(bc);
                        }

                        m++;
                        if (m%sizey == 0)
                        {
                            m = 0;
                            break;
                        }
                    }
                    else
                    {
                        if (tfont24[k].Msk[i] & (0x01 << j))
                        {

                            lcd_draw_point(x, y, fc);
                        }

                        x++;
                        if ((x - x0) == sizey)
                        {
                            x = x0;
                            y++;
                            break;
                        }
                    }
                }
            }

            break;
        }
    }
}

static void lcd_show_chinese_32x32(uint16_t x, uint16_t y, uint8_t *s, uint16_t fc, uint16_t bc, uint8_t sizey, uint8_t mode)
{
    uint8_t i,j,m = 0;
    uint16_t k;
    uint16_t HZnum;
    uint16_t TypefaceNum;
    uint16_t x0 = x;

    TypefaceNum = (sizey/8 + ((sizey%8)?1:0)) * sizey;

    HZnum = sizeof(tfont32) / sizeof(typFNT_GB32);

    for (k = 0; k < HZnum; k++)
    {
        if ((tfont32[k].Index[0] == *(s)) && (tfont32[k].Index[1] == *(s+1))
            && (tfont32[k].Index[2] == *(s+2)))
        {
            lcd_address_set(x, y, x+sizey-1, y+sizey-1);
            for (i = 0; i < TypefaceNum; i++)
            {
                for (j = 0; j < 8; j++)
                {
                    if (!mode)
                    {
                        if (tfont32[k].Msk[i] & (0x01 << j))
                        {
                            lcd_wr_data(fc);
                        }
                        else
                        {
                            lcd_wr_data(bc);
                        }

                        m++;
                        if (m%sizey == 0)
                        {
                            m = 0;
                            break;
                        }
                    }
                    else
                    {
                        if (tfont32[k].Msk[i] & (0x01 << j))
                        {
                            lcd_draw_point(x, y, fc);
                        }

                        x++;
                        if ((x-x0) == sizey)
                        {
                            x = x0;
                            y++;
                            break;
                        }
                    }
                }
            }

            break;
        }
    }
}

unsigned int lcd_init()
{
    unsigned int ret = 0;

#if LCD_ENABLE_SPI
    ret = IoTSpiInit(LCD_SPI_BUS, &iot_spi);
    if (ret != IOT_SUCCESS)
    {
        printf("%s, %s, %d: Spi init failed!\n",
         __FILE__, __func__, __LINE__);
        return IOT_FAILURE;
    }
#else

    IoTGpioInit(LCD_PIN_CS);
    IoTGpioSetDir(LCD_PIN_CS, IOT_GPIO_DIR_OUT);
    IoTGpioSetOutputVal(LCD_PIN_CS, IOT_GPIO_VALUE1);

    IoTGpioInit(LCD_PIN_CLK);
    IoTGpioSetDir(LCD_PIN_CLK, IOT_GPIO_DIR_OUT);
    IoTGpioSetOutputVal(LCD_PIN_CLK, IOT_GPIO_VALUE0);

    IoTGpioInit(LCD_PIN_MOSI);
    IoTGpioSetDir(LCD_PIN_MOSI, IOT_GPIO_DIR_OUT);
    IoTGpioSetOutputVal(LCD_PIN_MOSI, IOT_GPIO_VALUE0);
#endif

    IoTGpioInit(LCD_PIN_RES);
    IoTGpioSetDir(LCD_PIN_RES, IOT_GPIO_DIR_OUT);
    IoTGpioSetOutputVal(LCD_PIN_RES, IOT_GPIO_VALUE1);


    IoTGpioInit(LCD_PIN_DC);
    IoTGpioSetDir(LCD_PIN_DC, IOT_GPIO_DIR_OUT);
    IoTGpioSetOutputVal(LCD_PIN_DC, IOT_GPIO_VALUE0);


    LCD_RES_Clr();
    LOS_Msleep(100);
    LCD_RES_Set();
    LOS_Msleep(100);
    LOS_Msleep(500);
    lcd_wr_reg(0x11);

    LOS_Msleep(100);

    lcd_wr_reg(0X36);
    if (USE_HORIZONTAL == 0)
    {
        lcd_wr_data8(0x00);
    }
    else if (USE_HORIZONTAL == 1)
    {
        lcd_wr_data8(0xC0);
    }
    else if (USE_HORIZONTAL == 2)
    {
        lcd_wr_data8(0x70);
    }
    else
    {
        lcd_wr_data8(0xA0);
    }
    lcd_wr_reg(0X3A);
    lcd_wr_data8(0X05);

    lcd_wr_reg(0xb2);
    lcd_wr_data8(0x0c);
    lcd_wr_data8(0x0c);
    lcd_wr_data8(0x00);
    lcd_wr_data8(0x33);
    lcd_wr_data8(0x33);
    lcd_wr_reg(0xb7);
    lcd_wr_data8(0x35);

    lcd_wr_reg(0xbb);
    lcd_wr_data8(0x35);
    lcd_wr_reg(0xc0);
    lcd_wr_data8(0x2c);
    lcd_wr_reg(0xc2);
    lcd_wr_data8(0x01);
    lcd_wr_reg(0xc3);
    lcd_wr_data8(0x13);
    lcd_wr_reg(0xc4);
    lcd_wr_data8(0x20);
    lcd_wr_reg(0xc6);
    lcd_wr_data8(0x0f);
    lcd_wr_reg(0xca);
    lcd_wr_data8(0x0f);
    lcd_wr_reg(0xc8);
    lcd_wr_data8(0x08);
    lcd_wr_reg(0x55);
    lcd_wr_data8(0x90);
    lcd_wr_reg(0xd0);
    lcd_wr_data8(0xa4);
    lcd_wr_data8(0xa1);

    lcd_wr_reg(0xe0);
    lcd_wr_data8(0xd0);
    lcd_wr_data8(0x00);
    lcd_wr_data8(0x06);
    lcd_wr_data8(0x09);
    lcd_wr_data8(0x0b);
    lcd_wr_data8(0x2a);
    lcd_wr_data8(0x3c);
    lcd_wr_data8(0x55);
    lcd_wr_data8(0x4b);
    lcd_wr_data8(0x08);
    lcd_wr_data8(0x16);
    lcd_wr_data8(0x14);
    lcd_wr_data8(0x19);
    lcd_wr_data8(0x20);
    lcd_wr_reg(0xe1);
    lcd_wr_data8(0xd0);
    lcd_wr_data8(0x00);
    lcd_wr_data8(0x06);
    lcd_wr_data8(0x09);
    lcd_wr_data8(0x0b);
    lcd_wr_data8(0x29);
    lcd_wr_data8(0x36);
    lcd_wr_data8(0x54);
    lcd_wr_data8(0x4b);
    lcd_wr_data8(0x0d);
    lcd_wr_data8(0x16);
    lcd_wr_data8(0x14);
    lcd_wr_data8(0x21);
    lcd_wr_data8(0x20);
    lcd_wr_reg(0x29);

    return 0;
}

unsigned int lcd_deinit()
{
#if LCD_ENABLE_SPI
    IoTSpiDeinit(LCD_SPI_BUS);
#else
    IoTGpioDeinit(LCD_PIN_CS);
    IoTGpioDeinit(LCD_PIN_CLK);
    IoTGpioDeinit(LCD_PIN_MOSI);
#endif
    IoTGpioDeinit(LCD_PIN_RES);
    IoTGpioDeinit(LCD_PIN_DC);

    return 0;
}

void lcd_fill(uint16_t xsta, uint16_t ysta, uint16_t xend, uint16_t yend, uint16_t color)
{
    uint16_t i, j;


    lcd_address_set(xsta, ysta, xend-1, yend-1);

    for (i = ysta; i < yend; i++)
    {
        for (j = xsta; j < xend; j++)
        {
            lcd_wr_data(color);
        }
    }
}

void lcd_draw_point(uint16_t x, uint16_t y, uint16_t color)
{

    lcd_address_set(x, y, x, y);
    lcd_wr_data(color);
}

void lcd_draw_line(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color)
{
    uint16_t t;
    int xerr=0, yerr=0, delta_x, delta_y, distance;
    int incx, incy, uRow, uCol;


    delta_x = x2 - x1;
    delta_y = y2 - y1;

    uRow = x1;
    uCol = y1;

    if (delta_x > 0)
    {

        incx = 1;
    }
    else if (delta_x == 0)
    {

        incx = 0;
    }
    else
    {
        incx = -1;
        delta_x = -delta_x;
    }

    if (delta_y > 0)
    {
        incy = 1;
    }
    else if (delta_y == 0)
    {

        incy = 0;
    }
    else
    {
        incy = -1;
        delta_y = -delta_y;
    }

    if (delta_x > delta_y)
    {

        distance = delta_x;
    }
    else
    {
        distance = delta_y;
    }

    for (t = 0; t < distance+1; t++)
    {

        lcd_draw_point(uRow, uCol, color);
        xerr += delta_x;
        yerr += delta_y;
        if (xerr > distance)
        {
            xerr -= distance;
            uRow += incx;
        }
        if (yerr > distance)
        {
            yerr -= distance;
            uCol += incy;
        }
    }
}

void lcd_draw_rectangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2,uint16_t color)
{
    lcd_draw_line(x1, y1, x2, y1, color);
    lcd_draw_line(x1, y1, x1, y2, color);
    lcd_draw_line(x1, y2, x2, y2, color);
    lcd_draw_line(x2, y1, x2, y2, color);
}

void lcd_draw_circle(uint16_t x0, uint16_t y0, uint8_t r, uint16_t color)
{
    int a, b;

    a = 0;
    b = r;

    while (a <= b)
    {
        lcd_draw_point(x0-b, y0-a, color);
        lcd_draw_point(x0+b, y0-a, color);
        lcd_draw_point(x0-a, y0+b, color);
        lcd_draw_point(x0-a, y0-b, color);
        lcd_draw_point(x0+b, y0+a, color);
        lcd_draw_point(x0+a, y0-b, color);
        lcd_draw_point(x0+a, y0+b, color);
        lcd_draw_point(x0-b, y0+a, color);
        a++;

        if ((a*a+b*b) > (r*r))
        {
            b--;
        }
    }
}

void lcd_draw_triangle(uint16_t x0, uint16_t y0,uint16_t x1, uint16_t y1,uint16_t x2, uint16_t y2,  uint16_t color){

    lcd_draw_line(x0, y0, x1, y1, color);
    lcd_draw_line(x1, y1, x2, y2, color);
    lcd_draw_line(x2, y2, x0, y0, color);

}

void lcd_show_chinese(uint16_t x, uint16_t y, uint8_t *s,
    uint16_t fc, uint16_t bc, uint8_t sizey, uint8_t mode)
{








    for (uint32_t i = 0; i < strlen(s); i += 3, x += sizey)
    {
        if (sizey == 12)
        {
            lcd_show_chinese_12x12(x, y, &s[i], fc, bc, sizey, mode);
        }
        else if (sizey == 16)
        {
            lcd_show_chinese_16x16(x, y, &s[i], fc, bc, sizey, mode);
        }
        else if (sizey == 24)
        {
            lcd_show_chinese_24x24(x, y, &s[i], fc, bc, sizey, mode);
        }
        else if (sizey == 32)
        {
            lcd_show_chinese_32x32(x, y, &s[i], fc, bc, sizey, mode);
        }
        else
        {
            return;
        }
    }

}

void lcd_show_char(uint16_t x, uint16_t y, uint8_t num, uint16_t fc, uint16_t bc, uint8_t sizey, uint8_t mode)
{
    uint8_t temp,sizex,t,m = 0;
    uint16_t i;
    uint16_t TypefaceNum;
    uint16_t x0 = x;

    sizex = sizey/2;
    TypefaceNum = (sizex/8 + ((sizex%8)?1:0)) * sizey;


    num = num-' ';

    lcd_address_set(x, y, x+sizex-1, y+sizey-1);

    for (i = 0; i < TypefaceNum; i++)
    {
        if (sizey == 12)
        {

            temp = ascii_1206[num][i];
        }
        else if (sizey == 16)
        {

            temp = ascii_1608[num][i];
        }
        else if (sizey == 24)
        {

            temp = ascii_2412[num][i];
        }
        else if (sizey == 32)
        {

            temp = ascii_3216[num][i];
        }
        else
        {
            return;
        }

        for (t = 0; t < 8; t++)
        {
            if (!mode)
            {
                if (temp & (0x01 << t))
                {
                    lcd_wr_data(fc);
                }
                else
                {
                    lcd_wr_data(bc);
                }

                m++;
                if (m%sizex == 0)
                {
                    m = 0;
                    break;
                }
            }
            else
            {
                if (temp & (0x01 << t))
                {

                    lcd_draw_point(x, y, fc);
                }

                x++;
                if ((x - x0) == sizex)
                {
                    x = x0;
                    y++;
                    break;
                }
            }
        }
    }
}

void lcd_show_string(uint16_t x, uint16_t y, const uint8_t *p, uint16_t fc, uint16_t bc, uint8_t sizey, uint8_t mode)
{
    while (*p != '\0')
    {
        lcd_show_char(x, y, *p, fc, bc, sizey, mode);
        x += (sizey / 2);
        p++;
    }
}

void lcd_show_int_num(uint16_t x, uint16_t y, uint16_t num, uint8_t len, uint16_t fc, uint16_t bc, uint8_t sizey)
{
    uint8_t t,temp;
    uint8_t enshow=0;
    uint8_t sizex = sizey / 2;

    for (t=0; t<len; t++)
    {
        temp = (num/mypow(10,len-t-1)) % 10;
        if (enshow==0 && t<(len-1))
        {
            if (temp == 0)
            {
                lcd_show_char(x+t*sizex, y, ' ', fc, bc, sizey, 0);
                continue;
            }
            else
            {
                enshow = 1;
            }
        }
        lcd_show_char(x+t*sizex, y, temp+48, fc, bc, sizey, 0);
    }
}

void lcd_show_float_num1(uint16_t x, uint16_t y,float num,uint8_t len,uint16_t fc,uint16_t bc,uint8_t sizey)
{
    uint8_t t,temp, sizex;
    uint16_t num1;

    sizex = sizey / 2;
    num1 = num * 100;
    for (t=0; t<len; t++)
    {
        temp = (num1/mypow(10,len-t-1)) % 10;
        if (t == (len-2))
        {
            lcd_show_char(x+(len-2)*sizex, y, '.', fc, bc, sizey, 0);
            t++;
            len += 1;
        }
        lcd_show_char(x+t*sizex, y, temp+48, fc, bc, sizey, 0);
    }
}

void lcd_show_picture(uint16_t x, uint16_t y, uint16_t length, uint16_t width, const uint8_t *pic)
{
    uint16_t i,j;
    uint32_t k = 0;

    lcd_address_set(x, y, x+length-1, y+width-1);
    for (i=0; i<length; i++)
    {
        for (j=0; j<width; j++)
        {
            lcd_wr_data8(pic[k*2]);
            lcd_wr_data8(pic[k*2+1]);
            k++;
        }
    }
}

void lcd_show_text(int x, int y, char *str, int fc, int bc, int font_size, int mode)
{
    char *tmp_str = str;
    int cur_x=x;
    int cur_y=y;

    while(*tmp_str != '\0')
    {
        if(tmp_str[0] > 0){

            lcd_show_char(cur_x,cur_y,tmp_str[0], fc,bc,font_size, mode);
            tmp_str++;
            cur_x += font_size/2;
        }else{

            uint8_t cn_str[4]={tmp_str[0],tmp_str[1],tmp_str[2],0};
            lcd_show_chinese(cur_x,cur_y,cn_str, fc,bc,font_size, mode);
            tmp_str +=3;
            cur_x += font_size;
        }

        if(cur_x > LCD_W-font_size)
		{
			cur_x=0;
			cur_y+=font_size;
		}
    }
}
