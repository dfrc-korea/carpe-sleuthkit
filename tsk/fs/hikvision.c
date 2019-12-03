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

#include "tsk_hikvision.h"

uint8_t
hikvision_inode_walk(TSK_FS_INFO * fs, TSK_INUM_T start_inum,
    TSK_INUM_T end_inum, TSK_FS_META_FLAG_ENUM flags,
    TSK_FS_META_WALK_CB a_action, void *a_ptr)
{
    printf("\n%s\n",__FUNCTION__);
    
    char *myname = "hikvision_inode_walk";
    TSK_INUM_T inum;
    TSK_INUM_T end_inum_tmp;
    TSK_FS_FILE * fs_file;
    unsigned int myflags;

    tsk_error_reset();

    if(start_inum < fs->first_inum || start_inum > fs->last_inum){
        tsk_error_reset();
        tsk_error_set_errno(TSK_ERR_FS_WALK_RNG);
        tsk_error_set_errstr("%s: start inode: %s" PRIuINUM "", myname, start_inum);
        return 1;
    }
    if(end_inum < fs->first_inum || end_inum > fs->last_inum || end_inum < start_inum){
        tsk_error_reset();
        tsk_error_set_errno(TSK_ERR_FS_WALK_RNG);
        tsk_error_set_errstr("%s: end inode: %s" PRIuINUM "", myname, end_inum);
        return 1;
    }

    if (flags & TSK_FS_META_FLAG_ORPHAN) {
        flags |= TSK_FS_META_FLAG_UNALLOC;
        flags &= ~TSK_FS_META_FLAG_ALLOC;
        flags |= TSK_FS_META_FLAG_USED;
        flags &= ~TSK_FS_META_FLAG_UNUSED;
    }
    else {
        if (((flags & TSK_FS_META_FLAG_ALLOC) == 0) &&
            ((flags & TSK_FS_META_FLAG_UNALLOC) == 0)) {
            flags |= (TSK_FS_META_FLAG_ALLOC | TSK_FS_META_FLAG_UNALLOC);
        }
        /* If neither of the USED or UNUSED flags are set, then set them
         * both
         */
    
        if (((flags & TSK_FS_META_FLAG_USED) == 0) &&
            ((flags & TSK_FS_META_FLAG_UNUSED) == 0)) {
            flags |= (TSK_FS_META_FLAG_USED | TSK_FS_META_FLAG_UNUSED);
        }
    }
    
    /* If we are looking for orphan files and have not yet filled
     * in the list of unalloc inodes that are pointed to, then fill
     * in the list
     */
    
    if ((flags & TSK_FS_META_FLAG_ORPHAN)) {
        if (tsk_fs_dir_load_inum_named(fs) != TSK_OK) {
            tsk_error_errstr2_concat
                ("- hikvision_inode_walk: identifying inodes allocated by file names");
            return 1;
        }
    }
    if ((fs_file = tsk_fs_file_alloc(fs)) == NULL)
        return 1;

    return -1;
    
}

uint8_t hikvision_block_walk(TSK_FS_INFO * fs, TSK_DADDR_T start, TSK_DADDR_T end, 
    TSK_FS_BLOCK_WALK_FLAG_ENUM flags, TSK_FS_BLOCK_WALK_CB cb, void *ptr)
{
    printf("\n%s\n",__FUNCTION__);
    return -1;
}

static TSK_OFF_T hikvision_make_data_run(TSK_FS_INFO * fs_info, TSK_FS_ATTR * fs_attr, TSK_FS_META *fs_meta)
{
    TSK_FS_ATTR_RUN *data_run;
    HIKVISION_INFO* hikvision = (HIKVISION_INFO *)fs_info;
    data_run = tsk_fs_attr_run_alloc();
    if (data_run == NULL) {
        return 1;
    }

    data_run->offset = 0;
    data_run->addr = (tsk_getu64(fs_info->endian, hikvision->inodes[fs_meta->addr].i_block) / 0x1000);
    data_run->len = fs_meta->size;

    printf("addr(0x%lx), len(0x%x)\n", data_run->addr, data_run->len);
    // save the run
    if (tsk_fs_attr_add_run(fs_info, fs_attr, data_run)) {
        return 1;
    }

    return 0;

}

static uint8_t
hikvision_load_attrs(TSK_FS_FILE *fs_file)
{
    printf("\n%s\n",__FUNCTION__);
    TSK_FS_META *fs_meta = fs_file->meta;
    TSK_FS_INFO *fs_info = fs_file->fs_info;
    TSK_OFF_T length = 0;
    TSK_FS_ATTR *fs_attr;


    if ((fs_meta->attr != NULL)
        && (fs_meta->attr_state == TSK_FS_META_ATTR_STUDIED)) {
        return 0;
    }
    else if (fs_meta->attr_state == TSK_FS_META_ATTR_ERROR) {
        return 1;
    }

    if (fs_meta->attr != NULL) {
        tsk_fs_attrlist_markunused(fs_meta->attr);
    }
    else {
        fs_meta->attr = tsk_fs_attrlist_alloc();
    }
    
    length = roundup(fs_meta->size, fs_info->block_size);
    
    if ((fs_attr =
         tsk_fs_attrlist_getnew(fs_meta->attr,
                                TSK_FS_ATTR_NONRES)) == NULL) {
        return 1;
    }
    
    if (tsk_fs_attr_set_run(fs_file, fs_attr, NULL, NULL,
                            TSK_FS_ATTR_TYPE_DEFAULT, TSK_FS_ATTR_ID_DEFAULT,
                            fs_meta->size, fs_meta->size, length, 0, 0)) {
        return 1;
    }

    if (hikvision_make_data_run(fs_info, fs_attr, fs_meta)) {
        return 1;
    }
    fs_meta->attr_state = TSK_FS_META_ATTR_STUDIED;
    
    return 0;
}

static uint8_t
hikvision_dinode_load(HIKVISION_INFO * hikvision, TSK_INUM_T dino_inum,
    hikvision_inode * dino_buf)
{

    TSK_OFF_T addr;
    ssize_t cnt;
    TSK_INUM_T rel_inum;
    TSK_FS_INFO *fs = (TSK_FS_INFO *) & hikvision->fs_info;
    printf("\n%s, inum(0x%x)\n",__FUNCTION__, dino_inum);

    /*
     * Sanity check.
     * Use last_num-1 to account for virtual Orphan directory in last_inum.
     */
    if ((dino_inum < fs->first_inum) || (dino_inum > fs->last_inum)) {
        tsk_error_reset();
        tsk_error_set_errno(TSK_ERR_FS_INODE_NUM);
        tsk_error_set_errstr("hikvision_dinode_load: address: %" PRIuINUM,
            dino_inum);
        return 1;
    }

    if (dino_buf == NULL) {
        tsk_error_reset();
        tsk_error_set_errno(TSK_ERR_FS_ARG);
        tsk_error_set_errstr("hikvision_dinode_load: dino_buf is NULL");
        return 1;
    }
    addr = tsk_getu64(fs->endian, dino_buf->i_block);
    hikvision->current_inode = hikvision->current_inode + 1;
 
    if (cnt != hikvision->inode_size) {
        if (cnt >= 0) {
            tsk_error_reset();
            tsk_error_set_errno(TSK_ERR_FS_READ);
        }

        tsk_error_set_errstr2("hikvision_dinode_load: Inode %" PRIuINUM
            " from %" PRIuOFF, dino_inum, addr);

        return 1;
    }
    return 0;
}

static uint8_t
hikvision_dinode_copy(HIKVISION_INFO * hikvision, TSK_FS_META * fs_meta,
    TSK_INUM_T inum, const hikvision_inode * dino_buf)
{

    int i;
    TSK_FS_INFO *fs = (TSK_FS_INFO *) & hikvision->fs_info;
    TSK_INUM_T ibase = 0;
    printf("\n%s, 0x%x\n",__FUNCTION__, inum);

    if (dino_buf == NULL) {
        tsk_error_reset();
        tsk_error_set_errno(TSK_ERR_FS_ARG);
        tsk_error_set_errstr("hikvision_dinode_copy: dino_buf is NULL");
        return 1;
    }
  
    fs_meta->attr_state = TSK_FS_META_ATTR_EMPTY;
    if (fs_meta->attr) {
        tsk_fs_attrlist_markunused(fs_meta->attr);
    }

    fs_meta->mode = tsk_getu16(fs->endian, hikvision->inodes[inum].i_mode);

    fs_meta->nlink = 1;
    fs_meta->size = 0x40000000;
    fs_meta->addr = inum;

    if (fs_meta->mode == 2) {
        fs_meta->type = TSK_FS_META_TYPE_DIR;
    } else {
        fs_meta->type = TSK_FS_META_TYPE_REG;
    }

    /* the general size value in the inode is only 32-bits,
     * but the i_dir_acl value is used for regular files to
     * hold the upper 32-bits
     *
     * The RO_COMPAT_LARGE_FILE flag in the super block will identify
     * if there are any large files in the file system
     */

    fs_meta->uid = 0;
    fs_meta->gid = 0;

    //fs_meta->mtime = tsk_getu64(fs->endian, dino_buf->page_record_time);
    fs_meta->mtime = 150151151;
    fs_meta->atime = fs_meta->mtime;
    fs_meta->ctime = fs_meta->ctime;

    fs_meta->seq = 0;

    if (fs_meta->link) {
         free(fs_meta->link);
         fs_meta->link = NULL;
    }

    if (fs_meta->content_len != HIKVISION_FILE_CONTENT_LEN) {
         if (tsk_verbose) {
            fprintf(stderr, "hikvision.c: content_len is not HIKVISION_FILE_CONTENT_LEN\n");
         }

         if ((fs_meta =
                 tsk_fs_meta_realloc(fs_meta,
                     HIKVISION_FILE_CONTENT_LEN)) == NULL) {
             return 1;
         }
    }
    

    // Allocating datafork area in content_ptr
    // Contents after inode core must be copied to content ptr

    fs_meta->flags |= TSK_FS_META_FLAG_USED;
    fs_meta->flags |= TSK_FS_META_FLAG_ALLOC;

    fs_meta->content_type = TSK_FS_META_CONTENT_TYPE_DEFAULT;

    return 0;
}

static uint8_t 
hikvision_inode_lookup(TSK_FS_INFO * fs, TSK_FS_FILE * a_fs_file,  // = file_add_meta
    TSK_INUM_T inum)
{
    HIKVISION_INFO * hikvision = (HIKVISION_INFO *) fs;
    hikvision_inode * dino_buf = NULL;
    unsigned int size = 0;
    printf("\n%s, inum(0x%x)\n",__FUNCTION__, inum);

    if (a_fs_file == NULL) {
        tsk_error_set_errno(TSK_ERR_FS_ARG);
        tsk_error_set_errstr("hikvision_inode_lookup: fs_file is NULL");
        return 1;
    }

    if (a_fs_file->meta == NULL) {
        if (((a_fs_file->meta =      
            tsk_fs_meta_alloc(HIKVISION_FILE_CONTENT_LEN)) == NULL)){ // #define HIKVISION_FILE_CONTENT_LEN 
            return 1;
        }
    }
    else {
        tsk_fs_meta_reset(a_fs_file->meta);
    }
    

    if((dino_buf = (hikvision_inode *)tsk_malloc(HIKVISION_INOSIZE)) == NULL){
        return 1;
    }


    if (hikvision_dinode_load(hikvision, inum, dino_buf)){
        free(dino_buf);
        return 1;
    }

    if (hikvision_dinode_copy(hikvision, a_fs_file->meta, inum, dino_buf)){
        free(dino_buf);
        return 1;
    }


    free(dino_buf);
    
    return 0;
}

uint8_t hikvision_fscheck(TSK_FS_INFO * fs, FILE * HFile)
{
    printf("\n%s\n",__FUNCTION__);
    return -1;
}

//block_getflags
TSK_FS_BLOCK_FLAG_ENUM hikvision_block_getflags(TSK_FS_INFO * a_fs, TSK_DADDR_T a_addr)
{
    int flags = 0;

    if (a_addr == 0)
        return TSK_FS_BLOCK_FLAG_CONT | TSK_FS_BLOCK_FLAG_ALLOC;
    
    flags = TSK_FS_BLOCK_FLAG_ALLOC;
    flags |= TSK_FS_BLOCK_FLAG_CONT;

    return (TSK_FS_BLOCK_FLAG_ENUM)flags;
}

//fsstat
uint8_t hikvision_fsstat(TSK_FS_INFO * fs, FILE * hFile)
{
    HIKVISION_INFO * hikvision = (HIKVISION_INFO *) fs;
    hikvision_masterheader *mh = hikvision->fs;
    hikvision_meta *meta = hikvision->meta;

    printf("\n%s\n",__FUNCTION__);
    tsk_error_reset();
    tsk_fprintf(hFile, "FILE SYSTEM INFORMATION\n");
    tsk_fprintf(hFile, "--------------------------------------------\n");
    tsk_fprintf(hFile, "Data Block Count : %" PRIu32 "\n", tsk_getu32(fs->endian, mh->mh_datablock_count));
    tsk_fprintf(hFile, "Hardware Size : %" PRIu64 "\n", tsk_getu64(fs->endian, mh->mh_HardSize));
    tsk_fprintf(hFile, "HIKBTREE #1 Start Address : %" PRIu64 "\n", tsk_getu64(fs->endian, mh->mh_hikbtree_offset1));
    tsk_fprintf(hFile, "HIKBTREE #1 Size : %" PRIu64 "\n", tsk_getu32(fs->endian, mh->mh_hikbtree_size1));
    tsk_fprintf(hFile, "HIKBTREE #2 Start Address : %" PRIu64 "\n", tsk_getu64(fs->endian, mh->mh_hikbtree_offset2));
    tsk_fprintf(hFile, "HIKBTREE #2 Size : %" PRIu64 "\n", tsk_getu64(fs->endian, mh->mh_hikbtree_size2));
    tsk_fprintf(hFile, "Reset time : %" PRIu32 "\n\n\n", tsk_getu32(fs->endian, mh->mh_reset_time));
    
    tsk_fprintf(hFile, "META DATA AREA INFORMATION\n");
    tsk_fprintf(hFile, "--------------------------------------------\n");
    tsk_fprintf(hFile, "DVR ON Time : %" PRIu32 "\n", tsk_getu32(fs->endian, meta->meta_DVR_on_time));
    tsk_fprintf(hFile, "DVR OFF Time : %" PRIu32 "\n", tsk_getu32(fs->endian, meta->meta_DVR_off_time));

    return -1;
}

uint8_t hikvision_istat(TSK_FS_INFO * fs, TSK_FS_ISTAT_FLAG_ENUM flags, FILE * hFile, TSK_INUM_T inum,
            TSK_DADDR_T numblock, int32_t sec_skew)
{
    printf("\n%s\n",__FUNCTION__);
    return -1;
}

void hikvision_close(TSK_FS_INFO * fs)
{
    printf("\n%s\n",__FUNCTION__);
    HIKVISION_INFO * hikvision = (HIKVISION_INFO *) fs;

    fs->tag = 0;
    free(hikvision->fs);
    tsk_deinit_lock(&hikvision->lock);
    tsk_fs_free(fs);
    return;
}


TSK_RETVAL_ENUM hikvision_make_root_dir(HIKVISION_INFO * hikvision) {
    TSK_FS_INFO * fs;
    hikvision_hikbtree_pagelist *pagelist = hikvision->pagelist;
    hikvision_hikbtree_page *page = hikvision->page;
    uint8_t inode_cnt = 0, dinode_cnt = 0;
    ssize_t cnt;
    unsigned int len;
    uint64_t hikbtree_next_offset;
    uint64_t inode_offset;
    char buf[255];
 
    fs = &(hikvision->fs_info);

    // hikbtree1 count
    hikbtree_next_offset = hikvision->btree1_first_page;

    while(1) //search next page offset
    {
        dinode_cnt++;
        inode_cnt = 0;
        len = sizeof(hikvision_hikbtree_pagelist);
        if ((hikvision->pagelist = (hikvision_hikbtree_pagelist *) tsk_malloc(len)) == NULL) {
            fs->tag = 0;
            tsk_fs_free((TSK_FS_INFO *)hikvision);
            return TSK_ERR;
        }
        cnt = tsk_fs_read(fs, hikbtree_next_offset, (char *) hikvision->pagelist, len);

        if (cnt != len) {
            if (cnt >= 0) {
                tsk_error_reset();
                tsk_error_set_errno(TSK_ERR_FS_READ);
            }
            tsk_error_set_errstr2("hikvision_open: hikbtree pagelist");
            fs->tag = 0;
            free(hikvision->pagelist);
            tsk_fs_free((TSK_FS_INFO *)hikvision);
            return TSK_ERR;
        }
        while(1) //count total inode num
        {
            inode_offset = hikbtree_next_offset + sizeof(hikvision_hikbtree_pagelist) + HIKVISION_FIRSTDATA_OFFSET + sizeof(hikvision_hikbtree_page) * inode_cnt; 
            len = sizeof(hikvision_hikbtree_page);
            if ((hikvision->page = (hikvision_hikbtree_page *) tsk_malloc(len)) == NULL) {
                fs->tag = 0;
                tsk_fs_free((TSK_FS_INFO *)hikvision);
                return TSK_ERR;
            }
            cnt = tsk_fs_read(fs, inode_offset, (char *) hikvision->page, len);

            if (cnt != len) {
                if (cnt >= 0) {
                    tsk_error_reset();
                    tsk_error_set_errno(TSK_ERR_FS_READ);
                }
                tsk_error_set_errstr2("hikvision_open: hikbtree pagelist");
                fs->tag = 0;
                free(hikvision->page);
                tsk_fs_free((TSK_FS_INFO *)hikvision);
                return TSK_ERR;
            }
            if ((tsk_getu64(fs->endian, hikvision->page->page_empty1) == HIKVISION_RECORD_OFF) && (tsk_getu64(fs->endian, hikvision->page->page_status) == HIKVISION_RECORD_ON))
                inode_cnt++;
            else
                break;
        }

        hikbtree_next_offset = tsk_getu64(fs->endian, hikvision->pagelist->pagelist_next_offset);
        hikvision->total_inode_num = hikvision->total_inode_num + inode_cnt;
        if (hikbtree_next_offset == HIKVISION_RECORD_OFF) 
            break;
    }

   // hikbtree2 count
    hikbtree_next_offset = hikvision->btree2_first_page;

    while(1) //search next page offset
    {
        dinode_cnt++;
        inode_cnt = 0;
        len = sizeof(hikvision_hikbtree_pagelist);
        if ((hikvision->pagelist = (hikvision_hikbtree_pagelist *) tsk_malloc(len)) == NULL) {
            fs->tag = 0;
            tsk_fs_free((TSK_FS_INFO *)hikvision);
            return TSK_ERR;
        }
        cnt = tsk_fs_read(fs, hikbtree_next_offset, (char *) hikvision->pagelist, len);

        if (cnt != len) {
            if (cnt >= 0) {
                tsk_error_reset();
                tsk_error_set_errno(TSK_ERR_FS_READ);
            }
            tsk_error_set_errstr2("hikvision_open: hikbtree pagelist");
            fs->tag = 0;
            free(hikvision->pagelist);
            tsk_fs_free((TSK_FS_INFO *)hikvision);
            return TSK_ERR;
        }
        while(1) //count total inode num
        {
            inode_offset = hikbtree_next_offset + sizeof(hikvision_hikbtree_pagelist) + HIKVISION_FIRSTDATA_OFFSET + sizeof(hikvision_hikbtree_page) * inode_cnt; 
            len = sizeof(hikvision_hikbtree_page);
            if ((hikvision->page = (hikvision_hikbtree_page *) tsk_malloc(len)) == NULL) {
                fs->tag = 0;
                tsk_fs_free((TSK_FS_INFO *)hikvision);
                return TSK_ERR;
            }
            cnt = tsk_fs_read(fs, inode_offset, (char *) hikvision->page, len);

            if (cnt != len) {
                if (cnt >= 0) {
                    tsk_error_reset();
                    tsk_error_set_errno(TSK_ERR_FS_READ);
                }
                tsk_error_set_errstr2("hikvision_open: hikbtree pagelist");
                fs->tag = 0;
                free(hikvision->page);
                tsk_fs_free((TSK_FS_INFO *)hikvision);
                return TSK_ERR;
            }

            if ((tsk_getu64(fs->endian, hikvision->page->page_empty1) == HIKVISION_RECORD_OFF) && (tsk_getu64(fs->endian, hikvision->page->page_status) == HIKVISION_RECORD_ON))
                inode_cnt++;
            else
                break;
        }
        hikbtree_next_offset = tsk_getu64(fs->endian, hikvision->pagelist->pagelist_next_offset);
        hikvision->total_inode_num = hikvision->total_inode_num + inode_cnt;
        if (hikbtree_next_offset == HIKVISION_RECORD_OFF) 
            break;
    }
    hikvision->total_dinode_num = dinode_cnt;
    hikvision->total_inode_num = hikvision->total_inode_num + hikvision->total_dinode_num;

    hikvision_u32toArray(1024, hikvision->inodes[hikvision->current_inode].i_size);
    hikvision_u16toArray(HIKVISION_IN_DIR, hikvision->inodes[hikvision->current_inode].i_mode);
    hikvision_u64toArray(0, hikvision->inodes[hikvision->current_inode].i_block);

    hikvision->current_inode = hikvision->current_inode + 1;
    return TSK_OK;
}

TSK_RETVAL_ENUM hikvision_make_inode(HIKVISION_INFO * hikvision){
    TSK_FS_INFO * fs;
    hikvision_hikbtree_pagelist *pagelist = hikvision->pagelist;
    hikvision_hikbtree_page *page = hikvision->page;
    ssize_t cnt;
    unsigned int len;
    uint64_t hikbtree_next_offset;
    uint64_t inode_offset;
    uint8_t inode_cnt = 0;
    char buf[255];
    uint8_t root_dir;
    uint8_t parent_dir;
    int temp;

    fs = &(hikvision->fs_info);

    // hikbtree1 data inode make
    hikbtree_next_offset = hikvision->btree1_first_page;

    root_dir = 0;
    temp = 0;
    while(1) //fii sub dir inode
    {
        inode_cnt = 0;
        len = sizeof(hikvision_hikbtree_pagelist);
        if ((hikvision->pagelist = (hikvision_hikbtree_pagelist *) tsk_malloc(len)) == NULL) {
            fs->tag = 0;
            tsk_fs_free((TSK_FS_INFO *)hikvision);
            return TSK_ERR;
        }
        cnt = tsk_fs_read(fs, hikbtree_next_offset, (char *) hikvision->pagelist, len);

        if (cnt != len) {
            if (cnt >= 0) {
                tsk_error_reset();
                tsk_error_set_errno(TSK_ERR_FS_READ);
            }
            tsk_error_set_errstr2("hikvision_open: hikbtree pagelist");
            fs->tag = 0;
            free(hikvision->pagelist);
            tsk_fs_free((TSK_FS_INFO *)hikvision);
            return TSK_ERR;
        }

        hikvision_u32toArray(1024, hikvision->inodes[hikvision->current_inode].i_size);
        hikvision_u16toArray(HIKVISION_IN_DIR, hikvision->inodes[hikvision->current_inode].i_mode);
        hikvision_u32toArray(hikvision->current_inode, hikvision->dent[root_dir][temp].inode);
        sprintf(buf, "%d", hikvision->current_inode);
        hikvision_u16toArray(strlen(buf), hikvision->dent[root_dir][temp].name_len);
        strcpy(hikvision->dent[root_dir][temp].name, buf);

        parent_dir = hikvision->current_inode;
        hikvision->current_inode = hikvision->current_inode + 1;
        temp = temp + 1;
        while(1) //fill data inode
        {
            inode_offset = hikbtree_next_offset + sizeof(hikvision_hikbtree_pagelist) + HIKVISION_FIRSTDATA_OFFSET + sizeof(hikvision_hikbtree_page) * inode_cnt; 
            len = sizeof(hikvision_hikbtree_page);
            if ((hikvision->page = (hikvision_hikbtree_page *) tsk_malloc(len)) == NULL) {
                fs->tag = 0;
                tsk_fs_free((TSK_FS_INFO *)hikvision);
                return TSK_ERR;
            }
            cnt = tsk_fs_read(fs, inode_offset, (char *) hikvision->page, len);

            if (cnt != len) {
                if (cnt >= 0) {
                    tsk_error_reset();
                    tsk_error_set_errno(TSK_ERR_FS_READ);
                }
                tsk_error_set_errstr2("hikvision_open: hikbtree pagelist");
                fs->tag = 0;
                free(hikvision->page);
                tsk_fs_free((TSK_FS_INFO *)hikvision);
                return TSK_ERR;
            }
            if ((tsk_getu64(fs->endian, hikvision->page->page_empty1) == HIKVISION_RECORD_OFF) && (tsk_getu64(fs->endian, hikvision->page->page_status) == HIKVISION_RECORD_ON)) {
                hikvision_u32toArray(1024, hikvision->inodes[hikvision->current_inode].i_size);
                hikvision_u16toArray(HIKVISION_IN_REG, hikvision->inodes[hikvision->current_inode].i_mode);
                memcpy(hikvision->inodes[hikvision->current_inode].i_block, hikvision->page->page_datablock, 0x08);
                hikvision_u32toArray(hikvision->current_inode, hikvision->dent[parent_dir][inode_cnt].inode);
                sprintf(buf, "%d", hikvision->current_inode);
                hikvision_u16toArray(strlen(buf), hikvision->dent[parent_dir][inode_cnt].name_len);
                strcpy(hikvision->dent[parent_dir][inode_cnt].name, buf);

                hikvision->current_inode = hikvision->current_inode + 1;
                inode_cnt++;
            }
            else { 
                hikvision->num_entry[parent_dir] = inode_cnt;
                break;
            }
        }

        hikbtree_next_offset = tsk_getu64(fs->endian, hikvision->pagelist->pagelist_next_offset);

        if (hikbtree_next_offset == HIKVISION_RECORD_OFF) 
            break;
    }
  
    // hikbtree2 data inode make
    hikbtree_next_offset = hikvision->btree2_first_page;

    while(1) //fii sub dir inode
    {
        inode_cnt = 0;
        len = sizeof(hikvision_hikbtree_pagelist);
        if ((hikvision->pagelist = (hikvision_hikbtree_pagelist *) tsk_malloc(len)) == NULL) {
            fs->tag = 0;
            tsk_fs_free((TSK_FS_INFO *)hikvision);
            return TSK_ERR;
        }
        cnt = tsk_fs_read(fs, hikbtree_next_offset, (char *) hikvision->pagelist, len);

        if (cnt != len) {
            if (cnt >= 0) {
                tsk_error_reset();
                tsk_error_set_errno(TSK_ERR_FS_READ);
            }
            tsk_error_set_errstr2("hikvision_open: hikbtree pagelist");
            fs->tag = 0;
            free(hikvision->pagelist);
            tsk_fs_free((TSK_FS_INFO *)hikvision);
            return TSK_ERR;
        }

        hikvision_u32toArray(1024, hikvision->inodes[hikvision->current_inode].i_size);
        hikvision_u16toArray(HIKVISION_IN_DIR, hikvision->inodes[hikvision->current_inode].i_mode);
        hikvision_u32toArray(hikvision->current_inode, hikvision->dent[root_dir][temp].inode);
        sprintf(buf, "%d", hikvision->current_inode);
        hikvision_u16toArray(strlen(buf), hikvision->dent[root_dir][temp].name_len);
        strcpy(hikvision->dent[root_dir][temp].name, buf);

        parent_dir = hikvision->current_inode;
        hikvision->current_inode = hikvision->current_inode + 1;
        temp = temp + 1;
        while(1) //fill data inode
        {
            inode_offset = hikbtree_next_offset + sizeof(hikvision_hikbtree_pagelist) + HIKVISION_FIRSTDATA_OFFSET + sizeof(hikvision_hikbtree_page) * inode_cnt; 
            len = sizeof(hikvision_hikbtree_page);
            if ((hikvision->page = (hikvision_hikbtree_page *) tsk_malloc(len)) == NULL) {
                fs->tag = 0;
                tsk_fs_free((TSK_FS_INFO *)hikvision);
                return TSK_ERR;
            }
            cnt = tsk_fs_read(fs, inode_offset, (char *) hikvision->page, len);

            if (cnt != len) {
                if (cnt >= 0) {
                    tsk_error_reset();
                    tsk_error_set_errno(TSK_ERR_FS_READ);
                }
                tsk_error_set_errstr2("hikvision_open: hikbtree pagelist");
                fs->tag = 0;
                free(hikvision->page);
                tsk_fs_free((TSK_FS_INFO *)hikvision);
                return TSK_ERR;
            }
            if ((tsk_getu64(fs->endian, hikvision->page->page_empty1) == HIKVISION_RECORD_OFF) && (tsk_getu64(fs->endian, hikvision->page->page_status) == HIKVISION_RECORD_ON)) {
                hikvision_u32toArray(1024, hikvision->inodes[hikvision->current_inode].i_size);
                hikvision_u16toArray(HIKVISION_IN_REG, hikvision->inodes[hikvision->current_inode].i_mode);
                memcpy(hikvision->inodes[hikvision->current_inode].i_block, hikvision->page->page_datablock, 0x08);
                hikvision_u32toArray(hikvision->current_inode, hikvision->dent[parent_dir][inode_cnt].inode);
                sprintf(buf, "%d", hikvision->current_inode);
                hikvision_u16toArray(strlen(buf), hikvision->dent[parent_dir][inode_cnt].name_len);
                strcpy(hikvision->dent[parent_dir][inode_cnt].name, buf);
                hikvision->current_inode = hikvision->current_inode + 1;
                inode_cnt++;
            }
            else { 
                hikvision->num_entry[parent_dir] = inode_cnt;
                break;
            }
        }
        hikbtree_next_offset = tsk_getu64(fs->endian, hikvision->pagelist->pagelist_next_offset);
        if (hikbtree_next_offset == HIKVISION_RECORD_OFF) 
            break;
    }
    hikvision->num_entry[root_dir] = hikvision->total_dinode_num;

    return TSK_OK;
}


TSK_FS_INFO *
hikvision_open(TSK_IMG_INFO * img_info, TSK_OFF_T offset,
	TSK_FS_TYPE_ENUM ftype, uint8_t test)
{
    HIKVISION_INFO *hikvision;
    hikvision_masterheader *mh = hikvision->fs;
    hikvision_meta *meta = hikvision->meta;    
    hikvision_hikbtree_pagelist *pagelist = hikvision->pagelist;
    hikvision_hikbtree_header *hh = hikvision->hikbtree_hd;
    unsigned int len;
    TSK_FS_INFO *fs;
    ssize_t cnt;
    
    // clean up any error messages that are lying around
    tsk_error_reset();
    if (TSK_FS_TYPE_ISHIKVISION(ftype) == 0) {
        tsk_error_reset();
        tsk_error_set_errno(TSK_ERR_FS_ARG);
        tsk_error_set_errstr("Invalid FS Type in hikvision_open");
        return NULL;
    }
    if (img_info->sector_size == 0) {
        tsk_error_reset();
        tsk_error_set_errno(TSK_ERR_FS_ARG);
        tsk_error_set_errstr("hikvision_open: sector size is 0");
        return NULL;
    }
    if ((hikvision = (HIKVISION_INFO *) tsk_fs_malloc(sizeof(*hikvision))) == NULL)
        return NULL;

    fs = &(hikvision->fs_info);

    fs->ftype = ftype;
    fs->flags = 0;
    fs->img_info = img_info;
    fs->offset = offset;
    fs->tag = TSK_FS_INFO_TAG;
    
    //Read the masterheader.
     
    len = sizeof(hikvision_masterheader);

    if ((hikvision->fs = (hikvision_masterheader *) tsk_malloc(len)) == NULL) {
        fs->tag = 0;
        tsk_fs_free((TSK_FS_INFO *)hikvision);
        return NULL;
    }

    cnt = tsk_fs_read(fs, HIKVISION_MASTERHEADEROFF, (char *) hikvision->fs, len);

    if (cnt != len) {
        if (cnt >= 0) {
            tsk_error_reset();
            tsk_error_set_errno(TSK_ERR_FS_READ);
        }
        tsk_error_set_errstr2("hikvision_open: master header");
        fs->tag = 0;
        free(hikvision->fs);
        tsk_fs_free((TSK_FS_INFO *)hikvision);
        return NULL;
    }
    if(tsk_fs_guessu64(fs, hikvision->fs->mh_magicnum1, HIKVISION_FS_MAGIC1) || tsk_fs_guessu64(fs, hikvision->fs->mh_magicnum2, HIKVISION_FS_MAGIC2) 
		|| tsk_fs_guessu64(fs, hikvision->fs->mh_magicnum3, HIKVISION_FS_MAGIC3) || tsk_fs_guessu64(fs, hikvision->fs->mh_magicnum4, HIKVISION_FS_MAGIC4)){

		if (tsk_verbose){
            fprintf(stderr, "hikvision_open : superblock magic failed\n");
        }

        fs->tag = 0;
        free(hikvision->fs);
        tsk_fs_free((TSK_FS_INFO *)hikvision);
        tsk_error_reset();
        tsk_error_set_errno(TSK_ERR_FS_MAGIC);
        tsk_error_set_errstr("not an hikvision file system (magic)");
        
        if (tsk_verbose)
            fprintf(stderr, "hikvision_open : invalid magic\n");
        return NULL;
    }

    fs->endian = 0x01;
    fs->inum_count = tsk_getu32(fs->endian, hikvision->fs->mh_datablock_count);
    fs->first_inum = HIKVISION_FIRSTINO;
    fs->root_inum = HIKVISION_ROOTINO;
    hikvision->inode_size = HIKVISION_INOSIZE;
    hikvision->block_size = 0x1000;
    hikvision->meta_offset = tsk_getu64(fs->endian, hikvision->fs->mh_meta_start_offset);
    hikvision->hikbtree_offset1 = tsk_getu64(fs->endian, hikvision->fs->mh_hikbtree_offset1);
    hikvision->hikbtree_offset2 = tsk_getu64(fs->endian, hikvision->fs->mh_hikbtree_offset2);

    //Read the hikvision hikbtree1 header.
    len = sizeof(hikvision_hikbtree_header);
    if ((hikvision->hikbtree_hd = (hikvision_hikbtree_header *) tsk_malloc(len)) == NULL) {
        fs->tag = 0;
        tsk_fs_free((TSK_FS_INFO *)hikvision);
        return NULL;
    }
    cnt = tsk_fs_read(fs, hikvision->hikbtree_offset1, (char *) hikvision->hikbtree_hd, len);

    if (cnt != len) {
        if (cnt >= 0) {
            tsk_error_reset();
            tsk_error_set_errno(TSK_ERR_FS_READ);
        }
        tsk_error_set_errstr2("hikvision_open: hikbtree header");
        fs->tag = 0;
        free(hikvision->hikbtree_hd);
        tsk_fs_free((TSK_FS_INFO *)hikvision);
        return NULL;
    }
    hikvision->btree1_first_page = tsk_getu64(fs->endian, hikvision->hikbtree_hd->first_offset);
    
    //Read the hikvision hikbtree2 header.
    len = sizeof(hikvision_hikbtree_header);
    if ((hikvision->hikbtree_hd = (hikvision_hikbtree_header *) tsk_malloc(len)) == NULL) {
        fs->tag = 0;
        tsk_fs_free((TSK_FS_INFO *)hikvision);
        return NULL;
    }
    cnt = tsk_fs_read(fs, hikvision->hikbtree_offset2, (char *) hikvision->hikbtree_hd, len);

    if (cnt != len) {
        if (cnt >= 0) {
            tsk_error_reset();
            tsk_error_set_errno(TSK_ERR_FS_READ);
        }
        tsk_error_set_errstr2("hikvision_open: hikbtree header");
        fs->tag = 0;
        free(hikvision->hikbtree_hd);
        tsk_fs_free((TSK_FS_INFO *)hikvision);
        return NULL;
    }
    hikvision->btree2_first_page = tsk_getu64(fs->endian, hikvision->hikbtree_hd->first_offset);

    //Read the hikvision hikbtree1 pagelist.
    len = sizeof(hikvision_hikbtree_pagelist);
    if ((hikvision->pagelist = (hikvision_hikbtree_pagelist *) tsk_malloc(len)) == NULL) {
        fs->tag = 0;
        tsk_fs_free((TSK_FS_INFO *)hikvision);
        return NULL;
    }
    cnt = tsk_fs_read(fs, hikvision->btree1_first_page, (char *) hikvision->pagelist, len);

    if (cnt != len) {
        if (cnt >= 0) {
            tsk_error_reset();
            tsk_error_set_errno(TSK_ERR_FS_READ);
        }
        tsk_error_set_errstr2("hikvision_open: hikbtree pagelist");
        fs->tag = 0;
        free(hikvision->pagelist);
        tsk_fs_free((TSK_FS_INFO *)hikvision);
        return NULL;
    }

    hikvision->pagelist1_cnt = tsk_getu64(fs->endian, hikvision->pagelist->pagelist_cnt);

    //Read the hikvision hikbtree2 pagelist.
    len = sizeof(hikvision_hikbtree_pagelist);
    if ((hikvision->pagelist = (hikvision_hikbtree_pagelist *) tsk_malloc(len)) == NULL) {
        fs->tag = 0;
        tsk_fs_free((TSK_FS_INFO *)hikvision);
        return NULL;
    }
    cnt = tsk_fs_read(fs, hikvision->btree2_first_page, (char *) hikvision->pagelist, len);

    if (cnt != len) {
        if (cnt >= 0) {
            tsk_error_reset();
            tsk_error_set_errno(TSK_ERR_FS_READ);
        }
        tsk_error_set_errstr2("hikvision_open: hikbtree pagelist");
        fs->tag = 0;
        free(hikvision->pagelist);
        tsk_fs_free((TSK_FS_INFO *)hikvision);
        return NULL;
    }

    hikvision->pagelist2_cnt = tsk_getu64(fs->endian, hikvision->pagelist->pagelist_cnt);

    //Read the hikvision metadata.
    len = sizeof(hikvision_meta);
    if ((hikvision->meta = (hikvision_meta *) tsk_malloc(len)) == NULL) {
        fs->tag = 0;
        tsk_fs_free((TSK_FS_INFO *)hikvision);
        return NULL;
    }
    cnt = tsk_fs_read(fs, hikvision->meta_offset, (char *) hikvision->meta, len);

    if (cnt != len) {
        if (cnt >= 0) {
            tsk_error_reset();
            tsk_error_set_errno(TSK_ERR_FS_READ);
        }
        tsk_error_set_errstr2("hikvision_open: metadata");
        fs->tag = 0;
        free(hikvision->meta);
        tsk_fs_free((TSK_FS_INFO *)hikvision);
        return NULL;
    }
    hikvision->total_inode_num = 0;
    hikvision->current_inode = 0;
    hikvision_make_root_dir(hikvision);
    hikvision_make_inode(hikvision);

    fs->last_inum = hikvision->total_inode_num;

    /*
     * Calculate the block info
     */
    fs->dev_bsize = img_info->sector_size;
    fs->first_block = 0;
    fs->block_count = (tsk_getu64(fs->endian, hikvision->fs->mh_HardSize)/0x1000);
 
    fs->block_size = 0x1000; //(uint32_t) tsk_getu64(fs->endian, hikvision->fs->mh_datablock_size);
    fs->last_block_act = fs->last_block = fs->block_count - 1;

    fs->inode_walk = hikvision_inode_walk;
    fs->block_walk = hikvision_block_walk;
    fs->block_getflags = hikvision_block_getflags;
    fs->get_default_attr_type = tsk_fs_unix_get_default_attr_type;
    fs->load_attrs = hikvision_load_attrs;
    fs->file_add_meta = hikvision_inode_lookup;
    fs->dir_open_meta = hikvision_dir_open_meta;
    fs->fsstat = hikvision_fsstat;
    fs->fscheck = hikvision_fscheck;
    fs->istat = hikvision_istat;
    fs->name_cmp = tsk_fs_unix_name_cmp;
    fs->close = hikvision_close;
    
    tsk_init_lock(&hikvision->lock);

    return (fs);
}