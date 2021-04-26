#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tcm_interface.h"
#include "../crc/crc.h"
#include "../serial_dir/dmd_uart.h"
//#define TCM_DEBUG

uint32 Reverse32(uint32 x)
{
    return ((x&0x000000ff)<<24)|((x&0x0000ff00)<<8)|((x&0x00ff0000)>>8)|((x&0xff000000)>>24);
}

static int tcm_transmit_cmd_origin(struct tcm_dev *dev, void *cmd,int cmd_length/*, unsigned int flags, const char *desc*/){
	int i = 0;
	struct dmt_cmd *d_cmd;
	u8 *tx;
	u8 rx[512];
	int tx_len,rx_len;

	d_cmd = (struct dmt_cmd *)cmd;
	tx_len = HEADER_SIZE_BYTES+d_cmd->cmd_h.cmd_length;
	tx = (u8 *)malloc(tx_len);	
	tx[0] = d_cmd->cmd_h.rx_tx_flag;
	tx[1] = d_cmd->cmd_h.cmd_length>>8;
	tx[2] = d_cmd->cmd_h.cmd_length;
	tx[3] = d_cmd->cmd_h.xor_result;
	memcpy(&(tx[4]),d_cmd->tcm_data_buf,d_cmd->cmd_h.cmd_length-2);
	tx[4+d_cmd->cmd_h.cmd_length-2] = d_cmd->crc_result >> 8;
	tx[4+d_cmd->cmd_h.cmd_length-1] = d_cmd->crc_result;


	rx_len = send_recive(dev->uart_device.fd,tx,rx,tx_len);
	if(rx_len < 0){
		printf("tcm transmit cmd failed! err: %d\n",rx_len);
	}

	for(i = 0; i < rx_len; i++){
               	 if(i % 16 == 0) printf("\n");
		 printf("0x%02x ",*(rx+i));
        }
	printf("\n");

	free(tx);
	tx = NULL;
	return 0;
}

uint32 tcm_transmit_cmd(struct tcm_dev *dev,void *cmd, uint16 len,const char *desc){
	int i = 0;
        u8 *tx;
        u8 rx[512];
	int tx_len,rx_len;
	u8 *buf = (u8 *)cmd;
	int ret = 0;
	uint16 offset = 0;
	struct tcm_cmd cmd_buf;

	uint16 tcm_cmd_data_len = len-HEADER_SIZE_BYTES-CRC_LENTH-2;
	tx = (char *)malloc(len);	
	/*
	cmd : [cmd_headre(4byts) crc_value[2bytes] total_data_len(2 bytes) tcm_cmd(nbytes)]
	*/
	/*1. get cmd header*/
	memcpy(tx,buf,HEADER_SIZE_BYTES);

	/*2. get standard tcm cmd*/
	offset = HEADER_SIZE_BYTES;
	memcpy(&tx[offset],&buf[offset+CRC_LENTH+2],tcm_cmd_data_len);

	/*3. get crc result*/
	offset = HEADER_SIZE_BYTES+tcm_cmd_data_len;
	memcpy(&tx[offset],&buf[HEADER_SIZE_BYTES],CRC_LENTH);
	
	/*ready to send data*/
	tx_len = len - 2;  /*remove total_data_len of 2 bytes*/
	rx_len = send_recive(dev->uart_device.fd,tx,rx,tx_len);
	if(rx_len < 0){
		printf("TCM err occur  %s :\n",desc);
		ret = -1;
		goto out;
	}

#ifdef TCM_DEBUG
	printf(" %s \n",desc);	
	for( i = 0; i < len-2; i++){
		printf("0x%02x, ",tx[i]);
	}
	tx_len = len - 2;
	rx_len = send_recive(dev->uart_device.fd,tx,rx,tx_len);
#endif
	printf("%s :\n",desc);
	for(i = 0; i < rx_len; i++){
		if(i % 16 == 0) printf("\n");
		printf("0x%02x ",*(rx+i));
	}
	offset = HEADER_SIZE_BYTES + tcm_cmd_data_len + CRC_LENTH;
	//memcpy(&buf[offset],rx,rx_len);	

	struct cmd_header heder;
	struct tcm_cmd_common cmd_common;

	memcpy(&heder,rx,sizeof(struct cmd_header));
	if(heder.rx_tx_flag != RECV_CMD){
		ret = -2;
		goto out;
	}
	printf("rx_tx_flag: 0x%02x\n",heder.rx_tx_flag);
	memcpy(&cmd_common,&rx[HEADER_SIZE_BYTES],sizeof(struct tcm_cmd_common));	
	printf("data_recv_lenth: 0x%x  flag: 0x%x\n",Reverse32(cmd_common.data_lenth),sw16(cmd_common.flag));	
	
	printf("\n");
	memcpy(buf,rx,rx_len);	
	printf("addr of buf_offset: %p\n",(buf));
	for(i = 0; i < rx_len; i++){
		printf("0x%02x ",buf[i]);
	}
	printf("\n");
out:	
	free(tx);
	tx = NULL;
	return ret;
}

int tcm_cmmmond_xfer(struct tcm_dev *dev, const u8 * tcm_cmd, int cmd_lenth, u8* rx){
	width_t crc_result;
	struct dmt_cmd dmd_tcm_cmd;
	memset(&dmd_tcm_cmd,0,sizeof(dmd_tcm_cmd));
	dmd_tcm_cmd.cmd_h.rx_tx_flag = SEND_CMD;
	dmd_tcm_cmd.cmd_h.cmd_length = cmd_lenth + 2;
	dmd_tcm_cmd.cmd_h.xor_result = dmd_tcm_cmd.cmd_h.rx_tx_flag ^ dmd_tcm_cmd.cmd_h.cmd_length;
	dmd_tcm_cmd.tcm_data_buf = tcm_cmd;
	dmd_tcm_cmd.crc_result = crcCompute_dmt(tcm_cmd,cmd_lenth);
	tcm_transmit_cmd_origin(dev, &dmd_tcm_cmd, cmd_lenth);

	return 0;
}

int tcm_dev_init(struct tcm_dev *dev){
	int fd;
//	if(dev->uart_device){
	if(1){
		fd = uart_init(dev->uart_device.uart_name);
		if(fd < 0){
			return -1;
		}
		if(set_opt(fd, 115200, 8, 'N', 1) < 0){
			return -1;
		}
		dev->uart_device.fd = fd;
	}else{
	/*other device*/
		

	}

	return 0;
}

int tcm_dev_release(struct tcm_dev *dev){
	if(1){
		close(dev->uart_device.fd);			

	}else{
	 /*other device*/
	}

	free(dev);
	dev = NULL;
	
	return 0;
}

void show_help(void){
	printf("Usage:\n");
	printf("        h :       help\n");
	printf("        S :       tcm startup\n");
	printf("        r :       read pcr vaule\n");
	printf("        e :       extend pcr value\n");
	printf("        R :       reset the pcr\n");
	printf("        t :       tcm self test full\n");
	printf("        q :       quit\n");
}


