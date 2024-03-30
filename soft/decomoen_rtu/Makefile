#global
#CROSS:=arm-linux-
CROSS:=/work/toolchain/NUC972/host/usr/bin/arm-linux-
#CROSS:=/home/liming/work/nuc972_rtu2/toolchain/host/usr/bin/arm-linux-
#CROSS:=/home/stzn_1/wxm/ncu_972_build_tools/host/usr/bin/arm-linux-
LIB_PATH:=./libs/arm
#LIB_PATH:=./libs/linux
CC:=$(CROSS)gcc
STRIP:=$(CROSS)strip
#STRIP:=echo
CFLAG:=-g -std=c99
#CFLAG += -Wall
#CFLAG += -Werror

LFLAG:= -lpthread -lrt -L$(LIB_PATH) -L/home/jay/work/gerrit_972/buildroot/output/host/usr/arm-buildroot-linux-gnueabi/sysroot/usr/lib/ -L./ -lmodbus -lmxml -lm -lsqlite3
IFLAG:= -I./
DFLAG:= -D_POSIX_C_SOURCE=199309L -D_BSD_SOURCE -D_DEFAULT_SOURCE

C_FILES:= startup.c das.c das_os.c das_util.c das_docking.c block_msg_queue.c das_romfs.c

# board
IFLAG += -Iboard -Iboard/cfg
C_FILES += board/nv.c
C_FILES += board/board.c board/cpu_usage.c board/net_helper.c board/rtcrc32.c board/serial_helper.c board/storage.c 
C_FILES += board/storage_log.c board/xfer_dst_serial.c board/xfer_helper.c board/xfer_net.c board/zgb_std.c
C_FILES += board/cfg/analog_cfg.c board/cfg/auth_cfg.c board/cfg/board_cfg.c board/cfg/dev_cfg.c board/cfg/dido_cfg.c
C_FILES += board/cfg/host_cfg.c board/cfg/net_cfg.c board/cfg/tcpip_cfg.c board/cfg/uart_cfg.c board/cfg/ws_cfg.c
C_FILES += board/cfg/xfer_cfg.c board/cfg/zigbee_cfg.c
C_FILES += board/cfg/gprs_cfg.c

# drivers
IFLAG += -Idrivers/ad7689 -Idrivers/cjson -Idrivers/in_out -Idrivers/protomanage -Idrivers/varmanage -Idrivers/zigbee -Idrivers/gprs
C_FILES += drivers/ad7689/ad7689.c
C_FILES += drivers/cjson/cJSON.c
C_FILES += drivers/in_out/in_out.c
C_FILES += drivers/protomanage/protomanage.c
C_FILES += drivers/varmanage/varmanage.c
C_FILES += drivers/zigbee/zigbee.c
C_FILES += drivers/gprs/gprs_helper.c

# external
IFLAG += -Iexternal/aes -Iexternal/easylogger/inc -Iexternal/expression -Iexternal/httpclient -Iexternal/ini
IFLAG += -Iexternal/json-c -Iexternal/modbus -Iexternal/mxml -Iexternal/sqlite -Iexternal/webnet
# external->modbus
C_FILES += external/modbus/user_mb_app.c external/modbus/user_mb_app_m.c external/modbus/modbus_helper.c
# external->aes
C_FILES += external/aes/aes.c
# external->easylogger
C_FILES += external/easylogger/src/elog.c external/easylogger/src/elog_async.c external/easylogger/src/elog_buf.c
C_FILES += external/easylogger/src/elog_utils.c external/easylogger/port/elog_port.c
# external->expression
C_FILES += external/expression/ez_expparser.c external/expression/ez_tokenizer.c 
C_FILES += external/expression/ez_function0.c external/expression/ez_function1.c
C_FILES += external/expression/ez_string_function.c external/expression/ez_function2.c
C_FILES += external/expression/ez_exp_os.c external/expression/ez_hash.c
# external->httpclient
C_FILES += external/httpclient/httpclient.c
# external->ini
C_FILES += external/ini/iniparser.c
# external->webnet
C_FILES += external/webnet/module/webnet_auth.c external/webnet/module/webnet_cgi.c
C_FILES += external/webnet/base64.c external/webnet/mimetype.c external/webnet/module.c external/webnet/request.c external/webnet/session.c
C_FILES += external/webnet/sha1.c external/webnet/util.c external/webnet/webnet.c external/webnet/websocket.c
C_FILES += external/webnet/ws_vm.c external/webnet/ws_console.c

# reg
IFLAG += -Ireg
C_FILES += reg/rtu_crc32.c reg/rtu_des.c reg/rtu_md5.c reg/rtu_reg.c reg/rtu_sha.c

# threaddog
IFLAG += -Ithreaddog
C_FILES += threaddog/threaddog.c

#sdccp
IFLAG += -Isdccp -Isdccp/dm101
C_FILES += sdccp/bfifo.c sdccp/cc_bjdc.c sdccp/cc_md5.c sdccp/dlt645.c sdccp/dlt645_1997.c sdccp/hjt212.c
C_FILES += sdccp/sdccp_dust.c sdccp/sdccp_net.c sdccp/sdccp_serial.c
C_FILES += sdccp/dm101/dm_aes.c sdccp/dm101/dm_crc8.c sdccp/dm101/dm_crc32.c 
C_FILES += sdccp/dm101/dm_lib.c sdccp/dm101/dm_md5.c sdccp/dm101/dm101.c sdccp/dm101/os_platform.c

# test
IFLAG += -Itest
C_FILES += test/bytebuffer.c test/dev_helper.c test/mdcrc8.c test/mdcrc32.c test/server_com.c test/test.c
#C_FILES += sdccp/bfifo.c sdccp/cc_bjdc.c sdccp/cc_md5.c sdccp/dlt645.c sdccp/dlt645_1997.c sdccp/hjt212.c

FLAGS:= $(CFLAG) $(DFLAG) $(LFLAG)

QUIET_CC			= @echo "CC	$@";
QUIET_LINK			= @echo "LINK	$@";
QUIET_CLEAN			= @echo "CLEAN	$@";

das_srcs=$(C_FILES)
das_objs=$(das_srcs:.c=.o)

all:web das

das:$(das_objs)
	$(QUIET_LINK)$(CC) -o $@ $^ $(IFLAG) $(FLAGS)
	$(STRIP) --strip-debug --strip-unneeded das
	cp $@ /var/www/html/ota
#	cp $@ /home/liming/work/nuc972_rtu2/dm/app/das/bin/
#	cp das_ver /home/liming/work/nuc972_rtu2/dm/app/das/ver/
#	cp das.conf /home/liming/work/nuc972_rtu2/dm/app/das/conf

%.o:%.c 
	$(QUIET_CC)$(CC) -c $^ -o $@ $(IFLAG) $(FLAGS)

web:
	gcc -o packet_web packet_web.c -std=c99
	./packet_web ./reform ./das_romfs_web.c
	rm das_romfs.o -rf

clean:
	rm *.o $(das_objs) ./das ./packet_web ./das_romfs_web.c -rf

prefix=/usr/local
install: das
	install -m 0755 das $(prefix)/bin
.PHONY: install

