/*
 * LRUCache.h
 *
 *  Created on: Mar 11, 2014
 *      Author: chenhaoliu
 */

#ifndef LRUCACHE_H_
#define LRUCACHE_H_

#include <string>
#include <vector>
#include <map>

using namespace std;

template<class K, class T>
struct Node{
    K key;
    T data;
    Node *prev, *next;
};

class LRUCache{
private:
	map<string,Node<string,string>*>* hashmap;
	vector<Node<string,string>*> entries;
	Node<string,string>* head;
	Node<string,string>* tail;
	size_t limit_size;
	size_t current_size;
public:
	LRUCache(size_t size);
	virtual ~LRUCache();
	bool contains(string key);
	string get(string key);
	void put(string key, string data);
	size_t getSize();
	string getRecent();
	string getOld();
private:
	bool isFull(string data) {
		if (current_size + data.size() > limit_size) {
			return true;
		} else {
			return false;
		}
	}
    void detach(Node<string,string>* node) {
        node->prev->next = node->next;
        node->next->prev = node->prev;
    }
    void attach(Node<string,string>* node) {
        node->prev = head;
        node->next = head->next;
        head->next = node;
        node->next->prev = node;
    }
};

#endif /* LRUCACHE_H_ */
