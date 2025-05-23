#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#include <string.h>

#include <libs/queue.h>
#include <kernel/mem/pmm.h>

Queue* create_Queue(size_t size, DataType type)
{
	Queue *q = pmm_alloc_block(1);
	q->array = (QueueElement*)pmm_alloc_block(sizeof(QueueElement) * size);

	q->size = size;
	q->front = 0;
	q->near = 0;
	q->type = type;


	/* Clean mem*/
	memset(q->array, 0, sizeof(QueueElement) * size);

	return q;
}

bool isQueueFull(Queue *q)
{
	return (q->near + 1) % q->size == q->front;
}

bool isQueueEmpty(Queue *q)
{
	return q->front == q->near;
}

int enqueue(Queue* q, QueueElement element)
{
	if (isQueueFull(q))
		return -1;

	q->array[q->near] = element;

	q->near = (q->near + 1) % q->size;

	return 1;
}

QueueElement dequeue(Queue* q)
{
	if (isQueueEmpty(q))
	{
		QueueElement empty_element = {0};
		return empty_element;
	}

	QueueElement element = q->array[q->front];

	q->front = (q->front + 1) % q->size;

	return element;
}

void freeQueue(Queue* q)
{
	pmm_free_block(q->array);
	q->array = NULL;

	pmm_free_block(q);
	q = NULL;
}