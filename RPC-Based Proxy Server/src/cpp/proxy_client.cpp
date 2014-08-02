/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements. See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership. The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License. You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied. See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>

#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/transport/TSocket.h>
#include <thrift/transport/TTransportUtils.h>

#include "../gen-cpp/proxy.h"

using namespace std;
using namespace apache::thrift;
using namespace apache::thrift::protocol;
using namespace apache::thrift::transport;

using namespace proxy;
using namespace boost;

#define TS(x) ((x.tv_sec*1000000)+(x.tv_usec))

void usage(){
	fprintf(stderr,"Usage: ./proxyclient <URLs file> <Workload choice> <IP address of server>"\
			"\nURLs file: txt file with all URLs to be tested"\
			"\nWorkload choice: Workload 1 - Pick 48 Random URLs"\
			"\n                 Worklaod 2 - Repeat a grouped set of 12 URLs 4 times\n"
			"If workload 2; last argument is grouping factor\n"\
			"Provide 'localhost' as IP address of server if server on same machine\n");
	exit(EXIT_FAILURE);
}

int main(int argc, char** argv) {
  srand(123456789);
  if(argc != 4 && argc != 5)
	usage();

  char* server_ip = "localhost";
  if (argc == 4){
  	server_ip = argv[3];
  }
  if (argc == 5){
	server_ip = argv[4];
  }
  
  shared_ptr<TTransport> socket(new TSocket(server_ip, 9090));
  shared_ptr<TTransport> transport(new TBufferedTransport(socket));
  shared_ptr<TProtocol> protocol(new TBinaryProtocol(transport));
  proxyClient client(protocol);
 
  int i,j,k,l;
  int line_number;
  char buffer[100];

  string proxy_server_output;
  transport->open();

  FILE *fp = fopen(argv[1],"r");
  struct timeval start,end;
  unsigned long long int total_time=0;

  if (atoi(argv[2]) == 1){ // Workload 1 - Random URL
 	for (i=0;i<48;i++){
		line_number = (rand()%12) + 1;
		fprintf(stderr,"Line:\t%d\t",line_number);			
		for(j=0 ;j<line_number; j++){
			fscanf(fp,"%s\n",buffer);
		}
		fprintf(stderr,"%s\n",buffer);
		fseek(fp,0,SEEK_SET);
		
		//Measure performance metrics - Request serve time
		gettimeofday(&start, NULL);
		client.fetch_page(proxy_server_output, std::string(buffer));
		gettimeofday(&end, NULL);
		total_time += TS(end)-TS(start);
	}
  }

  if(atoi(argv[2]) == 2){ // Workload 2 - Grouped URL selection
	for(i=0;i<12/atoi(argv[3]);i++){ //Groups of 4 URLs
		for(j=0;j<4;j++){ // Each group repeats 4 times
			for (k=0;k<atoi(argv[3]);k++){ // Request each URL in the group
				line_number = i*atoi(argv[3])+k+1;
				fprintf(stderr,"Line:\t%d\t",line_number);			
				for(l=0 ;l<line_number; l++){
					fscanf(fp,"%s\n",buffer);
				}
				fprintf(stderr,"%s\n",buffer);
				fseek(fp,0,SEEK_SET);
						
				//Measure performance metrics - Request serve time
				gettimeofday(&start, NULL);
				client.fetch_page(proxy_server_output, std::string(buffer));
				gettimeofday(&end, NULL);
				total_time += TS(end)-TS(start);
			}
		}
	}
  }

  fprintf(stderr,"Time to complete workload: %3.2f sec\n",total_time/float(1000000));
  transport->close();
}
