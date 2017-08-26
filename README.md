# rdrand
FreeBSD Module Device Character for rdrand Intel Secure Key RNG

Working in FreeBSD 11.1 

You can build this module with just make 

Load the module:

kldload ./devrdrand.ko

usage with dd

dd if=/dev/rdrand of=./output.rnd bs=1M count=1

