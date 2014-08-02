/*
 * mcs_tree.h
 *
 *  Created on: Feb 21, 2014
 *      Author: chenhao
 */

#ifndef MCS_TREE_H_
#define MCS_TREE_H_

#include <mpi.h>
#include <math.h>
#include <stdio.h>

#define DUMMY -1

typedef struct node {
	short have_child[4];
	short child_ptr[2];
	short arrival_parent;
	short wakeup_parent;
} node;


#endif /* MCS_TREE_H_ */
