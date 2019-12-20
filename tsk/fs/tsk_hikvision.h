/*
** The Sleuth Kit
**
** Brian Carrier [carrier <at> sleuthkit [dot] org]
** Copyright (c) 2003-2011 Brian Carrier.  All rights reserved
**
** ICS Laboratory [515lab.ics <at> gmail [dot] com]
** Copyright (c) 2019 ICS Laboratory.  All rights reserved.
**
** This software is distributed under the Common Public License 1.0
*/

#ifndef _TSK_HIKVISION_H
#define _TSK_HIKVISION_H

#include "tsk_fs_i.h"

#ifdef __cplusplus
extern "C" {
#endif

extern TSK_RETVAL_ENUM
    hikvision_dir_open_meta(TSK_FS_INFO * a_fs, TSK_FS_DIR ** a_fs_dir,
    TSK_INUM_T a_addr);

#define HIKVISION_FS_MAGIC1 			0x48494B564953494F
#define HIKVISION_FS_MAGIC2 			0x4E4048414E475A48
#define HIKVISION_FS_MAGIC3 			0x4F55000000000000
#define HIKVISION_FS_MAGIC4 			0x0000000000000000
#define HIKVISION_METADATA_SIGNATURE 	0x52415454
#define HIKVISION_HIKBTREE_MAGIC 		0x48494B4254524545
#define HIKVISION_RECORD_ON 			0x0000000000000000
#define HIKVISION_RECORD_OFF 			0xFFFFFFFFFFFFFFFF
#define HIKVISION_MASTERHEADEROFF      	512
#define HIKVISION_FIRSTINO				0
#define HIKVISION_ROOTINO				0
#define HIKVISION_LASTINO				0xFFFFFFFFFFFFFFFF
#define HIKVISION_INOSIZE				48
#define HIKVISION_FILE_CONTENT_LEN		60
#define HIKVISION_FIRSTDATA_OFFSET		56
#define HIKVISION_MAXNAMELEN			255

#define HIKVISION_IN_REG 0x01
#define HIKVISION_IN_DIR 0x02

    
#define HIKVISION_DE_UNKNOWN         0
#define HIKVISION_DE_REG             1
#define HIKVISION_DE_DIR             2
#define HIKVISION_DE_CHR             3
#define HIKVISION_DE_BLK             4
#define HIKVISION_DE_FIFO            5
#define HIKVISION_DE_SOCK            6
#define HIKVISION_DE_LNK             7
#define HIKVISION_DE_MAX             8

//master header area
typedef struct hikvision_masterheader {
	uint8_t mh_empty1[16];
	uint8_t mh_magicnum1[8];
	uint8_t mh_magicnum2[8];
	uint8_t mh_magicnum3[8];
	uint8_t mh_magicnum4[8];
	uint8_t mh_empty2[24];
	uint8_t mh_HardSize[8]; //hardware size
	uint8_t mh_empty3[16];
	uint8_t mh_meta_start_offset[8]; //metadata start offset
	uint8_t mh_meta_size[8];
	uint8_t mh_empty4[8];
	uint8_t mh_data_start_offset[8]; //data area offset
	uint8_t mh_empty5[8];
	uint8_t mh_datablock_size[8]; //datablock size
	uint8_t mh_datablock_count[4];
	uint8_t mh_empty6[4];
	uint8_t mh_hikbtree_offset1[8]; //hikbetree#1 offset
	uint8_t mh_hikbtree_size1[8];
	uint8_t mh_hikbtree_offset2[8];
	uint8_t mh_hikbtree_size2[8];
	uint8_t mh_empty7[8];
	uint8_t mh_empty8[48];
	uint8_t mh_reset_time[4];
	uint8_t mh_empty9[12];
} hikvision_masterheader;

//meta data area
typedef struct hikvision_meta {
	uint8_t meta_empty1[16];
	uint8_t meta_empty2[12];
	uint8_t meta_DVR_on_time[4];
	uint8_t meta_DVR_off_time[4];
} hikvision_meta;

//hikbtree header area
typedef struct hikvision_hikbtree_header {
	uint8_t treehd_empty1[16];
	uint8_t hikbtree_signature[8]; //hikbtree signature
	uint8_t treehd_empty2[20];
	uint8_t hikbtree_created_time[4];
	uint8_t treehd_empty3[16];
	uint8_t footer_offset[8];
	uint8_t treehd_empty4[8];
	uint8_t pagelist_offset[8];
	uint8_t first_offset[8];
} hikvision_hikbtree_header;

//hikbtree pagelist
typedef struct hikvision_hikbtree_pagelist {
	uint8_t pagelist_cnt[4];
	uint8_t pagelist_empty1[4];
	uint8_t pagelist_current_offset[8];
	uint8_t pagelist_empty2[16];	
	uint8_t pagelist_next_offset[8];
} hikvision_hikbtree_pagelist;


/*
 * Inode
 */
typedef struct hikvision_inode{
    uint8_t i_mode[2];      /* u16 */
    uint8_t i_size[4];      /* u32 */
    uint8_t i_flags[4];
    uint8_t i_block[8]; 	/*s32 */
} hikvision_inode;

//hikbtree page
typedef struct hikvision_hikbtree_page {
	uint8_t page_empty1[8];
	uint8_t page_status[8];
	uint8_t page_empty2[1];
	uint8_t page_channel[1];
	uint8_t page_empty3[6];
	uint8_t page_record_time[8];
	uint8_t page_datablock[8];
	uint8_t page_empty4[8];
} hikvision_hikbtree_page;

typedef struct hikvision_dent{
    uint8_t inode[4];       /* u32 */
    uint8_t name_len[2];    /* u16 */
    char name[255];
} hikvision_dent;

typedef struct {
    TSK_FS_INFO fs_info;					/* super class */
    hikvision_masterheader *fs;				/* master header area */
	hikvision_meta *meta;					/* meta data area */
	hikvision_hikbtree_header *hikbtree_hd;	/* hikbtree header area */
	hikvision_hikbtree_pagelist *pagelist;	/* hikbtree pagelist area */
	hikvision_hikbtree_page *page;

   	tsk_lock_t lock;

   	uint32_t pagelist1_cnt;					/* hikbtree1 total page */
	uint32_t pagelist2_cnt;					/* hikbtree2 total page */
   	
   	uint64_t btree1_first_page;					/* hikbtree1 first page offset */
   	uint64_t btree2_first_page;					/* hikbtree1 first page offset */
    uint8_t inode_size;						/* inode size */
    uint64_t block_size;    				/* size of each block */
    uint64_t hikbtree_offset1;				/* hikbtree area1 start offset */
    uint64_t hikbtree_offset2;				/* hikbtree area1 start offset */
    uint64_t meta_offset;					/* meta area start offset */
    uint8_t current_inode;
    uint8_t total_inode_num;
    uint8_t total_dinode_num;

    hikvision_inode inodes[1000];
    hikvision_dent dent[1000][1000];
    uint8_t num_entry[1000];
} HIKVISION_INFO;

static inline void * hikvision_u64toArray(int x, uint8_t target[]) {
	uint8_t *p = (uint8_t *)&x;
	int i;
	for(i = 0; i < 8; i++) {
	    target[i] = p[i];
	}
}

static inline void hikvision_u32toArray(int x, uint8_t target[]) {
	uint8_t *p = (uint8_t *)&x;
	int i;
	for(i = 0; i < 4;i++) {
	    target[i] = p[i];
	}

}

static inline uint8_t * hikvision_u16toArray(int x, uint8_t target[]) {
	uint8_t *p = (uint8_t *)&x;
	uint8_t result[2];
	int i;
	for(i = 0; i < 2; i++) {
	    target[i] = p[i];
	}
}

#endif