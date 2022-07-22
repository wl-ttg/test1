#include <hiredis/hiredis.h>
#include "host_q.h"
#include "rupay.h"
#include "signal_handlers.h"

/* 
 * ===  FUNCTION  ======================================================================
 *         Name : NPCI_remove_queues
 *      Created : 2016-12-08 16:07:22
 *       Author : Sathishkumar D
 *  Description : Signal Handler. It will remove the current Queues.
 * =====================================================================================
 */
void NPCI_remove_queues(int signo)
{
	int msgid;
	if ((msgid = open_msg_queue(npci_req_key)) >= 0) {
		if (msgctl(msgid, IPC_RMID, NULL) == 0) {
			printf("NPCI Request Message queue was deleted.\n");
		}
	}

	if ((msgid = open_msg_queue(npci_res_key)) >= 0) {
		if (msgctl(msgid, IPC_RMID, NULL) == 0) {
			printf("NPCI Response Message queue was deleted.\n");
		}
	}
}

/* 
 * ===  FUNCTION  ======================================================================
 *         Name : NPCI_register_signals
 *      Created : 2016-12-08 16:10:44
 *       Author : Sathishkumar D 
 *  Description : This will register signals for this NPCI Processor.
 * =====================================================================================
 */
void NPCI_register_signals()
{
	struct sigaction sa, sa2, sa3;

	// Setup the sighub handler
	sa.sa_handler = SIG_IGN;
	sa2.sa_handler = NPCI_remove_queues;
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
 *         Name : Start_SubProcesses
 *      Created : 2017-04-11 12:15:43
 *       Author : Sathishkumar D
 *  Description : This will start not-running NPCI sub-processes.
 * =====================================================================================
 */
void Start_SubProcesses(ProcMonitor *proc_stat, int nprocess, int npciSocket, char *process_name)
{
	/* NPCI Reader */
	if (!proc_stat[SUBP_READER].status && (proc_stat[SUBP_READER].pid = fork()) == 0) {
		strncat(process_name, ": npci reader process",strlen(process_name)+2);
		NPCI_Reader(npciSocket);
	}

	/* NPCI Writer */
	if (!proc_stat[SUBP_WRITER].status && (proc_stat[SUBP_WRITER].pid = fork()) == 0) {
		strncat(process_name, ": npci writer process",strlen(process_name)+2);
		NPCI_Writer(npciSocket);
	}

	/* NPCI Monitor */
	if (!proc_stat[SUBP_MONITOR].status && (proc_stat[SUBP_MONITOR].pid = fork()) == 0) {
		strncat(process_name, ": npci monitor process",strlen(process_name)+2);
		NPCI_Monitor();
	}
}

/* 
 * ===  FUNCTION  ======================================================================
 *         Name : NPCI_Monitor
 *      Created : 2017-03-06 18:46:03
 *       Author : Sathishkumar D 
 *  Description : This will always monitor request/response received from NPCI.
 * =====================================================================================
 */
void NPCI_Monitor() {
	long q_map_id;
	int rc, mtype, qread_timeout, qwrite_timeout, timeout = 3;
	struct NPCI_st iso_res;
	unsigned char response[KB3], map_key[50], map_value[50];
	redisContext *c = NULL;
	qread_timeout = qwrite_timeout = 5;

	log_info(NPCI, "NPCI Monitor started. PID : [%d]\n", getpid());

	/* Making connection with Redis Server */
	if ((c = redis_connect(NULL, FALSE, timeout)) == NULL) {
		exit(EXIT_FAILURE);
	}
	log_info(NPCI, "Monitor : Successfully connected to Redis Server.\n");

	/* Memory Allocation */
	isosenddata = (struct iso8583fmt *) malloc(sizeof(struct iso8583fmt));
	isoparsedata = (struct iso8583fmt *) malloc(sizeof(struct iso8583fmt));

	while (1) {
		/* Read message from Q */
		if ((rc = npci_msg_queue_read(npci_req_id, NPCI_Q_RES_MTYPE, &iso_res, qread_timeout)) <= 0) {
			if (rc == -1) {
				log_error(NPCI, "NPCI Queue removed.\n");
				break;
			}
			/* No message in NPCI Request Q, Timeout */
			continue;
		}

		/* FIXME : For Debug */
		log_debug(NPCI, "NPCI Response : \n");
		//log_hexa(NPCI, iso_res.data, iso_res.len);
		
		/* Parsing ISO data */
		memset(isoparsedata, 0, sizeof(struct iso8583fmt));
		memset(response, 0, sizeof(response));
		memcpy(response, iso_res.data, iso_res.len);
		rc = Parse_Crdb_Isofmt_2( response, iso_res.len, RUPAY_FMT );

		/* Find out message type */
		if (strcmp(isoparsedata->msg_type, NETWORK_REQ) == 0) {
			/* Network Request */
			handle_npci_request( isoparsedata );
			continue;
		}
		else if (strcmp(isoparsedata->msg_type, NETWORK_RES) == 0) {
			/* Network Response */
			handle_npci_response( isoparsedata );
			continue;
		}
		else {
			/* Other request/response */
			sprintf(map_key, "%s:%s", isoparsedata->field_value[41], isoparsedata->field_value[37]);
		}

		/* Mapping with Redis Server */
		if ((get_qmap_key(c, map_key, map_value)) < 0) {
			if (c == NULL || c->err) {
				/* Redis connection is failed. */
				if (c) {
					log_error(SWITCH, "NPCI Monitor : Redis Connection Error : %s\n", c->errstr);
					redisFree(c);
				}
				log_error(SWITCH, "NPCI Monitor : Redis Connection error occurred.\n");
				break;
			}
			else {
				/* Key Expired / Key NotAvailable */
				log_error(NPCI, "Monitor : Mapping key value is NOT available in Redis. Discarding..[%s]\n", map_key);
				continue;
			}
		}

		/* Write data to NPCI Response Queue */
		q_map_id = atol(map_value);
		iso_res.txn_id = q_map_id;
		rc = npci_msg_queue_write(npci_res_id, iso_res.txn_id, iso_res, qwrite_timeout);
		log_info(NPCI, "Response Data written to NPCI Response Q [Id : %ld]. RC : [%d]\n", q_map_id, rc);
	}

	log_info(NPCI, "NPCI Monitor completed. Exit.\n");

	/* Disconnects and frees the context */
	if (c) {
		redisFree(c);
	}

	free(isosenddata);
	free(isoparsedata);
	exit(0);
}

/* 
 * ===  FUNCTION  ======================================================================
 *         Name : NPCI_Writer
 *      Created : 2016-12-09 12:16:30
 *       Author : Sathishkumar D
 *  Description : This will always read request from Q, then write into NPCI.
 * =====================================================================================
 */
void NPCI_Writer(int npciSocket)
{
	int rc, nwrite, qread_timeout = 5, q_holdtimeout = 15, timeout = 3;
	time_t now, last_echo_sent_on = 0, last_sent_on = 0, time_now = 0;
	struct NPCI_st iso_req;
	redisContext *c = NULL;

	log_info(NPCI, "NPCI Writer started. PID : [%d]\n", getpid());

	/* Making connection with Redis Server */
	if ((c = redis_connect(NULL, FALSE, timeout)) == NULL) {
		exit(EXIT_FAILURE);
	}
	log_info(NPCI, "NPCI Writer : Successfully connected to Redis Server.\n");

	/* Memory Allocation */
	isosenddata = (struct iso8583fmt *) malloc(sizeof(struct iso8583fmt));
	isoparsedata = (struct iso8583fmt *) malloc(sizeof(struct iso8583fmt));

	time(&now);
	time(&last_sent_on);
	time(&last_echo_sent_on);

	/* Send - LogOn */
	if ((rc = send_network_request(npciSocket, c, LOGON)) != 0) {
		log_error(NPCI, "Not able to send LogOn. Exiting.\n");
		socket_close(npciSocket);
		exit(2);
	}

	/* Made successful communication with NPCI */
	while (1)
	{
		/* Time in Epoch */ 
		time(&now);

		/* Read message from Q */
		if ((rc = npci_msg_queue_read(npci_req_id, NPCI_Q_REQ_MTYPE, &iso_req, qread_timeout)) <= 0) {
			if (rc == -1) {
				log_error(NPCI, "NPCI Queue removed.\n");
				break;
			}
			else {
				//log_debug(NPCI, "No message in NPCI Request Q, Timeout. RC : %d\n", rc);
				if ((now - last_echo_sent_on) > ECHO_INTERVAL) {
					send_network_request(npciSocket, c, ECHOTEST);
					last_echo_sent_on = now;
				}
				continue;
			}
		}

		if (iso_req.len <= 0) {
			log_error(NPCI, "NPCI ISO Request length is less than (or) zero. Pls check.\n");
			continue;
		}

		time_now = 0;
		time(&time_now);

		/* Checking message Q request time */
		if ((time_now - iso_req.q_time) > q_holdtimeout) {
			log_error(NPCI, "NPCI Message Q hold (read) timeout reached. Discarding this request.\n");
			continue;
		}

		last_sent_on = now;
		if ((rc = Comms_Layer(iso_req.data, npciSocket, iso_req.len, SEND)) <= 0) {
			log_error(NPCI, "Not able to send request to NPCI.\n");
			break;
		}

		log_info(NPCI, "Data pushed to NPCI. Written nBytes : %d\n", rc);
		//sleep(1);  /* Enable for debug-mode only */
	}

	log_info(NPCI, "NPCI Writer completed. Exit.\n");

	/* Disconnects and frees the context */
	if (c) {
		redisFree(c);
	}

	free(isosenddata);
	free(isoparsedata);
	socket_close(npciSocket);
	exit(0);
}


/* 
 * ===  FUNCTION  ======================================================================
 *         Name : NPCI_Reader
 *      Created : 2016-12-09 12:20:31
 *       Author : Sathishkumar D
 *  Description : This will always read data from NPCI socket.
 * =====================================================================================
 */
void NPCI_Reader(int npciSocket)
{
	short int read_len;
	int rc, nread, hdr_timeout = 5, timeout = 5, qwrite_timeout = 5;
	unsigned char response[KB3], header[3], header_len[3];
	struct NPCI_st iso_res;

	log_info(NPCI, "NPCI Reader started. PID : [%d]\n", getpid());

	/* Made successful communication with NPCI */
	while (1) {
		read_len = 0;
		memset(header, 0, sizeof(header));
		memset(header_len, 0, sizeof(header_len));

		/* Read "2 Bytes" TCP Header */
		if ((nread = socket_readn( npciSocket, header, HEADER_LEN, hdr_timeout )) <= 0) {
			if (nread == -2) {
				/* Timeout. No data received from NPCI. */
				continue;
			}
			else {
				log_error(NPCI, "NPCI Socket connection error [%d]. Conn Status : %s\n", nread, (nread==0) ? "Closed" : "Unknown");
				break;
			}
		}

		if (nread != HEADER_LEN) {
			log_error(NPCI, "Invalid NPCI TCP Header length.\n");
			break;
		}

		/* Finding length of original data */
		memcpy(header_len, header+1, 1);
		memcpy(header_len+1, header, 1);
		memcpy(&read_len, header_len, 2);
		log_info(NPCI, "No of bytes to read : %d\n", read_len);

		if (read_len > sizeof(response)) {
			log_error(NPCI, "Data length is higher than buffer size. Increase buffer size. Received below header : \n");
			log_hexa(NPCI, header, HEADER_LEN);
			break;
		}

		/* Read "read_len" data */
		memset(response, 0, sizeof(response));
		if ((nread = socket_readn( npciSocket, response, read_len, timeout )) < 0) {
			if (nread == -2) {
				/* Timeout. No data received from NPCI. */
				continue;
			}
			else {
				log_error(NPCI, "NPCI Socket connection error [%d]. Conn Status : %s\n", nread, (nread==0) ? "Closed" : "Unknown");
				break;
			}
		}

		if (nread != read_len) {
			log_error(NPCI, "Not read full data. nread != read_len. nread = %d , read_len = %d\n", nread, read_len);
			break;
		}

		/* Write data to NPCI Response Queue */
		iso_res.txn_id = NPCI_Q_RES_MTYPE;
		iso_res.len = nread;
		memcpy(iso_res.data, response, nread);

		rc = npci_msg_queue_write( npci_req_id, iso_res.txn_id, iso_res, qwrite_timeout );
		log_info(NPCI, "Reader : Data written to NPCI Q. RC : [%d]\n", rc);
	}

	log_info(NPCI, "NPCI Reader completed.\n");
	socket_close(npciSocket);
	exit(0);
}

/* 
 * ===  FUNCTION  ======================================================================
 *         Name : NPCI_Processor
 *      Created : 2016-12-08 17:08:55
 *       Author : Sathishkumar D 
 *  Description : NPCI PROCESSOR
 * =====================================================================================
 */
int NPCI_Processor(char *process_name, int host_route_id)
{
	/* Initialization */
	int rc, npciSocket, mpid, wpid, rpid, timeout = 30;
	ProcMonitor proc_stat[3];
	memset(proc_stat, 0, sizeof(proc_stat));

	/* Connecting to Logger */
	connect_logger();

	/* Register signals */
	NPCI_register_signals();

	/* Open database connection */
	/*DB connection is not required, Since all master data are stored in shared memory*/
	//conn = open_database();

	/*Implemented in shared memory*/
	//getIsoStruct(isofmt); 	/*config.iso_format*/
	//get_iso8583_response(); 	/*config.iso_fields*/
	//load_host_details();		/*config.host_route_config*/

	/* Close database connection */
	//close_database();

	/* NPCI Host Communication details */
	strcpy(npci_ip, hostDtls_Shm->hostDtls[host_route_id].host_ip);
	npci_port = hostDtls_Shm->hostDtls[host_route_id].host_port;

	log_info(NPCI, "NPCI : Host Routing : %d , IP : %s , Port : %d\n", host_route_id, npci_ip, npci_port);
	log_info(NPCI, "NPCI Processor is started.\n");

	while (1) {
		/* Monitor Sub-Processes */
		Monitor_SubProcesses(&proc_stat, sizeof(proc_stat)/sizeof(ProcMonitor));
		Status_SubProcesses(&proc_stat, sizeof(proc_stat)/sizeof(ProcMonitor));

		if (!proc_stat[SUBP_READER].status || !proc_stat[SUBP_WRITER].status || !proc_stat[SUBP_MONITOR].status) {
			socket_close(npciSocket);
			Stop_SubProcesses(&proc_stat, sizeof(proc_stat)/sizeof(ProcMonitor), SUBP_READER);
			Stop_SubProcesses(&proc_stat, sizeof(proc_stat)/sizeof(ProcMonitor), SUBP_WRITER);
			Stop_SubProcesses(&proc_stat, sizeof(proc_stat)/sizeof(ProcMonitor), SUBP_MONITOR);
			NPCI_remove_queues(0);

			/* NPCI Socket Connection */
			if ((npciSocket = socket_connect(npci_ip, npci_port, timeout)) < 0) {
				log_error(NPCI, "Connection to NPCI Failed. NPCI Ip: %s, NPCI Port: %d. Retrying...\n", npci_ip, npci_port);
				sleep(5);
				continue;
			}
			log_info(NPCI, "Socket : Successfully connected to NPCI Network.\n");

			/* Init Message Queue */
			NPCI_queues_init( host_route_id );

			/* Updates status of Sub-Processes */
			Monitor_SubProcesses(&proc_stat, sizeof(proc_stat)/sizeof(ProcMonitor));
		}

		/* Start Sub-Processes */
		if (!proc_stat[SUBP_READER].status || !proc_stat[SUBP_WRITER].status || !proc_stat[SUBP_MONITOR].status) {
			Start_SubProcesses(proc_stat, sizeof(proc_stat)/sizeof(ProcMonitor), npciSocket, process_name);
		}

		sleep(5);
	}

	exit(0);
}

