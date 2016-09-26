mkfs -b 1024 mydisk 1440
mount -o loop mydisk /mnt
umount /mnt
a.out -d
