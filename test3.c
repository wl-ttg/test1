#include <hiredis/hiredis.h>
#include "rupay.h"
#include "host_q.h"
#include "signal_handlers.h"

/* 
 * ===  FUNCTION  ======================================================================
 *         Name : del_rupay_queues
 *      Created : 2016-12-07 18:04:00
 *       Author : Sathishkumar D 
 *  Description : Signal Handler. It will remove the current Queues.
 * =====================================================================================
 */
void RuPay_remove_queues()
{
	int msgid;
	if ((msgid = open_msg_queue(rupay_req_key)) >= 0) {
		if (msgctl(msgid, IPC_RMID, NULL) == 0) {
			printf("Rupay Request Message queue was deleted.\n");
		}
	}

	if ((msgid = open_msg_queue(rupay_res_key)) >= 0) {
		if (msgctl(msgid, IPC_RMID, NULL) == 0) {
			printf("Rupay Response Message queue was deleted.\n");
		}
	}
	exit(EXIT_SUCCESS);
}

/* 
 * ===  FUNCTION  ======================================================================
 *         Name : register_signals
 *      Created : 2016-12-07 18:08:24
 *       Author : Sathishkumar D 
 *  Description : This will register signals for this Hitachi Q Processor.
 * =====================================================================================
 */
void RuPay_register_signals()
{
	struct sigaction sa, sa2, sa3;

	// Setup the sighub handler
	sa.sa_handler = SIG_IGN;
	sa2.sa_handler = RuPay_remove_queues;
	sa3.sa_handler = alarm_handler;

	// Restart the system call, if at all possible
	sa.sa_flags = SA_RESTART|SA_NOCLDWAIT;
	sa2.sa_flags = SA_RESTART;
	sa3.sa_flags = 0;	// Do not set SA_RESTART

	// Block every signal during the handler
	sigfillset(&sa.sa_mask);
	sigfillset(&sa2.sa_mask);
	sigfillset(&sa3.sa_mask);

	/* Registering signal handler */
	sigaction(SIGCHLD, &sa, NULL);
	sigaction(SIGTERM, &sa2, NULL);
	sigaction(SIGALRM, &sa3, NULL);
}

/* 
 * ===  FUNCTION  ======================================================================
 *         Name : set_mq_map_key
 *      Created : 2017-03-09 16:39:55
 *       Author : Sathishkumar D 
 *  Description : This sets response map key value to Redis Server.
 * =====================================================================================
 */
int set_mq_map_key(const char *tid, const char *rrn, long res_qid)
{
	int rc, timeout = 3, key_expire_at = 50;
	char map_key[50], map_value[50];
	redisContext *c;
	redisReply *reply;

	/* Making connection with Redis Server */
	if ((c = redis_connect(NULL, FALSE, timeout)) == NULL) {
		return -2;
	}

	sprintf(map_key, "%s:%s", tid, rrn);
	sprintf(map_value, "%ld", res_qid);

	if ((rc = set_qmap_key(c, map_key, map_value, key_expire_at)) != 0) {
		/* Error : Not able to set map key in Redis. */
		if (c)	redisFree(c);
		return rc;
	}

	/* Disconnects and frees the context */
	if (c)	redisFree(c);
	return 0;
}

/* 
 * ===  FUNCTION  ======================================================================
 *         Name : validate_host_request
 *      Created : 29-11-2016 12:41:51
 *       Author : Sathishkumar D 
 *  Description : This will validate the hitachi request. Throw error.
 * =====================================================================================
 */
int validate_host_request(struct host_st *req)
{
	int q_holdtimeout = 15;
	time_t time_now = 0;

	// FIXME : Commented.
	//time(&time_now);
	//
	///* Checking message Q request time */
	//if ((time_now - req->q_time) > q_holdtimeout) {
	//	log_error(SWITCH, "RuPay Message Q hold (read) timeout reached. Discarding this request.\n");
	//	return -2;
	//}

	return 0;
}

/* 
 * ===  FUNCTION  ======================================================================
 *         Name : process_rupay_messageQ
 *      Created : 07-12-2016 17:36:27
 *       Author : Sathishkumar D
 *  Description : This will process RuPay Q.
 * =====================================================================================
 */
int process_RuPay_messageQ( char *sink_node )
{
	/* Initialization */
	pid_t pid;
	int stat, ret, rc, timeout = 30;
	struct msqid_ds buf;

	log_info(SWITCH, "RUPAY Queue Processor is started.\n");

	/* Register signals */
	RuPay_register_signals();

	/* Init Message Queue */
	RuPay_queues_init();

	/* Memory allocation */
	rupay_req = (struct host_st *) malloc(sizeof(struct host_st));
	rupay_res = (struct host_st *) malloc(sizeof(struct host_st));

	while(1) {
		/* Initialize Memory */
		memset(rupay_req, 0, sizeof(struct host_st));
		memset(rupay_res, 0, sizeof(struct host_st));

		/* Reading from Message Queue */
		if ((rc = rupay_msg_queue_read( rupay_req_id, 0, rupay_req, timeout )) < 0) {
			if (rc == -1) {
				log_error(SWITCH, "ERROR : RuPAY Queue does NOT exist. Exiting.\n");
				exit(EXIT_FAILURE);
			}

			log_debug(SWITCH, "No Message in RuPay MsgQueue.\n");
			continue;
		}

		/* Creating New Process */
		if ((pid = fork()) == 0)
		{
			isosenddata = (struct iso8583fmt *) malloc(sizeof(struct iso8583fmt));
			isoparsedata = (struct iso8583fmt *) malloc(sizeof(struct iso8583fmt));
			memset(rupay_res, 0, sizeof(struct host_st));
			memcpy(rupay_res, rupay_req, sizeof(struct host_st) );

			/* Communicating with Hitachi */
			if ((rc = process_rupay_Q_request( rupay_req, rupay_res )) != 0) {
				/* Error */
				rupay_res->status_code = rc;
				log_error(SWITCH, "ERROR : While processing RuPay Q request. RC : [%d]\n", rc);
			}

			/* Writing response to Message Queue */
			rupay_msg_queue_write( rupay_res_id, rupay_res->txn_id, *rupay_res, timeout );

			free(isosenddata);
			free(isoparsedata);
			exit(EXIT_SUCCESS);
		}
		else
		{
			/* Parent */
			if (pid < 0) {
				/* Failed to create process */
				log_error(SWITCH, "ERROR : fork() : %s\n", strerror(errno));

				memcpy(rupay_res, rupay_req, sizeof(struct host_st) );
				rupay_res->status_code = -3;

				/* Writing response to Message Queue */
				rupay_msg_queue_write( rupay_res_id, rupay_res->txn_id, rupay_res, timeout );
			}
		}
	}

	log_info(SWITCH, "Rupay Q Processor completed. Exit..\n");
	exit(EXIT_SUCCESS);
}

