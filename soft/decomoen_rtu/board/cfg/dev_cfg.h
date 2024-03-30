
#ifndef __DEV_CFG_H__
#define __DEV_CFG_H__

void vDevCfgInit( void );
void vDevRefreshTime( void );

void SaveAdcCalInfoToFs( void );
//void ReadAdcCalInfoFromFs( void );
void vAdcCalCfgInit(void);

#endif

