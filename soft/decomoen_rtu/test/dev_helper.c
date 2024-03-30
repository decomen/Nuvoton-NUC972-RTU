

#include <board.h>


/*

//擦除大小为块大小64KB,数据以64K为单位进行写入。
//nv分区:SPIFLASH偏移  0x90000 - 0x100000  (0x70000--->448K) (nv分区，存放出厂信息等)

*/


void gen_dev_json_file()
{
    cJSON *json =  cJSON_CreateObject();
    if(json){
        cJSON_AddStringToObject(json, "DeviceSn", g_sys_info.SN);
        cJSON_AddStringToObject(json, "DeviceName", g_sys_info.DEV_ID);
        cJSON_AddStringToObject(json, "DeviceHwVer", g_sys_info.HW_VER);
        cJSON_AddStringToObject(json, "DeviceSwVer", g_sys_info.SW_VER);
        cJSON_AddStringToObject(json, "DeviceHwId", g_sys_info.HW_ID);
        cJSON_AddStringToObject(json, "DeviceProDate", g_sys_info.PROD_DATE);
        cJSON_AddStringToObject(json, "VerDesc", g_sys_info.DESC);
        cJSON_AddNumberToObject(json, "DeviceModel", g_sys_info.DEV_MODEL);
        cJSON_AddNumberToObject(json, "DeviceRegStatus", g_sys_info.REG_STATUS);
        cJSON_AddNumberToObject(json, "DeviceRemain", g_sys_info.TEST_REMAIN);
    }
    char *json_str = cJSON_PrintUnformatted(json);
    printf("json_str: %s\r\n", json_str);
    FILE * fp = NULL;
    fp = fopen("/tmp/deviceInfo.json", "w+");
	if(fp==NULL){
		printf("Cannot open %s!\n", "/tmp/deviceInfo.json");
		return;
	}
    fprintf(fp, "%s",json_str);
    fclose(fp);

    cJSON_Delete(json);
}


static void vStrTrim(char*pStr)
{
    char *pTmp = pStr;
    while (*pStr != '\0')
    {
        if (*pStr != ' ')
        {
         *pTmp++ = *pStr;
        }
        ++pStr;
    }
    *pTmp = '\0';
}


static int m_read_ver(char *path)
{
    char buf[1024] = {0};
    //char str[20] = {0};
    int ver_num = 0;
    FILE *file = NULL;
    file = fopen(path,"r");
    if(file){
        while(1){
            if(fgets(buf,sizeof(buf),file) != NULL){
                vStrTrim(buf);
                if((buf[0] != ';') && (buf[0] != '#') && (buf[0] != '@')){
                    //printf("%s\n",buf);
                    sscanf(buf,"ver=%d",&ver_num);
                }else {
                    continue;
                }
            }else {
                break;
            }
        }
    }
	
    return ver_num;  
    fclose(file);
}

/*
static int get_cmd_result(const char* cmd, char* result)
{
	char buffer[256] = {0};
	FILE* pipe = popen(cmd, "r");
	if (!pipe) {
		return -1;
	}
	while(!feof(pipe)) {
		if(fgets(buffer, sizeof(buffer), pipe)) {
			strcat(result, buffer);
		}
	}
	pclose(pipe);
	return 0;
}
*/

static int format_systime(char *sys_date,char *ver_date)
{
	struct   tm ptm; 
	long     ts; 
	int      y,m,d,h,n,s; 
	ts   =   time(NULL); 
	das_localtime_r(&ts, &ptm); 

	y   =   ptm.tm_year+1900;    //年 
	m   =   ptm.tm_mon+1;        //月 
	d   =   ptm.tm_mday;         //日 
	h   =   ptm.tm_hour;         //时 
	n   =   ptm.tm_min;          //分 
	s   =   ptm.tm_sec;          //秒

	sprintf(sys_date,"%04d/%02d/%02d %02d:%02d:%02d",y,m,d,h,n,s);

    int pdate = y%2000*10000+m*100+d;
    
    sprintf(ver_date,"%d",pdate);
	return 0;
}


static int vGetSysRuntime()
{
	char runtime[128]= {0};
    struct sysinfo info;
    if (sysinfo(&info)) {
        fprintf(stderr, "Failed to get sysinfo, errno:%u, reason:%s\n",errno, strerror(errno));
        return -1;
    }
    long timenum=info.uptime;
    int runday=timenum/86400;
    int runhour=(timenum%86400)/3600;
    int runmin=(timenum%3600)/60;
    int runsec=timenum%60;
    sprintf(runtime,"%d天%d时%d分%d秒",runday,runhour,runmin,runsec);
    return (timenum/60); 
}

#if 0

static int vGetDevUUID(char *uid,char *mac)
{

#define DM_READ_ID_CTL    _IO('R',100)
#define DEV_NAME		"/dev/dm_wdg"  

    int fd = -1;
	fd = open(DEV_NAME, O_RDWR);
	if (fd == -1)
	{
		printf("Cannot open %s!\n", DEV_NAME);
		return -1;
	}

    unsigned long long id = 0;
	if(ioctl(fd,DM_READ_ID_CTL,&id)<0){	
		printf("---ioctl set fail\n");
		return -1;
	}
    close(fd);

    unsigned char *pid = (unsigned char *)&id;
    unsigned short usCrc16 = 0;
    unsigned int usCrc32 = 0;
    unsigned char buf[8] = { 0};
	memcpy(buf,pid,8);
    usCrc16 = usMDCRC16((unsigned char *)buf, sizeof(buf));
    usCrc32 = ulMDCrc32(0, (void *)buf, sizeof(buf));
    memcpy(&mac[0], &usCrc16, 2);
    memcpy(&mac[2], &usCrc32, 4);
    mac[0] /= 2;
    mac[0] *= 2;

    unsigned int tmp_buf[5] = {0};
    int i= 0;
    for(i = 0 ; i < 5; i++){
        id += i;
        tmp_buf[i] =  ulMDCrc32(0, (void *)&id, 8);
    }

    unsigned char id_buf[20] = {0};
    memcpy(id_buf,(unsigned char *)tmp_buf,20);

    for(i = 0 ; i < sizeof(id_buf); i++){
        char str[10] = {0};
        sprintf(str, "%02x",id_buf[i]);
        strcat(uid,str);
    }

    for(i = 0 ;  i < 6 ; i++){
        char str[10] = {0};
        sprintf(str, "%02x",mac[i]);
        strcat(uid,str); 
        //printf("mac: %02x\r\n",mac[i]);
    }    

    return 0;
    
}

#else 

int vGetDevUUID(char *uid,char *mac)
{

#define DM_READ_ID_CTL    _IO('R',100)
#define DEV_NAME		"/dev/dm_wdg"  

    int fd = -1;
	fd = open(DEV_NAME, O_RDWR);
	if (fd == -1)
	{
		printf("Cannot open %s!\n", DEV_NAME);
		return -1;
	}

    unsigned long long id = 0;
	if(ioctl(fd,DM_READ_ID_CTL,&id)<0){	
		printf("---ioctl set fail\n");
		return -1;
	}
    close(fd);

    unsigned char *pid = (unsigned char *)&id;
    unsigned short usCrc16 = 0;
    unsigned int usCrc32 = 0;
    unsigned char buf[8] = { 0};
	memcpy(buf,pid,8);
    usCrc16 = das_crc16((unsigned char *)buf, sizeof(buf));
    usCrc32 = ulMDCrc32(0, (void *)buf, sizeof(buf));
    memcpy(&mac[0], &usCrc16, 2);
    memcpy(&mac[2], &usCrc32, 4);
    mac[0] /= 2;
    mac[0] *= 2;

    unsigned int tmp_buf[4] = {0};
    int i= 0;
    for(i = 0 ; i < 4; i++){
        id += i;
        tmp_buf[i] =  ulMDCrc32(0, (void *)&id, 8);
    }

	//unsigned int hwid[4] = { SIM->UIDH, SIM->UIDMH, SIM->UIDML, SIM->UIDL };
	char *phwid = (char *)tmp_buf;
	sprintf(uid, "%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X", phwid[0],phwid[1],phwid[2],phwid[3],phwid[4],phwid[5],
		phwid[6],phwid[7],phwid[8],phwid[9],phwid[10],phwid[11],phwid[12],phwid[13],phwid[14], phwid[15]);
	
    return 0;
    
}

#endif

static void DevInfoDefault(void)
{
    int  s_ver_no = m_read_ver("/usr/fs/version");
	int  h_ver_no = m_read_ver("/etc/h_ver");
    memset(&g_sys_info, 0 , sizeof(das_system_info_t));

    char ver_date[16] = {0};
    format_systime(g_sys_info.SYS_DATE, ver_date);
	g_sys_info.RUNTIME = vGetSysRuntime();
	g_sys_info.DEV_MODEL = DM_C503H14GZ;
	char ver_str[128] = {0};
	DM_VER_FORMAT(ver_str, DM_C503H14GZ_PRODUCT_NAME, s_ver_no, h_ver_no, MD_VER_TYPE_DEBUG, ver_date, SOFT_VER_TYPE_PRODUCT);
	strcpy(g_sys_info.DESC , ver_str);
	char ver_h[32] = {0};
	char ver_s[32] = {0};
	sprintf(ver_h,"%d",h_ver_no);
	sprintf(ver_s,"%d",s_ver_no);
	memset(g_sys_info.HW_VER,0,DAS_HW_VER_LEN);
	memset(g_sys_info.SW_VER,0,DAS_SW_VER_LEN);
	strcpy(g_sys_info.HW_VER,ver_h);
    strcpy(g_sys_info.SW_VER,ver_s);
    char mac[6]={0};
    vGetDevUUID(g_sys_info.HW_ID,mac);

    g_sys_info.REG_STATUS = 0;
    g_sys_info.TEST_REMAIN = 0;
    
    strcpy(g_sys_info.PROD_DATE ,"2018/01/01 08:00:00");
    sprintf( g_sys_info.SN, "RTU2_%02x%02x%02x%02x%02x%02x",mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);
    //strcpy(g_xDevInfo.DEV_ID,"SSSSSSSSSSSSSSSSSSSSSS1");
    strcpy(g_sys_info.DEV_ID,g_sys_info.SN);
    
}

int ReadDevInfo(void)
{
    s_devinfo_packet_t packet;
	memset(&packet,0,sizeof(s_devinfo_packet_t));
	block_read(NV_DEV_FILE, FACTORY_OFFSET, &packet, sizeof(s_devinfo_packet_t));
	unsigned int crc32 = ulMDCrc32(0, &packet.devinfo, sizeof(das_system_info_t));
	if(crc32 != packet.ulCrc32){
		printf("\n>>>>>>>>>>>>>>> not set factory info\r\n");
        printf(">>>>>>>>>>>>>>> not set factory info\r\n");

        DevInfoDefault();
        return -1;
	}else {
	    printf("    -->read devinfo success\r\n\n");
        g_sys_info = packet.devinfo;
         char ver_date[16] = {0};
        format_systime(g_sys_info.SYS_DATE,ver_date);
	    g_sys_info.RUNTIME = vGetSysRuntime();
		 char mac[6]={0};
  	     vGetDevUUID(g_sys_info.HW_ID,mac);
        return 0;
    }
}


int WriteDevInfo(char *DevId, char *psn, char *pro_date)
{
    DevInfoDefault();
    if(DevId && (DevId[0]) > 0) {
        memset(g_sys_info.DEV_ID,0,DAS_DEV_ID_LEN);
        memset(g_sys_info.SN,0,DAS_SN_LEN);
        memset(g_sys_info.PROD_DATE,0,DAS_TIME_LEN);
        
        memcpy(g_sys_info.DEV_ID,DevId,sizeof(DevPN_t));
        memcpy(g_sys_info.SN,psn,sizeof(DevSN_t));
       // strcpy(g_sys_info.PROD_DATE,"2018/05/23 16:05:00");
        strcpy(g_sys_info.PROD_DATE, pro_date);
    }
    s_devinfo_packet_t packet;
	memset(&packet,0,sizeof(s_devinfo_packet_t));
    packet.devinfo = g_sys_info;
    packet.ulCrc32 = ulMDCrc32(0, &packet.devinfo, sizeof(das_system_info_t));
    block_erase(FACTORY_OFFSET);
    block_write(NV_DEV_FILE, FACTORY_OFFSET, &packet, sizeof(s_devinfo_packet_t));
    
    printf("write devinfo devId: %s\r\n\n", g_sys_info.DEV_ID);
    return 0;
}


/*
int main(int argc, char *argv[])
{
	
    if(argc == 4){
        if(strcmp(argv[1] , "write") == 0){
            WriteDevInfo(argv[2],argv[3]);
            return 0;
        }else {
            printf("argv error\r\n");
            printf("%s write [id]\n",argv[0]);
        }
    }
	
    ReadDevInfo();

   if(argc == 2){
       if(strcmp(argv[1] , "gen_json") == 0){
            gen_dev_json_file();
            return 0;
       }
   }
   
    printf("  -> DEV_ID: %s\n", g_sys_info.DEV_ID);
	printf("  -> SN_ID: %s\n", g_sys_info.SN);
	printf("  -> UUID: %s\n", g_sys_info.HW_ID);
	printf("  -> PROD_DATE: %s\n", g_sys_info.PROD_DATE);
	printf("  -> REG_STATUS: %d\n", g_sys_info.REG_STATUS);
	printf("  -> TEST_REMAIN: %d\n", g_sys_info.TEST_REMAIN);
	printf("  -> DEV_MODEL: 0x%x\n", g_sys_info.DEV_MODEL);		
	printf("  -> SYS_DATE : %s\n", g_sys_info.SYS_DATE);
	printf("  -> RUNTIME: %d minutes\n", g_sys_info.RUNTIME);
	printf("  -> DESC: %s\n", g_sys_info.DESC);
	printf("  -> HW_VER: %s\n", g_sys_info.HW_VER);
	printf("  -> SW_VER: %s\n", g_sys_info.SW_VER);

  
			
	while(1)
	{
        sleep(10);
	}
	return 0;
}
*/
