#!/bin/bash
for file in /media/root/robcore/android/Alucard-Kernel-jfltexx-my-tw-5.0/patches/wq/*
do
case "$file" in
     *.patch) patch -p1 -N < "$file" ;;
     *) ;;
esac
done
