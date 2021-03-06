/* RPC based web proxy
 * Project by Srinivas Ramachandran & Chenhao Liu
 * Date: 27 March 2014
 */

Directory structure:

The project involves a fair amount of code automatically generated using 'thrift' apache server. The code generated by thrift automatically is in the gen-cpp folder and the user-defined proxy client and proxy server code is in the 'cpp' folder. The gen-cpp folder also contains the required headers and cpp files of the various caching policies implemented (LRU, Random and FIFO).

Compiling:

Change to the 'cpp' directory and run 'make all'. The Makefile automatically takes care of compiling the dependencies iff the directory structure mentioned above is preserved i.e, the gen-cpp folder which has the automatic thrift code must be present one level up.

'make clean' removes the generated binaries and cleans up the install

Running:

1. Start the proxy server first. There are 2 server programs that implement different caching policies. The 'proxyserver_lru' implements LRU and the 'proxyserver' implements random & fifo policies.

  * Usage for fifo and random policies
 
    ./proxyserver <cache size in KB> <caching policy>
	Eg: ./proxyserver 256 fifo
        ./proxyserver 512 random

  * Usage for LRU proxy server

    ./proxyserver_lru <cache size in Bytes>
    Eg: ./proxyserver_lru 256

In both servers, the cache Hit ratio can be observed by pressing 'Ctrl-C'. There is a registered signal handler to print the hit ratio.

2. Start the client as follows.

  * ./proxyclient <URLs list file> <workload choice> <IP address of server>
    Provide 'localhost' as IP address of server if server is on the same machine
    For workload 2, there is one more argument 'URL grouping degree' which determines how many URls are grouped together.
	Eg: ./proxyclient ../testcases/urls.txt 1 localhost
    Eg: ./proxyclient ../testcases/urls.txt 2 4 localhost // Workload 2 with 4 urls grouped together. Refer the report for details about the grouping of URls  
