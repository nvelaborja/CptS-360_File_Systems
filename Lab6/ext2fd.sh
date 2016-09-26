dd if=/dev/zero of=testdisk bs=1024 count=1440
mkfs -b 1024 testdisk 400000 # enter y to let mkfs to proceed
mount -o loop testdisk /mnt
(cd /mnt; ls; mkdir a; cd a; mkdir b; cd b; mkdir c; touch d; cd /mnt; ls)
umount /mnt
