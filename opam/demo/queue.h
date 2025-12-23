#ifndef QUEUE_H
#define QUEUE_H

#include <stdio.h>
#include <stdbool.h>

#define SIZE 5

typedef struct {
    int data[SIZE];
    int front;
    int rear;
    int count;
} Queue;

// Initialize the queue
static inline void initQueue(Queue *q) {
    q->front = 0;
    q->rear = -1;
    q->count = 0;
}

// Check if queue is empty
static inline bool isEmpty(Queue *q) {
    return q->count == 0;
}

// Check if queue is full
static inline bool isFull(Queue *q) {
    return q->count == SIZE;
}

// Remove and return the front item
static inline int dequeue(Queue *q) {
    if (isEmpty(q)) {
        printf("Queue is empty. Cannot dequeue.\n");
        return -1;
    }
    int val = q->data[q->front];
    q->front = (q->front + 1) % SIZE;
    q->count--;
    return val;
}

// Add an item to the queue, dequeueing front if full
static inline void enqueue(Queue *q, int value) {
    if (isFull(q)) {
        int removed = dequeue(q);
        printf("Queue full. Dequeued: %d to make space.\n", removed);
    }
    q->rear = (q->rear + 1) % SIZE;
    q->data[q->rear] = value;
    q->count++;
    printf("Enqueued: %d\n", value);
}

// Peek at the front item
static inline int peek(Queue *q) {
    if (isEmpty(q)) {
        printf("Queue is empty. No front element.\n");
        return -1;
    }
    return q->data[q->front];
}

// Display queue contents
static inline void displayQueue(Queue *q) {
    if (isEmpty(q)) {
        printf("Queue is empty.\n");
        return;
    }

    printf("Queue contents: ");
    for (int i = 0; i < q->count; i++) {
        int idx = (q->front + i) % SIZE;
        printf("%d ", q->data[idx]);
    }
    printf("\n");
}

#endif // QUEUE_H

