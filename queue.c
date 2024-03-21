#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "queue.h"

/* Notice: sometimes, Cppcheck would find the potential NULL pointer bugs,
 * but some of them cannot occur. You can suppress them by adding the
 * following line.
 *   cppcheck-suppress nullPointer
 */


/* Create an empty queue */
struct list_head *q_new()
{
    struct list_head *head = malloc(sizeof(struct list_head));
    if (!head)
        return NULL;

    INIT_LIST_HEAD(head);
    return head;
}

/* Free all storage used by queue */
void q_free(struct list_head *head)
{
    if (!head)
        return;

    element_t *curr = NULL, *tmp = NULL;
    list_for_each_entry_safe (curr, tmp, head, list) {
        q_release_element(curr);
    }
    free(head);
}

/* Insert an element at head of queue */
bool q_insert_head(struct list_head *head, char *s)
{
    if (!head)
        return false;

    element_t *new_element = malloc(sizeof(element_t));
    if (!new_element)
        return false;

    new_element->value = strdup(s);
    if (!new_element->value) {
        free(new_element);
        return false;
    }

    list_add(&new_element->list, head);
    return true;
}

/* Insert an element at tail of queue */
bool q_insert_tail(struct list_head *head, char *s)
{
    if (!head)
        return false;

    element_t *new_element = malloc(sizeof(element_t));
    if (!new_element)
        return false;

    new_element->value = strdup(s);
    if (!new_element->value) {
        free(new_element);
        return false;
    }

    list_add_tail(&new_element->list, head);
    return true;
}

/* Remove an element from head of queue */
element_t *q_remove_head(struct list_head *head, char *sp, size_t bufsize)
{
    if (!head || list_empty(head))
        return NULL;

    element_t *target = list_first_entry(head, element_t, list);
    if (sp) {
        int orig_len = strlen(target->value);
        int copy_len = (orig_len < bufsize - 1) ? orig_len : bufsize - 1;
        strncpy(sp, target->value, copy_len);
        sp[copy_len] = '\0';
    }

    list_del(&target->list);
    return target;
}

/* Remove an element from tail of queue */
element_t *q_remove_tail(struct list_head *head, char *sp, size_t bufsize)
{
    if (!head || list_empty(head))
        return NULL;

    element_t *target = list_last_entry(head, element_t, list);
    if (sp) {
        int orig_len = strlen(target->value);
        int copy_len = (orig_len < bufsize - 1) ? orig_len : bufsize - 1;
        strncpy(sp, target->value, copy_len);
        sp[copy_len] = '\0';
    }
    list_del(&target->list);
    return target;
}

/* Return number of elements in queue */
int q_size(struct list_head *head)
{
    if (!head)
        return 0;

    int size = 0;
    struct list_head *curr;
    list_for_each (curr, head)
        size++;
    return size;
}

/* Delete the middle node in queue */
bool q_delete_mid(struct list_head *head)
{
    if (!head || list_empty(head))
        return false;

    struct list_head *currNext = head->next, *currPrev = head->prev;
    while (currNext != currPrev && currNext->next != currPrev) {
        currNext = currNext->next;
        currPrev = currPrev->prev;
    }
    element_t *del = list_entry(currNext, element_t, list);
    list_del(&del->list);
    free(del->value);
    free(del);
    return true;
}

static void q_delete_dup_free_helper(struct list_head *del)
{
    if (!del)
        return;

    element_t *del_e = list_entry(del, element_t, list);
    list_del_init(&del_e->list);
    free(del_e->value);
    free(del_e);
}

/* Delete all nodes that have duplicate string */
bool q_delete_dup(struct list_head *head)
{
    if (!head)
        return false;

    // Need to sort the list first
    q_sort(head, 0);

    struct list_head **indir = &head->next;
    struct list_head *del = NULL;
    element_t *e, *next_e;
    while (*indir != head && (*indir)->next != head) {
        e = list_entry(*indir, element_t, list);
        next_e = list_entry((*indir)->next, element_t, list);
        if (!strcmp(e->value, next_e->value)) {
            while (*indir != head && (*indir)->next != head) {
                e = list_entry(*indir, element_t, list);
                next_e = list_entry((*indir)->next, element_t, list);
                if (!strcmp(e->value, next_e->value)) {
                    del = *indir;
                    *indir = (*indir)->next;
                    q_delete_dup_free_helper(del);
                    del = *indir;
                } else {
                    *indir = (*indir)->next;
                    break;
                }
            }
            q_delete_dup_free_helper(del);
            del = NULL;
        } else
            indir = &(*indir)->next;
    }
    return true;
}

/* Swap every two adjacent nodes */
void q_swap(struct list_head *head)
{
    struct list_head *curr = head->next;
    while (curr != head && curr->next != head) {
        list_move(curr, curr->next);
        curr = curr->next;
    }
}

/* Reverse elements in queue */
void q_reverse(struct list_head *head)
{
    if (!head || list_empty(head))
        return;

    struct list_head *curr = head, *nxt = NULL;
    while (nxt != head) {
        nxt = curr->next;
        curr->next = curr->prev;
        curr->prev = nxt;
        curr = nxt;
    }
}

/* Reverse the nodes of the list k at a time */
void q_reverseK(struct list_head *head, int k)
{
    // https://leetcode.com/problems/reverse-nodes-in-k-group/
}

static void merge(struct list_head *head,
                  struct list_head *left,
                  struct list_head *right,
                  bool descend)
{
    while (!list_empty(left) && !list_empty(right)) {
        element_t *l = list_first_entry(left, element_t, list);
        element_t *r = list_first_entry(right, element_t, list);
        if ((strcmp(l->value, r->value) <= 0) ^ descend)
            list_move_tail(left->next, head);
        else
            list_move_tail(right->next, head);
    }

    if (!list_empty(left))
        list_splice_tail_init(left, head);
    if (!list_empty(right))
        list_splice_tail_init(right, head);
}

static struct list_head *get_list_mid(struct list_head *head)
{
    struct list_head **indir = &head;
    for (struct list_head *fast = head->next;
         fast != head && fast->next != head; fast = fast->next->next)
        indir = &(*indir)->next;
    return *indir;
}

static void merge_sort(struct list_head *head, bool descend)
{
    if (!head || list_empty(head) || list_is_singular(head))
        return;

    LIST_HEAD(left);
    LIST_HEAD(right);
    struct list_head *mid = get_list_mid(head);
    list_cut_position(&left, head, mid);
    list_splice_init(head, &right);
    merge_sort(&left, descend);
    merge_sort(&right, descend);
    merge(head, &left, &right, descend);
}

/* Sort elements of queue in ascending/descending order */
void q_sort(struct list_head *head, bool descend)
{
    merge_sort(head, descend);
}

/* Remove every node which has a node with a strictly less value anywhere to
 * the right side of it */
int q_ascend(struct list_head *head)
{
    if (!head || list_empty(head))
        return 0;

    LIST_HEAD(descend_list);
    element_t *curr_e = NULL, *last_e = NULL;
    struct list_head *curr = NULL, *tmp = NULL;
    list_for_each_safe (curr, tmp, head) {
        curr_e = list_entry(curr, element_t, list);
        while (q_size(&descend_list) > 0) {
            last_e = list_last_entry(&descend_list, element_t, list);
            if (strcmp(curr_e->value, last_e->value) < 0) {
                list_del_init(&last_e->list);
                free(last_e->value);
                free(last_e);
            } else
                break;
        }
        list_move_tail(curr, &descend_list);
    }
    list_splice_init(&descend_list, head);
    return q_size(head);
}

/* Remove every node which has a node with a strictly greater value anywhere to
 * the right side of it */
int q_descend(struct list_head *head)
{
    if (!head || list_empty(head))
        return 0;

    LIST_HEAD(descend_list);
    element_t *curr_e = NULL, *last_e = NULL;
    struct list_head *curr = NULL, *tmp = NULL;
    list_for_each_safe (curr, tmp, head) {
        curr_e = list_entry(curr, element_t, list);
        while (q_size(&descend_list) > 0) {
            last_e = list_last_entry(&descend_list, element_t, list);
            if (strcmp(curr_e->value, last_e->value) > 0) {
                list_del_init(&last_e->list);
                free(last_e->value);
                free(last_e);
            } else
                break;
        }
        list_move_tail(curr, &descend_list);
    }
    list_splice_init(&descend_list, head);
    return q_size(head);
}

/* Merge all the queues into one sorted queue, which is in ascending/descending
 * order */
int q_merge(struct list_head *head, bool descend)
{
    if (!head || list_empty(head))
        return 0;

    queue_contex_t *qc_first = list_first_entry(head, queue_contex_t, chain);
    queue_contex_t *curr = NULL;
    list_for_each_entry (curr, head, chain) {
        if (qc_first == curr)
            continue;
        list_splice_init(curr->q, qc_first->q);
        qc_first->size += curr->size;
        curr->size = 0;
    }
    q_sort(qc_first->q, descend);
    return qc_first->size;
}
