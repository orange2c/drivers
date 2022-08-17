# ST7789V驱动
适用于5.x版本，linux项目自带的st7789v驱动有问题不能正常使用
## 驱动放置
将 fbtft-core.c 和 fb_st7789v.c 存放到路径 linux/drivers/staging/fbtft 下，替换原有文件
## 启用驱动
进入menuconfig，按顺序启用到st7789v这个驱动选项即可

	Device Drivers  --->　
		[*] Staging drivers  --->　
			<*>   Support for small TFT LCD display modules  ---> 
				<*>   FB driver for the ST7789V LCD Controller 

## 设备树示例

```
&spi0{
    status = "okay";
    st7789v: st7789v@0{
        compatible = "sitronix,st7789v";
        reg = <0>;
        status = "okay";
        spi-max-frequency = <320000>;
        spi-cpol;
        spi-cpha;
        rotate = <0>;
        fps = <30>;
        buswidth = <8>;
        rgb;
        dc-gpios    = <&pio 1 5 GPIO_ACTIVE_HIGH>;  // PB5 
        reset-gpios = <&pio 1 4 GPIO_ACTIVE_HIGH>;  // PB4
        debug = <0>;
    };
};
```

