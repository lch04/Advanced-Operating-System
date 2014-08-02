#include "rvm.h"
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <string.h>
#include <dirent.h>
#include <fcntl.h>
#include <errno.h>
#include <fstream>
#include <iostream>
#include <vector>

#define NAME_LITERAL_SIZE 128

const char* logName = "lrvmlog";
RVMNode* rvmList = NULL;
TRANSNODE* transList = NULL;
DATA* logList = NULL;
rvm_t RVMNodeID = 0;
trans_t TransactionID = 1;

static void handle_error(char* func, char* error) {
	fprintf(stderr, "Failed due to %s in function: %s\n", error, func);
	exit(EXIT_FAILURE);
}

static void insert(char* cur_seg, char* data, int data_len){

	if (logList == NULL){
		logList = (DATA*)malloc(sizeof(DATA));
		if (!logList){
			printf("Error!\n");
			exit(1);
		}
		logList->data = data;
		logList->segname = cur_seg;
		logList->len = data_len;
		logList->next = NULL;
	}
	else{
		DATA* tmp = logList, *prev = NULL;
		while (tmp){
			if(strcmp(tmp->segname, cur_seg)){
				prev = tmp;
				tmp = tmp->next;
			}
			else{
				if (tmp->len >= data_len){
					memcpy(tmp->data, data, data_len);
					free(data);
					return;
				}
				free(tmp->data);
				tmp->data = data;
				tmp->len = data_len;
				return;
			}
		}
		tmp = (DATA*)malloc(sizeof(DATA));
		tmp->next = NULL;
		tmp->len = data_len;
		tmp->data = data;
		tmp->segname = cur_seg;
		prev->next = tmp;
	}
}

static void cleanup(DATA *node){
	if (node == NULL)
		return;
	cleanup(node->next);
	free(node->segname);
	free(node->data);
	free(node);
}

static void readLog(char* logName) {
	std::ifstream infile(logName);
	std::string cur_line;
	std::string cur_segment;
	std::string data;
	int size_of_data;
	int read_line = 0;
	char tmp;
	std::vector<char> segment_data;

	if (infile.is_open()){
		while(!infile.eof()){
			if (!read_line) {
				getline(infile, cur_line);
			}
			if (cur_line == ""){
				continue;
			}
			else if (cur_line == "BOS"){
				//printf("Found BOS\n");
				getline(infile, cur_line);
				cur_segment = (char*)cur_line.c_str();
				//printf("Found segment : %s\n",cur_segment.c_str());
				getline(infile, cur_line);
				size_of_data = atoi((char*)cur_line.c_str());
				//printf("Found size: %d\n",size_of_data);
				read_line = 1;
			}
			else {//found a data line
				for (int i=0;i<size_of_data-1;i++) {
					infile.get(tmp);
					segment_data.push_back(tmp);
				}
//				printf("Finished reading %d chars...\n",size_of_data);
//				for (int i=0;i<size_of_data-1;i++) {
//					std::cout<<segment_data[i];
//				}
//				printf("\n");
				infile.get(tmp);
				infile.get(tmp);
				infile.get(tmp);
				if(tmp == 'E'){
					infile.get(tmp);
					if (tmp == 'O'){
						infile.get(tmp);
						if(tmp == 'S') {
							//printf("Got EOS\n");
							read_line = 0;
							infile.get(tmp);
							getline(infile, cur_line);
						}
					}
				}
				int segname_len = strlen((char*)cur_segment.c_str());
				char* node_segname = (char*)malloc(sizeof(char)*(segname_len+1));
				char* node_data = (char*)malloc(sizeof(char)*(size_of_data+1));

				memcpy(node_segname, (char*)cur_segment.c_str(), segname_len);
				memcpy(node_data, &segment_data[0], size_of_data);
				node_segname[segname_len] = '\0';
				node_data[size_of_data] = '\0';
				insert(node_segname, node_data, size_of_data);
				data = "";
				cur_segment = "";
				cur_line = "";
				segment_data.clear();
			}
		}
		//print();
	}
}

static int isRVMExist(const char *directory, rvm_t* rvm_id) {
	if (directory && rvm_id) {
		RVMNode* node = rvmList;
		while (node) {
			if (!strcmp(node->directory, directory)) {
				*rvm_id = node->id;
				return 1;
			}
			node = node->next;
		}
	} else {
		handle_error("isRVMExist", "null argument(s)");
	}
	return 0;
}

static rvm_t getNextRVMID() {
	return RVMNodeID++;
}

static trans_t getNextTransactionID() {
	return TransactionID++;
}

static void createLog(char* logPath) {
	if (logPath) {
		FILE* fd = fopen(logPath, "w+");
		if (!fd) {
			handle_error("createLog", "fopen");
		}
		fclose(fd);
	} else {
		handle_error("createLog", "null argument");
	}
}

static int isFileExist(const char* directory, const char* filename) {
    int isFileExists = 0;
    if (directory && filename) {
    	DIR *dir;
    	struct dirent *entry;
    	if ((dir = opendir(directory)) != NULL) {
    		while ((entry = readdir(dir)) != NULL) {
    			isFileExists = !strcmp(entry->d_name,filename);
    			if (isFileExists) {
    				break;
    			}
    		}
    		closedir(dir);
    	} else {
    		handle_error("isFileExist", "opendir");
    	}
    } else {
    	handle_error("isFileExist", "null argument(s)");
    }
    return isFileExists;
}

/*
* - Initialize the library with the specified directory as backing store. *
*/
rvm_t rvm_init(const char *directory) {
	rvm_t rvm_id = 0;
	struct stat dir;
	int isDirExists = 0;
	// if directory is not NULL
    if (directory) {
    	// if a rvm node with this directory already exists, simply return its id
    	if (isRVMExist(directory, &rvm_id)) {
    		printf("RVM node id is: %lu\n", rvm_id);
    		return rvm_id;
    	}
    	// check whether a file with the same name exists
    	if (!stat(directory, &dir)) {
    		isDirExists = 1;
    		// check whether this file is a directory
    		if (!S_ISDIR(dir.st_mode)) {
    	    	fprintf(stderr, "rvm_init: File with name %s already exists\n", directory);
    	    	exit(EXIT_FAILURE);
    		}
    	}
    	// create new folder
    	if (!isDirExists) {
            if (mkdir(directory, 0777)) {
                fprintf(stderr, "rvm_init: Failed during mkdir\n");
                exit(EXIT_FAILURE);
            }
    	}
    	// create new rvm node
    	RVMNode* rvm = (RVMNode*) malloc(sizeof(RVMNode));
    	if (!rvm) {
    		handle_error("rvm_init", "malloc");
    	}
    	memset(rvm, 0, sizeof(RVMNode));
    	// set directory info
    	rvm->directory = directory;
    	// set id info
    	rvm->id = getNextRVMID();
    	rvm_id = rvm->id;
    	// set logPath info
    	rvm->logPath = (char*) malloc(sizeof(char)*NAME_LITERAL_SIZE);
//    	fprintf(stdout, "dir is: %s, logname is: %s\n", rvm->directory, logName);
    	sprintf(rvm->logPath, "%s/%s\0", rvm->directory, logName);
  //  	fprintf(stdout, "logpath is: %s\n", logpath);*/
    	//fprintf(stdout, "logpath is: %s\n", rvm->logPath);
    	//exit(EXIT_SUCCESS);
    	// set up log file
    	if (!isFileExist(directory, logName)) {
    		createLog(rvm->logPath);
    	}
    	//rvm->transactions = NULL;
    	// insert rvm node into list
    	rvm->next = rvmList;
    	rvmList = rvm;
    	return rvm_id;
    } else {
    	handle_error("rvm_init", "null argument");
    }
}

RVMNode* getRVMNode(rvm_t rvm) {
	RVMNode* node = rvmList;
	while (node) {
		if (node->id == rvm) {
			return node;
		}
		node = node->next;
	}
	return NULL;
}

static void insertSegmentToRVM(RVMNode* rvm, SEGNODE* seg) {
	// NOTE THE ORDER IS REVERSED;
	if (rvm && seg) {
		seg->next = rvm->segments;
		rvm->segments = seg;
	} else {
		handle_error("insertSegmentToRVM", "null argument(s)");
	}
}

static void* createSegment(RVMNode* rvm, const char *segname, int size_to_create) {
	int c = 0;
	char segmentName[NAME_LITERAL_SIZE];
	sprintf(segmentName, "%s/%s", rvm->directory, segname);
	char buf = 'a';
	int segFile = open(segmentName, O_CREAT|O_RDWR, S_IRWXU);
	if (segFile != -1) {
		for (int i = 0; i < size_to_create; i++) {
		    write(segFile, &buf, 1);
		}
		close(segFile);
		segFile = open(segmentName, O_RDWR|O_CREAT, S_IRWXU);
		SEGNODE* segNode = (SEGNODE*) malloc(sizeof(SEGNODE));
		if (segNode) {
			segNode->name = (char*)segname;
			segNode->size = size_to_create;
			segNode->isInTransaction = 0;
			segNode->isMapped = 1;
			segNode->next = NULL;
			segNode->transID = 0;
			segNode->address = mmap(0, size_to_create, PROT_READ|PROT_WRITE, MAP_PRIVATE, segFile, 0);
			if (segNode->address != MAP_FAILED) {
				insertSegmentToRVM(rvm, segNode);
				close(segFile);
				return segNode->address;
			} else {
				close(segFile);
				handle_error("createSegment", "mmap");
			}
		} else {
			close(segFile);
			handle_error("createSegment", "malloc");
		}
	} else {
		handle_error("createSegment", "open");
	}
}

static SEGNODE* getSegmentNode(RVMNode* rvmNode, const char *segname) {
	if (rvmNode && segname) {
		SEGNODE* node = rvmNode->segments;
		while (node) {
			if (!strcmp(node->name, segname)) {
				return node;
			}
			node = node->next;
		}
		return NULL;
	} else {
		handle_error("getSegmentNode", "null argument(s)");
	}
}

static void reconstructLog(RVMNODE* rvmNode, void* address, const char* segname) {
	readLog(rvmNode->logPath);
	DATA* logTmp = logList;
	while (logTmp) {
		if (!strcmp(logTmp->segname, segname)) {
			memcpy(address, logTmp->data, logTmp->len);
		}
		logTmp = logTmp->next;
	}
	return;
}

static int sizeOnDisk(RVMNODE* rvmNode, const char* segname) {
	struct stat st;
	char fileName[NAME_LITERAL_SIZE];
	sprintf(fileName, "%s/%s", rvmNode->directory, segname);
	if (stat(fileName, &st) == 0) {
		return (int)st.st_size;
	}
	return 0;
}
/*
* - map a segment from disk into memory. If the segment does not already exist,
* then create it and give it size size_to_create. If the segment exists but is shorter than size_to_create,
* then extend it until it is long enough. It is an error to try to map the same segment twice.
*/
void *rvm_map(rvm_t rvm, const char *segname, int size_to_create) {
	// check whether the arguments are valid
	if (!segname) {
		handle_error("rvm_unmap", "null segment name");
	}
	// get rvm node using given id
	RVMNode* rvmNode = getRVMNode(rvm);
	if(rvmNode) {
		char* dir = (char*)rvmNode->directory;
		// check existence of segment file
		if (isFileExist(dir, segname)) {
			SEGNODE* node = getSegmentNode(rvmNode, segname);
			if (node) {
				if (node->isMapped) {
					fprintf(stderr, "Segment with given name is already mapped\n");
					exit(EXIT_FAILURE);
				} else {
					if (node->size >= size_to_create) {
						node->isMapped = 1;
						char segName[NAME_LITERAL_SIZE];
						sprintf(segName, "%s/%s", rvmNode->directory, node->name);
						int segment = open(segName, O_APPEND, S_IRWXU);
						if (segment == -1) {
							handle_error("rvm_map", "open");
						}
						node->address = mmap(0, node->size, PROT_READ|PROT_WRITE, MAP_PRIVATE, segment, 0);
						if (node->address != MAP_FAILED) {
							close(segment);
							reconstructLog(rvmNode, node->address, segname);
							return node->address;
						} else {
							close(segment);
							handle_error("rvm_map", "mmap");
						}
					} else {
						char c = 'a';
						char segName[NAME_LITERAL_SIZE];
						sprintf(segName, "%s/%s", rvmNode->directory, node->name);
						int segment = open(segName, O_APPEND, S_IRWXU);
						if (segment == -1) {
							handle_error("rvm_map", "open");
						}
						int diff = size_to_create - node->size;
						while ((diff--) > 0) {
							write(segment, &c, 1);
						}
						close(segment);
						segment = open(segName, O_RDWR, S_IRWXU);
						node->size = size_to_create;
						node->address = mmap(0, size_to_create, PROT_READ|PROT_WRITE, MAP_PRIVATE, segment, 0);
						node->isMapped = 1;
						if (node->address != MAP_FAILED) {
							close(segment);
							reconstructLog(rvmNode, node->address, segname);
							return node->address;
						} else {
							close(segment);
							handle_error("rvm_map", "mmap");
						}
					}
				}
			} else {
				void* tmp = NULL;
				// = createSegment(rvmNode, segname, size_to_create);
				char segName[NAME_LITERAL_SIZE];
				sprintf(segName, "%s/%s", rvmNode->directory, segname);
				int segment_file;
				int size_on_disk = sizeOnDisk(rvmNode, segname);
				if (size_on_disk == 0) {
					handle_error("rvm_map", "zero byte segment size");
				}
				if (size_to_create <= size_on_disk) {
					segment_file = open(segName, O_RDWR, S_IRWXU);
					tmp = mmap(0, size_to_create, PROT_READ|PROT_WRITE, MAP_PRIVATE, segment_file, 0);
					if (tmp != MAP_FAILED) {
						close(segment_file);
						reconstructLog(rvmNode, tmp, segname);
						SEGNODE* segNode = (SEGNODE*) malloc(sizeof(SEGNODE));
						if (segNode) {
							segNode->name = (char*)segname;
							segNode->size = size_to_create;
							segNode->isInTransaction = 0;
							segNode->isMapped = 1;
							segNode->next = NULL;
							segNode->transID = 0;
							segNode->address = tmp;
							insertSegmentToRVM(rvmNode, segNode);
						} else {
							handle_error("rvm_map", "malloc");
						}
						return tmp;
					} else {
						close(segment_file);
						handle_error("rvm_map", "mmap");
					}
				} else {
					char c = 'a';
					segment_file = open(segName, O_APPEND, S_IRWXU);
					if (segment_file == -1) {
						handle_error("rvm_map", "open");
					}
					int diff = size_to_create - size_on_disk;
					while ((diff--) > 0) {
						write(segment_file, &c, 1);
					}
					close(segment_file);
					segment_file = open(segName, O_RDWR, S_IRWXU);
					tmp = mmap(0, size_to_create, PROT_READ|PROT_WRITE, MAP_PRIVATE, segment_file, 0);
					if (tmp != MAP_FAILED) {
						close(segment_file);
						reconstructLog(rvmNode, tmp, segname);
						SEGNODE* segNode = (SEGNODE*) malloc(sizeof(SEGNODE));
						if (segNode) {
							segNode->name = (char*)segname;
							segNode->size = size_to_create;
							segNode->isInTransaction = 0;
							segNode->isMapped = 1;
							segNode->next = NULL;
							segNode->transID = 0;
							segNode->address = tmp;
							insertSegmentToRVM(rvmNode, segNode);
						} else {
							handle_error("rvm_map", "malloc");
						}
						return tmp;
					} else {
						close(segment_file);
						handle_error("rvm_map", "mmap");
					}
				}
			}
		} else {
			return createSegment(rvmNode, segname, size_to_create);
		}
	} else {
		handle_error("rvm_map", "invalid rvm node");
	}
}
/*
* - unmap a segment from memory.
*/
void rvm_unmap(rvm_t rvm, void *segbase) {
	// check whether the arguments are valid
	if (!segbase) {
		handle_error("rvm_unmap", "null segment name");
	}
	RVMNode* rvmNode = getRVMNode(rvm);

/*	SEGNODE* node = rvmNode->segments;
	while (node) {
		fprintf(stdout, "size is: %d\n", node->size);
		fprintf(stdout, "name is: %s\n", node->name);
		fprintf(stdout, "address is: %p\n", node->address);
		fprintf(stdout, "Segbase address is: %p\n", segbase);
		fprintf(stdout, "isInTransaction is: %d\n", node->isInTransaction);
		fprintf(stdout, "tranID is: %d\n", node->transID);
		fprintf(stdout, "isMapped is: %d\n", node->isMapped);
		node = node->next;
	}*/

	if (rvmNode) {
		SEGNODE* tmp = rvmNode->segments, *prev = NULL;
		while (tmp) {
			if (tmp->address == segbase) {
				if (tmp->isMapped == 0) {
					handle_error("rvm_unmap", "segment not mapped yet");
				}
				if (tmp->isInTransaction == 1) {
					handle_error("rvm_unmap", "cannot unmap an in-transaction segment");
				}
				if (munmap(tmp->address, tmp->size) == -1) {
					handle_error("rvm_unmap", "munmap");
				}
				tmp->address = NULL;
				tmp->isMapped = 0;
				//tmp->size = 0;
				tmp->transID = 0;
				tmp->isInTransaction = 0;
				// don't reset name because Srini said so
				// because if unmap and map again, and segment is removed from rvm node, then
				// we will create a new segment which overwrites the existing file on disk
			}
			tmp = tmp->next;
		}
	} else {
		handle_error("rvm_unmap", "invalid rvm node");
	}
}
/*
*- destroy a segment completely, erasing its backing store.
* This function should not be called on a segment that is currently mapped.
*/
void rvm_destroy(rvm_t rvm, const char *segname) {
	// check whether the arguments are valid
	if (!segname) {
		handle_error("rvm_destroy", "null segment name");
	}
	// get rvm node using given id
	RVMNode* rvmNode = getRVMNode(rvm);

	if (rvmNode) {
		SEGNODE* tmp = rvmNode->segments, *prev = NULL;
		while (tmp) {
			if (!strcmp(tmp->name, segname)) {
				if (tmp->isMapped == 1) {
					handle_error("rvm_destroy", "cannot destroy a mapped segment");
				}
				if (prev) {
					prev->next = tmp->next;
				}
				char cmd[NAME_LITERAL_SIZE];
				sprintf(cmd,"rm %s/%s", rvmNode->directory, tmp->name);
				system(cmd);
				free (tmp);
				return;
			}
			prev = tmp;
			tmp = tmp->next;
		}
	} else {
		handle_error("rvm_destroy", "invalid rvm node");
	}
}
/*
*- begin a transaction that will modify the segments listed in segbases.
* If any of the specified segments is already being modified by a transaction,
* then the call should fail and return (trans_t) -1.
* Note that trant_t needs to be able to be typecasted to an integer type.
*/
trans_t rvm_begin_trans(rvm_t rvm, int numsegs, void **segbases) {
	// check whether the arguments are valid
	if (!segbases) {
		handle_error("rvm_begin_trans", "null segment bases");
	}
	// get rvm node using given id
	int found = 0;
	RVMNode* rvmNode = getRVMNode(rvm);

	trans_t id = getNextTransactionID();

	if (rvmNode) {
		for (int i = 0; i < numsegs; ++i) {
			SEGNODE* tmp = rvmNode->segments;
			found = 0;
			while (tmp) {
				if (tmp->address == segbases[i]) {
					found = 1;
					if (tmp->isMapped && tmp->isInTransaction && tmp->transID != 0) {
						handle_error("rvm_begin_trans", "segment already in transaction");
					} else {
						tmp->isInTransaction = 1;
						tmp->transID = id;
						break;
					}
				}
				tmp = tmp->next;
			}
			if (!found) {
				handle_error("rvm_begin_trans", "no matching segment found");
			}
		}
		TRANSNODE* trans = (TRANSNODE*) malloc(sizeof(TRANSNODE));
		if (!trans) {
			handle_error("rvm_begin_trans", "malloc");
		} else {
			trans->id = id;
			trans->numseg = numsegs;
			trans->rvm = rvm;
			trans->segbases = segbases;
			trans->undo = NULL;
			// insert to transList
			trans->next = transList;
			transList = trans;
		}
		return id;
	} else {
		handle_error("rvm_begin_trans", "invalid rvm node");
	}
}

static TRANSNODE* getTransNode(trans_t tid) {
	TRANSNODE* tmp = transList;
	while (tmp) {
		if (tmp->id == tid) {
			return tmp;
		}
		tmp = tmp->next;
	}
	return NULL;
}

static void insertUndoRecord(TRANSNODE* trans, UNDORECORD* undo) {
	if (trans && undo) {
		undo->next = trans->undo;
		trans->undo = undo;
	} else {
		handle_error("insertUndoRecord", "null argument(s)");
	}
}

/*
* - declare that the library is about to modify a specified range of memory in the specified segment.
* The segment must be one of the segments specified in the call to rvm_begin_trans.
* Your library needs to ensure that the old memory has been saved, in case an abort is executed.
* It is legal call rvm_about_to_modify multiple times on the same memory area.
*/
void rvm_about_to_modify(trans_t tid, void *segbase, int offset, int size) {
	// check whether the arguments are valid
	if (!segbase) {
		handle_error("rvm_about_to_modify", "null segment base");
	}
	// get trans node using given tid
	int validSegBase = 0;
	TRANSNODE* transNode = getTransNode(tid);
	if (transNode) {
		void** segBases = transNode->segbases;
		unsigned long numsegs = transNode->numseg;
		for (int i = 0; i < numsegs; ++i) {
			if (segBases[i] == segbase) {
				validSegBase = 1; // TODO: maybe we should also check the size and offset...
				break; // FOUND A MATCH
			}
		}
		if (!validSegBase) {
			handle_error("rvm_about_to_modify", "invalid segbase");
		}
		UNDORECORD* undo = (UNDORECORD*) malloc(sizeof(UNDORECORD));
		if (!undo) {
			handle_error("rvm_about_to_modify", "malloc");
		} else {
			undo->segBase = segbase;
			undo->size = size;
			undo->offset = offset;
			undo->next = NULL;
			undo->address = malloc(sizeof(char)*size);
			if (!undo->address) {
				handle_error("rvm_about_to_modify", "malloc");
			} else {
				memcpy(undo->address, (char*)segbase+offset, size);
				insertUndoRecord(transNode, undo);
			}
		}
	} else {
		handle_error("rvm_about_to_modify", "null transnode");
	}
}

static int isUniqueSegment(void* segBase, UNIQUESEGS* uni) {
	UNIQUESEGS* unisegs = uni;
	if (segBase && unisegs) {
		while (unisegs) {
			if (segBase == unisegs->address) {
				return 0;
			}
			unisegs = unisegs->next;
		}
		return 1;
	}
}

static void cleanTransaction(trans_t tid) {
	TRANSNODE* transNode = getTransNode(tid);
	if (!transNode) {
		return;
	}
	RVMNODE* rvmNode = getRVMNode(transNode->rvm);
	if (rvmNode) {
		// RESET isInTransaction and TransID of the segments involved in the Transaction
		for (int i = 0; i < transNode->numseg; i++) {
			SEGNODE* tmp = rvmNode->segments;
			while (tmp) {
				if (tmp->address == transNode->segbases[i]) {
					tmp->isInTransaction = 0;
					tmp->transID = 0;
					break;
				}
				tmp = tmp->next;
			}
		}
		// REMOVE THE TRANSNODE FROM THE LIST OF TRANSACTIONS
		TRANSNODE* transTmp = transList, *prev = NULL;
		while (transTmp) {
			if (transTmp->id == tid) {
				if (prev) {
					prev->next = transTmp->next;
				} else {
					transList = transTmp->next;
				}
				free(transTmp);
			}
			prev = transTmp;
			transTmp = transTmp->next;
		}
	} else {
		handle_error("cleanTransaction", "null argument(s)");
	}
}

/*
* - commit all changes that have been made within the specified transaction.
* When the call returns, then enough information should have been saved to disk
* so that, even if the program crashes, the changes will be seen by the program when it restarts.
*/
void rvm_commit_trans(trans_t tid) {
	// get trans node using given tid
	TRANSNODE* transNode = getTransNode(tid);
	if (!transNode) {
		return;
	}
	RVMNODE* rvmNode = getRVMNode(transNode->rvm);
	if (rvmNode) {
		int logFile = open(rvmNode->logPath, O_WRONLY|O_APPEND);
		if (logFile != -1) {
			UNDORECORD* undos = transNode->undo;
			if (undos) {
				// EXTRACT UNIQUE SEGMENTS THAT HAVE BEEN MODIFIED IN THIS TRANSACTION
				UNIQUESEGS* unisegs = (UNIQUESEGS*) malloc(sizeof(UNIQUESEGS));
				unisegs->address = undos->segBase;
				unisegs->next = NULL;
				while (undos) {
					if (isUniqueSegment(undos->segBase, unisegs)) {
						UNIQUESEGS* newEntry = (UNIQUESEGS*) malloc(sizeof(UNIQUESEGS));
						newEntry->address = undos->segBase;
						newEntry->next = unisegs;
						unisegs = newEntry;
					}
					undos = undos->next;
				}
				// WRITE UNIQUE SEGMENTS TO LOG FILE
				SEGNODE* segs = rvmNode->segments;
				char bos[4] = "BOS";
				char eos[4] = "EOS";
				char nl[2] = "\n";
				char size[5];
				UNIQUESEGS* prev = unisegs;
				while (prev) {
					SEGNODE* tmp = segs;
					while (tmp) {
						if (tmp->address == prev->address) {
							write(logFile, bos, strlen(bos));
							write(logFile, nl, strlen(nl));
							write(logFile, tmp->name, strlen(tmp->name));
							write(logFile, nl, strlen(nl));
							sprintf(size, "%lu", tmp->size);
							write(logFile, size, strlen(size));
							write(logFile, nl, strlen(nl));
							write(logFile, tmp->address, tmp->size);
							write(logFile, nl, strlen(nl));
							write(logFile, eos, strlen(eos));
							write(logFile, nl, strlen(nl));
							write(logFile, nl, strlen(nl));
						}
						tmp = tmp->next;
					}
					prev = prev->next;
				}
				// FREE UP THE UNIQUE SEGMENTS
				prev = NULL;
				while (unisegs) {
					if (prev) {
						free(prev);
					}
					prev = unisegs;
					unisegs = unisegs->next;
				}
				free(prev);
			} else {
				return; // NO CHANGES HAVE BEEN DONE YET; NO NEED TO WRITE TO LOG
			}
			close(logFile);
			// FREE UP THE UNDO RECORDS
			UNDORECORD* undoprev = NULL;
			undos = transNode->undo;
			while (undos) {
				if (undoprev) {
					free(undoprev->address);
					free(undoprev);
				}
				undoprev = undos;
				undos = undos->next;
			}
			free(undoprev->address);
			free(undoprev);
			transNode->undo = NULL;
			// RESET isInTransaction and transID fields in segnodes
			cleanTransaction(tid);
		} else {
			handle_error("rvm_commit_trans", "open");
		}
	} else {
		handle_error("rvm_commit_trans", "null argument(s)");
	}
}

/*
*- undo all changes that have happened within the specified transaction.
*/
void rvm_abort_trans(trans_t tid) {
	// get trans node using given tid
	TRANSNODE* transNode = getTransNode(tid);

	if (transNode) {
		UNDORECORD* undos = transNode->undo;
		if (undos) {
			// REVERT ALL CHANGES
			while (undos) {
				memcpy(((char*)undos->segBase) + undos->offset, undos->address, undos->size);
				undos = undos->next;
			}
			// FREE UP THE UNDO RECORDS
			UNDORECORD* prev = NULL;
			undos = transNode->undo;
			while (undos) {
				if (prev) {
					free(prev->address);
					free(prev);
				}
				prev = undos;
				undos = undos->next;
			}
			free(prev->address);
			free(prev);
			transNode->undo = NULL;
			// CLEAN UP TRANS NODE
			cleanTransaction(tid);
		} else {
			return;
		}
	} else {
		handle_error("rvm_abort_trans", "null argument");
	}
}

static void print(){
	DATA* tmp = logList;
	while(tmp){
		printf("Found segment: %s\n",tmp->segname);
		int count = tmp->len;
		for (int i = 0; i < count; ++i) {
			printf("%c",tmp->data[i]);
		}
		printf("\n");
		tmp = tmp->next;
	}
}

/*
* play through any committed or aborted items in the log file(s)
* and shrink the log file(s) as much as possible.
*/
void rvm_truncate_log(rvm_t rvm) {
	RVMNode* rvmNode = getRVMNode(rvm);
	if (rvmNode) {
		readLog(rvmNode->logPath);
		char cmd[NAME_LITERAL_SIZE];
		sprintf(cmd, "rm %s", rvmNode->logPath);
		system(cmd);
		int fd = open(rvmNode->logPath, O_CREAT, S_IRWXU);
		close(fd);
		DATA* tmp = logList;
		while (tmp) {
			sprintf(cmd, "rm %s/%s", rvmNode->directory, tmp->segname);
			system(cmd);
			sprintf(cmd, "%s/%s", rvmNode->directory, tmp->segname);
			fd = open(cmd, O_CREAT|O_RDWR, S_IRWXU);
//			for (int i = 0; i < tmp->len; ++i) {
//				std::cout<<tmp->data[i];
//			}
//			printf("\n");
			write(fd, tmp->data, tmp->len);
			close(fd);
			tmp = tmp->next;
		}
		cleanup(logList);
	} else {
		handle_error("rvm_truncate_log", "invalid rvm node");
	}
}
