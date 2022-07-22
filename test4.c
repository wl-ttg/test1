#include "host_q.h"
#include "rupay.h"

/* 
 * ===  FUNCTION  ======================================================================
 *         Name : unset_field
 *      Created : 2016-12-20 15:46:18
 *       Author : Sathishkumar D 
 *  Description : This will unset give field.
 * =====================================================================================
 */
static void unset_field(int field_no)
{
	strcpy(isosenddata->field_value[field_no], "");
}

/* 
 * ===  FUNCTION  ======================================================================
 *         Name : set_pos_entry_mode
 *      Created : 2016-12-17 18:35:02
 *       Author : Sathishkumar D 
 *  Description : This sets POS Entry Mode.
 * =====================================================================================
 */
static void set_pos_entry_mode(char *pos_entry_mode, struct host_st *req)
{
	char pan_entry_mode[3], pin_entry_mode[2];

	if (strlen(req->emv)) {
		/* ICC Chip */
		strcpy(pan_entry_mode, "05");
	}
	else {
		if ( (strncmp(req->pos_entry_mode, "090", 3) == 0 || strncmp(req->pos_entry_mode, "080", 3) == 0) && ((req->c_serv_code[0] == '2') || (req->c_serv_code[0] == '6')) ) {
			/* Fall back */
			strcpy(pan_entry_mode, "80");
		}
		else if (strncmp(req->pos_entry_mode, "001", 3) == 0 || strncmp(req->pos_entry_mode, "010", 3) == 0) {
			/* Manual Key Entry */
			strcpy(pan_entry_mode, "01");
		}
		else {
			/* Magnetic Stripe */
			strcpy(pan_entry_mode, "02");
		}
	}

	if (strlen(req->pin_data)) {
		strcpy(pin_entry_mode, "1");
	}
	else {
		strcpy(pin_entry_mode, "1");
	}

	sprintf(pos_entry_mode, "%s%s", pan_entry_mode, pin_entry_mode);
}

/* 
 * ===  FUNCTION  ======================================================================
 *         Name : set_card_seq_no
 *      Created : 2017-02-02 13:15:40
 *       Author : Sathishkumar D 
 *  Description : This sets card sequence no from EMV tag - 5F34
 * =====================================================================================
 */
static void set_card_seq_no(char *card_seq_no, struct host_st *req)
{
	char *ptr, len[3], data[4];
	memset(len, 0, sizeof(len));
	memset(data, 0, sizeof(data));

	if (strlen(req->emv)) {

		ptr = strstr(req->emv, "5F34");	// EMV Tag - 5F34 - Card Sequence Number
		if(ptr) {
			ptr += 4;
			strncpy(len, ptr, 2);		// Tag Length
			strncpy(data, ptr+2, atoi(len)*2);
			strcpy(card_seq_no, data);
		}else{
			strcpy(card_seq_no, "000");
		}
	}
}

/* 
 * ===  FUNCTION  ======================================================================
 *         Name : set_f48_addnl_data
 *      Created : 2016-12-17 18:48:15
 *       Author : Sathishkumar D
 *  Description : This sets DE-48 Additional Data.
 * =====================================================================================
 */
static void set_f48_addnl_data(char *addnl_data, struct host_st *req)
{
	int rc = 0;

	rc = sprintf(addnl_data, "051%03d%s", strlen(R_PRODUCT_CODE), R_PRODUCT_CODE);	// Mandatory for all messages
	rc += sprintf(addnl_data+rc, "078002%02d", R_ENC_TLE | R_ENC_UKPT);		// Mandatory for all messages

	/* Populate this tag only for Debit card txns and for small merchants only */
	if ((req->is_small_merchant == 1) && (strcasecmp(req->card_assoc, "RUPAY") == 0) && (strcasecmp(req->card_type, "DEBIT") == 0)) {
		rc += sprintf(addnl_data+rc, "083001%.1s", R_SMALL_MER );
	}
}

/* 
 * ===  FUNCTION  ======================================================================
 *         Name : set_pos_data_code
 *      Created : 2016-12-17 19:09:49
 *       Author : Sathishkumar D
 *  Description : This sets POS Data Code.
 * =====================================================================================
 */
static void set_pos_data_code(char *pos_data_code, struct host_st *req)
{
	char sf1[2], sf2[2], sf4[2], sf7[2], sf8[2], sf9[2];
	long sf13;

	/* SF7 - Card Data Input Mode , SF1 - Card Data Input Capability */
	if (strlen(req->emv)) {
		strcpy(sf1, "2");	/* ICC */
		strcpy(sf7, "3");
	}
	else {
		if ( (strncmp(req->pos_entry_mode, "090", 3) == 0 || strncmp(req->pos_entry_mode, "080", 3) == 0) && ((req->c_serv_code[0] == '2') || (req->c_serv_code[0] == '6')) ) {
			strcpy(sf1, "4");	/* Fall back */
			strcpy(sf7, "2");
		}
		else if (strncmp(req->pos_entry_mode, "001", 3) == 0 || strncmp(req->pos_entry_mode, "010", 3) == 0) {
			strcpy(sf1, "6");	/* Manual Key Entry */
			strcpy(sf7, "7");
		}
		else {
			strcpy(sf1, "1");	/* Magnetic Stripe */
			strcpy(sf7, "2");
		}
	}

	/* Cardholder Authentication Capability */
	if (strlen(req->pin_data)) {
		strcpy(sf2, "2");
	}
	else {
		strcpy(sf2, "1");
	}

	/* Terminal Operating Environment */
	if (strstr(req->pos_type, "mpos")) {
		strcpy(sf4, "7");
	}
	else {
		strcpy(sf4, "1");
	}

	/* Cardholder Authentication Method */
	if (strlen(req->pin_data)) {
		strcpy(sf8, "2");
	}
	else {
		strcpy(sf8, "3");
	}

	/* Cardholder Authentication Entity */
	if (strlen(req->emv)) {
		strcpy(sf9, "1");
	}
	else {
		strcpy(sf9, "0");
	}

	/* Zip Code */
	sf13 = atol(req->mer_pincode);

	sprintf(pos_data_code, "%s%s1%s12%s%s%s039%09ld%-20s", sf1, sf2, sf4, sf7, sf8, sf9, sf13, req->mer_name);
}

/* 
 * ===  FUNCTION  ======================================================================
 *         Name : set_pos_cond_code
 *      Created : 2018-03-26 16:36:45
 *       Author : Sathishkumar D 
 *  Description : This functions sets pos condition code.
 * =====================================================================================
 */
static void set_pos_cond_code(char *pos_cond_code, struct host_st *req)
{
	if (strcmp(req->pos_cond_code, "06") == 0) {
		/* Preauth request received. As of now, we are setting as "Normal". */
		strcpy(pos_cond_code, "00");	// Default : Normal
	}
	else {
		strcpy(pos_cond_code, "00");	// Default : Normal
	}
}

/* 
 * ===  FUNCTION  ======================================================================
 *         Name : conv_to_UTC
 *      Created : 2016-12-22 16:55:33
 *       Author : Sathishkumar D
 *  Description : Converting DE-12 & DE-13 (Local Date/Time) to UTC Format.
 * =====================================================================================
 */
static void conv_to_UTC(char *datetime_utc, char *l_date, char *l_time)
{
	time_t now, conv_t;
	char date_time[15];
	struct tm conv_time, time_st, gmt_time;

	time(&now);
	gmtime_r(&now, &time_st);

	sprintf(date_time, "%04d%s%s", time_st.tm_year+1900, l_date, l_time); // YYYYMMDDhhmmss
	conv_struct_tm_fmt(&conv_time, "YYYYMMDDhhmmss", date_time);
	conv_t = mktime(&conv_time);

	gmtime_r(&conv_t, &gmt_time);
	conv_date_fmt(&gmt_time, "MMDDhhmmss", datetime_utc);
}

/* 
 * ===  FUNCTION  ======================================================================
 *         Name : set_current_UTC
 *      Created : 2016-12-22 18:57:21
 *       Author : Sathishkumar D 
 *  Description : This sets current UTC in given buffer.
 * =====================================================================================
 */
static void set_current_UTC(char *datetime_utc)
{
	time_t gmt_now;
	struct tm gmt_time;

	time(&gmt_now);
	gmtime_r(&gmt_now, &gmt_time);
	conv_date_fmt(&gmt_time, "MMDDhhmmss", datetime_utc);
}

/* 
 * ===  FUNCTION  ======================================================================
 *         Name : set_f90_orig_data
 *      Created : 2016-12-20 13:01:35
 *       Author : Sathishkumar D 
 *  Description : This sets original data elements for VOID/REVERSAL.
 * =====================================================================================
 */
static void set_f90_orig_data(char *orig_data, struct host_st *req)
{
	time_t now, conv_t;
	struct tm conv_time, time_st1, gmt_time;
	char orig_txn_date[5], orig_txn_time[7], date_time[15], datetime_utc[15];

	memset(orig_txn_date, 0, sizeof(orig_txn_date));
	memset(orig_txn_time, 0, sizeof(orig_txn_time));

	/* Copying orig txn date/time */
	if (strlen(req->process_time) == 10) {
		strncpy(orig_txn_date, req->process_time, 4);
		strncpy(orig_txn_time, req->process_time+4, 6);

		// For getting current year (UTC)
		time(&now);
		gmtime_r(&now, &time_st1);

		sprintf(date_time, "%04d%s%s", time_st1.tm_year+1900, orig_txn_date, orig_txn_time); // YYYYMMDDhhmmss
		conv_struct_tm_fmt(&conv_time, "YYYYMMDDhhmmss", date_time);
		conv_t = mktime(&conv_time);

		gmtime_r(&conv_t, &gmt_time);
		conv_date_fmt(&gmt_time, "MMDDhhmmss", datetime_utc);

		/* Replacing Current Txn Date/Time with Original Txn Date/Time */
		set_current_UTC(isosenddata->field_value[7]);
		strcpy(isosenddata->field_value[12], orig_txn_time);  /* hhmmss */
		strcpy(isosenddata->field_value[13], orig_txn_date);  /* MMDD */
	}
	else {
		// For getting current year (UTC)
		time(&now);
		gmtime_r(&now, &time_st1);

		sprintf(date_time, "%04d%s%s", time_st1.tm_year+1900, req->req_date, req->req_time); // YYYYMMDDhhmmss
		conv_struct_tm_fmt(&conv_time, "YYYYMMDDhhmmss", date_time);
		conv_t = mktime(&conv_time);

		gmtime_r(&conv_t, &gmt_time);
		conv_date_fmt(&gmt_time, "MMDDhhmmss", datetime_utc);
		set_current_UTC(isosenddata->field_value[7]);
	}

	/* Forming original data-elements - DE #90 */
	if (strcmp(req->card_scheme, "D") == 0)
		sprintf(orig_data, "%04d%06d%s%011d%011d", atoi(AUTH_REQ), atoi(req->stan_no), datetime_utc, req->acquirer_id, 0);
	else
		sprintf(orig_data, "%04d%06d%s%011d%011d", atoi(FINANCIAL_REQ), atoi(req->stan_no), datetime_utc, req->acquirer_id, 0);
}

/*
 * ===  FUNCTION  ======================================================================
 *         Name : form_rupay_iso_request
 *      Created : 2016-12-08 11:34:22
 *       Author : Sathishkumar D
 *  Description : This will form RuPay iso request.
 * =====================================================================================
 */
int form_rupay_iso_request(struct host_st *req)
{
	time_t now, gmt_now, conv_t;
	struct iso8583res *fieldFmt;
	struct tm time_now, gmt_time;
	char datetime_utc[15];

	memset(isoparsedata, 0, sizeof(struct iso8583fmt));
	memset(isosenddata, 0, sizeof(struct iso8583fmt));

	/* Get current date/time details */
	time(&now);
	time(&gmt_now);

	//localtime_r(&now, &time_now);
	//conv_date_fmt(&time_now, "mmdd", req->req_date);
	//conv_date_fmt(&time_now, "hhmmss", req->req_time);

	//gmtime_r(&gmt_now, &gmt_time);
	//conv_date_fmt(&gmt_time, "MMDDhhmmss", datetime_utc);

	strcpy(isosenddata->field_value[3], req->pcode);
	conv_to_UTC(datetime_utc, req->req_date, req->req_time);
	strcpy(isosenddata->field_value[7], datetime_utc);
	strcpy(isosenddata->field_value[11], req->stan_no);
	strcpy(isosenddata->field_value[12], req->req_time);
	strcpy(isosenddata->field_value[13], req->req_date);
	strcpy(isosenddata->field_value[18], req->mcc);
	strcpy(isosenddata->field_value[19], ACQ_COUNTRY_CODE);
	set_pos_entry_mode(isosenddata->field_value[22], req);
	set_card_seq_no(isosenddata->field_value[23], req);
	set_pos_cond_code(isosenddata->field_value[25], req);
	sprintf(isosenddata->field_value[32], "%06d", req->acquirer_id);
	strcpy(isosenddata->field_value[37], req->rrn);
	strcpy(isosenddata->field_value[41], req->tid);
	strcpy(isosenddata->field_value[42], req->mid);
	sprintf(isosenddata->field_value[43], "%-23s%-13s%-2s%-2s", req->mer_name, req->mer_city, req->mer_state, req->mer_country); /* (1-23 address 24-36 city 37-38 state 39-40 country) */
	set_f48_addnl_data(isosenddata->field_value[48], req);
	strcpy(isosenddata->field_value[49], ACQ_COUNTRY_CODE);
	set_pos_data_code(isosenddata->field_value[61], req);

	switch (atoi(req->mtype)) {
		case 100:
		case 200:
			/* Generic values */
			if (strcmp(req->card_scheme, "D") == 0)
				strcpy(isosenddata->msg_type, AUTH_REQ);     // DMS CARDS - MTI 0100
			else
				strcpy(isosenddata->msg_type, FINANCIAL_REQ);  // SMS CARDS - MTI 0200

			strcpy(isosenddata->field_value[2], req->pan);
			strcpy(isosenddata->field_value[4], req->amount);
			strcpy(isosenddata->field_value[35], req->track2);
			strcpy(isosenddata->field_value[40], req->c_serv_code);
			strcpy(isosenddata->field_value[55], req->emv);

			/* Specific Values */
			if (atoi(req->pcode)/10000 == 0) {
				/* SALE */
				sprintf(isosenddata->field_value[3], "%2s%2s%2s", R_PURCHASE, req->from_acc_type, req->to_acc_type);
				strcpy(isosenddata->field_value[52], req->pin_data);
				/* If track2 is present, DE-14 (expiry date) should not be present */
				if (strlen(req->track2) == 0)
					strcpy(isosenddata->field_value[14], req->exp_date);
			}
			else if (atoi(req->pcode)/10000 == 2) {
				/* VOID - REVERSAL */
				strcpy(isosenddata->msg_type, REVERSAL_REQ);
				strcpy(isosenddata->field_value[4], req->orig_amount);
				strcpy(isosenddata->field_value[38], req->appr_code);
				strcpy(isosenddata->field_value[39], R_VOID_RES_CODE);
				unset_field(40);
				unset_field(48);
				unset_field(61);
				set_f90_orig_data(isosenddata->field_value[90], req);

				if (strcmp(req->orig_txn_type, "SCB") == 0) {
					sprintf(isosenddata->field_value[3], "%2s%2s%2s", R_CASH_BACK, req->from_acc_type, req->to_acc_type);
					sprintf(isosenddata->field_value[54], "%2s%2s%3sD%12s", CASH_BACK_TYPE, CASH_BACK_TYPE, ACQ_CURRENCY_CODE, req->orig_scb_amt);
				}
				else if (strcmp(req->orig_txn_type, "CASHONLY") == 0) {
					sprintf(isosenddata->field_value[3], "%2s%2s%2s", R_CASH_ONLY, req->from_acc_type, req->to_acc_type);
				}
				else {
					sprintf(isosenddata->field_value[3], "%2s%2s%2s", R_PURCHASE, req->from_acc_type, req->to_acc_type);
				}
			}
			else if (atoi(req->pcode)/10000 == 20) {
				/* REFUND */
			}
			else if (atoi(req->pcode)/10000 == 9) {
				/* CASH ONLY  - SALE & CASH BACK */
				strcpy(isosenddata->field_value[52], req->pin_data);
				if (atoi(req->cash_back_amt) == 0 && atoi(req->cash_only_amt) > 0) {
					sprintf(isosenddata->field_value[3], "%2s%2s%2s", R_CASH_ONLY, req->from_acc_type, req->to_acc_type);
				}
				else {
					/* SALE & CASH BACK */
					sprintf(isosenddata->field_value[3], "%2s%2s%2s", R_CASH_BACK, req->from_acc_type, req->to_acc_type);
					sprintf(isosenddata->field_value[54], "%2s%2s%3sD%12s", CASH_BACK_TYPE, CASH_BACK_TYPE, ACQ_CURRENCY_CODE, req->cash_back_amt);
				}
			}
			else if (atoi(req->pcode) / 10000 == 1){
				log_info(SWITCH, "forming Micro ATM CWD transactions\n");
				strcpy(isosenddata->msg_type, FINANCIAL_REQ);
				strcpy(isosenddata->field_value[18], R_MICRO_MCC);
				strcpy(isosenddata->field_value[32], R_MICRO_ACQ);
				strcpy(isosenddata->field_value[52], req->pin_data);
				strcpy(isosenddata->field_value[60], R_MICRO_INDICATION);
				unset_field(40);
				unset_field(48);
				unset_field(61);
				sprintf(isosenddata->field_value[61], "%09ld", atol(req->mer_pincode));
				/* If track2 is present, DE-14 (expiry date) should not be present */
				if (strlen(req->track2) == 0)
					strcpy(isosenddata->field_value[14], req->exp_date);
			}
			else if (atoi(req->pcode) / 10000 == 31){
				log_info(SWITCH, "forming Micro ATM - BAL INQUIRY transactions\n");
				strcpy(isosenddata->msg_type, FINANCIAL_REQ);
				//strcpy(isosenddata->field_value[4], "000000000000");
				strcpy(isosenddata->field_value[18], R_MICRO_MCC);
				strcpy(isosenddata->field_value[32], R_MICRO_ACQ);
				strcpy(isosenddata->field_value[52], req->pin_data);
				strcpy(isosenddata->field_value[60], R_MICRO_INDICATION);
				unset_field(40);
				unset_field(48);
				unset_field(61);
				sprintf(isosenddata->field_value[61], "%09ld", atol(req->mer_pincode));
				/* If track2 is present, DE-14 (expiry date) should not be present */
				if (strlen(req->track2) == 0)
					strcpy(isosenddata->field_value[14], req->exp_date);
			}
			break;
		case 400:
			/* Generic values */
			strcpy(isosenddata->msg_type, REVERSAL_REQ);
			strcpy(isosenddata->field_value[2], req->pan);

			if (atoi(req->pcode)/10000 == 2) {
				/* VOID - REVERSAL */
				strcpy(isosenddata->field_value[4], req->orig_amount);
			}
			else {
				/* REVERSAL */
				strcpy(isosenddata->field_value[4], req->amount);
			}

			gmtime_r(&gmt_now, &gmt_time);
			conv_date_fmt(&gmt_time, "MMDDhhmmss", datetime_utc);
			strcpy(isosenddata->field_value[7], datetime_utc);

			strcpy(isosenddata->field_value[38], req->appr_code);
			strcpy(isosenddata->field_value[55], req->emv);
			unset_field(48);
			unset_field(61);
			set_f90_orig_data(isosenddata->field_value[90], req);

			if (strlen(req->orig_txn_type)) {
				// ORIGINAL TXN FOUND
				if (strcmp(req->orig_txn_type, "SCB") == 0) {
					sprintf(isosenddata->field_value[3], "%2s%2s%2s", R_CASH_BACK, req->from_acc_type, req->to_acc_type);
					sprintf(isosenddata->field_value[54], "%2s%2s%3sD%12s", CASH_BACK_TYPE, CASH_BACK_TYPE, ACQ_CURRENCY_CODE, req->orig_scb_amt);
				}
				else if (strcmp(req->orig_txn_type, "CASHONLY") == 0) {
					sprintf(isosenddata->field_value[3], "%2s%2s%2s", R_CASH_ONLY, req->from_acc_type, req->to_acc_type);
				}else if (strcmp(req->orig_txn_type, "MICRO-CASH-WITHDRAWL") == 0) {
					sprintf(isosenddata->field_value[3], "%2s%2s%2s", R_MICRO_CWD, req->from_acc_type, req->to_acc_type);
				}
				else {
					sprintf(isosenddata->field_value[3], "%2s%2s%2s", R_PURCHASE, req->from_acc_type, req->to_acc_type);
				}
			}
			else {
				// ORIGINAL TXN NOT FOUND
				if (atoi(req->pcode)/10000 == 9) {
					if (atoi(req->cash_back_amt) == 0 && atoi(req->cash_only_amt) > 0) {
						sprintf(isosenddata->field_value[3], "%2s%2s%2s", R_CASH_ONLY, req->from_acc_type, req->to_acc_type);
					}
					else {
						sprintf(isosenddata->field_value[3], "%2s%2s%2s", R_CASH_BACK, req->from_acc_type, req->to_acc_type);
						sprintf(isosenddata->field_value[54], "%2s%2s%3sD%12s", CASH_BACK_TYPE, CASH_BACK_TYPE, ACQ_CURRENCY_CODE, req->cash_back_amt);
					}
				}
				else {
					sprintf(isosenddata->field_value[3], "%2s%2s%2s", R_PURCHASE, req->from_acc_type, req->to_acc_type);
				}
			}

			if (strcmp(req->resp_code, "E1") == 0 || strcmp(req->resp_code, "E2") == 0 ) {
				strcpy(isosenddata->field_value[39], req->resp_code);   // POS - EMV - Online Decline
			}else if ((strlen(req->appr_code)) && (atoi(req->pcode) / 10000 == 1)){
				strcpy(isosenddata->field_value[39], R_REV_RES_CODE_3);   // Terminal Failure for MATM - RC 21
			}
			else { 
				if (strlen(req->appr_code))
					strcpy(isosenddata->field_value[39], R_REV_RES_CODE_1);   // Terminal Failure - RC 22
				else
					strcpy(isosenddata->field_value[39], R_REV_RES_CODE_2);   // Acquirer Timeout - RC 68
			}
			if (atoi(req->pcode) / 10000 == 1){
				strcpy(isosenddata->field_value[18], R_MICRO_MCC);
				strcpy(isosenddata->field_value[32], R_MICRO_ACQ);
				strcpy(isosenddata->field_value[60], R_MICRO_INDICATION);
			}
			break;
		default:
			log_error(SWITCH, "Not handled Message Type : %s\n", req->mtype);
			memset(isosenddata, 0, sizeof(struct iso8583fmt));
			break;
	}

	/* Get present fields - ISO8583 */
	if ((fieldFmt = get_present_fields(isosenddata)) == NULL) {
		return -1;
	}

	return Build_IsoResp( fieldFmt->msg_type, fieldFmt->present_fields, 0, RUPAY_FMT);
}

/* 
 * ===  FUNCTION  ======================================================================
 *         Name : parse_rupay_iso_response
 *      Created : 2016-12-08 11:45:27
 *       Author : Sathishkumar D
 *  Description : This will form hitachi iso response.
 * =====================================================================================
 */
int parse_rupay_iso_response(struct host_st *res, unsigned char *res_buffer, int iso_len)
{
	int out_len = 0;

	log_info(SWITCH, "Parsing Rupay ISO response..\n");

	/* Parsing ISO response */
	bzero(isoparsedata, sizeof(struct iso8583fmt));
	out_len = Parse_Crdb_Isofmt_2(res_buffer, iso_len, RUPAY_FMT);

	res->status_code = 0;  // Means, It processed request successfully.

	strcpy(res->mtype, isoparsedata->msg_type);
	strcpy(res->pan, isoparsedata->field_value[2]);
	strcpy(res->pcode, isoparsedata->field_value[3]);
	strcpy(res->amount, isoparsedata->field_value[4]);
	strcpy(res->stan_no, isoparsedata->field_value[11]);
	strcpy(res->res_time, isoparsedata->field_value[12]);
	strcpy(res->res_date, isoparsedata->field_value[13]);
	sprintf(res->process_time, "%s%s", res->res_date, res->res_time);

	strcpy(res->rrn, isoparsedata->field_value[37]);
	strcpy(res->appr_code, isoparsedata->field_value[38]);
	strcpy(res->resp_code, isoparsedata->field_value[39]);
	strcpy(res->message_code, isoparsedata->field_value[39]);

	strcpy(res->tid, isoparsedata->field_value[41]);
	strcpy(res->mid, isoparsedata->field_value[42]);
	strcpy(res->emv, isoparsedata->field_value[55]);
	strcpy(res->micro_bal, isoparsedata->field_value[54]);

	return 0;
}

/* 
 * ===  FUNCTION  ======================================================================
 *         Name : process_rupay_Q_request
 *      Created : 2016-12-07 20:33:25
 *       Author : Sathishkumar D 
 *  Description : This is a main function to process Q request.
 * =====================================================================================
 */
int process_rupay_Q_request(struct host_st *req, struct host_st *res)
{
	int rc, req_len, res_len;
	int req_msgq_key, res_msgq_key;
	unsigned char res_buffer[2048];
	struct NPCI_st npci_req, npci_res;

	memset(&npci_req, 0, sizeof(struct NPCI_st));
	memset(&npci_res, 0, sizeof(struct NPCI_st));
	memset(isoparsedata, 0, sizeof(struct iso8583fmt));

	log_info(SWITCH, "Processing RuPay Q request...\n");

	req_msgq_key = hostDtls_Shm->hostDtls[req->host_route_type].host_req_key;
	res_msgq_key = hostDtls_Shm->hostDtls[req->host_route_type].host_res_key;

	log_info(SWITCH, "Rupay Q Routing : Route Id -> %d , Req Key : %d , Res Key : %d\n", req->host_route_type, req_msgq_key, res_msgq_key);

	/* Validate RuPay Host Q Request */
	if ((rc = validate_host_request(req)) != 0) {
		log_error(SWITCH, "RuPay: Host request validation failed.\n");
		return -2;
	}

	/* Form RuPay Request */
	if ((req_len = form_rupay_iso_request(req)) <= 0) {
		/* Error */
		log_error(SWITCH, "Not able to form RuPay ISO request\n");
		return -2;
	}

	// debug
	//log_hexa(SWITCH, isosenddata->buffer, isosenddata->msg_length);

	if (isosenddata->msg_length <= 0 || req_len <= 0) {
		/* Not having valid request message to Send */
		log_error(SWITCH, "Invalid formed RuPay request length. Length -> ISO [%d] , RC [%d]\n", isosenddata->msg_length, req_len);
		return -2;
	}

	/* Put Mapping key value in Redis Server */
	if (set_mq_map_key(req->tid, req->rrn, req->txn_id) != 0) {
		log_error(SWITCH, "Not able push map key in Redis Server. Pls check.\n");
		return -2;
	}

	/* Communicate to NPCI Network */
	time(&npci_req.q_time);
	npci_req.txn_id = req->txn_id;
	npci_req.len = isosenddata->msg_length;
	memcpy(npci_req.data, isosenddata->buffer, isosenddata->msg_length);

	res_len = NPCI_Q_Comms( npci_req.txn_id, npci_req, &npci_res, req_msgq_key, res_msgq_key );

	if (res_len <= 0) {
		if (res_len == -1) {
			/* send() : Error */
			return -2;
		}
		else {
			/* recv() : Error */
			log_error(SWITCH, "Not able to receive response from NPCI.\n");
			return -3;
		}
	}

	memset(res_buffer, 0, sizeof(res_buffer));
	memcpy(res_buffer, npci_res.data, npci_res.len);

	/* Parsing RuPay Response */
	if ((rc = parse_rupay_iso_response(res, res_buffer, npci_res.len)) < 0) {
		/* Error */
		log_error(SWITCH, "Not able to parse response rupay response.\n");
		return -3;
	}

	return 0;
}

