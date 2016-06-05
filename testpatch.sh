#!/bin/bash
for file in /media/root/robcore/android/Hulk-Kernel-V2-ref/patches/*
do
case "$file" in
     *.patch) patch -p1 -N --dry-run < "$file" ;;
     *) ;;
esac
done
