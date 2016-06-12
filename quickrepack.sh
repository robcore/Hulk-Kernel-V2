#!/bin/bash
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
