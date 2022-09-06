#!/bin/bash
mkdir -p drivers/diy/st7789v
cp st7789v.c drivers/diy/st7789v/

#创建drivers/diy/st7789v/路径下的Kconfig与Makefile
cat > drivers/diy/st7789v/Kconfig <<EOF
config DIY_ST7789V
	bool "lcd st7789v "
	default y

EOF
echo -e '\nobj-$(CONFIG_DIY_ST7789V)	+= st7789v.o' > drivers/diy/st7789v/Makefile

#创建drivers/diy/路径下的Kconfig与Makefile
if [ ! -f "drivers/diy/Kconfig" ]
then
cat >> drivers/diy/Kconfig <<EOF
menuconfig DIY
	bool "DIY_Drivers"
	default y

EOF
#如果diy文件夹下没有kconfig，则插入drivers下的主kconfig和makefile
sed -i '$i\source "drivers/diy/Kconfig"'  drivers/Kconfig
echo -e '\nobj-$(CONFIG_DIY)		+= diy/' >> drivers/Makefile
fi

cat >> drivers/diy/Kconfig <<EOF
if DIY
	source "drivers/diy/st7789v/Kconfig"
endif

EOF
echo -e '\nobj-$(CONFIG_DIY)		+= st7789v/' >> drivers/diy/Makefile

