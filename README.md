## Power tools

### What is this?

If you don't know what is this, you probably get in the wrong place. XDD



### Instruction

#### 1. read_power.c

This is a tool to get your power information without root privilege via RAPL interface encapsulated in C language by [powercap](https://github.com/powercap/powercap). The program would automatically check CPU numbers of machine, and print PACKAGE zone and DRAM zone power consumption of each CPU every user-specific time inteval.

Before using it, check if your linux kernel support Intel RAPL. Generally, you could find that directory /sys/class/powercap/intel_rapl exists if it's supported. Besides, [powercap](https://github.com/powercap/powercap) should be installed or loaded to your environment.

To compile this tool, here take gcc for example:

```bash
gcc read_power.c -o read_power -O2 -Wall -lpowercap
```

Sampling interval and times can be specified using command line parameters:

* `-i`: Time inteval in millisecond. For example, `-i 500` means sampling energy consumption every 0.5 second. If not specified, 1 second is taken by default.
* `-t`: Total times. For example, `-t 10` means sampling for 10 times. If not specified, 4294967295 is taken by default in most situation.

For some specific usage, there are several macro `#define` in code which are commented by default:

* `OUTPUT_TO_FILE`: In some scenarios, you may want not want data printed to screen, but output to file. This macro define allow you to easily do this. `powerlog_0.txt` is the default output file. By pass parameter `-f indexNumber`, you can change output file to `powerlog_indexNumber.txt`.
* `CPU_FREQUENCY_NEEDED`: This macro define indicates the program cocurrently sampling CPU frequency data. Should be used with `OUTPUT_TO_FILE`, and the result would be in `freqlog_indexNumber.txt`. Notice that the function is completed by using pipe to call `cat` command to read /proc/cpuinfo, and this can cause the sampling interval is greater than expected (could be 5-20ms, depends on machine). Frequency of every cores would be recorded.
* `ALWAYS_FLUSH`: In some scenarios, you may not exit the program normally. (Like `kill`) If some output contents is still in output cache, some information would be missing. This macro define indicates the program to flush output cache every output line to avoid missing information in abnormal exit.

#### 2. auto_collect

In this directory there are two scripts for automatically collecting the HPL performance information under different RAPL power limitation.

To use there scripts, you run both of them in two shell terminal. (It's recommended to run `auto_run.sh` firstly.) Notice that it could be necessary to place these two scripts, read_power tool and HPL program in the same directory.

Some parameter in scripts should be changed on different machine.

