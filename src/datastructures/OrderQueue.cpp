#include "OrderQueue.h"

void OrderQueue::push(Order order) { q.push(order); }

Order OrderQueue::popNext() { Order order=q.front();q.pop();return order; }

bool OrderQueue::empty() const { return q.empty(); }

int OrderQueue::size() const { return static_cast<int>(q.size()); }
