##
 # ===  FILE  ==========================================================================
 #         Name : Makefile
 #      Created : 2018-12-03 13:44:53 IST
 #       Author : Sathishkumar D
 # =====================================================================================
##
CC = gcc
INCLUDES  = -I include/ -I  /usr/include/libxml2 -I /usr/local/include -I /usr/java/jdk1.8.0_231-amd64/include/ -I /usr/java/jdk1.8.0_231-amd64/include/linux/ -L /usr/lib -L /usr/pgsql-9.0/lib -L /usr/java/jdk1.8.0_231-amd64/jre/lib/amd64/server/
LIBS1  = -lxml2 -lssl -lcrypto -lcurl -lpthread -lexpat -lpq -lm -lrt -lsqlite3 -lhiredis -lcjson -ljvm
LIBS2  = -lcrypto -lm -lrt
LIBS3  = -lcrypto -lm -lrt -lhiredis -lpq
FISLIB  = -lcrypto -lm -lrt -lhiredis -lssl -lpq
OBJDIR = ./obj
STATIC_LIBS =

VPATH = ./src/ ./obj/ ./include/
vpath %.o obj
vpath %.h include
vpath %.c src/agent src/atos src/crm src/gp src/host src/hsm src/iso src/itzcash src/mvisa src/npci src/pos src/RGCS src/settle src/wallet src/wlpfo src/fis


#######################################
# Compilation Mode
#######################################
ifeq ($(MODE) , DEBUG)
CFLAGS := $(CFLAGS) -Wall -g -D LOGDEBUG
else
CFLAGS := $(CFLAGS) -O3
endif


#######################################
# Compilation of source files
#######################################
$(OBJDIR)/%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<


#######################################
# Define all the Object files
#######################################
AGENTOBJ	= agent_psr.o
GPFILEOBJ	= 98byte_file.o
CRMSYNCOBJ	= crm_sync.o crm_sqlite.o
MVISAOBJ	= mvisa_handler.o mvisa_functions.o
HOSTSETTLEMOBJ	= host_settle_processor.o host_settle_functions.o
RGCSOBJ		= rgcs_file_generator.o rgcs_functions.o rgcs_config.o
MAINOBJ		= iso_host.o signal_handlers.o logger.o iso_listener.o iso_comms.o iso_dblib.o iso_8583.o iso_8583_Parser.o iso_8583_Maker.o strfns.o msg_queue_interface.o newconfig.o dbconfig.o shm_functions.o overload_protect.o op_shm_util.o wlpfo_db.o wlpfo_functions.o
COMMOBJ		= iso_http_stat.o 3des.o iso_make_field.o keygen.o scheduler.o
HSMOBJ		= mpos_hsm_psr.o hsm_functions.o hsm_db.o iso_hsm_make_field.o hsm_comms.o hsm_functionalities.o socket_comms.o hsm_load_balancer.o hsm_host_monitor.o hsm_services.o hsm_host_processor.o adyton_interface.o
POSOBJ		= innoviti_comms.o innoviti_funcs.o iso_pos_psr.o emi_txn_psr.o pos_tms.o btree.o pzm_q_processor.o pos_functions.o download_fns.o ssl_connection.o generate_postxns.o binrange.o thin_client.o itzcash_processor.o atos_functions.o settlement.o host_functions.o cJSON.o wallet_comms.o wallet_function.o
HOSTOBJ		= hitachi_q_processor.o hitachi_q_comms.o routing_functions.o npci_processor.o npci_comms.o rupay_q_comms.o rupay_q_processor.o smart_routing.o redis_comms.o process.o host_functions.o fis_processor.o
PZM_PSR_OBJS	= iso_host.o signal_handlers.o logger.o newconfig.o socket_comms.o iso_listener.o iso_comms.o msg_queue_interface.o strfns.o pzm_q_processor.o  iso_8583.o  shm_functions.o  host_functions.o adyton_interface.o
ITZCASH_PSR_OBJS= iso_host.o signal_handlers.o logger.o newconfig.o socket_comms.o iso_listener.o iso_comms.o msg_queue_interface.o strfns.o pzm_q_processor.o  iso_8583.o  shm_functions.o  itzcash_processor.o host_functions.o adyton_interface.o
ATOS_PSR_OBJS	= iso_host.o logger.o newconfig.o socket_comms.o iso_listener.o iso_comms.o msg_queue_interface.o strfns.o iso_8583.o shm_functions.o host_functions.o atos_processor.o iso_8583_Maker.o iso_8583_Parser.o adyton_interface.o
HSM_PSR_OBJS	= iso_host.o signal_handlers.o iso_8583.o logger.o newconfig.o socket_comms.o iso_listener.o msg_queue_interface.o strfns.o hsm_comms.o hsm_load_balancer.o hsm_host_monitor.o hsm_services.o hsm_host_processor.o redis_comms.o iso_hsm_make_field.o process.o adyton_interface.o
RUPAY_PSR_OBJS	= iso_host.o signal_handlers.o iso_8583.o logger.o newconfig.o socket_comms.o iso_listener.o iso_comms.o msg_queue_interface.o strfns.o host_functions.o redis_comms.o iso_hsm_make_field.o shm_functions.o iso_8583_Maker.o iso_8583_Parser.o rupay_q_comms.o rupay_q_processor.o adyton_interface.o
OP_SHM_UTIL_OBJS= iso_host.o signal_handlers.o iso_8583.o logger.o newconfig.o socket_comms.o iso_listener.o msg_queue_interface.o strfns.o redis_comms.o overload_protect.o op_shm_util.o adyton_interface.o
FIS_PSR_OBJS  = iso_host.o signal_handlers.o iso_8583.o logger.o newconfig.o socket_comms.o iso_comms.o iso_dblib.o iso_listener.o msg_queue_interface.o strfns.o  fis_processor.o  redis_comms.o process.o shm_functions.o host_functions.o iso_8583_Maker.o iso_8583_Parser.o npci_comms.o adyton_interface.o
WLPFO_PSR_OBJS  = iso_host.o signal_handlers.o iso_8583.o logger.o newconfig.o socket_comms.o iso_comms.o iso_dblib.o iso_listener.o msg_queue_interface.o strfns.o  wlpfo_host_monitor.o wlpfo_functions.o wlpfo_processor.o wlpfo_db.o redis_comms.o process.o shm_functions.o host_functions.o iso_8583_Maker.o iso_8583_Parser.o dbconfig.o adyton_interface.o

ALL_OBJS	= $(MAINOBJ) $(HSMOBJ) $(COMMOBJ) $(POSOBJ) $(HOSTOBJ) $(CRMSYNCOBJ) $(HOSTSETTLEMOBJ) $(MVISAOBJ) $(RGCSOBJ) $(GPFILEOBJ)


#######################################
# Final compilation
#######################################
mrl_switch.out:	CC += -D MAIN_MODULE -D MRL_POS -D HSM_KEYS
mrl_switch.out: $(patsubst %.o,$(OBJDIR)/%.o, $(ALL_OBJS))
	$(CC) $(INCLUDES) $(LIBS1) $(STATIC_LIBS) $^ -o bin/$@
	@echo "Compiled successfully.."

gp_pos_hsm_module.out: CC += -D MAIN_MODULE -D GP_POS -D HSM_KEYS -D D_RUPAY_PSR -D NPCI_PSR
gp_pos_hsm_module.out: $(patsubst %.o,$(OBJDIR)/%.o, $(ALL_OBJS))
	$(CC) $(INCLUDES) $(LIBS1) $(STATIC_LIBS) $^ -o bin/$@
	@echo "Compiled successfully.."

gp_98byte_filegen.out: CC += -D MAIN_MODULE -D GP_98BYTE_FILE_GEN -D HSM_KEYS
gp_98byte_filegen.out: $(patsubst %.o,$(OBJDIR)/%.o, $(ALL_OBJS))
	$(CC) $(INCLUDES) $(LIBS1) $(STATIC_LIBS) $^ -o bin/$@
	@echo "Compiled successfully.."

npci_processor.out: CC += -D MAIN_MODULE -D NPCI_PSR -D HSM_KEYS
npci_processor.out: $(patsubst %.o,$(OBJDIR)/%.o, $(ALL_OBJS))
	$(CC) $(INCLUDES) $(LIBS1) $(STATIC_LIBS) $^ -o bin/$@
	@echo "Compiled successfully.."

rgcs_file_gen_module.out: CC += -D MAIN_MODULE -D RGCS_FILE_GEN -D HSM_KEYS -D $(STAGINGTYPE) -D $(STAGINGFILE)
rgcs_file_gen_module.out: $(patsubst %.o,$(OBJDIR)/%.o, $(ALL_OBJS))
	$(CC) $(INCLUDES) $(LIBS1) $(STATIC_LIBS) $^ -o bin/$@
	@echo "Compiled successfully.."

mrl_key_injection.out: CC += -D MAIN_MODULE -D MT_KEY_INJECT -D HSM_KEYS
mrl_key_injection.out: $(patsubst %.o,$(OBJDIR)/%.o, $(ALL_OBJS))
	$(CC) $(INCLUDES) $(LIBS1) $(STATIC_LIBS) $^ -o bin/$@
	@echo "Compiled successfully.."

mrl_switch_pzm_psr.out:	CC += -D SUB_MODULE -D D_HITACHI_PSR
mrl_switch_pzm_psr.out: $(patsubst %.o,$(OBJDIR)/%.o, $(PZM_PSR_OBJS))
	$(CC) $(INCLUDES) $(LIBS2) $(STATIC_LIBS) $^ -o bin/$@
	@echo "Compiled successfully.."

mrl_switch_itzcash_psr.out: CC += -D SUB_MODULE -D D_ITZCASH_PSR
mrl_switch_itzcash_psr.out: $(patsubst %.o,$(OBJDIR)/%.o, $(ITZCASH_PSR_OBJS))
	$(CC) $(INCLUDES) $(LIBS2) $(STATIC_LIBS) $^ -o bin/$@
	@echo "Compiled successfully.."

mrl_switch_rupay_psr.out: CC += -D SUB_MODULE -D D_RUPAY_PSR
mrl_switch_rupay_psr.out: $(patsubst %.o,$(OBJDIR)/%.o, $(RUPAY_PSR_OBJS))
	$(CC) $(INCLUDES) $(LIBS3) $(STATIC_LIBS) $^ -o bin/$@
	@echo "Compiled successfully.."

mrl_switch_atos_psr.out: CC += -D SUB_MODULE -D D_ATOS_PSR
mrl_switch_atos_psr.out: $(patsubst %.o,$(OBJDIR)/%.o, $(ATOS_PSR_OBJS))
	$(CC) $(INCLUDES) $(LIBS2) $(STATIC_LIBS) $^ -o bin/$@
	@echo "Compiled successfully.."

mrl_switch_hsm_psr.out: CC += -D SUB_MODULE -D D_HSM_PSR -D HSM_KEYS
mrl_switch_hsm_psr.out: $(patsubst %.o,$(OBJDIR)/%.o, $(HSM_PSR_OBJS))
	$(CC) $(INCLUDES) $(LIBS3) $(STATIC_LIBS) $^ -o bin/$@
	@echo "Compiled successfully.."

op_shm_util.out: CC += -D SUB_MODULE -D D_OP_SHM_UTIL
op_shm_util.out: $(patsubst %.o,$(OBJDIR)/%.o, $(OP_SHM_UTIL_OBJS))
	$(CC) $(INCLUDES) $(LIBS3) $(STATIC_LIBS) $^ -o bin/$@
	@echo "Compiled successfully.."

mrl_switch_schedular.out: CC += -D MAIN_MODULE -D HSM_KEYS -D REVERSAL_SCHEDULAR
mrl_switch_schedular.out: $(patsubst %.o,$(OBJDIR)/%.o, $(ALL_OBJS))
	$(CC) $(INCLUDES) $(LIBS1) $(STATIC_LIBS) $^ -o bin/$@
	@echo "Compiled successfully.."

fis_psr.out: CC += -D SUB_MODULE -D D_FIS_PSR
fis_psr.out: $(patsubst %.o,$(OBJDIR)/%.o, $(FIS_PSR_OBJS))
	$(CC) $(INCLUDES) $(FISLIB) $(STATIC_LIBS) $^ -o bin/$@
	@echo "Compiled successfully.."
mrl_switch_wlpfo_psr.out: CC += -D SUB_MODULE -D D_WLPFO_PSR
mrl_switch_wlpfo_psr.out: $(patsubst %.o,$(OBJDIR)/%.o, $(WLPFO_PSR_OBJS))
	$(CC) $(INCLUDES) $(LIBS3) $(STATIC_LIBS) $^ -o bin/$@
	@echo "Compiled successfully.."

op_shm_det.out: CC += -D SUB_MODULE -D D_OP_SHM_INS
op_shm_det.out: $(patsubst %.o,$(OBJDIR)/%.o, $(OP_SHM_UTIL_OBJS))
	$(CC) $(INCLUDES) $(LIBS3) $(STATIC_LIBS) $^ -o bin/$@
	@echo "Compiled successfully.."

#######################################
# Makefile compilation options
# OPTIONS -> mrl_switch.out  gp_pos_hsm_module.out  gp_98byte_filegen.out  npci_processor.out  rgcs_file_gen_module.out  mrl_key_injection.out  mrl_switch_pzm_psr.out  mrl_switch_itzcash_psr.out  mrl_switch_hsm_psr.out mrl_switch_atos_psr.out mrl_switch_rupay_psr.out mrl_switch_schedular.out
#######################################
all: mrl_switch.out

clean-obj:
	-find . -name '*.o' -delete
	-find obj/ -name '*.o' -delete

clean:
	-find . -name '*.o' -delete
	-rm -f bin/mrl_switch.out  bin/gp_pos_hsm_module.out  bin/gp_98byte_filegen.out  bin/npci_processor.out  bin/rgcs_file_gen_module.out
	-rm -f bin/mrl_key_injection.out  bin/mrl_switch_pzm_psr.out  bin/mrl_switch_itzcash_psr.out  bin/mrl_switch_hsm_psr.out
	-rm -f bin/mrl_switch_atos_psr.out  bin/mrl_switch_rupay_psr.out bin/op_shm_util.out bin/mrl_switch_schedular.out
	-rm -f bin/fis_psr.out
	-rm -f bin/mrl_switch_wlpfo_psr.out
	-rm -f bin/op_shm_det.out

recompile: clean all
	@echo "Recompiled successfully.."

