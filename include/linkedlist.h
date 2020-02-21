#pragma once

class ListNode {
public:
	constexpr ListNode(void)
	 : next(this), prev(this) {}

	constexpr ListNode(ListNode *head)
	 : next(this), prev(this) { this->InsertBefore(head); }

	void InsertAfter(ListNode *node) {
		this->prev = node;
		this->next = node->next;
		node->next->prev = this;
		node->next = this;
	}

	void InsertBefore(ListNode *node) {
		this->next = node;
		this->prev = node->prev;
		node->prev->next = this;
		node->prev = node;
	}

	void Insert(ListNode *node) {
		this->InsertAfter(node);
	}

	void Remove(void) {
		this->prev->next = this->next;
		this->next->prev = this->prev;
		this->next = this->prev = this;
	}

	ListNode *Next(void) { return this->next; }
	ListNode *Prev(void) { return this->prev; }

	bool Empty(void) {
		return (this->next == this);
	}

private:
	ListNode *next, *prev;
};

using ListHead = ListNode;
