Herein you will find the source code for COMP 445 Lab 3.

The program is feature-complete. It implements selective-repeat pipelined reliable control scheme. It is not NACK-based, only timeout-based (as described in the textbook). It has a default window size of 4, a packet payload size of 1000 bytes, and a timeout value of 300ms, but that is parametrizable.

There are four programs. Client and Server are the usual FTP programs used since Lab 1. Sender and Receiver are special programs used to benchmark quickly. They require little or no user input to run.

The benchmarks were successfully run, but the data is odd at first sight: even at a delay rate of 50%, the total number of packets sent remained very close (and sometimes identical) to the ideal number of packets sent. This is because the drop rate was at 0, and because my selective-repeat protocol was not NACK-based, packets were rarely re-sent. Timeouts rarely happened because of the way the router simulates packet delay; it will only occasionally swap packets, and with a large enough window size, this doesn't affect the selective-repeat protocol.

It is useless to show a data table of the observed values, because the number of packets sent always fell within the interval [9485,9511], for all window sizes and delay percentages. The file used for testing was 9,484,315 bytes (it is included in the source code for the Sender program) and the per-packet payload size was 1000 bytes, hence the lower bound of 9485 packets in the interval.

Student: Philippe Milot
ID: 9164111
