# Hey! I'm Filing Here

In this lab, I successfully implemented a 1 MiB ext2 file system with 2 directories, 1 regular file, and 1 symbolic link

## Building

Run the following to compile the executable:

```shell
make
```

## Running

First you need to make a directory called `mnt` which is the directory we will use to mount our file system to.
```shell
mkdir mnt
```

Next, run the following to create cs111-base.img, mount your file system, and change your current working directory to `mnt`.

```shell
./ext2-create
sudo mount -o loop cs111-base.img mnt
cd mnt
```

If you run `ls -ain`, the output is:
```shell
total 7
  2 drwxr-xr-x 3    0    0 1024 Dec   6 02:27 .
379 drwxr-xr-x 4 1000 1000 4096 Dec   6 02:27 ..
 13 lrw-r--r-- 1 1000 1000   11 Dec   6 02:27 hello -> hello-world
 12 -rw-r--r-- 1 1000 1000   12 Dec   6 02:27 hello-world
 11 drwxr-xr-x 2    0    0 1024 Dec   6 02:27 lost+found
```
To dump the file system information run `dumpe2fs cs111-base.img`
To check that the filesystem is correct run `fsck.ext2 cs111-base.img`

## Cleaning up

Run the following commands:
```shell
unmount to ummount the filesystem when you're done
sudo umount mnt

rmdir mnt to delete the mounting directory

make clean
```
