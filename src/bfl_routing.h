#ifndef BFL_ROUTING_H
#define BFL_ROUTING_H



#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <errno.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <openssl/md5.h>
#include "logger.h"


struct bfl_Txn_Details{
	int rule_id;
	char intl_domestic[8];
	char rule_type[16];
	char rule_value_option1[16];
	char rule_value_option2[16];
	int priority_id;
	int ruleIDResult;
	char bfl_FinalBankCode[10];
	char MccCode[5];
	char MccCategory[64];
	int tidRuleOveride;
	char tidFocusGroup[64];
	char IssuerBankCode[10];
};

struct bfl_Txn_Details bfl_rule_dtls;

#endif
