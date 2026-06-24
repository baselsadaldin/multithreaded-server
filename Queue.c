//
// Created by student on 1/14/25.
#include "Queue.h"

// Function to create a new node
struct Node* createNode(int value,struct timeval arrival) {
    struct Node* newNode = (struct Node*)malloc(sizeof(struct Node));
    if (!newNode) {
        fprintf(stderr, "Memory allocation failed\n");
        exit(EXIT_FAILURE);
    }
    newNode->m_value = value;
    newNode->m_next = NULL;
    newNode->m_arrival = arrival;
    return newNode;
}
// Function to initialize a queue
struct Queue* initializeQueue() {
    // Allocate memory for the Queue
    struct Queue* queue = (struct Queue*)malloc(sizeof(struct Queue));
    if (queue == NULL) {
        exit(EXIT_FAILURE); // Exit if malloc fails
    }
    // Initialize the Queue fields
    queue->m_size = 0;
    queue->m_head = NULL;
    queue->m_tail = NULL;
    return queue; // Return the pointer to the new Queue
}
// Function to add a node to the waiting queue
void addWaitingNode(struct Queue* queue, struct Node* newNode)
{
    if (queue->m_tail == NULL) { // Queue is empty
        queue->m_head = queue->m_tail = newNode;
    } else {
        queue->m_tail->m_next = newNode; // Link the new node to the current tail
        queue->m_tail = newNode;         // Update the tail
    }
    queue->m_size++;
}
//Function to add a node to the running queue
void addWorkingNode(struct Queue* queue, struct Node* newNode,int threadID)
{
    ///Calculating the time it took for the thread to be dispatched
    struct timeval rightNow;
    gettimeofday(&rightNow, NULL);
    timersub(&rightNow, &newNode->m_arrival, &newNode->m_dispatch);
    setHandlerThread(newNode,threadID);
    ///From here it the same
    addWaitingNode(queue,newNode);
}
// Function to remove and return the front node
struct Node* popFront(struct Queue* queue) {
    if (queue->m_head == NULL) { // Queue is empty
        return NULL;
    }
    struct Node* frontNode = queue->m_head;
    queue->m_head = queue->m_head->m_next; // Update head to the next node

    if (queue->m_head == NULL) { // If the queue is now empty
        queue->m_tail = NULL;
    }

    queue->m_size--;
    frontNode->m_next = NULL; // Detach the node
    return frontNode;
}
// Function to free the queue and all its nodes
void freeQueue(struct Queue* queue) {
    struct Node* current = queue->m_head;
    while (current != NULL) {
        struct Node* temp = current;
        current = current->m_next;
        free(temp);
    }
    queue->m_head = queue->m_tail = NULL;
    queue->m_size = 0;
}
int getSize(struct Queue* queue)
{
    return queue->m_size;
}
void setHandlerThread(struct Node* node,int handlerThread)
{
    node->m_handlerThread = handlerThread;
}
int getValue(struct Node* node)
{
    return node->m_value;
}
void removeByFD(struct Queue* queue, int fd) {
    if (queue->m_head == NULL) {
        return;
    }
    struct Node* current = queue->m_head;
    struct Node* previous = NULL;
    // Traverse the queue to find the node with the matching fd
    while (current != NULL) {
        if (current->m_value == fd) {
            // Node to remove is found
            if (previous == NULL) {
                // Node to remove is the head
                queue->m_head = current->m_next;
                if (queue->m_head == NULL) {
                    // If the queue becomes empty after removal, update tail
                    queue->m_tail = NULL;
                }
            } else {
                // Node to remove is in the middle or end
                previous->m_next = current->m_next;
                if (current == queue->m_tail) {
                    // If the node is the tail, update the tail
                    queue->m_tail = previous;
                }
            }
            free(current); // Free the removed node
            queue->m_size--;
            return;
        }
        previous = current;
        current = current->m_next;
    }
}
struct timeval getArrival(struct Node* node)
{
    return node->m_arrival;
}
struct timeval getDispatch(struct Node* node)
{
    return node->m_dispatch;
}
int getHandlerThread(struct Node* node)
{
    return node->m_handlerThread;
}
void removeNode(struct Queue* queue, struct Node* Node) {
    if (queue->m_head == NULL) {
        return;
    }
    struct Node* current = queue->m_head;
    struct Node* previous = NULL;
    // Traverse the queue to find the node with the matching fd
    while (current != NULL) {
        if (current == Node) {
            // Node to remove is found
            if (previous == NULL) {
                // Node to remove is the head
                queue->m_head = current->m_next;
                if (queue->m_head == NULL) {
                    // If the queue becomes empty after removal, update tail
                    queue->m_tail = NULL;
                }
            } else {
                // Node to remove is in the middle or end
                previous->m_next = current->m_next;
                if (current == queue->m_tail) {
                    // If the node is the tail, update the tail
                    queue->m_tail = previous;
                }
            }
            free(current); // Free the removed node
            queue->m_size--;
            return;
        }
        previous = current;
        current = current->m_next;
    }
}

int removeByIndex(struct Queue* queue, int index) {
    // Check if the queue is empty
    if (queue->m_size == 0 || index < 0 || index >= queue->m_size) {
        return -1;
    }

    struct Node* current = queue->m_head;
    struct Node* previous = NULL;

    // Traverse to the desired index
    for (int i = 0; i < index; i++) {
        previous = current;
        current = current->m_next;
    }

    // Remove the node
    if (previous == NULL) {
        // If removing the head
        queue->m_head = current->m_next;
        if (queue->m_head == NULL) {
            // If the queue becomes empty
            queue->m_tail = NULL;
        }
    } else {
        previous->m_next = current->m_next;
        if (current->m_next == NULL) {
            // If removing the tail
            queue->m_tail = previous;
        }
    }
    int fd = current->m_value;
    // Free the removed node
    free(current);
    queue->m_size--;
    return fd;
}
struct Node* popBack(struct Queue* queue) {
    struct Node* current = queue->m_head;
    struct Node* previous = NULL;

    // Traverse to the last node
    while (current->m_next != NULL) {
        previous = current;
        current = current->m_next;
    }

    // If there is only one node in the queue
    if (previous == NULL) {
        queue->m_head = NULL;
        queue->m_tail = NULL;
    } else {
        previous->m_next = NULL;
        queue->m_tail = previous;
    }

    queue->m_size--;

    // Return the popped node
    return current;
}