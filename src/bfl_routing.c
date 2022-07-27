/*
 * ===================================================================================
 *
 *       Filename   :  bfl_routing.c
 *
 *       Description:  This contains all the routing logic and validation based on focus group for the BFL project
 *
 *       Version    :  1.0
 *       Created    :  Monday 27 June 2022 03:34:06  IST
 *       Revision   :  none
 *       Compiler   :  gcc
 *
 *       Author     :  Seetharaman V (), seetharaman.v@worldline.com
 *       Company    :  MRL Posnet
 *
 * ===================================================================================
*/
#include "bfl_routing.h"
#include "../iso/iso_8583.h"
#include "../hsm/structures.h"
#include "../hsm/hsm.h"
#include "host_q.h"


#define TRUE 1
#define FALSE 0


/*==================== FUNCTION=========================================================
 *         Name : bfl_fetchRoutingBankCode
 *      Created : 2022-07-03 11:39:55 IST
 *       Author : Seetharaman V
 *  Description : Get Routing bank code based on Focus group or Terminal routing
 *		  configuration and Rule IC
 * =====================================================================================
*/

int bfl_fetchRoutingBankCode()
{
	int ret = 0;
	int iTIDBankCodeExists = 0;
	//bfl_Txn_Details *bfl_rule_dtls = NULL;

	log_info(SWITCH, "bfl_fetchRoutingBankCode\n");

	log_info(SWITCH,"***************Bank Code from POS : %s**************\n",mpos_det->bank_code);
	
	ret = bfl_LoadData();         			   			 //Load dummy transaction details

	ret = bfl_GetRuleIdDetails();						 //identify the rule from the rule table
	if(ret == FALSE)
		return ret;

	ret = bfl_GetMccCategory();				 		 //idetify merchant category from mcc and bank code
	if(ret == FALSE)
		return ret;

	if(is_Tid_Overide()){
		ret = bfl_FindBankCodefromTID(&iTIDBankCodeExists);		 //Fetch bank code from TID overide table
		log_info(SWITCH,"Final bank code by TID rules : %s\n",bfl_rule_dtls.bfl_FinalBankCode);
		if(ret == FALSE)
		{
			ret = bfl_FindBankCodefromFG(iTIDBankCodeExists); 	 //Fetch bank code from Focus Group
		}
	}
	else
	{
		ret = bfl_FindBankCodefromFG(iTIDBankCodeExists);		 //Fetch bank code from Focus Group
		if(ret == FALSE)
			return ret;
	}

	strcpy(mpos_det->bank_code,bfl_rule_dtls.bfl_FinalBankCode);			//Modify POS Bank Code
	log_info(SWITCH,"****************Modified Bank Code from POS : %s************\n",mpos_det->bank_code);

	if(is_modifyRequired()){

		ret = bfl_modifyTxnDetails();			       		//Modify TID and MID
		if(ret == FALSE)
			return ret;
	}
}

/*====================FUNCTION=========================================================
 *         Name : bfl_GetRuleIdDetails
 *      Created : 2022-07-03 13:14:55 IST
 *       Author : Seetharaman V
 *  Description : Get Rule ID based on bfl_GetRuleIdDetails, rule_type and rule_value 
 * =====================================================================================
*/

int bfl_GetRuleIdDetails()
{

	int row,col,i,ruleIDResult = 0;
	char query[KB2] = {0};
	PGresult *rs = NULL;

	log_info(SWITCH, "bfl_GetRuleIdDetails");	

	sprintf(query,"select rule_id,is_active from bfl.rule_reference_master where (intl_domestic = '%s') AND (rule_type = '%s') AND (rule_value = '%s'or rule_value = '%s') ORDER BY priority_id ASC",bfl_rule_dtls.intl_domestic,bfl_rule_dtls.rule_type,bfl_rule_dtls.rule_value_option1,bfl_rule_dtls.rule_value_option2);
	log_info(SWITCH, "Rule ID  Query  = %s\n",query);
	rs = query_database (query,DONT_USE,DONT_USE,0,NULL);
	if (PQresultStatus(rs) != PGRES_TUPLES_OK) {
		PQclear(rs);
		return FALSE;
	}
        
	row = PQntuples(rs);

	if(row > 0)
	{
		
		if(strcasecmp(PQgetvalue(rs, 0, 1),"t") != 0)
		{
			log_info(SWITCH, "rule ID %d is Inactive\n",atoi(PQgetvalue(rs, 0, 0)));
			PQclear(rs);
			return FALSE;
		}
        	bfl_rule_dtls.ruleIDResult  = atoi(PQgetvalue(rs, 0, 0));
		log_info(SWITCH, "ruleIDResult  = %d\n",bfl_rule_dtls.ruleIDResult);

	}
	else
	{
		log_info(SWITCH, "Rule match not found\n");
		PQclear(rs);
		return FALSE;

	}
        PQclear(rs);
	return TRUE;
}


/*====================FUNCTION=========================================================
 *         Name : bfl_GetMccCategory
 *      Created : 2022-07-03 13:14:55 IST
 *       Author : Seetharaman V
 *  Description : Get Merchant Category based on MCC Code and Bank Code 
 * =====================================================================================
*/

int bfl_GetMccCategory()
{
	
        int row,col = 0;
	char query[KB2] = {0};
	PGresult *rs = NULL;
	char buffer[10];

	log_info(SWITCH, "bfl_GetMccCategory");

	sprintf(query,"SELECT mcc_category,is_active FROM bfl.mcc_category_master  WHERE bank_code = '%s' AND mcc = '%d'",mpos_det->bank_code,mpos_det->mcc_code);
	log_info(SWITCH, "MCC Category  Query  = %s\n",query);
	rs = query_database (query,DONT_USE,DONT_USE,0,NULL);
	if (PQresultStatus(rs) != PGRES_TUPLES_OK) {
		 PQclear(rs);
		 return FALSE;
	 }

	row = PQntuples(rs);

	if(row > 0)
	{	

		if(strcasecmp(PQgetvalue(rs, 0, 1),"t") != 0)
		{
			log_info(SWITCH, "MCC Category %s is inactive\n",PQgetvalue(rs, 0, 0));
			PQclear(rs);
			return FALSE;
		}

		log_info(SWITCH, "MCC Category Found = %s\n",PQgetvalue(rs, 0, 0));
		strcpy(bfl_rule_dtls.MccCategory,PQgetvalue(rs, 0, 0));
	}else
	{
		log_info(SWITCH, "MCC Category match not found");
		PQclear(rs);
		return FALSE;
	}

	PQclear(rs);
	return TRUE;
}


/*====================FUNCTION=========================================================
 *         Name : is_Tid_Overide
 *      Created : 2022-07-03 13:14:55 IST
 *       Author : Seetharaman V
 *  Description : To Check whether any TID override rule available or not 
 * =====================================================================================
*/

int is_Tid_Overide()
{

        int row,col,iActive = 0;
	char query[KB2] = {0};
	PGresult *rs = NULL;
	char buffer[10];


	log_info(SWITCH, "is_Tid_Overide");

	sprintf(query,"SELECT rule_override,focus_group from mrlpay.terminals t WHERE tid = '%s'", mpay_request.tid );
	log_info(SWITCH, "TID Overwrite Query  = %s\n",query);
	rs = query_database (query,DONT_USE,DONT_USE,0,NULL);
	if (PQresultStatus(rs) != PGRES_TUPLES_OK) {
		 PQclear(rs);
		 return FALSE;
	 }

	row = PQntuples(rs);

	if(row > 0)
	{
		log_info(SWITCH, "TID Overide  = %s\n",PQgetvalue(rs, 0, 0));
		memset(buffer, 0, sizeof(buffer));
		strcpy(buffer,PQgetvalue(rs, 0, 0));
		strcpy(bfl_rule_dtls.tidFocusGroup,PQgetvalue(rs, 0, 1));
	}
		
 	if(strcasecmp(buffer,"t") == 0)
	{
		bfl_rule_dtls.tidRuleOveride = 1;	
		PQclear(rs);
		return TRUE;
	}
	else
	{
		bfl_rule_dtls.tidRuleOveride = 0;
		PQclear(rs);
		return FALSE;
	}
}


/*====================FUNCTION=========================================================
 *         Name : bfl_FindBankCodefromTID
 *      Created : 2022-07-03 13:14:55 IST
 *       Author : Seetharaman V
 *  Description : Fetch Bank code based on Terminal ID override Rule ID and Merchant 
 *                Category
 * =====================================================================================
*/
int bfl_FindBankCodefromTID(int * iTIDBankCodeExists)
{
        int row,col,i,iActive = 0;
	char query[KB2] = {0};
	PGresult *rs = NULL;

	*iTIDBankCodeExists = 0;

	log_info(SWITCH, "bfl_FindBankCodefromTID");

	sprintf(query,"SELECT bank_code,is_active FROM bfl.tid_routing_rule WHERE rule_id = '%d' AND terminals_id = '%s' AND mcc_category = '%s' OR rule_id = '3' AND bank_code = '%s' AND terminals_id = '%s' AND mcc_category = '%s'",bfl_rule_dtls.ruleIDResult,mpay_request.tid,bfl_rule_dtls.MccCategory,bfl_rule_dtls.IssuerBankCode,mpay_request.tid,bfl_rule_dtls.MccCategory);
	
	log_info(SWITCH, "TID Query  = %s\n",query);
	rs = query_database (query,DONT_USE,DONT_USE,0,NULL);
	if (PQresultStatus(rs) != PGRES_TUPLES_OK) {
		 PQclear(rs);
		 return FALSE;
	}

	 row = PQntuples(rs);
	 
	 log_info(SWITCH, "No. of result = %d\n",row);

	 log_info(SWITCH, "00. TID based Bankcode = %s\n",PQgetvalue(rs, 0, 0));
	 log_info(SWITCH, "00. TID based Bankcode = %s\n",PQgetvalue(rs, 1, 0));
	 
	 if(row > 0)
	 {

		for(i = 0; i < row ; i++){

			if(strcasecmp(PQgetvalue(rs, i, 1),"t") != 0)
				continue;

			iActive ++;
			
			iTIDBankCodeExists = 1;
			if(strcmp(PQgetvalue(rs, i, 0),bfl_rule_dtls.IssuerBankCode) == 0)
			{	
				log_info(SWITCH, "11. TID based Bankcode = %s\n",PQgetvalue(rs, i, 0));
				strcpy(bfl_rule_dtls.bfl_FinalBankCode,PQgetvalue(rs, i, 0));
				PQclear(rs);
				return TRUE;
			}
			else
			{
				log_info(SWITCH, "12. TID based Bankcode = %s\n",PQgetvalue(rs, i, 0));
				strcpy(bfl_rule_dtls.bfl_FinalBankCode,PQgetvalue(rs, i, 0));
			}
		}
        }else
	{
		log_info(SWITCH, "TID Rule not available");
		PQclear(rs);
		return FALSE;
	}

	if(iActive == 0)
	{
		 log_info(SWITCH, "No Active Match TID rule");
		 PQclear(rs);		//no active TID Overwrite rule
		 return FALSE;
	}
	 
	PQclear(rs);
        return FALSE;

}

/*====================FUNCTION=========================================================
 *         Name : bfl_FindBankCodefromFG
 *      Created : 2022-07-03 13:14:55 IST
 *       Author : Seetharaman V
 *  Description : Fetch Bank code based on Rule ID, MCC Category and Focus Group name
 * =====================================================================================
*/

int bfl_FindBankCodefromFG(int iTIDBankCodeExists)
{
        int row,col,i,iActive = 0;
	char query[KB2] = {0};
	PGresult *rs = NULL;
	char buffer[10];

	log_info(SWITCH, "bfl_FindBankCodefromFG");

	sprintf(query,"SELECT bank_code,is_active FROM bfl.focus_group_info WHERE rule_id = '%d' AND mcc_category = '%s' AND focus_group = '%s' OR rule_id = '3' AND bank_code = '%s' AND mcc_category = '%s' AND focus_group = '%s'",bfl_rule_dtls.ruleIDResult,bfl_rule_dtls.MccCategory,bfl_rule_dtls.tidFocusGroup,bfl_rule_dtls.IssuerBankCode,bfl_rule_dtls.MccCategory,bfl_rule_dtls.tidFocusGroup);

	log_info(SWITCH, "TID Query  = %s\n",query);
	rs = query_database (query,DONT_USE,DONT_USE,0,NULL);
	if (PQresultStatus(rs) != PGRES_TUPLES_OK) {
		 PQclear(rs);
		 return FALSE;
	 }

	 row = PQntuples(rs);
	 
	 log_info(SWITCH, "No. of result = %d\n",row);	

	 if(row > 0)
	 {
		for(i = 0; i < row ; i++){

			if(strcasecmp(PQgetvalue(rs, i, 1),"t") != 0)
				continue;

			iActive ++;

			if(strcmp(PQgetvalue(rs, i, 0),bfl_rule_dtls.IssuerBankCode) == 0)
			{
				log_info(SWITCH, "11. FG based Bankcode = %s\n",PQgetvalue(rs, i, 0));
				strcpy(bfl_rule_dtls.bfl_FinalBankCode,PQgetvalue(rs, i, 0));
				PQclear(rs);
				return TRUE;
			}
			else
			{
				log_info(SWITCH, "12. FG based Bankcode = %s\n",PQgetvalue(rs, i, 0));
				memset(buffer,0,sizeof(buffer));
				strcpy(buffer,PQgetvalue(rs, i, 0));
			}
		}
	 }
	 else{
		 log_info(SWITCH, "FG Rule not found");
		 PQclear(rs);
		 return FALSE;
	 }

	 if(iActive == 0)
	 {
		 log_info(SWITCH, "No Active Match FG  rule");
		 PQclear(rs);		//no active focus group
		 return FALSE;
	 }

	 if(iTIDBankCodeExists != 1)
	 {
		 log_info(SWITCH, "FG Bank code  = %s\n",buffer);
		 strcpy(bfl_rule_dtls.bfl_FinalBankCode,buffer);
	 }

	 PQclear(rs);
	 return TRUE;

}


/*====================FUNCTION=========================================================
 *         Name : is_modifyRequired
 *      Created : 2022-07-04 10:14:55 IST
 *       Author : Seetharaman V
 *  Description : Check whether to overwrite TID AND MID for the bank host
 * =====================================================================================
*/
int is_modifyRequired()
{

	if((strcmp(bfl_rule_dtls.bfl_FinalBankCode,"00032") == 0 )||(strcmp(bfl_rule_dtls.bfl_FinalBankCode,"00032") == 0))
	{
		log_info(SWITCH, "TID and MID Modification Required\n");
		return TRUE;
	}else
	{
		log_info(SWITCH, "TID and MID Modification not Required\n");
		return FALSE;
	}

}

/*====================FUNCTION=========================================================
 *         Name : bfl_modifyTxnDetails
 *      Created : 2022-07-04 10:14:55 IST
 *       Author : Seetharaman V
 *  Description : Overwrite TID and MID based on Bank Code
 * =====================================================================================
*/
int bfl_modifyTxnDetails()
{

        int row,col = 0;
	char query[KB2] = {0};
	PGresult *rs = NULL;
	char buffer[10];

	log_info(SWITCH, "bfl_modifyTxnDetails\n");

	sprintf(query,"SELECT terminals_id, mid  FROM mrlpay.terminals_extension te WHERE tid = '%s'",mpay_request.tid );
	log_info(SWITCH, "bfl_modifyTxnDetails  Query  = %s\n",query);
	rs = query_database (query,DONT_USE,DONT_USE,0,NULL);
	if (PQresultStatus(rs) != PGRES_TUPLES_OK) {
		 PQclear(rs);
		 return FALSE;
	 }

	row = PQntuples(rs);
	if(row > 0)
	{
		log_info(SWITCH, "Actual TID : %s\n",mpay_request.processor_tid);
		log_info(SWITCH, "Actual MID : %s\n",mpay_request.processor_mid);
		//strcpy(mpay_request.tid,PQgetvalue(rs, 0, 0)); 
		//strcpy(mpay_request.mid,PQgetvalue(rs, 0, 1));
		strcpy(mpay_request.processor_tid,PQgetvalue(rs, 0, 0));
		strcpy(mpay_request.processor_mid,PQgetvalue(rs, 0, 1));
		log_info(SWITCH, "Modified TID : %s\n",mpay_request.processor_tid);
		log_info(SWITCH, "Modified MID : %s\n",mpay_request.processor_mid);
		PQclear(rs);
		return TRUE;
	}else
	{
		log_info(SWITCH, "Extended TID and MIC record not found\n");
		PQclear(rs);
		return FALSE;
	}	
}

int bfl_LoadData()
{
	strcpy(bfl_rule_dtls.intl_domestic,"INT");
	strcpy(bfl_rule_dtls.rule_type,"CARD");
	strcpy(bfl_rule_dtls.rule_value_option1,"CRDB");
	//strcpy(bfl_rule_dtls.rule_value_option2,"ONUS");
	strcpy(bfl_rule_dtls.IssuerBankCode,"00031");
	//strcpy(bfl_rule_dtls.MccCategory,"INSURANCE");	

}


/*int bfl_GetRuleIdDetails()
{

	int row,col,i,ruleIDResult,ipriority = 0;
	char query[KB2] = {0};
	PGresult *rs = NULL;
	char field1[16],field2[16],field3[16];
	int rule_id = 1;
	char dbRuleBuffer[100];
	char txnRuleBuffer[10][100];
	//bfl_Txn_Details bfl_rule_dtls;

	log_info(SWITCH, "bfl_GetRuleIdDetails");	

	//sprintf(query,"SELECT intl_domestic,rule_value,rule_value,is_active FROM bfl.rule_reference_master",rule_id);
	sprintf(query,"SELECT * FROM bfl.rule_reference_master",rule_id);
	rs = query_database (query,DONT_USE,DONT_USE,0,NULL);
	if (PQresultStatus(rs) != PGRES_TUPLES_OK) {
		PQclear(rs);
		return FALSE;
	}
        
	row = PQntuples(rs);
	col = PQnfields(rs);
	
	memset(txnRuleBuffer[0], 0, sizeof(txnRuleBuffer[0]));
	sprintf(txnRuleBuffer[0],"%s%s%s",bfl_rule_dtls.intl_domestic,bfl_rule_dtls.rule_type,bfl_rule_dtls.rule_value_option1);

        memset(txnRuleBuffer[1], 0, sizeof(txnRuleBuffer[1]));
	sprintf(txnRuleBuffer[1],"%s%s%s",bfl_rule_dtls.intl_domestic,bfl_rule_dtls.rule_type,bfl_rule_dtls.rule_value_option2);


	for(i = 0; i < row ; i++){
		memset(field1, 0, sizeof(field1));	
		strcpy(field1,PQgetvalue(rs, i, 2));
		memset(field2, 0, sizeof(field2));
		strcpy(field2,PQgetvalue(rs, i, 3));
		memset(field3, 0, sizeof(field3));
		strcpy(field3,PQgetvalue(rs, i, 4));

		memset(dbRuleBuffer, 0, sizeof(dbRuleBuffer));
		sprintf(dbRuleBuffer,"%s%s%s",field1,field2,field3);
		
		//log_info(SWITCH, "txnRuleBuffer = %s\n",txnRuleBuffer);
		//log_info(SWITCH, "dbRuleBuffer = %s\n",dbRuleBuffer);

		if((strcasecmp(txnRuleBuffer[0],dbRuleBuffer) == 0) || (strcasecmp(txnRuleBuffer[1],dbRuleBuffer) == 0))
		{
			if((atoi(PQgetvalue(rs, i, 5)) < ipriority) || (ipriority == 0))
			{
				ipriority = atoi(PQgetvalue(rs, i, 5));
				ruleIDResult = atoi(PQgetvalue(rs, i, 1));
				log_info(SWITCH, "ruleIDResult  = %d\n",ruleIDResult);
			}
		}
        }
        PQclear(rs);
        
	bfl_rule_dtls.ruleIDResult = ruleIDResult;

	return ruleIDResult;

}
*/

