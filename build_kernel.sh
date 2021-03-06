#!/bin/bash
rm -rf $(pwd)/out;
rm $(pwd)/arch/arm/boot/dhd.ko;
rm $(pwd)/arch/arm/boot/zImage;
rm $(pwd)/arch/arm/boot/boot.img-zImage;
# clean up leftover junk
find . -type f \( -iname \*.rej \
				-o -iname \*.orig \
				-o -iname \*.bkp \
				-o -iname \*.ko \) \
					| parallel rm -fv {};

export ARCH=arm
export CROSS_COMPILE=/opt/toolchains/arm-cortex_a15-linux-gnueabihf_5.3/bin/arm-cortex_a15-linux-gnueabihf-
export USE_SEC_FIPS_MODE=true
export KCONFIG_NOTIMESTAMP=true
make clean;
make distclean;
make mrproper;
mkdir $(pwd)/out;
export ARCH=arm
export CROSS_COMPILE=/opt/toolchains/arm-cortex_a15-linux-gnueabihf_5.3/bin/arm-cortex_a15-linux-gnueabihf-
#export USE_SEC_FIPS_MODE=true
export KCONFIG_NOTIMESTAMP=true
cp $(pwd)/arch/arm/configs/OK3_defconfig $(pwd)/out/.config;
make ARCH=arm -j7 O=$(pwd)/out oldconfig;
make ARCH=arm -S -s -j7 O=$(pwd)/out;
cp $(pwd)/out/arch/arm/boot/zImage $(pwd)/arch/arm/boot/zImage;
cp $(pwd)/out/drivers/net/wireless/bcmdhd/dhd.ko $(pwd)/arch/arm/boot/dhd.ko;
mv $(pwd)/arch/arm/boot/zImage $(pwd)/arch/arm/boot/boot.img-zImage;
cd /media/root/robcore/AIK;
rm -rf hulk-new;
cp -R -p S4-Machinex-8.x hulk-new;
cp -p /media/root/robcore/android/Hulk-Kernel-V2/arch/arm/boot/dhd.ko $(pwd)/hulk-new/system/lib/modules/dhd.ko;
rm $(pwd)/split_img/boot.img-zImage;
cp -p /media/root/robcore/android/Hulk-Kernel-V2/arch/arm/boot/boot.img-zImage $(pwd)/split_img/boot.img-zImage;
rm image-new.img;
sh repackimg.sh;
cp -p image-new.img $(pwd)/hulk-new/boot.img
