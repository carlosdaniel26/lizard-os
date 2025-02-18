#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

typedef enum {
    TYPE_BOOL,
    TYPE_INT8,
    TYPE_INT16,
    TYPE_INT32,
    TYPE_INT64,
    TYPE_FLOAT,
    TYPE_DOUBLE,
    TYPE_CHAR,
    TYPE_STRING
} DataType;

typedef union {
    bool b;

    uint8_t i8;
    uint16_t i16;
    uint32_t i32;
    uint64_t i64;

    float f;
    double d;
    char c;
    char *s;
} Data;

typedef struct QueueElement{
    Data value;
} QueueElement;

typedef struct Queue {
	struct QueueElement *array;
    DataType type;
	size_t size;

	size_t front;	/*	First pos of the Queue	*/
	size_t near;	/*	Last pos of the Queue	*/
} Queue;

Queue* create_Queue(size_t size, DataType type);
bool isQueueEmpty(Queue *q);
int enqueue(Queue* q, QueueElement element);
QueueElement dequeue(Queue* q);
void freeQueue(Queue* q);