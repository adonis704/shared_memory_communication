gcc -o sender1 sender1.c
gcc -o sender2 sender2.c
gcc -o receiver1 receiver1.c
gcc -o receiver2 receiver2.c
gcc -pthread -o crash_test crash_test.c
gcc -o reader reader.c