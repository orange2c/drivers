# 介绍
这是一个linux5.x驱动，用spi控制使用st7789v这个驱动的屏幕。


驱动加载后会创建/dev/fb0 /dev/tty0节点，使用rg565格式，fb节点里用2个字节表示一个像素，小心大小端问题。

使用方式：
1.	编写设备树
2.	将st7789v.c存放到linux项目内，并修改一些文件以使其编译进内核。我将这个步骤所需的操作都写成一个shell脚本，调用即可。
3.  在menuconfig内启用驱动选项
# 1.编写设备树
## 节点属性
- compatible	:	"st7789v,orange2c";
- dc-gpios	:	有的屏幕叫dc，有的叫rs。命令/数据引脚。
- reset-gpios	:	复位引脚
- rotate	:	旋转角度，顺时针
- width	：	宽度，以旋转之后的界面方向
- hight	：	高度
- color_Inversion	: 	如果有此定义，则启用反色。

## 设备树示例
这是我在v3s中使用spi0驱动屏幕的设备树节点
~~~
&spi0{
    status = "okay";
    st7789v: st7789v@0{
        compatible = "st7789v,orange2c";
        reg = <0>;
        status = "okay";
        spi-max-frequency = <30000000>;
        spi-cpol;
        spi-cpha;
		
        rotate	= <90>;
		width	= <320>;
		hight	= <240>;
        dc-gpios    = <&pio 1 5 GPIO_ACTIVE_HIGH>;  // PB5 
        reset-gpios = <&pio 1 4 GPIO_ACTIVE_HIGH>;  // PB4
    };
	
};
~~~

# 2.使用我的脚本
本文件夹下有一个install_st7789v.sh的脚本文件，用于自动修改linux项目下的相关文件。
## 使用
1. 将脚本与驱动文件存放到Linux项目路径下,比如我电脑的linxu项目路径是/home/work/linux
~~~
cp install_st7789v.sh /home/cb/linux
cp st7789v.c /home/work/linux
~~~
2. 在linux项目路径下执行install_st7789v.sh
~~~
sudo bash install_st7789v.sh
~~~
## sh脚本功能介绍
1.	创建文件夹 drivers/diy/st7789v，并将st7789v.c复制进去
2.  在drivers/diy和下一级的st7789v文件夹内各创建Makefile和Kconfig文件
3.  修改drivers路径下的Kconfig和Makefile，在末尾追加一句对diy文件夹内Kconfig和Makefile的引用

# 3.启用驱动
进入menuconfig，按顺序启用到st7789v这个驱动选项即可

（由于我们追加进Kconfig的引用语句位于最后，所以DIY_Drivers这个选项在Device Drivers界面内位于最后一个选项）

	Device Drivers  --->　
		[*] DIY_Drivers  --->
			[*]   lcd st7789v  
