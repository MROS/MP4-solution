#include "../linked_list.hpp"
#include <stdio.h>

bool push_one() {
	LinkedList<int> list;
	list.push_back(1);
	
	bool ok = true;

	ok = ok && (list.head->value == 1);
	ok = ok && (list.tail->value == 1);
	
	return ok;
}

bool push_ten() {
	bool ok = true;

	LinkedList<int> list;
	for (int i = 0; i < 10; i++) {
		list.push_back(i);
	}

	ok = ok && (list.head->prev == nullptr);
	ok = ok && (list.tail->next == nullptr);

	Node<int> *iter = list.head;
	for (int i = 0; i < 10; i++) {
		ok = ok && (iter->value == i);
		iter = iter->next;
	}

	iter = list.tail;
	for (int i = 9; i >= 0; i--) {
		ok = ok && (iter->value == i);
		iter = iter->prev;
	}

	return ok;
}

bool erase_ten() {

	LinkedList<int> list;
	for (int i = 0; i < 10; i++) {
		list.push_back(i);
	}

	Node<int> *mid_node = list.head->next->next;
	list.erase(mid_node);

	Node <int> *iter = list.head;
	int count = 0;
	while (list.head != nullptr) {
		list.erase(list.head);
		count += 1;
	}
	return count == 9;
}

int main() {
	if (push_one()) {
		printf("塞入一個正確\n");
	} else {
		printf("塞入一個錯誤\n");
	}

	if (push_ten()) {
		printf("塞入十個正確\n");
	} else {
		printf("塞入十個錯誤\n");
	}

	if (erase_ten()) {
		printf("刪除十個正確\n");
	} else {
		printf("刪除十個錯誤\n");
	}
}
