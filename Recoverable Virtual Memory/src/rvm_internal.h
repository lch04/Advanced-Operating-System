#include <stdio.h>
#include <string.h>

typedef unsigned long rvm_t;
typedef unsigned long trans_t;

typedef struct data_t{
	char* segname;
	char* data;
	int len;
	struct data_t* next;
} DATA;

typedef struct UniqueSegments {
	void* address;
	struct UniqueSegments* next;
} UNIQUESEGS;

typedef struct UndoRecord {
	void* segBase;    // segment base address
	unsigned long offset;    // offset to segment base address
	unsigned long size;    // size of memory chunk in bytes
	void* address;		// points to the backup region
	struct UndoRecord* next;    // points to next undo record
} UNDORECORD;

typedef struct TransactionNode {
	trans_t id;    // unique id for each transaction
	unsigned long numseg;    // number of segments this transaction modifies
	void** segbases;	// points to list of segments to be modified
	struct TransactionNode* next;    // points to next transaction node
	rvm_t rvm;    //  id of corresponding RVM node
	UNDORECORD* undo;    // points to undo record
	// TODO: something to preserve order of segments being modified
} TRANSNODE;

typedef struct SegmentNode {
	char* name;    // segment name
	unsigned long size;    // segment size in bytes
	void* address;    // starting address of segment
	int isInTransaction;    // whether the segment is already in one transaction
	trans_t transID;    // id of transaction modifying this segment
	int isMapped;	// indicates whether this segment is already mapped
	struct SegmentNode* next;    // points to next segment
} SEGNODE;

typedef struct RVMNode {
	rvm_t id;	// unique id for each RVM node
	const char* directory;	// directory info of the node
	char* logPath;	// path info of the log file in directory
	FILE* logFile;	// file descriptor of the log file
	SEGNODE* segments;    // keeps track of associated segments
	struct RVMNode* next;	// points to next RVM node
	//TRANSNODE* transactions;    // keeps track of associated transactions
} RVMNODE;
