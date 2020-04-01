/**
##########################################################################
# If not stated otherwise in this file or this component's LICENSE
# file the following copyright and licenses apply:
#
# Copyright 2019 RDK Management
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
##########################################################################
**/



#ifndef _LINKEDLIST_H
#define	_LINKEDLIST_H

template<typename K, typename V>
struct LinkedListNode {
	LinkedListNode *pPrev;
	LinkedListNode *pNext;
	V value;
	K key;
};

template<typename K, typename V>
class LinkedList {
private:
	map<K, LinkedListNode<K, V> *> _allNodes;
	LinkedListNode<K, V> *_pHead;
	LinkedListNode<K, V> *_pTail;
	LinkedListNode<K, V> *_pCurrent;
	bool _denyNext;
	bool _denyPrev;
public:

	inline LinkedList() {
		_pHead = NULL;
		_pTail = NULL;
		_pCurrent = NULL;
		_denyNext = false;
		_denyPrev = false;
	}

	inline ~LinkedList() {
		Clear();
	}

	inline bool HasKey(const K &key) {
		return _allNodes.find(key) != _allNodes.end();
	}

	inline LinkedListNode<K, V> * AddHead(const K &key, const V &value) {
		if (MAP_HAS1(_allNodes, key)) {
#ifndef ANDROID
			ASSERT("Item already present inside the list");
#endif
			return NULL;
		}
		LinkedListNode<K, V> *pNode = new LinkedListNode<K, V>();
		pNode->value = value;
		pNode->key = key;
		if (_pHead == NULL) {
			pNode->pNext = pNode->pPrev = NULL;
			_pHead = _pTail = pNode;
		} else {
			InsertBefore(_pHead, pNode);
		}
		_allNodes[key] = pNode;
		return pNode;
	}

	inline LinkedListNode<K, V> * AddTail(const K &key, const V &value) {
		if (MAP_HAS1(_allNodes, key)) {
#ifndef ANDROID
			ASSERT("Item already present inside the list");
#endif
			return NULL;
		}
		LinkedListNode<K, V> *pNode = new LinkedListNode<K, V>();
		pNode->value = value;
		pNode->key = key;
		if (_pTail == NULL) {
			pNode->pNext = NULL;
			pNode->pPrev = NULL;
			_pHead = _pTail = pNode;
		} else {
			InsertAfter(_pTail, pNode);
		}
		_allNodes[key] = pNode;
		return pNode;
	}

	void Clear() {
		_pCurrent = NULL;
		_denyNext = false;
		_denyPrev = false;
		while (_pHead != NULL)
			Remove(_pHead);
		_pHead = NULL;
		_pTail = NULL;
	}

	inline size_t Size() {
		return _allNodes.size();
	}

	inline void Remove(const K &key) {
		if (!MAP_HAS1(_allNodes, key))
			return;
		Remove(_allNodes[key]);
	}

	inline void Remove(LinkedListNode<K, V> *pNode) {
		if (pNode == NULL)
			return;

		if (!MAP_HAS1(_allNodes, pNode->key))
			return;

		if (pNode->pPrev == NULL)
			_pHead = pNode->pNext;
		else
			pNode->pPrev->pNext = pNode->pNext;

		if (pNode->pNext == NULL)
			_pTail = pNode->pPrev;
		else
			pNode->pNext->pPrev = pNode->pPrev;

		_denyNext = false;
		_denyPrev = false;
		if (_pCurrent == pNode) {
			if (_pCurrent->pPrev != NULL) {
				_pCurrent = _pCurrent->pPrev;
				_denyPrev = (_pCurrent != NULL);
			} else {
				_pCurrent = _pCurrent->pNext;
				_denyNext = (_pCurrent != NULL);
			}
		}
		_allNodes.erase(pNode->key);
		delete pNode;
	}

	inline LinkedListNode<K, V> *MoveHead() {
		_denyNext = false;
		_denyPrev = false;
		return (_pCurrent = _pHead);
	}

	inline LinkedListNode<K, V> *MovePrev() {
		if (_denyPrev) {
			_denyPrev = false;
			return _pCurrent;
		}
		return (_pCurrent = (_pCurrent != NULL) ? _pCurrent->pPrev : NULL);
	}

	inline LinkedListNode<K, V> *Current() {
		return _pCurrent;
	}

	inline LinkedListNode<K, V> *MoveNext() {
		if (_denyNext) {
			_denyNext = false;
			return _pCurrent;
		}
		return (_pCurrent = (_pCurrent != NULL) ? _pCurrent->pNext : NULL);
	}

	inline LinkedListNode<K, V> *MoveTail() {
		_denyNext = false;
		_denyPrev = false;
		return (_pCurrent = _pTail);
	}

private:

	inline void InsertAfter(LinkedListNode<K, V> *pPosition, LinkedListNode<K, V> *pNode) {
		pNode->pPrev = pPosition;
		pNode->pNext = pPosition->pNext;
		if (pPosition->pNext == NULL)
			_pTail = pNode;
		else
			pPosition->pNext->pPrev = pNode;
		pPosition->pNext = pNode;
	}

	inline void InsertBefore(LinkedListNode<K, V> *pPosition, LinkedListNode<K, V> *pNode) {
		pNode->pPrev = pPosition->pPrev;
		pNode->pNext = pPosition;
		if (pPosition->pPrev == NULL)
			_pHead = pNode;
		else
			pPosition->pPrev->pNext = pNode;
		pPosition->pPrev = pNode;
	}
};

#endif	/* _LINKEDLIST_H */
