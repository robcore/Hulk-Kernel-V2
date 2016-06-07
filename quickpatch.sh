#!/bin/bash
patch -p1 < /media/root/robcore/android/Hulk-Kernel-V2/patches/0194-From-7dff948320cf7e1be5363ac09e7812f4b3591b7d-Mon-Se.patch
patch -p1 < /media/root/robcore/android/Hulk-Kernel-V2/patches/0195-From-2a6c2f0b87ab0285fe0615cb494aa364eefc6f45-Mon-Se.patch
patch -p1 < /media/root/robcore/android/Hulk-Kernel-V2/patches/0196-From-dcfd8611ee2b6ec107a50c5825cb64a75f84bac3-Mon-Se.patch
patch -p1 < /media/root/robcore/android/Hulk-Kernel-V2/patches/0197-squashed-crypto-unregister-commits.patch
patch -p1 < /media/root/robcore/android/Hulk-Kernel-V2/patches/0198-crypto-unregister-des.patch
patch -p1 < /media/root/robcore/android/Hulk-Kernel-V2/patches/0199-From-a254c1cf41cff994485bef073c86e48a2bbbcfd5-Mon-Se.patch
patch -p1 < /media/root/robcore/android/Hulk-Kernel-V2/patches/0200-3-more-squashed-crypto-unregister-commits.patch
patch -p1 < /media/root/robcore/android/Hulk-Kernel-V2/patches/0201-From-5e59f5a013b63b889c6932abccff21e83ce9cefe-Mon-Se.patch
patch -p1 < /media/root/robcore/android/Hulk-Kernel-V2/patches/0202-From-800d99b1d6ae21b0400f476fc328b2a7cf30efee-Mon-Se.patch
patch -p1 < /media/root/robcore/android/Hulk-Kernel-V2/patches/0203-From-8295d9f7750702f52db957d80cbe3c41d9ec9bb6-Mon-Se.patch
echo "finished"
