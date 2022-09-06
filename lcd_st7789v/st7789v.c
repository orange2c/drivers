#include "linux/module.h"  //驱动模块基本头文件
#include "linux/types.h"	//一些类型定义
#include "linux/fs.h"	//fops，文件接口必须
#include "linux/spi/spi.h"	//spi子系统的头文件
#include "linux/fb.h"
#include "linux/cdev.h"	//cdev，创建登记驱动节点时需要
#include "linux/gpio/consumer.h"
#include <linux/delay.h>
#include <linux/timex.h>
#include "linux/of.h"

// #define lcd.width 320
// #define lcd.hight 240

/*
设备树预留节点
rotate:	角度,顺时针
width:	宽度，以旋转之后的界面方向
hight:	高度
color_Inversion:	如果有此定义，则启用反色。
st7789v_debug:	如果有此定义，则不创建fb节点，改为在屏幕上输出一副测试图像

*/

struct 
{
	int rotate;	//旋转角度,顺时针
	int width;	//宽度，以旋转之后的方向看
	int hight;	//高度
	int big_side; //记录较长那条边的数值（宽跟高谁大）

	bool color_Inversion; //是否反色
	bool st7789v_debug; //是否进入debug，只输出一张测试图片而不创建fb节点

	struct spi_device *spi;
	struct spi_message msg;
	struct gpio_desc *pin_reset;
	struct gpio_desc *pin_dc;

}lcd;


/***********************spi发送函数**************************/
char tx_data[2] = { 0 };
char rx_data[2] = { 0 };
struct spi_transfer transfer = 
{
	.tx_buf = tx_data,
	.rx_buf = rx_data,
	.len = 1,
	.bits_per_word = 8,
	.cs_change_delay = 2,
};
void spi_transfer_go( void )
{
	spi_message_init( &lcd.msg ); //对message进行初始化
	spi_message_add_tail( &transfer, &lcd.msg ); 
	spi_sync( lcd.spi, &lcd.msg ); //同步，阻塞发送，不触发回调
}
void spi_send_byte( uint8_t data )
{
	tx_data[0] = data;
	transfer.len = 1;
	transfer.tx_buf = tx_data,
	transfer.rx_buf = rx_data,
	spi_transfer_go();
};
void lcd_write_command(uint8_t data)
{
	gpiod_set_value( lcd.pin_dc, 0 );
	spi_send_byte( data );
}
void lcd_write_data(uint8_t data)
{
	gpiod_set_value( lcd.pin_dc, 1 );
	spi_send_byte( data );
}

void lcd_write_buf(uint16_t *buf, size_t count )
{
	gpiod_set_value( lcd.pin_dc, 1 );
	spi_write(lcd.spi,  buf, count*2 );
}
void lcd_reset(void)
{
	gpiod_set_value( lcd.pin_reset, 1 );
	msleep( 1 );
	gpiod_set_value( lcd.pin_reset, 0 );
	msleep( 1 );
	gpiod_set_value( lcd.pin_reset, 1 );
	msleep( 1 );
}


void lcd_set_address(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2)
{
    lcd_write_command(0x2a);
	lcd_write_data( x1>>8 );
	lcd_write_data( x1 );
	lcd_write_data( x2>>8 );
	lcd_write_data( x2 );
    lcd_write_command(0x2b);
	lcd_write_data( y1>>8 );
	lcd_write_data( y1 );
	lcd_write_data( y2>>8 );
	lcd_write_data( y2 );
    lcd_write_command(0x2C);
}

uint16_t rgb565( uint8_t r, uint8_t g, uint8_t b )
{
	r *=  0xff / 0x1f;
	g *=  0xff / 0x3f;
	b *=  0xff / 0x1f;
	return ((r<<11) + (g<<5) + b);
}

void draw_check( void )
{
	uint16_t i;
	unsigned int time = jiffies;

	uint16_t *buf = kmalloc( lcd.big_side*2, GFP_KERNEL);
	uint16_t range_color = rgb565( time%7, time%11, time%19 ) ; 
	uint16_t top_left_color = rgb565( 0xff, 0, 0xff);
	uint16_t around_color = rgb565( 0xff, 0, 0);
	uint16_t center_color = rgb565( 0, 0xff, 0);

	//全屏变色
	for( i=0; i<lcd.width; i++ )
		buf[i] = range_color;
	lcd_set_address( 0, 0, lcd.width, lcd.hight );
	for( i=0; i<lcd.hight; i++ )
		lcd_write_buf( buf, lcd.width  );

	//左上角画个10*10方块
	for( i=0; i<10; i++ )
		buf[i] = top_left_color; //红色
	lcd_set_address( 10, 10, 19, 19 );
	for( i=0; i<10; i++ )
		lcd_write_buf( buf, 10  );

	//四边画一个宽度为1的边框
	for( i=0; i<lcd.big_side; i++ )
		buf[i] = around_color; 
	lcd_set_address( 0, 0, lcd.width, 0 );
	lcd_write_buf( buf, lcd.width  );
	lcd_set_address( 0, lcd.hight-1, lcd.width, lcd.hight-1 );
	lcd_write_buf( buf, lcd.width  );
	lcd_set_address( 0, 0, 0, lcd.hight );
	lcd_write_buf( buf, lcd.hight  );
	lcd_set_address( lcd.width-1, 0, lcd.width, lcd.hight-1 );
	lcd_write_buf( buf, lcd.width  );

	for( i=0; i<20; i++ )
		buf[i] = center_color;
	lcd_set_address( (lcd.width/2)-9, lcd.hight/2, (lcd.width/2)+9, lcd.hight/2 );
	lcd_write_buf( buf, 20 );
	lcd_set_address( lcd.width/2, (lcd.hight/2)-9, lcd.width/2, (lcd.hight/2)+9 );
	lcd_write_buf( buf, 20 );

	kfree(buf);
}


void st7789_reg_init( struct spi_device *spi )
{
	lcd_reset();
	if( lcd.color_Inversion )
		lcd_write_command(0x21); //开启反色
	else
		lcd_write_command(0x20); //关闭反色

    lcd_write_command(0x36); //设置buf方向顺序
	switch( lcd.rotate )
	{
		case 0:	
			lcd_write_data(0x00); //标准方向
			break;
			
		case 90:
			lcd_write_data(0x60); //90度旋转
			break;

		case 180:
			lcd_write_data(0xc0); //180度旋转
			break;

		case 270:
			lcd_write_data(0xa0); //270度
			break;
	// lcd_write_data(0x80); //y轴镜像
	// lcd_write_data(0x40); //x轴镜像
	}

    lcd_write_command(0x11); //退出睡眠模式
	
    /* RGB 5-6-5-bit  */
    lcd_write_command(0x3A);
    lcd_write_data(0x65);
    /* Porch Setting */

	lcd_write_command(0xB0);
	lcd_write_data(0x00);
	lcd_write_data(0xf8); //设置为LSB模式,以应对芯片是小端模式带来的问题

    lcd_write_command(0xB2);
    lcd_write_data(0x0C);
    lcd_write_data(0x0C);
    lcd_write_data(0x00);
    lcd_write_data(0x33);
    lcd_write_data(0x33);
    /* Gate Control */
    lcd_write_command(0xB7);
    lcd_write_data(0x35);
    /* VCOM Setting */
    lcd_write_command(0xBB);
    lcd_write_data(0x19);
    /* LCM Control */
    lcd_write_command(0xC0);
    lcd_write_data(0x2C);
    /* VDV and VRH Command Enable */
    lcd_write_command(0xC2);
    lcd_write_data(0x01);
    /* VRH Set */
    lcd_write_command(0xC3);
    lcd_write_data(0x12);
    /* VDV Set */
    lcd_write_command(0xC4);
    lcd_write_data(0x20);
    /* Frame Rate Control in Normal Mode */
    lcd_write_command(0xC6);
    lcd_write_data(0x0F);
    /* Power Control 1 */
    lcd_write_command(0xD0);
    lcd_write_data(0xA4);
    lcd_write_data(0xA1);
    /* Positive Voltage Gamma Control */
    lcd_write_command(0xE0);
    lcd_write_data(0xD0);
    lcd_write_data(0x04);
    lcd_write_data(0x0D);
    lcd_write_data(0x11);
    lcd_write_data(0x13);
    lcd_write_data(0x2B);
    lcd_write_data(0x3F);
    lcd_write_data(0x54);
    lcd_write_data(0x4C);
    lcd_write_data(0x18);
    lcd_write_data(0x0D);
    lcd_write_data(0x0B);
    lcd_write_data(0x1F);
    lcd_write_data(0x23);
    /* Negative Voltage Gamma Control */
    lcd_write_command(0xE1);
    lcd_write_data(0xD0);
    lcd_write_data(0x04);
    lcd_write_data(0x0C);
    lcd_write_data(0x11);
    lcd_write_data(0x13);
    lcd_write_data(0x2C);
    lcd_write_data(0x3F);
    lcd_write_data(0x44);
    lcd_write_data(0x51);
    lcd_write_data(0x2F);
    lcd_write_data(0x1F);
    lcd_write_data(0x1F);
    lcd_write_data(0x20);
    lcd_write_data(0x23);
    /* Display Inversion On */

	lcd_write_command(0x29);
	
}

static inline unsigned int chan_to_field(unsigned int chan, struct fb_bitfield *bf) 
{ 
    chan &= 0xffff; 
    chan >>= 16 - bf->length; 
    return chan << bf->offset; 
}
static u32 pseudo_palette[16];  //调色板缓存区
static int myfb_setcolreg(u32 regno, u32 red,u32 green, u32 blue,u32 transp, struct fb_info *info) 
{ 
    unsigned int val; 
    if (regno > 16) 
    { 
        return 1; 
    } 
    val  = chan_to_field(red,   &info->var.red); 
    val |= chan_to_field(green, &info->var.green); 
    val |= chan_to_field(blue,  &info->var.blue); 
    pseudo_palette[regno] = val; 
    return 0; 
} 
struct fb_info *myfb;
struct fb_ops myops = 
{
	.owner = THIS_MODULE,
	.fb_write       = fb_sys_write,
	.fb_fillrect    = sys_fillrect,    /*用像素行填充矩形框，通用库函数*/ 
    .fb_copyarea    = sys_copyarea,    /*将屏幕的一个矩形区域复制到另一个区域，通用库函数*/ 
    .fb_imageblit   = sys_imageblit,   /*显示一副图像，通用库函数*/
	.fb_setcolreg   = myfb_setcolreg,  /*设置颜色寄存器*/

};
struct fb_var_screeninfo myvar = 
{
	.rotate = 90,

	.bits_per_pixel = 16,
	.nonstd = 1,

	.red.offset = 11,
	.red.length = 5,
	.green.offset = 5,
	.green.length = 6,
	.blue.offset = 0,
	.blue.length = 5,
	.transp.offset = 0,
	.transp.length = 0,

	.activate = FB_ACTIVATE_NOW,
	.vmode = FB_VMODE_NONINTERLACED,
};
struct fb_fix_screeninfo myfix = 
{
	.type = FB_TYPE_PACKED_PIXELS,
	.visual         = FB_VISUAL_TRUECOLOR, 
    .accel          = FB_ACCEL_NONE,//没有使用硬件加速 
    .id             = "myfb",

};

static struct task_struct *fb_thread; 
int refersh( void *data ) 
{
	while(1)
	{
		 if (kthread_should_stop()) 
            break; 
		// ssleep(1);
		lcd_set_address( 0, 0, lcd.width -1 , lcd.hight -1 );
		lcd_write_buf( (uint16_t *)myfb->screen_buffer, lcd.width * lcd.hight );
	}
	return 0;
}
static void myfb_update(struct fb_info *fbi, struct list_head *pagelist) 
{ 
	printk("开始刷新\r\n");
    //比较粗暴的方式，直接全部刷新 
    fbi->fbops->fb_pan_display(&fbi->var,fbi);  //将应用层数据刷到 FrameBuffer 缓存中 
}
static struct fb_deferred_io myfb_defio = { 
    .delay           = HZ/20, 
    .deferred_io     = &myfb_update, 
};


//查找设备树本节点下的特定属性，存储到lcd结构体内
int tree_match( struct device_node *node) 
{
	uint8_t i;
	//查找引脚属性
	#define MATCH_GPIO_COUNT 2
	const char *m_gpio_str[MATCH_GPIO_COUNT] = {"reset", "dc" };
	struct gpio_desc **m_gpio_value[MATCH_GPIO_COUNT] = { &(lcd.pin_reset), &(lcd.pin_dc) };

	//查找的带数字参数的属性
	#define MATCH_NUM_COUNT 3
	const char *m_num_str[MATCH_NUM_COUNT] = {"width", "hight", "rotate" };
	int *m_num_value[MATCH_NUM_COUNT] = { &(lcd.width), &(lcd.hight), &(lcd.rotate)  };

	//查找是否存在的属性
	#define MATCH_bool_COUNT 2
	const char *m_bool_str[MATCH_bool_COUNT] = {"color_Inversion", "st7789v_debug" };
	bool *m_bool_value[MATCH_bool_COUNT] = { &(lcd.color_Inversion), &(lcd.st7789v_debug)  };
	struct property *pro;

	for( i=0; i<MATCH_GPIO_COUNT; i++  )
	{
		*(m_gpio_value[i]) = devm_gpiod_get_index( &lcd.spi->dev, m_gpio_str[i], 0, GPIOD_OUT_HIGH )  ;
		if( IS_ERR( *(m_gpio_value[i]) ) )	
		{
			printk("ERROR:%s-gpios not find", m_gpio_str[i] );
			return -ENOKEY;
		}
		else
			printk("%s-gpios is find", m_gpio_str[i] );
	}

	for( i=0; i<MATCH_NUM_COUNT; i++  )
	{
		if( of_property_read_u32_index( node, m_num_str[i], 0, m_num_value[i] ) )
		{
			printk("ERROR:%snot find\r\n", m_num_str[i]);
			return -ENOKEY;
		}
		printk("%s:%d\r\n", m_num_str[i], *(m_num_value[i]));
	}

	for( i=0; i<MATCH_bool_COUNT; i++  )
	{
		pro = of_find_property( node, m_bool_str[i], NULL );
		if(  pro == NULL )
			*(m_bool_value[i]) = false;
		else
			*(m_bool_value[i]) = true;
		printk("%s=%d\r\n", m_bool_str[i],  *(m_bool_value[i]) );
	}

	if( lcd.width > lcd.hight)
		lcd.big_side = lcd.width;
	else
		lcd.big_side = lcd.hight;

	return 0;
}

int	st7789_probe(struct spi_device *spi)
{
	int err;
	size_t gmem_size;
	uint8_t *gmem_addr;
	
	lcd.spi = spi;
	if( tree_match( spi->dev.of_node ) ) //查找设备树内所需的属性
		return -ENOKEY; 
	
	st7789_reg_init( spi ); //初始化相关寄存器
	if( lcd.st7789v_debug )
	{
		printk("debug:仅输出测试图像\r\n");
		draw_check();
		return 0;
	}

	myvar.xres = lcd.width,
	myvar.yres = lcd.hight,
	myvar.xres_virtual = lcd.width,
	myvar.yres_virtual = lcd.hight,
	myfix.line_length    = lcd.width*2, //16 bit = 2byte 
	myfb = framebuffer_alloc( sizeof( struct fb_info ), &spi->dev );

	gmem_size = lcd.width * lcd.hight * 2; //设置显存大小，16bit 占 2 字节 
    gmem_addr = kmalloc(gmem_size,GFP_KERNEL);  //分配 Frame Buffer 显存
	myfb->screen_buffer = gmem_addr;//设置显存地址
	myfb->screen_size = gmem_size;//设置显存大小
	myfb->var = myvar;
	myfb->fix = myfix;
	myfb->fbops = &myops;
	myfb->fix.smem_len = gmem_size;
	myfb->fix.smem_start = (u32)gmem_addr;

	myfb->pseudo_palette = pseudo_palette;

	myfb->fix.smem_len = gmem_size;//设置应用层显存大小 
	myfb->fix.smem_start = (u32)gmem_addr;//设置应用层数据地址

// memset((void *)myfb->fix.smem_start, 0, myfb->fix.smem_len); //内存置为0

	myfb->fbdefio = &myfb_defio; //设置刷新参数 
	fb_deferred_io_init(myfb);  //初始化刷新机制

	err = register_framebuffer( myfb );

	fb_thread= kthread_run(refersh, myfb, "fb_refersh"); 
	printk("err:%d\r\n", err);

	return 0;

}

int st7789_remove(struct spi_device *spi)
{
	printk( "驱动模块卸载\r\n" );
	if( lcd.st7789v_debug )
		return 0;

	fb_deferred_io_cleanup(myfb); //清除刷新机制
	unregister_framebuffer( myfb );
	framebuffer_release( myfb );

	return 0;
}
struct of_device_id match_table = {
	.compatible = "st7789v,orange2c",
};
struct spi_driver  st7789_driver = {
	.probe = st7789_probe,
	.remove = st7789_remove,
	.driver = {
		.name = "st7789v_driver",
		.owner = THIS_MODULE,
		.of_match_table = &match_table,
	}
};

static int __init drv_module_init( void )
{
	return spi_register_driver( &st7789_driver );
}
static void __exit drv_module_exit( void )
{
	spi_unregister_driver( &st7789_driver );
}
module_init( drv_module_init );
module_exit( drv_module_exit );
MODULE_LICENSE( "GPL" );
