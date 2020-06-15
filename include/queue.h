//
// Created by up0 on 2020/6/3.
//

#ifndef INCLUDE_QUEUE_H_
/*
 * Tail queue definitions.
 */
#define    _TAILQ_HEAD(name, type, qual)                    \
struct name {                                \
    qual type *tqh_first;        /* first element */        \
    qual type *qual *tqh_last;    /* addr of last next element */    \
}
#define TAILQ_HEAD(name, type)    _TAILQ_HEAD(name, struct type,)

#define    TAILQ_HEAD_INITIALIZER(head)                    \
    { NULL, &(head).tqh_first }

#define    _TAILQ_ENTRY(type, qual)                    \
struct {                                \
    qual type *tqe_next;        /* next element */        \
    qual type *qual *tqe_prev;    /* address of previous next element */\
}
#define TAILQ_ENTRY(type)    _TAILQ_ENTRY(struct type,)

/*
 * Tail queue functions.
 */
#define    TAILQ_INIT(head) do {                        \
    (head)->tqh_first = NULL;                    \
    (head)->tqh_last = &(head)->tqh_first;                \
} while (/*CONSTCOND*/0)

#define    TAILQ_INSERT_HEAD(head, elm, field) do {            \
    if (((elm)->field.tqe_next = (head)->tqh_first) != NULL)    \
        (head)->tqh_first->field.tqe_prev =            \
            &(elm)->field.tqe_next;                \
    else                                \
        (head)->tqh_last = &(elm)->field.tqe_next;        \
    (head)->tqh_first = (elm);                    \
    (elm)->field.tqe_prev = &(head)->tqh_first;            \
} while (/*CONSTCOND*/0)

#define    TAILQ_INSERT_TAIL(head, elm, field) do {            \
    (elm)->field.tqe_next = NULL;                    \
    (elm)->field.tqe_prev = (head)->tqh_last;            \
    *(head)->tqh_last = (elm);                    \
    (head)->tqh_last = &(elm)->field.tqe_next;            \
} while (/*CONSTCOND*/0)

#define    TAILQ_INSERT_AFTER(head, listelm, elm, field) do {        \
    if (((elm)->field.tqe_next = (listelm)->field.tqe_next) != NULL)\
        (elm)->field.tqe_next->field.tqe_prev =        \
            &(elm)->field.tqe_next;                \
    else                                \
        (head)->tqh_last = &(elm)->field.tqe_next;        \
    (listelm)->field.tqe_next = (elm);                \
    (elm)->field.tqe_prev = &(listelm)->field.tqe_next;        \
} while (/*CONSTCOND*/0)

#define    TAILQ_INSERT_BEFORE(listelm, elm, field) do {            \
    (elm)->field.tqe_prev = (listelm)->field.tqe_prev;        \
    (elm)->field.tqe_next = (listelm);                \
    *(listelm)->field.tqe_prev = (elm);                \
    (listelm)->field.tqe_prev = &(elm)->field.tqe_next;        \
} while (/*CONSTCOND*/0)

#define    TAILQ_REMOVE(head, elm, field) do {                \
    if (((elm)->field.tqe_next) != NULL)                \
        (elm)->field.tqe_next->field.tqe_prev =        \
            (elm)->field.tqe_prev;                \
    else                                \
        (head)->tqh_last = (elm)->field.tqe_prev;        \
    *(elm)->field.tqe_prev = (elm)->field.tqe_next;            \
} while (/*CONSTCOND*/0)

#define    TAILQ_FOREACH(var, head, field)                    \
    for ((var) = ((head)->tqh_first);                \
        (var);                            \
        (var) = ((var)->field.tqe_next))

#define    TAILQ_FOREACH_REVERSE(var, head, headname, field)        \
    for ((var) = (*(((struct headname *)((head)->tqh_last))->tqh_last));    \
        (var);                            \
        (var) = (*(((struct headname *)((var)->field.tqe_prev))->tqh_last)))

#define    TAILQ_CONCAT(head1, head2, field) do {                \
    if (!TAILQ_EMPTY(head2)) {                    \
        *(head1)->tqh_last = (head2)->tqh_first;        \
        (head2)->tqh_first->field.tqe_prev = (head1)->tqh_last;    \
        (head1)->tqh_last = (head2)->tqh_last;            \
        TAILQ_INIT((head2));                    \
    }                                \
} while (/*CONSTCOND*/0)

/*
 * Tail queue access methods.
 */
#define    TAILQ_EMPTY(head)        ((head)->tqh_first == NULL)
#define    TAILQ_FIRST(head)        ((head)->tqh_first)
#define    TAILQ_NEXT(elm, field)        ((elm)->field.tqe_next)

#define    TAILQ_LAST(head, headname) \
    (*(((struct headname *)((head)->tqh_last))->tqh_last))
#define    TAILQ_PREV(elm, headname, field) \
    (*(((struct headname *)((elm)->field.tqe_prev))->tqh_last))

#define INCLUDE_QUEUE_H_

#endif // MEM_INCLUDE_QUEUE_H_
