#include "host_q.h"
#include "rupay.h"
#include <hiredis/hiredis.h>

/* 
 * ===  FUNCTION  ======================================================================
 *         Name : send_network_request
 *      Created : 2016-12-17 12:44:39
 *       Author : Sathishkumar D
 *  Description : This will form network request and send it to NPCI.
 * =====================================================================================
 */
int send_network_request(int npciSocket, redisContext *c, char *network_code)
{
	int len, rc;
	long stan = 0;
	time_t gmt_now;
	char datetime_utc[15];
	struct tm gmt_time;
	struct iso8583res *fieldFmt;

	memset(isosenddata, 0, sizeof(struct iso8583fmt));
	memset(isoparsedata, 0, sizeof(struct iso8583fmt));

	/* Get stan */
	stan = get_stan(c);
	log_info(NPCI, "NPCI Network stan : %ld\n", stan);

	if (stan == 0) {
		log_error(SWITCH, "Not able to get NPCI stan from redis\n");
		return -2;
	}

	/* Get current date/time details */
	time(&gmt_now);
	gmtime_r(&gmt_now, &gmt_time);
	conv_date_fmt(&gmt_time, "MMDDhhmmss", datetime_utc);

	strcpy(isosenddata->msg_type, NETWORK_REQ);
	strcpy(isosenddata->field_value[7], datetime_utc);
	sprintf(isosenddata->field_value[11], "%06ld", stan);
	strcpy(isosenddata->field_value[70], network_code);  // Network Management Information Code

	fieldFmt = get_present_fields(isosenddata);
	if ((len = Build_IsoResp( fieldFmt->msg_type, fieldFmt->present_fields, 0, RUPAY_FMT)) <= 0) {
		log_error(NPCI, "Not able to form NPCI Network request. Network Code : %s.\n", network_code);
		return -2;
	}

	log_debug(NPCI, "NPCI Network Request [%s] : \n", network_code);
	log_hexa(NPCI, isosenddata->buffer, isosenddata->msg_length);

	if ((rc = Comms_Layer( isosenddata->buffer, npciSocket, isosenddata->msg_length, SEND)) <= 0) {
		log_error(NPCI, "Not able to send NPCI Network request. RC : [%d]\n", rc);
		shutdown(npciSocket, 2);
		close(npciSocket);
		return -3;
	}

	log_info(NPCI, "NPCI Network request sent successfully\n");
	return 0;
}

/* 
 * ===  FUNCTION  ======================================================================
 *         Name : is_network_message
 *      Created : 2016-12-19 12:55:31
 *       Author : Sathishkumar D
 *  Description : This will identify whether network (OR) financial request.
 * =====================================================================================
 */
int is_network_message(unsigned char *data)
{
	int mtype;
	char message_type[5];

	memset(message_type, 0, sizeof(message_type));
	memcpy(message_type, data, 4);   // First '4' bytes will be MTI
	mtype = atoi(message_type);

	/* If it is network request from NPCI... */
	if (mtype == atoi(NETWORK_REQ)) {
		return 1;   // Network Request
	}
	else if (mtype == atoi(NETWORK_RES)) {
		return 2;   // Network Response
	}

	return 0;   // FALSE
}

/* 
 * ===  FUNCTION  ======================================================================
 *         Name : process_npci_cutover
 *      Created : 2017-08-15 19:33:11
 *       Author : Sathishkumar D 
 *  Description : Processing NPCI cutover received at EOD.
 * =====================================================================================
 */
void process_npci_cutover()
{
	int nrows;
	PGresult *res, *res2;
	char query[KB1];

	conn = open_database();
	if (conn == NULL) {
		log_error(NPCI, "Not able to open DB connection. Pls check.\n");
		return;
	}

	sprintf(query, "SELECT cutoff_time::timestamp(0) from rupay_cutoff_times where business_date = now()::date");
	res = query_database(query, DONT_USE, DONT_USE, 0, NULL);

	if (PQresultStatus(res) != PGRES_TUPLES_OK) {
		log_error(NPCI, "NPCI cutover checking query failed.\n");
		PQclear(res);
		close_database();
		return;
	}

	if ((nrows = PQntuples(res)) <= 0) {
		sprintf(query, "INSERT INTO rupay_cutoff_times (business_date, cutoff_time) VALUES ( now()::date , now() )");
		res2 = query_database(query, DONT_USE, DONT_USE, 0, NULL);

		if (PQresultStatus(res2) != PGRES_COMMAND_OK) {
			log_error(NPCI, "NPCI cutover inserting query failed.\n");
			PQclear(res);
			PQclear(res2);
			close_database();
			return;
		}

		log_info(NPCI, "Cutover processed successfully.\n");
		PQclear(res);
		PQclear(res2);
	}
	else {
		log_error(NPCI, "Already we have received cutover from NPCI. Skipping this cutover.\n");
		PQclear(res);
	}

	/* Close DB connection */
	close_database();
}

/* 
 * ===  FUNCTION  ======================================================================
 *         Name : handle_npci_request
 *      Created : 2016-12-19 12:55:31
 *       Author : Sathishkumar D
 *  Description : This will handle network request from NPCI.
 * =====================================================================================
 */
int handle_npci_request(struct iso8583fmt *isodata)
{
	int rc, len, qwrite_timeout = 15;
	struct NPCI_st iso_res;
	struct iso8583res *fieldFmt;

	memset(&iso_res, 0, sizeof(struct NPCI_st));
	memset(isosenddata, 0, sizeof(struct iso8583fmt));

	if (strcmp(isodata->field_value[70], LOGON) == 0) {
		log_info(NPCI, "LogOn received.\n");
	}
	else if (strcmp(isodata->field_value[70], LOGOFF) == 0) {
		log_info(NPCI, "LogOff received.\n");
	}
	else if (strcmp(isodata->field_value[70], CUTOVER) == 0) {
		log_info(NPCI, "Cutover received.\n");
		process_npci_cutover();
	}
	else if (strcmp(isodata->field_value[70], ECHOTEST) == 0) {
		log_info(NPCI, "EchoMsg received.\n");
	}
	else {
		log_error(NPCI, "Unknown request received from NPCI. Pls check.\n");
		return -1;
	}

	/* Copying all ISO parse data elements to ISO send data elements. Form ISO. */
	strcpy(isosenddata->msg_type, NETWORK_RES);
	strcpy(isosenddata->field_value[7], isodata->field_value[7]);
	strcpy(isosenddata->field_value[11], isodata->field_value[11]);
	strcpy(isosenddata->field_value[15], isodata->field_value[15]);
	strcpy(isosenddata->field_value[39], ISO8583_SUCCESS);
	strcpy(isosenddata->field_value[70], isodata->field_value[70]);

	fieldFmt = get_present_fields(isosenddata);
	if ((len = Build_IsoResp( fieldFmt->msg_type, fieldFmt->present_fields, 0, RUPAY_FMT)) <= 0) {
		log_error(NPCI, "Not able to form NPCI Network response.\n");
		return -2;
	}

	log_debug(NPCI, "NPCI Network Response : \n");
	log_hexa(NPCI, isosenddata->buffer, isosenddata->msg_length);

	/* Write data to NPCI Writer Queue */
	iso_res.q_time = time(&iso_res.q_time);
	iso_res.txn_id = NPCI_Q_REQ_MTYPE;    // Internal purpose only. Network response for NPCI
	iso_res.len = isosenddata->msg_length;
	memcpy(iso_res.data, isosenddata->buffer, isosenddata->msg_length);

	rc = npci_msg_queue_write(npci_req_id, iso_res.txn_id, iso_res, qwrite_timeout);
	log_info(NPCI, "NPCI Network Response sent successfully. RC : [%d]\n", rc);

	return 0;
}

/* 
 * ===  FUNCTION  ======================================================================
 *         Name : handle_npci_response
 *      Created : 2016-12-19 12:54:28
 *       Author : Sathishkumar D 
 *  Description : This will handle network response got from NPCI.
 * =====================================================================================
 */
int handle_npci_response(struct iso8583fmt *isodata)
{
	if (strcmp(isodata->field_value[70], LOGON) == 0) {
		log_info(NPCI, "LogOn Response received.\n");
	}
	else if (strcmp(isodata->field_value[70], LOGOFF) == 0) {
		log_info(NPCI, "LogOff Response received.\n");
	}
	else if (strcmp(isodata->field_value[70], ECHOTEST) == 0) {
		log_info(NPCI, "EchoMsg Response received.\n");
	}
	else {
		log_error(NPCI, "Unknown network response received from NPCI. Pls check.\n");
		return -1;
	}

	return 0;
}

