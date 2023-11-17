# A Kernel Seedling
TODO: intro

## Building
```shell
TODO: cmd for build
To build the kernel module: make
```

## Running
```shell
TODO: cmd for running binary
To insert the kernel module and see the number of running processes:
sudo insmod proc_count.ko
cat /proc/count
```
TODO: 
Expected result:
A single integer will be printed representing the number of running processes (or tasks) currently active on the system.

## Cleaning Up
```shell
TODO: 
To remove the kernel module from the system:
sudo rmmod proc_count
```

## Testing
```python
python -m unittest
```
TODO: results

Expected result:
All tests should pass successfully, indicating that the kernel module is working as expected.


Report which kernel release version you tested your module on
(hint: use `uname`, check for options with `man uname`).
It should match release numbers as seen on https://www.kernel.org/.


```shell
uname -r -s -v
```
TODO: kernel version is Linux 5.14.8-arch1-1 #SMP PREEMPT Sun, 26 Sep 2021 19:36:15 +0000
