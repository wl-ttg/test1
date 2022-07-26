#ifndef CRM_SYNC_H   
#define CRM_SYNC_H

#include <errno.h>
#include "../hsm/structures.h"
#include "../hsm/hsm.h"
#include <sys/ipc.h>
#include <sys/msg.h>

#include <sqlite3.h>
#include<unistd.h>
#include<sys/ipc.h>
#include<sys/sem.h>
#include<semaphore.h>
#include <sys/wait.h>
#include <sys/types.h>

#define MSG_TYPE  		5
#define SHM_KEY 		3456
#define MAX_INSERT_RETRY	3
#define MAX_UPDATE_RETRY	3
#define MAX_PROCESS_DATA 	25
#define SSL_API_NAME 		"mrltxn"
#define SSL_API_ID 		"6057DF295639719B7866C65B4AA595E5"
#define SSL_ACTION 		"INBOUND"
#define SSL_ENTITY_NAME 	"vtiger_merchanttransaction"
#define MAX_ITERATION_PROCESS 	10                                                                                                                    
#define SUCC_FIFO		"saf_success.fifo"
#define FAIL_FIFO		"saf_failure.fifo"
#define LIVEFEED_SUCC_FIFO	"livefeed_saf_success.fifo"
#define LIVEFEED_FAIL_FIFO	"livefeed_saf_failure.fifo"
#define MAX_ID_LEN 		15
#define ZERO                    0 
#define IS_WL_MERCHANT		1
#define MAX_REQ_DATA		20

/*Livefeed SAF*/
#define MRL_LIVEFEED_SAF	1
#define MMS_LIVEFEED_SAF	2

struct crm_sync {
	char mid[20];
	char tid[20];
	char message_type[20];
	char processing_code[20];
	char rrn[20];
	char approval_code[20];
	char response_code[20];
	char created_date[30];
	char stan[20];
	char batch_no[30];
	char amount[30];
	char masked_card_no[40];
	char tip[30];
	char emv[30];
	char enc_pan[100];
	char statistics_data[4096];
	char invoice[20];
	int pin_entered;
	int fallback_indicator;
	char host_name[30];
	int txn_id;
	int servers_sync;
};

struct crm_sync *parsed_crm_req;
struct crm_sync *parsed_crm_resp;

typedef struct {
	char mid[20];
	char tid[20];
	char message_type[20];
	char processing_code[20];
	char rrn[20];
	char approval_code[20];
	char response_code[20];
	char created_date[30];
	char stan[20];
	char batch_no[30];
	char amount[30];
	char masked_card_no[40];
	char tip[30];
	char emv[30];
	char enc_pan[100];
	char statistics_data[4096];
	char invoice[20];
	int pin_entered;
	int fallback_indicator;
	char host_name[30];
	char txn_Id[20];
	char currency_code[6];
	char currency_name[6];
	char exchange_rate[12];
	char inr_amt[15];
	char inr_tip[15];
	char markup_percentage[6];
	char surcharge_amount[15];
	char cgst_amount[15];
	char sgst_amount[15];
	char orig_rrn[20];
	int txn_type;
	int txn_category;
	char card_type[8];
	int servers_sync;
	char association[24];
} Livefeed_t;

Livefeed_t *livefeed_resp_data[10];
Livefeed_t *livefeed_request_data;

struct crm_sync_req
{
        long type ;
        struct crm_sync mtr ;
};



struct atos_crm_sync_req
{
        long type ;
        Livefeed_t mtr ;
};


struct crm_resp 
{
	char resp_code[10];
	char resp_msg[40];
};
struct crm_resp *crmresp;

union semun
{
	int val;
	struct semid_ds *buf;
	unsigned short *array;
};

enum livefeed_servers_sync {
	MMS_LIVEFEED	 = 1,	/* MMSFE */
	MMS_LIVEFEED_BE	 = 2,	/* MMSFE & MMSBE_additional */
	NON_MMS_LIVEFEED = 3,	/* MMSBE_additional */
	PAYMONK_LIVEFEED  = 4,  /*PAYMONK */
};

char saf_table_name[100];
char livefeed_buffer[10240];
char tmp_livefeed_buffer[10240];

int crm_alive_process_count;
int saf_alive_process_count;
int saf_count_flag;
int mms_livefeed_saf;
int livefeed_saf_flag;
int process_terminated;

/* function declaration */
int crm_sync_data ( void );
int get_crmsync_data ( void );
int get_livefeed_crmsync_data ( void );
void crmsync_child_process_handler(int signo);

int sync_data ( struct crm_sync * crm , char *url, char *action, char *timeout_buf,int conn_type );
int crmsync_xml_parse( unsigned char * buf , int len , int more ) ;
int get_current_time (unsigned char * datetime_now) ;
int crmsync_queue_struct_read (  int key , struct crm_sync * mtr ) ;
int crm_queue_req_struct_write(  int key , struct crm_sync *mtr  ) ;

int livefeed_queue_struct_read(int key , Livefeed_t *mtrr);
int livefeed_queue_struct_write(int key , Livefeed_t *mtr);
void form_multiple_request(Livefeed_t * crmm[] , int count);
int process_multiple_safdata(char *api, char *action, int timeout, int conn_type, int process_max_count, int succ_fifo_fd, int fail_fifo_fd);

/* Function Declaration for SAF */
int create_saf_db();
int insert_saf_db(char *xml, char *rrn, int conn_type);
int select_saf_db(char *data, char *id, char *conn_type, int connection_type);
int update_saf_db(int id, int status);
int store_in_backup_file (int id, char *xml);

#endif
