#ifndef MMC_QUEUE_H
#define MMC_QUEUE_H

static inline bool mmc_req_is_special(struct request *req)
{
	return req &&
		(req_op(req) == REQ_OP_FLUSH ||
		 req_op(req) == REQ_OP_DISCARD ||
		 req_op(req) == REQ_OP_SECURE_ERASE);
}

struct request;
struct task_struct;

struct mmc_blk_request {
	struct mmc_request	mrq;
	struct mmc_command	sbc;
	struct mmc_command	cmd;
	struct mmc_command	stop;
	struct mmc_data		data;
	int			retune_retry_done;
};

enum mmc_packed_type {
	MMC_PACKED_NONE = 0,
	MMC_PACKED_WRITE,
};

#define mmc_packed_cmd(type)	((type) != MMC_PACKED_NONE)
#define mmc_packed_wr(type)	((type) == MMC_PACKED_WRITE)

struct mmc_packed {
	struct list_head	list;
	__le32			cmd_hdr[1024];
	unsigned int		blocks;
	u8			nr_entries;
	u8			retries;
	s16			idx_failure;
};

struct mmc_queue_req {
	struct request		*req;
	struct mmc_blk_request	brq;
	struct scatterlist	*sg;
	char			*bounce_buf;
	struct scatterlist	*bounce_sg;
	unsigned int		bounce_sg_len;
	struct mmc_async_req	mmc_active;
	enum mmc_packed_type	cmd_type;
	struct mmc_packed	*packed;
	struct mmc_cmdq_req	cmdq_req;
};

struct mmc_queue {
	struct mmc_card		*card;
	struct task_struct	*thread;
	struct semaphore	thread_sem;
	unsigned int		flags;
#define MMC_QUEUE_SUSPENDED	(1 << 0)
#define MMC_QUEUE_NEW_REQUEST	(1 << 1)
	void			*data;
	struct request_queue	*queue;
	struct mmc_queue_req	mqrq[2];
	struct mmc_queue_req	*mqrq_cur;
	struct mmc_queue_req	*mqrq_prev;
	struct mmc_queue_req	*mqrq_cmdq;
	struct work_struct	cmdq_err_work;

	struct completion	cmdq_pending_req_done;
	struct completion	cmdq_shutdown_complete;
	struct request		*cmdq_req_peeked;

	int (*cmdq_issue_fn)(struct mmc_queue *,
			     struct request *);
	void (*cmdq_complete_fn)(struct request *);
	void (*cmdq_error_fn)(struct mmc_queue *);
	enum blk_eh_timer_return (*cmdq_req_timed_out)(struct request *);
	void (*cmdq_shutdown)(struct mmc_queue *);
};

extern int mmc_init_queue(struct mmc_queue *, struct mmc_card *, spinlock_t *,
			  const char *, int);
extern void mmc_cleanup_queue(struct mmc_queue *);
extern int mmc_queue_suspend(struct mmc_queue *, int);
extern void mmc_queue_resume(struct mmc_queue *);

extern unsigned int mmc_queue_map_sg(struct mmc_queue *,
				     struct mmc_queue_req *);
extern void mmc_queue_bounce_pre(struct mmc_queue_req *);
extern void mmc_queue_bounce_post(struct mmc_queue_req *);

extern int mmc_packed_init(struct mmc_queue *, struct mmc_card *);
extern void mmc_packed_clean(struct mmc_queue *);

extern int mmc_access_rpmb(struct mmc_queue *);
extern int mmc_cmdq_init(struct mmc_queue *mq, struct mmc_card *card);
extern void mmc_cmdq_clean(struct mmc_queue *mq, struct mmc_card *card);
#endif
