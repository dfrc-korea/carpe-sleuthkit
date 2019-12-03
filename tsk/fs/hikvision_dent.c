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

/**
 * \file hikvision_dent.c
 * Contains the internal TSK file name processing code for Ext2 / ext3
 */

#include <ctype.h>
#include "tsk_fs_i.h"
#include "tsk_hikvision.h"

static uint8_t
hikvision_dent_copy(HIKVISION_INFO * hikvision,
    hikvision_dent dent, TSK_FS_NAME *fs_name, TSK_FS_FILE *fs_file)
{
    TSK_FS_INFO *fs = &(hikvision->fs_info);
    printf("%s, inum(0x%x)\n",__FUNCTION__, tsk_getu32(fs->endian, dent.inode));
    strncpy(fs_name->name, dent.name, tsk_getu16(fs->endian, dent.name_len));
    fs_name->name[tsk_getu16(fs->endian, dent.name_len)] = '\0';
    fs_name->type = TSK_FS_NAME_TYPE_UNDEF;
    fs_name->meta_addr = (TSK_INUM_T)tsk_getu32(fs->endian, dent.inode);

    if (tsk_getu16(fs->endian, dent.name_len) >= fs_name->name_size){
        tsk_error_reset();
        tsk_error_set_errno(TSK_ERR_FS_ARG);
        tsk_error_set_errstr
            ("HIKVISION_dent_copy: Name Space too Small %d %" PRIuSIZE "",
           dent.name_len, fs_name->name_size);
        return 1;
    }

    switch (tsk_getu16(fs->endian, hikvision->inodes[tsk_getu32(fs->endian,dent.inode)].i_mode)) {
        case HIKVISION_DE_REG:
            fs_name->type = TSK_FS_NAME_TYPE_REG;
            printf("reg\n");
            break;
        case HIKVISION_DE_DIR:
            fs_name->type = TSK_FS_NAME_TYPE_DIR;
            printf("dir\n");
            break;
        case HIKVISION_DE_CHR:
            fs_name->type = TSK_FS_NAME_TYPE_CHR;
            break;
        case HIKVISION_DE_BLK:
            fs_name->type = TSK_FS_NAME_TYPE_BLK;
            break;
        case HIKVISION_DE_FIFO:
            fs_name->type = TSK_FS_NAME_TYPE_FIFO;
            break;
        case HIKVISION_DE_SOCK:
            fs_name->type = TSK_FS_NAME_TYPE_SOCK;
            break;
        case HIKVISION_DE_LNK:
            fs_name->type = TSK_FS_NAME_TYPE_LNK;
            break;
        case HIKVISION_DE_UNKNOWN:
        default:
            fs_name->type = TSK_FS_NAME_TYPE_UNDEF;
            printf("und\n");
            break;
    }
    fs_name->flags = 0;

    return 0;
}


static TSK_RETVAL_ENUM
hikvision_dent_parse(HIKVISION_INFO * hikvision, TSK_FS_DIR * a_fs_dir, uint8_t a_is_del, TSK_LIST ** list_seen, hikvision_dent* dent, TSK_INUM_T inum)
{
    TSK_FS_INFO *fs = &(hikvision->fs_info);
    
    TSK_FS_NAME *fs_name;
    TSK_FS_FILE *fs_file = a_fs_dir->fs_file;
    
    
    //uint8_t ftype;
    uint64_t i;

    if ((fs_name = tsk_fs_name_alloc(HIKVISION_MAXNAMELEN + 1, 0)) == NULL)
        return TSK_ERR;

    for (i = 0; i < hikvision->num_entry[inum]; i++)
    {
        uint8_t namelen;
        uint32_t inode;
        char* name;
        
        namelen = tsk_getu16(fs->endian, dent[i].name_len);
        inode = tsk_getu32(fs->endian, dent[i].inode);
        printf("%s, inum(0x%x)\n",__FUNCTION__, inode);
        name = (char*)tsk_malloc(sizeof(char) * (namelen + 1));
        name[namelen] = '\0';

        memcpy(name, dent[i].name, namelen);
        printf("ino(%d), name(%s)\n", inode, name);        
        if (inode > fs->last_inum || namelen > HIKVISION_MAXNAMELEN || namelen == 0) {
            break;
        }

        if (hikvision_dent_copy(hikvision, dent[i], fs_name, fs_file)) {
            tsk_fs_name_free(fs_name);
            return TSK_ERR;
        }

        fs_name->flags = TSK_FS_NAME_FLAG_ALLOC;

        if (tsk_fs_dir_add(a_fs_dir, fs_name)) {
            tsk_fs_name_free(fs_name);
            return TSK_ERR;
        }


    }

    tsk_fs_name_free(fs_name);
    return TSK_OK;
}


TSK_RETVAL_ENUM 
hikvision_dir_open_meta(TSK_FS_INFO * a_fs, TSK_FS_DIR ** a_fs_dir,
    TSK_INUM_T a_addr)
{
    HIKVISION_INFO * hikvision = (HIKVISION_INFO *) a_fs;
    TSK_FS_DIR * fs_dir;
    TSK_LIST *list_seen = NULL;
    TSK_OFF_T size;

    //printf("%s\n",__FUNCTION__);
    
    TSK_RETVAL_ENUM retval_tmp;
    TSK_RETVAL_ENUM retval_final = TSK_OK;

    if (a_addr < a_fs->first_inum || a_addr > a_fs->last_inum) {
        tsk_error_reset();
        tsk_error_set_errno(TSK_ERR_FS_WALK_RNG);
        tsk_error_set_errstr("hikvision_dir_open_meta: inode value: %" PRIuINUM
            "\n", a_addr);
        return TSK_ERR;
    }
    else if (a_fs_dir == NULL) {
        tsk_error_reset();
        tsk_error_set_errno(TSK_ERR_FS_ARG);
        tsk_error_set_errstr
            ("hikvision_dir_open_meta: NULL fs_attr argument given");
        return TSK_ERR;
    }

    if (tsk_verbose) {
        tsk_fprintf(stderr,
            "hikvision_dir_open_meta: Processing directory %" PRIuINUM
            "\n", a_addr);
    }

    fs_dir = *a_fs_dir;

    if (fs_dir) {
        tsk_fs_dir_reset(fs_dir);
        fs_dir->addr = a_addr;
    }
    else {
        if((*a_fs_dir = fs_dir =
                tsk_fs_dir_alloc(a_fs, a_addr, 128)) == NULL) {
            return TSK_ERR;
        }
    }

    //아래부터 수정 필요
    //printf("addr(0x%x)\n\n", a_addr);

    if ((fs_dir->fs_file =
        tsk_fs_file_open_meta(a_fs, NULL, a_addr)) == NULL) { // inode_lookup -> content_ptr 채움
        fprintf(stderr, "hikvision_fs_dir_open_meta: failed to obtain fs_file meta info\n");
        tsk_error_reset();
        tsk_error_errstr2_concat("- hikvision_dir_open_meta");
        return TSK_COR;
    }

    retval_tmp =
        hikvision_dent_parse(hikvision, fs_dir, (fs_dir->fs_file->meta->
            flags & TSK_FS_META_FLAG_UNALLOC) ? 1 : 0, &list_seen,
        hikvision->dent[a_addr], a_addr);

        // if we are listing the root directory, add the Orphan directory entry
    if (a_addr == a_fs->root_inum) {
        TSK_FS_NAME *fs_name = tsk_fs_name_alloc(256, 0);
        if (fs_name == NULL)
            return TSK_ERR;

        if (tsk_fs_dir_make_orphan_dir_name(a_fs, fs_name)) {
            tsk_fs_name_free(fs_name);
            return TSK_ERR;
        }

        if (tsk_fs_dir_add(fs_dir, fs_name)) {
            tsk_fs_name_free(fs_name);
            return TSK_ERR;
        }
        tsk_fs_name_free(fs_name);
    }

    return retval_final;
}

