Modified files:
vm_pageout.c

To build our modified kernel, move vm_pageout.c into /sys/vm/ and build and install the kernel.

Added files:
1) set_pagemode.sh:
Run "chmod 777 set_pagemode.sh"
To set use FreeBSD clock paging:
./set_pagemode.sh 0
To set FIFO paging:
./set_pagemode.sh 1

2) check_pagemode.sh
Run "chmod 777 check_pagemode.sh"
This will print out the current pagemode you are in

3) set_benchmark_flag.sh
Run "chmod 777 set_benchmark_flag.sh"
To turn on benchmarking printout:
./set_benchmark_flag.sh 1
To turn off benchmarking printout:
./set_benchmark_flag.sh 0
To build and installing the kernel, it is reccomended to turn off
the benchmarking flag

4) check_benchmark_flag.sh
Run "chmod 777 check_benchmark_flag.sh"
This will print out the current benchmark flag you are in

5) set_pageout_update.sh
Run "chmod 777 set_pageout_update.sh"
To change pageout_update_period for a shorter time:
./set_pageout_update.sh 10
To reset to default pageout_update_period:
./set_pageout_update.sh 600

6) check_pageout_update.sh
Run "chmod 777 check_pageout_update.sh"
This will print out the current pageout_update_period you are in