The kprobe is made to hit on the "ht530_driver_write" function of ht530_drv.c file.

So we can get the address of this function after the module is inserted into the kernel. 
	"cat /proc/kallsyms | grep ht530_drv" command in the console displays the symbols in text, bss and data section addresses of the ht530_drv module.

We can get the address of the global variable of "ht530_drv.c file" again from the "cat /proc/kallsyms | grep ht530_drv" command 
under the bss or the data section (we have considered KEY as the global varible for our testing)

We can get the address of a local variable of "ht530_driver_write" function by following the steps given below:
- A test_local variable of type int is initialised to 50 under "ht530_driver_write" function
- In the prehandler routine of the kprobe, we can get the location of the stack base pointer(bp) from the pt_regs structure ( ie regs->bp in the code)
- Now we can subtract the offset of the local variable (ie 0x38, since the variable is initialised at the start of the function)from the address of the stack base pointer to get the address of the local variable.
- The value of the variable is sent to the user.

These addresses are written into a structure "Saddress" and accessed at the kernel from the "Mprobe_device_write" call. When the probe hits "ht530_driver_write" function 
the prehandler routine is executed which puts all the information into the ring/cirular buffer (which has 10 nodes). The buffer values are displayed at the user after a bunch of 10 write operations

Each node of the ring buffer contains this following structure:
typedef struct {
        uint64_t TSC;
        unsigned long  ADDR;
        pid_t PID;
        int G_VALUE;
        int L_VALUE;
}data, *Pdata;

The timestamp is displayed by writing an asm code to get the rdtsc value from registers %edx(first half) and %eax(second half).


STEPS FOR TESTING THE MODULE(Refer to Mprobe_TestProcedure_Screenshot.png)

1) chmod +x module_test.sh

2) gcc -pthread -o test kmain.c -Wall

3) ./module_test.sh

4) cat /proc/kallsyms | grep ht530_drv

5)take the address correspnding to "ht530_driver_write" for the location of instruction where the probe has to hit

6)./test

7)You will be allowed to type the address for probe insertion, enter the address of "ht530_driver_write"
	eg:Give an address where the probe has to hit
	   e09b5290

8) dmesg (we get an output similar to output_dmesg file)

9) Remove the Mprobe_drv and ht530_drv Module : rmmod Mprobe_drv.ko
				         rmmod ht530_drv.ko

	
	