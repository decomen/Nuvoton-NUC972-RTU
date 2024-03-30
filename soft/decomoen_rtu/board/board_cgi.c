

DEF_CGI_HANDLER(devReset)
{
    rt_kprintf("vDoSystemReset\n");

    WEBS_PRINTF("{\"ret\":%d}", RT_EOK);
    WEBS_DONE(200);

    vDoSystemReset();
}

DEF_CGI_HANDLER(factoryReset)
{
    rt_kprintf("factoryReset\n");

    WEBS_PRINTF("{\"ret\":%d}", RT_EOK);
    WEBS_DONE(200);

    //dfs_mkfs( "elm", BOARD_SPIFLASH_FLASH_NAME );
    board_cfg_del_all();
    vDoSystemReboot();
}

DEF_CGI_HANDLER(diskFormat)
{
    rt_err_t err = RT_EOK;
    rt_kprintf("diskFormat\n");
    const char *szJSON = CGI_GET_ARG("arg");
    cJSON *pCfg = szJSON ? cJSON_Parse(szJSON) : RT_NULL;
    if (pCfg) {
        const char *disk = cJSON_GetString(pCfg, "disk", "");
        if (disk) {
            if (0 == strcmp(disk, "spiflash")) {
                rt_kprintf("diskFormat spiflash\n");
                /*if (dfs_mkfs("elm", BOARD_SPIFLASH_FLASH_NAME) != 0) {
                    err = RT_ERROR;
                }*/
            } else if (0 == strcmp(disk, "sdcard")) {
                rt_kprintf("diskFormat sdcard\n");
                /*if (dfs_mkfs("elm", BOARD_SDCARD_NAME) != 0) {
                    err = RT_ERROR;
                }*/
            } else {
                err = RT_ERROR;
            }
        } else {
            err = RT_ERROR;
        }
    }
    cJSON_Delete(pCfg);

    WEBS_PRINTF("{\"ret\":%d}", err);
    WEBS_DONE(200);

    vDoSystemReboot();
}

DEF_CGI_HANDLER(listDir)
{
    /*struct dfs_fd fd;
    struct dirent dirent;
    struct stat stat;
    int length;
    char *fullpath = RT_NULL;;
    rt_bool_t first = RT_TRUE;
    const char *path = CGI_GET_ARG("path");
    
    WEBS_PRINTF("{\"ret\":0,\"list\":[");
    if (path) {
        fullpath = RT_NULL;
        if (dfs_file_open(&fd, path, DFS_O_DIRECTORY) == 0) {
            do {
                rt_memset(&dirent, 0, sizeof(struct dirent));
                length = dfs_file_getdents(&fd, &dirent, sizeof(struct dirent));
                if (length > 0) {
                    rt_memset(&stat, 0, sizeof(struct stat));
                    fullpath = dfs_normalize_path(path, dirent.d_name);
                    if (fullpath == RT_NULL) break;
                    if (dfs_file_stat(fullpath, &stat) == 0) {
                        cJSON *pItem = cJSON_CreateObject();
                        if(pItem) {
                            if (!first) WEBS_PRINTF(",");
                            first = RT_FALSE;
                            cJSON_AddStringToObject(pItem, "name", dirent.d_name);
                            cJSON_AddStringToObject(pItem, "type", DFS_S_ISDIR(stat.st_mode)?"dir":"file");
                            cJSON_AddNumberToObject(pItem, "size", stat.st_size);
                            {
                                struct tm lt;
                                char date[16] = { 0 };
                                localtime_r(&stat.st_mtime, &lt);
                                rt_sprintf(date, "%04d/%02d/%02d %02d:%02d:%02d",
                                           lt.tm_year + 1900,
                                           lt.tm_mon + 1,
                                           lt.tm_mday,
                                           lt.tm_hour,
                                           lt.tm_min,
                                           lt.tm_sec
                                          );
                                cJSON_AddStringToObject(pItem, "mtime", date);
                            }
                            char *szRetJSON = cJSON_PrintUnformatted(pItem);
                            if(szRetJSON) {
                                WEBS_PRINTF(szRetJSON);
                                rt_free(szRetJSON);
                            }
                        }
                        cJSON_Delete(pItem);
                    } else {
                        //WEBS_PRINTF("]}");
                    }
                    rt_free(fullpath);
                }
            }while(length > 0);

            dfs_file_close(&fd);
        } else {
            
        }
    }
    WEBS_PRINTF("]}");
    WEBS_DONE(200);*/
    WEBS_PRINTF("{\"ret\":0,\"list\":[]}");
    WEBS_DONE(200);
}

