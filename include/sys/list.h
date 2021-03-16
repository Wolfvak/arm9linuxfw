#pragma once

typedef struct listNode_s listNode_s;

#define listHead_s listNode_s

typedef struct listNode_s {
	listNode_s *prev, *next;
} listNode_s;

#define listNext(n)	((n)->next)
#define listPrev(n)	((n)->prev)

#define listHead(h)	listNext(h)
#define listTail(h)	listPrev(h)

#define listNodeInit(n)	do{(n)->prev = (n)->next = (n);}while(0)
#define listHeadInit(h)	listNodeInit(h)

#define listNodeEmbedded(n)	((n) != ((n)->prev))

#define listEmpty(h)	(!listNodeEmbedded(h))
#define listAdd(n, h)	listAddTail(n, h)

static inline void listRemove(listNode_s *n) {
	n->next->prev = n->prev;
	n->prev->next = n->next;
	n->prev = n->next = n;
}

static inline void listAddHead(listNode_s *n, listHead_s *h) {
	n->next = h->next;
	h->next->prev = n;
	n->prev = h;
	h->next = n;
}

static inline void listAddTail(listNode_s *n, listHead_s *h) {
	n->prev = h->prev;
	h->prev->next = n;
	n->next = h;
	h->prev = n;
}

static inline bool listAppendIfNotEmbedded(listNode_s *n, listHead_s *h) {
	if (!listNodeEmbedded(n)) {
		listAddTail(n, h);
		return true;
	}
	return false;
}
