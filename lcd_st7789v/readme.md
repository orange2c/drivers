# 介绍
这是一个linux5.x驱动，用spi控制使用st7789v这个驱动的屏幕。

使用方式：

1.	编写设备树
2.	修改linux项目的makefile文件以编译进内核，开机自动启动。为方便使用，我编写了一个脚本来自动修改linux项目下相关文件。

# 编写设备树
## 所需节点属性
compatible	:	"st7789v,orange2c";

dc-gpios	:	有的屏幕叫dc，有的叫rs。命令/数据引脚。

reset-gpios	:	复位引脚

rotate	:	旋转角度，顺时针

width	：	宽度，以旋转之后的界面方向

hight	：	高度

color_Inversion	: 	如果有此定义，则启用反色。

## 示例
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

# 使用我的脚本
本文件夹下有一个install_st7789v.sh的脚本文件，用于自动修改linux项目下的相关文件。
## 使用
1. 将脚本与驱动文件存放到Linux项目路径下,比如我电脑的linxu项目路径是/home/work/linux
~~~
cp install_st7789v.sh /home/cb/linux
cp st7789v.c /home/work/linux
~~~
2. 在linux项目路径下执行install_st7789v.sh
~~~
bash install_st7789v.sh
~~~
## sh脚本功能介绍
1.	创建文件夹 drivers/diy/st7789v
2.	将st7789v.c文件复制进st7789v文件夹
3.	在diy及st7789v文件夹内创建makefile和kconfig，默认启用本驱动
4.	修改drivers路径下的主kconfig和makefile文件，末尾追加引用diy文件夹下的kconfig和makefile



