/*
 * Earliest deadline first I/O scheduler.
 * Copyright (c) 2008 - 2012 William Pitcock <nenolod@dereferenced.org>
 *
 * This is a simple earliest deadline first scheduler which does merging based
 * on deadlines.  It is designed for use with virtual disks, provided by for example,
 * a hypervisor, or a hardware RAID card.  It may also work well with SSDs.
 *
 * It has no tunables, except for timeslice quanta and read/write weights.
 */

#include <linux/blkdev.h>
#include <linux/elevator.h>
#include <linux/bio.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/version.h>

/*
 * TODO:
 *   - convert read_list/write_list to an rbtree indexed by
 *     logical deadline
 *   - ioprio support
 */

struct edf_data {
	/* list of requests doing i/o */
	struct list_head read_list;
	struct list_head write_list;

	/* performance counters */
	unsigned int batched_requests;
	unsigned int merged_requests;

	/* settings */
	unsigned int timeslice_quanta;
	unsigned int read_weight;
	unsigned int write_weight;
};

/* Timeslice quanta.
 * The timeslice quanta determines the length of deadline expiry, by
 * applying a ratio to the timeslice quanta to yield each deadline.
 */
static const int timeslice_quanta = 2 * HZ;

/* Deadline management.
 * These functions determine read and write deadline expiry times by
 * using the timeslice quanta.
 */
static inline unsigned long
edf_read_expiry(struct edf_data *edf, struct request *rq)
{
	return jiffies + (edf->timeslice_quanta * edf->read_weight);
}

static inline unsigned long
edf_write_expiry(struct edf_data *edf, struct request *rq)
{
	return jiffies + (edf->timeslice_quanta * edf->write_weight);
}

/* Request merger.
 * If the request provided as 'next' expires before our iterated
 * 'node', we shift 'next' to the left of 'node'.
 * This yields a list ordered by pending deadlines.
 */
static void
edf_merge_request(struct request_queue *q, struct request *node,
		  struct request *next)
{
	struct edf_data *edf = q->elevator->elevator_data;

	/* one of these nodes is already cleared and should not be rescheduled. */
	if (list_empty(&node->queuelist) || list_empty(&next->queuelist))
		return;

	if (time_before(rq_fifo_time(next), rq_fifo_time(node)))
	{
		list_move(&node->queuelist, &next->queuelist);
		rq_set_fifo_time(node, rq_fifo_time(next));

		edf->merged_requests++;
	}

	rq_fifo_clear(next);
}

/* Request management code */
static void
edf_add_request(struct request_queue *q, struct request *rq)
{
	struct edf_data *edf = q->elevator->elevator_data;
	const int dir = rq_data_dir(rq);
	unsigned long deadline;
	struct list_head *head;

	deadline = dir == READ ? edf_read_expiry(edf, rq) :
			edf_write_expiry(edf, rq);
	head = dir == READ ? &edf->read_list : &edf->write_list;

	rq_set_fifo_time(rq, deadline);
	list_add_tail(&rq->queuelist, head);
}

/* Dispatch code */
static inline int
edf_dispatch_queue(struct request_queue *q, struct edf_data *edf,
		   struct list_head *head)
{
	struct request *node, *next;
	int ctr = 0;

	list_for_each_entry_safe(node, next, head, queuelist)
	{
		if (time_before(jiffies, rq_fifo_time(node)))
			break;

		rq_fifo_clear(node);
		elv_dispatch_add_tail(node->q, node);

		edf->batched_requests++;
		ctr++;
	}

	return ctr;
}

static int
edf_dispatch(struct request_queue *q, int force)
{
	struct edf_data *edf = q->elevator->elevator_data;
	int ctr = 0;

	ctr += edf_dispatch_queue(q, edf, &edf->read_list);
	ctr += edf_dispatch_queue(q, edf, &edf->write_list);

	return ctr;
}

/* List management code */
#if LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,38)
static int
edf_queue_empty(struct request_queue *q)
{
	struct edf_data *edf = q->elevator->elevator_data;

	return list_empty(&edf->read_list) && list_empty(&edf->write_list);
}
#endif

static struct request *
edf_former_request(struct request_queue *q, struct request *rq)
{
	struct edf_data *edf = q->elevator->elevator_data;
	struct list_head *head;

	head = rq_data_dir(rq) == READ ? &edf->read_list : &edf->write_list;

	if (rq->queuelist.prev == head)
		return NULL;

	return list_entry(rq->queuelist.prev, struct request, queuelist);
}

static struct request *
edf_latter_request(struct request_queue *q, struct request *rq)
{
	struct edf_data *edf = q->elevator->elevator_data;
	struct list_head *head;

	head = rq_data_dir(rq) == READ ? &edf->read_list : &edf->write_list;

	if (rq->queuelist.next == head)
		return NULL;

	return list_entry(rq->queuelist.next, struct request, queuelist);
}

/* Constructor/destructor. */
static void *
edf_init_queue(struct request_queue *q)
{
	struct edf_data *edf;

	edf = kmalloc_node(sizeof(*edf), GFP_KERNEL | __GFP_ZERO, q->node);
	if (edf == NULL)
		return NULL;

	INIT_LIST_HEAD(&edf->read_list);
	INIT_LIST_HEAD(&edf->write_list);

	edf->timeslice_quanta = timeslice_quanta;
	edf->read_weight = 2;
	edf->write_weight = 4;

	q->elevator->elevator_data = edf;

	return edf;
}

static void
edf_exit_queue(struct elevator_queue *e)
{
	struct edf_data *edf = e->elevator_data;

	BUG_ON(!list_empty(&edf->read_list));
	BUG_ON(!list_empty(&edf->write_list));

	kfree(edf);
}

/* sysfs -- basically lifted verbatim from deadline iosched */
static ssize_t
edf_sysfs_var_show(int var, char *page)
{
	return sprintf(page, "%d\n", var);
}

static ssize_t
edf_sysfs_var_store(int *var, const char *page, size_t count)
{
	char *p = (char *) page;

	*var = simple_strtol(p, &p, 10);
	return count;
}

#define SHOW_FUNCTION(__FUNC, __VAR, __CONV)				\
	static ssize_t __FUNC(struct elevator_queue *e, char *page)	\
	{								\
		struct edf_data *edf = e->elevator_data;		\
		int __data = __VAR;					\
		if (__CONV)						\
			__data = jiffies_to_msecs(__data);		\
		return edf_sysfs_var_show(__data, (page));		\
	}

SHOW_FUNCTION(edf_sysfs_read_weight_show, edf->read_weight, 0);
SHOW_FUNCTION(edf_sysfs_write_weight_show, edf->write_weight, 0);
SHOW_FUNCTION(edf_sysfs_timeslice_quanta_show, edf->timeslice_quanta, 1);
SHOW_FUNCTION(edf_sysfs_batched_requests_show, edf->batched_requests, 0);
SHOW_FUNCTION(edf_sysfs_merged_requests_show, edf->merged_requests, 0);
#undef SHOW_FUNCTION

#define STORE_FUNCTION(__FUNC, __PTR, MIN, MAX, __CONV)						\
	static ssize_t __FUNC(struct elevator_queue *e, const char *page, size_t count)		\
	{											\
		struct edf_data *edf = e->elevator_data;					\
		int __data;									\
		int ret = edf_sysfs_var_store(&__data, (page), count);				\
		if (__data < (MIN))								\
			__data = (MIN);								\
		else if (__data > (MAX))							\
			__data = (MAX);								\
		if (__CONV)									\
			*(__PTR) = msecs_to_jiffies(__data);					\
		else										\
			*(__PTR) = __data;							\
		return ret;									\
	}
#define DUMMY_STORE_FUNCTION(__FUNC, __PTR, MIN, MAX, __CONV)					\
	static ssize_t __FUNC(struct elevator_queue *e, const char *page, size_t count)		\
	{											\
		return count;									\
	}

STORE_FUNCTION(edf_sysfs_read_weight_store, &edf->read_weight, 0, INT_MAX, 0);
STORE_FUNCTION(edf_sysfs_write_weight_store, &edf->write_weight, 0, INT_MAX, 0);
STORE_FUNCTION(edf_sysfs_timeslice_quanta_store, &edf->timeslice_quanta, 0, INT_MAX, 1);

DUMMY_STORE_FUNCTION(edf_sysfs_batched_requests_store, &edf->batched_requests, 0, INT_MAX, 0);
DUMMY_STORE_FUNCTION(edf_sysfs_merged_requests_store, &edf->merged_requests, 0, INT_MAX, 0);
#undef STORE_FUNCTION
#undef DUMMY_STORE_FUNCTION

#define EDF_FS_ATTR(name) \
	__ATTR(name, S_IRUGO | S_IWUSR, edf_sysfs_##name##_show, edf_sysfs_##name##_store)

static struct elv_fs_entry edf_attrs[] = {
	EDF_FS_ATTR(read_weight),
	EDF_FS_ATTR(write_weight),
	EDF_FS_ATTR(timeslice_quanta),
	EDF_FS_ATTR(batched_requests),
	EDF_FS_ATTR(merged_requests),
	__ATTR_NULL
};

#undef EDF_FS_ATTR

static struct elevator_type iosched_edf = {
	.ops = {
		.elevator_merge_req_fn = edf_merge_request,
		.elevator_dispatch_fn = edf_dispatch,
		.elevator_add_req_fn = edf_add_request,
		.elevator_former_req_fn = edf_former_request,
		.elevator_latter_req_fn = edf_latter_request,
		.elevator_init_fn = edf_init_queue,
		.elevator_exit_fn = edf_exit_queue,
#if LINUX_VERSION_CODE <= KERNEL_VERSION(2, 6, 38)
		.elevator_queue_empty_fn = edf_queue_empty,
#endif
	},

	.elevator_attrs = edf_attrs,
	.elevator_name = "edf",
	.elevator_owner = THIS_MODULE,
};

static int __init edf_init(void)
{
	return elv_register(&iosched_edf);
}

static void __exit edf_exit(void)
{
	elv_unregister(&iosched_edf);
}

module_init(edf_init);
module_exit(edf_exit);

MODULE_AUTHOR("William Pitcock");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("edf IO scheduler");
