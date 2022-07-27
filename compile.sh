#!/bin/bash
#
make clean-obj mrl_switch.out
make clean-obj mrl_switch_schedular.out
make clean-obj npci_processor.out
make clean-obj gp_pos_hsm_module.out
make clean-obj gp_98byte_filegen.out
make clean-obj mrl_key_injection.out
make clean-obj mrl_switch_pzm_psr.out
make clean-obj mrl_switch_hsm_psr.out
make clean-obj mrl_switch_itzcash_psr.out
make clean-obj mrl_switch_atos_psr.out
make clean-obj mrl_switch_rupay_psr.out
make clean-obj fis_psr.out
make clean-obj mrl_switch_wlpfo_psr.out
make clean-obj op_shm_util.out
make clean-obj op_shm_det.out
make clean-obj

# RGCS
./configure --enable-rgcs --set-sms
make clean-obj rgcs_file_gen_module.out && cp -vf bin/rgcs_file_gen_module.out  bin/rgcs_file_gen_module_SMS.out
./configure --enable-rgcs --set-dms
make clean-obj rgcs_file_gen_module.out && cp -vf bin/rgcs_file_gen_module.out  bin/rgcs_file_gen_module_DMS.out
./configure --enable-rgcs --set-petro
make clean-obj rgcs_file_gen_module.out && cp -vf bin/rgcs_file_gen_module.out  bin/rgcs_file_gen_module_PETRO.out
./configure --enable-rgcs --set-creadj
make clean-obj rgcs_file_gen_module.out && cp -vf bin/rgcs_file_gen_module.out  bin/rgcs_file_gen_module_CR_ADJ.out
# iRGCS
./configure --enable-irgcs --set-sms
make clean-obj rgcs_file_gen_module.out && cp -vf bin/rgcs_file_gen_module.out  bin/irgcs_file_gen_module_SMS.out
./configure --enable-irgcs --set-dms
make clean-obj rgcs_file_gen_module.out && cp -vf bin/rgcs_file_gen_module.out  bin/irgcs_file_gen_module_DMS.out
./configure --enable-irgcs --set-petro
make clean-obj rgcs_file_gen_module.out && cp -vf bin/rgcs_file_gen_module.out  bin/irgcs_file_gen_module_PETRO.out
./configure --enable-irgcs --set-creadj
make clean-obj rgcs_file_gen_module.out && mv -vf bin/rgcs_file_gen_module.out  bin/irgcs_file_gen_module_CR_ADJ.out
make clean-obj
./configure
