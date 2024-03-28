#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "queue.h"

/* Notice: sometimes, Cppcheck would find the potential NULL pointer bugs,
 * but some of them cannot occur. You can suppress them by adding the
 * following line.
 *   cppcheck-suppress nullPointer
 */

typedef int (*list_cmp_func_t)(void *,
                               const struct list_head *,
                               const struct list_head *);

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
    element_t *e = NULL, *next_e = NULL;
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
    if (!head || list_empty(head))
        return;

    int count = 1;
    struct list_head *curr = head->next, *prev = head, *nxt = curr->next;
    LIST_HEAD(tmp_list);
    while (curr != head) {
        if (count % k == 0) {
            list_cut_position(&tmp_list, prev, curr);
            q_reverse(&tmp_list);
            list_splice_init(&tmp_list, prev);
            prev = nxt->prev;
        }
        count++;
        curr = nxt;
        nxt = nxt->next;
    }
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

static int compare(void *priv,
                   const struct list_head *a,
                   const struct list_head *b)
{
    if (a == b)
        return 0;
    element_t *a_e = list_entry(a, element_t, list);
    element_t *b_e = list_entry(b, element_t, list);
    int res = strcmp(a_e->value, b_e->value);

    if (priv)
        *((int *) priv) += 1;

    return res;
}

static inline size_t run_size(struct list_head *head)
{
    if (!head)
        return 0;
    if (!head->next)
        return 1;
    return (size_t) (head->next->prev);
}

struct pair {
    struct list_head *head, *next;
};

static size_t stk_size;

static struct list_head *merge_run(void *priv,
                                   list_cmp_func_t cmp,
                                   struct list_head *a,
                                   struct list_head *b)
{
    struct list_head *head = NULL;
    struct list_head **tail = &head;

    for (;;) {
        /* if equal, take 'a' -- important for sort stability */
        if (cmp(priv, a, b) <= 0) {
            *tail = a;
            tail = &(*tail)->next;
            a = a->next;
            if (!a) {
                *tail = b;
                break;
            }
        } else {
            *tail = b;
            tail = &(*tail)->next;
            b = b->next;
            if (!b) {
                *tail = a;
                break;
            }
        }
    }
    return head;
}

static void build_prev_link(struct list_head *head,
                            struct list_head *tail,
                            struct list_head *list)
{
    tail->next = list;
    do {
        list->prev = tail;
        tail = list;
        list = list->next;
    } while (list);

    /* The final links to make a circular doubly-linked list */
    tail->next = head;
    head->prev = tail;
}

static void merge_final(void *priv,
                        list_cmp_func_t cmp,
                        struct list_head *head,
                        struct list_head *a,
                        struct list_head *b)
{
    struct list_head *tail = head;

    for (;;) {
        /* if equal, take 'a' -- important for sort stability */
        if (cmp(priv, a, b) <= 0) {
            tail->next = a;
            a->prev = tail;
            tail = a;
            a = a->next;
            if (!a)
                break;
        } else {
            tail->next = b;
            b->prev = tail;
            tail = b;
            b = b->next;
            if (!b) {
                b = a;
                break;
            }
        }
    }

    /* Finish linking remainder of list b on to tail */
    build_prev_link(head, tail, b);
}

static struct pair find_run(void *priv,
                            struct list_head *list,
                            list_cmp_func_t cmp)
{
    size_t len = 1;
    struct list_head *next = list->next, *head = list;
    struct pair result;

    if (!next) {
        result.head = head, result.next = next;
        return result;
    }

    if (cmp(priv, list, next) > 0) {
        /* decending run, also reverse the list */
        struct list_head *prev = NULL;
        do {
            len++;
            list->next = prev;
            prev = list;
            list = next;
            next = list->next;
            head = list;
        } while (next && cmp(priv, list, next) > 0);
        list->next = prev;
    } else {
        do {
            len++;
            list = next;
            next = list->next;
        } while (next && cmp(priv, list, next) <= 0);
        list->next = NULL;
    }
    head->prev = NULL;
    head->next->prev = (struct list_head *) len;
    result.head = head, result.next = next;
    return result;
}

static struct list_head *merge_at(void *priv,
                                  list_cmp_func_t cmp,
                                  struct list_head *at)
{
    size_t len = run_size(at) + run_size(at->prev);
    struct list_head *prev = at->prev->prev;
    struct list_head *list = merge_run(priv, cmp, at->prev, at);
    list->prev = prev;
    list->next->prev = (struct list_head *) len;
    --stk_size;
    return list;
}

static struct list_head *merge_force_collapse(void *priv,
                                              list_cmp_func_t cmp,
                                              struct list_head *tp)
{
    while (stk_size >= 3) {
        if (run_size(tp->prev->prev) < run_size(tp)) {
            tp->prev = merge_at(priv, cmp, tp->prev);
        } else {
            tp = merge_at(priv, cmp, tp);
        }
    }
    return tp;
}

static struct list_head *merge_collapse(void *priv,
                                        list_cmp_func_t cmp,
                                        struct list_head *tp)
{
    int n;
    while ((n = stk_size) >= 2) {
        if ((n >= 3 &&
             run_size(tp->prev->prev) <= run_size(tp->prev) + run_size(tp)) ||
            (n >= 4 && run_size(tp->prev->prev->prev) <=
                           run_size(tp->prev->prev) + run_size(tp->prev))) {
            if (run_size(tp->prev->prev) < run_size(tp)) {
                tp->prev = merge_at(priv, cmp, tp->prev);
            } else {
                tp = merge_at(priv, cmp, tp);
            }
        } else if (run_size(tp->prev) <= run_size(tp)) {
            tp = merge_at(priv, cmp, tp);
        } else {
            break;
        }
    }

    return tp;
}


void timsort(void *priv, struct list_head *head, bool descend)
{
    if (!head || list_empty(head) || list_is_singular(head))
        return;

    stk_size = 0;
    list_cmp_func_t cmp = compare;
    struct list_head *list = head->next, *tp = NULL;
    if (head == head->prev)
        return;

    /* Convert to a null-terminated singly-linked list. */
    head->prev->next = NULL;

    do {
        /* Find next run */
        struct pair result = find_run(priv, list, cmp);
        result.head->prev = tp;
        tp = result.head;
        list = result.next;
        stk_size++;
        tp = merge_collapse(priv, cmp, tp);
    } while (list);

    /* End of input; merge together all the runs. */
    tp = merge_force_collapse(priv, cmp, tp);

    /* The final merge; rebuild prev links */
    struct list_head *stk0 = tp, *stk1 = stk0->prev;
    while (stk1 && stk1->prev)
        stk0 = stk0->prev, stk1 = stk1->prev;
    if (stk_size <= 1) {
        build_prev_link(head, head, stk0);
        return;
    }
    merge_final(priv, cmp, head, stk1, stk0);
}
