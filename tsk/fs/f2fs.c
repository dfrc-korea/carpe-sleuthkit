#include "tsk_fs_i.h"
#include "tsk_f2fs.h"
#include <stdbool.h>

static uint8_t
f2fs_dinode_load(F2FS_INFO * f2fs, TSK_INUM_T dino_inum,
    f2fs_inode * dino_buf)
{
    int nat_inum;
    TSK_OFF_T addr;
    ssize_t cnt;
    TSK_INUM_T rel_inum;
    TSK_FS_INFO *fs = (TSK_FS_INFO *) & f2fs->fs_info;

    if (dino_buf == NULL) {
    tsk_error_reset();
    tsk_error_set_errno(TSK_ERR_FS_ARG);
    tsk_error_set_errstr("f2fs_dinode_load: dino_buf is NULL");
    return 1;
    }
    
    int i = 0;
    while (1) {
        rel_inum = (TSK_INUM_T)tsk_getu32(fs->endian, f2fs->fs_nat->entries[i].ino);
			
        if (rel_inum >= dino_inum)
            break;
        i++;
    }

    if ((addr = (TSK_OFF_T) (tsk_getu32(fs->endian, f2fs->fs_nat->entries[i].block_addr))) != 0)
    {
        addr *= F2FS_BLKSIZE;
        cnt = tsk_fs_read(fs, addr, (char *) dino_buf, f2fs->inode_size);
        if (cnt != f2fs->inode_size) {   
            if (cnt >= 0) {
                tsk_error_reset();
                tsk_error_set_errno(TSK_ERR_FS_READ);
            }
            tsk_error_set_errstr2("f2fs_dinode_load: Inode %" PRIuINUM " from %" PRIdOFF, dino_inum, addr);
            return 1;
        }
    }
    return 0;
    

}


static uint8_t
parse_dr_node(F2FS_INFO * f2fs, uint32_t addr, uint32_t *addr_ptr, int *itr) {
    direct_node *dr_node;
    TSK_FS_INFO *fs = (TSK_FS_INFO *) &f2fs->fs_info;
    
    if ((dr_node = (direct_node *)tsk_malloc(sizeof(direct_node))) == NULL) {
        return 1;
    }

    int cnt = tsk_fs_read(fs, addr * fs->block_size, (char*)dr_node, f2fs->inode_size);
    if (cnt != f2fs->inode_size) {
        if (cnt > 0) {
            tsk_error_reset();
            tsk_error_set_errno(TSK_ERR_FS_READ);
        }
        free(dr_node);
        tsk_error_set_errstr2("f2fs_dinode_load: Inode %" PRIuINUM " from %" PRIdOFF, -1, addr);
        return 1;
    }
    
    int i;
    for (i = 0; i < DEF_ADDRS_PER_BLOCK; i++)
    {
        uint32_t tmp = tsk_getu32(fs->endian, dr_node->addr[i]);
        if (tsk_getu32(fs->endian, dr_node->addr[i]))
        {
            addr_ptr[*itr] = tsk_getu32(fs->endian, dr_node->addr[i]);
            *itr += 1;
        } else
            break;
    }
    
    free(dr_node);
    
    return 0;
}

static uint8_t
parse_idr_node(F2FS_INFO * f2fs, uint32_t addr, uint32_t *addr_ptr, int *itr, Boolean bisD) {
    TSK_FS_INFO *fs = (TSK_FS_INFO*)&f2fs->fs_info;

    if (bisD) {
        parse_idr_node(f2fs, addr, addr_ptr, itr, bisD);
    } else {
        indirect_node *idr_node;
        if ((idr_node = (indirect_node *)tsk_malloc(sizeof(indirect_node))) == NULL) {
            return 1;
        }
        int cnt = tsk_fs_read(fs, addr * fs->block_size, (char *) idr_node, f2fs->inode_size);
        if (cnt != f2fs->inode_size) {
            if (cnt >= 0) {
                tsk_error_reset();
                tsk_error_set_errno(TSK_ERR_FS_READ);
            }
            free(idr_node);
            tsk_error_set_errstr2("f2fs_dinode_load: Inode %" PRIuINUM " from %" PRIdOFF, -1, addr);
            return 1;
        }

        int i;
        for (i = 0; i < DEF_ADDRS_PER_BLOCK; i++)
        {
            if (tsk_getu32(fs->endian, idr_node->nid[i])) {
                uint32_t tmpaddr = tsk_getu32(fs->endian, idr_node->nid[i]);
                tmpaddr = tsk_getu32(fs->endian, f2fs->fs_nat->entries[tmpaddr].block_addr);
                parse_dr_node(f2fs, tmpaddr, addr_ptr, itr);
            } else
                break;
        }
        free(idr_node);
    }

    return 0;
}


static uint8_t
f2fs_dinode_copy(F2FS_INFO * f2fs, TSK_FS_FILE * fs_file,
    TSK_INUM_T inum, const f2fs_inode * dino_buf)
{
	TSK_FS_META * fs_meta;
	fs_meta = fs_file->meta;
    int i;
    int ptr_in_ino;
    TSK_FS_INFO *fs = (TSK_FS_INFO *) & f2fs->fs_info;
    f2fs_sb *sb = f2fs->fs;
    if (dino_buf == NULL) {
        tsk_error_reset();
        tsk_error_set_errno(TSK_ERR_FS_ARG);
        tsk_error_set_errstr("f2fs_dinode_copy: dino_buf is NULL");
        return 1;
    }
    fs_meta->attr_state = TSK_FS_META_ATTR_EMPTY;
    if (fs_meta->attr) {
        tsk_fs_attrlist_markunused(fs_meta->attr);
    }
    // set the type

    switch (tsk_getu16(fs->endian, dino_buf->i_mode) & F2FS_IN_FMT) {
    case F2FS_IN_REG:
        fs_meta->type = TSK_FS_META_TYPE_REG;
        break;
    case F2FS_IN_DIR:
        fs_meta->type = TSK_FS_META_TYPE_DIR;
        break;
    case F2FS_IN_SOCK:
        fs_meta->type = TSK_FS_META_TYPE_SOCK;
        break;
    case F2FS_IN_LNK:
        fs_meta->type = TSK_FS_META_TYPE_LNK;
        break;
    case F2FS_IN_BLK:
        fs_meta->type = TSK_FS_META_TYPE_BLK;
        break;
    case F2FS_IN_CHR:
        fs_meta->type = TSK_FS_META_TYPE_CHR;
        break;
    case F2FS_IN_FIFO:
        fs_meta->type = TSK_FS_META_TYPE_FIFO;
        break;
    default:
        fs_meta->type = TSK_FS_META_TYPE_UNDEF;
        break;
    }
    // set the mode

    fs_meta->mode = 0;
    if (tsk_getu16(fs->endian, dino_buf->i_mode) & F2FS_IN_ISUID)
        fs_meta->mode |= TSK_FS_META_MODE_ISUID;
    if (tsk_getu16(fs->endian, dino_buf->i_mode) & F2FS_IN_ISGID)
        fs_meta->mode |= TSK_FS_META_MODE_ISGID;
    if (tsk_getu16(fs->endian, dino_buf->i_mode) & F2FS_IN_ISVTX)
        fs_meta->mode |= TSK_FS_META_MODE_ISVTX;

    if (tsk_getu16(fs->endian, dino_buf->i_mode) & F2FS_IN_IRUSR)
        fs_meta->mode |= TSK_FS_META_MODE_IRUSR;
    if (tsk_getu16(fs->endian, dino_buf->i_mode) & F2FS_IN_IWUSR)
        fs_meta->mode |= TSK_FS_META_MODE_IWUSR;
    if (tsk_getu16(fs->endian, dino_buf->i_mode) & F2FS_IN_IXUSR)
        fs_meta->mode |= TSK_FS_META_MODE_IXUSR;

    if (tsk_getu16(fs->endian, dino_buf->i_mode) & F2FS_IN_IRGRP)
        fs_meta->mode |= TSK_FS_META_MODE_IRGRP;
    if (tsk_getu16(fs->endian, dino_buf->i_mode) & F2FS_IN_IWGRP)
        fs_meta->mode |= TSK_FS_META_MODE_IWGRP;
    if (tsk_getu16(fs->endian, dino_buf->i_mode) & F2FS_IN_IXGRP)
        fs_meta->mode |= TSK_FS_META_MODE_IXGRP;

    if (tsk_getu16(fs->endian, dino_buf->i_mode) & F2FS_IN_IROTH)
        fs_meta->mode |= TSK_FS_META_MODE_IROTH;
    if (tsk_getu16(fs->endian, dino_buf->i_mode) & F2FS_IN_IWOTH)
        fs_meta->mode |= TSK_FS_META_MODE_IWOTH;
    if (tsk_getu16(fs->endian, dino_buf->i_mode) & F2FS_IN_IXOTH)
        fs_meta->mode |= TSK_FS_META_MODE_IXOTH;

    fs_meta->nlink = tsk_getu32(fs->endian, dino_buf->i_links);
    
    fs_meta->addr = inum;
    fs_meta->uid =tsk_getu32(fs->endian, dino_buf->i_uid);
    fs_meta->gid =tsk_getu32(fs->endian, dino_buf->i_gid);
    fs_meta->mtime = tsk_getu64(fs->endian, dino_buf->i_mtime);
    fs_meta->atime = tsk_getu64(fs->endian, dino_buf->i_atime);
    fs_meta->ctime = tsk_getu64(fs->endian, dino_buf->i_ctime);

    fs_meta->mtime_nano = tsk_getu32(fs->endian, dino_buf->i_mtime_nsec) >> 2;
    fs_meta->atime_nano = tsk_getu32(fs->endian, dino_buf->i_atime_nsec) >> 2;
    fs_meta->ctime_nano = tsk_getu32(fs->endian, dino_buf->i_ctime_nsec) >> 2;
    fs_meta->crtime = 0;
    fs_meta->crtime_nano = 0;
    fs_meta->seq = 0;

	fs_meta->size = tsk_getu64(fs->endian, dino_buf->i_size);
  
    if (fs_meta->link) {
        free(fs_meta->link);
        fs_meta->link = NULL;
    }
	fs_meta->inline_content = dino_buf->i_inline[0];
 
	if (fs_meta->inline_content == 0x0b)
	{
		uint8_t * buf;
		buf = (uint8_t *)fs_meta->content_ptr;
	
		memset(buf, 0, tsk_gets64(fs->endian, dino_buf->i_size));
		for (i = 0; i < tsk_gets64(fs->endian, dino_buf->i_size); i++)
		{
			buf[i] = (uint8_t)dino_buf->i_reldata[i+4];
		}
	
	}
	else if (fs_meta->type != TSK_FS_META_TYPE_DIR)
	{
        int itr = 2;
        uint32_t *addr_ptr;
        unsigned *buf;
        direct_node * a;
        addr_ptr = (uint32_t *) fs_meta->content_ptr;
        addr_ptr[0] = tsk_gets32(fs->endian, dino_buf->i_ext.blk);
        addr_ptr[1] = tsk_gets32(fs->endian, dino_buf->i_ext.len);
      
        for (i = 0; i < DEF_ADDRS_PER_INODE; i++)
        {
            if (tsk_getu32(fs->endian, dino_buf->i_addr[i]))
            {
                addr_ptr[itr] = tsk_getu32(fs->endian, dino_buf->i_addr[i]);
                itr++;
            } else
            {
                break;
            }
            
        }

        int adrcnt;        
        for (adrcnt = 0; adrcnt < 5; adrcnt++) {
            uint32_t tmp = tsk_getu32(fs->endian, dino_buf->i_nid.addr[adrcnt]);
            if (tmp == 0)
                break;
        }

        for (i = 0; i < adrcnt; i++) {
            uint32_t tmp = tsk_getu32(fs->endian, dino_buf->i_nid.addr[i]);
            uint32_t addr = tsk_getu32(fs->endian, f2fs->fs_nat->entries[tmp].block_addr);
            if (i < 2) { // dr
                parse_dr_node(f2fs, addr, addr_ptr, &itr);
            }
            else if (i < 4) { // idr
                parse_idr_node(f2fs, addr, addr_ptr, &itr, 0);
            } else { // ddr
//                parse_idr_node(f2fs, addr, addr_ptr, &itr, 1);
            }
            
        }

        fs_meta->content_len = itr;
    }	
    else
    {
        uint8_t * buf;

		buf = (uint8_t *) fs_meta->content_ptr;
        for (i = 0; i < 180; i++)
        {
            buf[i*11] = (uint8_t)dino_buf->i_dentry.dentry[i].hash_code[0];
            buf[(i*11)+1] = (uint8_t)dino_buf->i_dentry.dentry[i].hash_code[1];
            buf[(i*11)+2] = (uint8_t)dino_buf->i_dentry.dentry[i].hash_code[2];
            buf[(i*11)+3] = (uint8_t)dino_buf->i_dentry.dentry[i].hash_code[3];
            buf[(i*11)+4] = (uint8_t)dino_buf->i_dentry.dentry[i].ino[0];
            buf[(i*11)+5] = (uint8_t)dino_buf->i_dentry.dentry[i].ino[1];
            buf[(i*11)+6] = (uint8_t)dino_buf->i_dentry.dentry[i].ino[2];
            buf[(i*11)+7] = (uint8_t)dino_buf->i_dentry.dentry[i].ino[3];
            buf[(i*11)+8] = (uint8_t)dino_buf->i_dentry.dentry[i].name_len[0];
            buf[(i*11)+9] = (uint8_t)dino_buf->i_dentry.dentry[i].name_len[1];
            buf[(i*11)+10] = dino_buf->i_dentry.dentry[i].file_type;
        }
        i = F2FS_DENTRY_MAX;
        for (int itr_num = 0; itr_num <F2FS_DENTRY_FN_ARY ; itr_num++)
        {
            for (int n = 0; n < 8; n++)
                buf[i + itr_num*8 + n] = (uint8_t)dino_buf->i_dentry.filename[itr_num][n];
        }
    }
    fs_meta->flags |= (fs_meta->ctime ?
        TSK_FS_META_FLAG_USED : TSK_FS_META_FLAG_UNUSED);
    return 0;    
}

static uint8_t
f2fs_inode_lookup(TSK_FS_INFO * fs, TSK_FS_FILE * a_fs_file,
    TSK_INUM_T inum)
{
    F2FS_INFO *f2fs = (F2FS_INFO *) fs;
    f2fs_inode *dino_buf = NULL;
    unsigned int size = 0;

	if (a_fs_file == NULL) {
        tsk_error_set_errno(TSK_ERR_FS_ARG);
        tsk_error_set_errstr("f2fs_inode_lookup: fs_file is NULL");
        return 1;
    }
    if (a_fs_file->meta == NULL) {
        if ((a_fs_file->meta =
                tsk_fs_meta_alloc((DEF_ADDRS_PER_BLOCK) * 100)) == NULL)
            return 1;
    }
    else {
        tsk_fs_meta_reset(a_fs_file->meta);
    }
    // see if they are looking for the special "orphans" directory
    
    size =
        f2fs->inode_size >
        sizeof(f2fs_inode) ? f2fs->inode_size : sizeof(f2fs_inode);
    
    if ((dino_buf = (f2fs_inode *) tsk_malloc(size)) == NULL) {
        return 1;
    }

	if (f2fs_dinode_load(f2fs, inum, dino_buf)) {
        free(dino_buf);
        return 1;
    }
	if (f2fs_dinode_copy(f2fs, a_fs_file,inum, dino_buf)) {
        free(dino_buf);
        return 1;
    }
	free(dino_buf);

    return 0;
}

uint8_t f2fs_inode_walk(TSK_FS_INFO * fs, TSK_INUM_T start_inum,
    TSK_INUM_T end_inum, TSK_FS_META_FLAG_ENUM flags,
    TSK_FS_META_WALK_CB a_action, void *a_ptr)
{
    char *myname = "f2fs_inode_walk";
    F2FS_INFO *ext2fs = (F2FS_INFO *) fs;
    TSK_INUM_T inum;
    TSK_INUM_T end_inum_tmp;
    TSK_INUM_T ibase = 0;
    TSK_FS_FILE *fs_file;
    unsigned int myflags;
    f2fs_inode *dino_buf = NULL;
    unsigned int size = 0;

    // clean up any error messages that are lying around
    tsk_error_reset();

    /*
     * Sanity checks.
     */
    if (start_inum < fs->first_inum || start_inum > fs->last_inum) {
        tsk_error_reset();
        tsk_error_set_errno(TSK_ERR_FS_WALK_RNG);
        tsk_error_set_errstr("%s: start inode: %" PRIuINUM "", myname,
            start_inum);
        return 1;
    }

    if (end_inum < fs->first_inum || end_inum > fs->last_inum
        || end_inum < start_inum) {
        tsk_error_reset();
        tsk_error_set_errno(TSK_ERR_FS_WALK_RNG);
        tsk_error_set_errstr("%s: end inode: %" PRIuINUM "", myname,
            end_inum);
        return 1;
    }    
}

uint8_t f2fs_block_walk(TSK_FS_INFO * a_fs, TSK_DADDR_T a_start_blk,
    TSK_DADDR_T a_end_blk, TSK_FS_BLOCK_WALK_FLAG_ENUM a_flags,
    TSK_FS_BLOCK_WALK_CB a_action, void *a_ptr)
{
    char *myname = "f2fs_block_walk";
    TSK_FS_BLOCK *fs_block;
    TSK_DADDR_T addr;

    // clean up any error messages that are lying around
    tsk_error_reset();

    /*
     * Sanity checks.
     */
    if (a_start_blk < a_fs->first_block || a_start_blk > a_fs->last_block) {
        tsk_error_reset();
        tsk_error_set_errno(TSK_ERR_FS_WALK_RNG);
        tsk_error_set_errstr("%s: start block: %" PRIuDADDR, myname,
            a_start_blk);
        return 1;
    }
    if (a_end_blk < a_fs->first_block || a_end_blk > a_fs->last_block
        || a_end_blk < a_start_blk) {
        tsk_error_reset();
        tsk_error_set_errno(TSK_ERR_FS_WALK_RNG);
        tsk_error_set_errstr("%s: end block: %" PRIuDADDR, myname,
            a_end_blk);
        return 1;
    }

    /* Sanity check on a_flags -- make sure at least one ALLOC is set */
    if (((a_flags & TSK_FS_BLOCK_WALK_FLAG_ALLOC) == 0) &&
        ((a_flags & TSK_FS_BLOCK_WALK_FLAG_UNALLOC) == 0)) {
        a_flags |=
            (TSK_FS_BLOCK_WALK_FLAG_ALLOC |
            TSK_FS_BLOCK_WALK_FLAG_UNALLOC);
    }
    if (((a_flags & TSK_FS_BLOCK_WALK_FLAG_META) == 0) &&
        ((a_flags & TSK_FS_BLOCK_WALK_FLAG_CONT) == 0)) {
        a_flags |=
            (TSK_FS_BLOCK_WALK_FLAG_CONT | TSK_FS_BLOCK_WALK_FLAG_META);
    }


    if ((fs_block = tsk_fs_block_alloc(a_fs)) == NULL) {
        return 1;
    }

    tsk_fs_block_free(fs_block);
    return 0;
}
static TSK_OFF_T
f2fs_make_data_run(TSK_FS_INFO * fs_info, TSK_FS_ATTR * fs_attr,
    TSK_FS_META *fs_meta, uint32_t content_ptr_prv, uint32_t content_ptr, uint32_t len)
{
    TSK_FS_ATTR_RUN *data_run;
    data_run = tsk_fs_attr_run_alloc();
    if (data_run == NULL) {
        return 1;
    }
    data_run->offset = content_ptr_prv;
    data_run->addr = content_ptr;
    data_run->len = len;

    if (tsk_fs_attr_add_run(fs_info, fs_attr, data_run)) {
        return 1;
    }

    return 0;
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static uint8_t
f2fs_load_attrs(TSK_FS_FILE *fs_file)
{
    TSK_FS_META *fs_meta = fs_file->meta;
    TSK_FS_INFO *fs_info = fs_file->fs_info;
    TSK_OFF_T length = 0;
    F2FS_INFO *f2fs = (F2FS_INFO *) fs_info;
    TSK_FS_ATTR *fs_attr;
    uint32_t * cont_ptr = (TSK_OFF_T *)fs_meta->content_ptr;
    uint32_t cont_len;
    int i;

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
    if (TSK_FS_TYPE_ISF2FS(fs_info->ftype) == 0) {
        tsk_error_set_errno(TSK_ERR_FS_INODE_COR);
        tsk_error_set_errstr
        ("f2fs_load_attr: Called with non f2fs file system: %x",
         fs_info->ftype);
        return 1;
    }
    
    length = roundup(fs_meta->size * fs_info->block_size, fs_info->block_size);
    if ((fs_attr =
         tsk_fs_attrlist_getnew(fs_meta->attr,
                                TSK_FS_ATTR_NONRES)) == NULL) {
        return 1;
    }

    if (tsk_fs_attr_set_run(fs_file, fs_attr, NULL, NULL,
                            TSK_FS_ATTR_TYPE_DEFAULT, TSK_FS_ATTR_ID_DEFAULT,
                            fs_meta->size * fs_info->block_size, fs_meta->size * fs_info->block_size, length, 0, 0)) {
        return 1;
    }

    if (fs_meta->inline_content == 0xb)
    {
		fs_meta->attr = tsk_fs_attrlist_alloc();
		if ((fs_attr =
			tsk_fs_attrlist_getnew(fs_meta->attr,
				TSK_FS_ATTR_RES)) == NULL) {
			free(cont_ptr);
			return 1;
		}

		// Set the details in the fs_attr structure
		if (tsk_fs_attr_set_str(fs_file, fs_attr, "DATA",
			TSK_FS_ATTR_TYPE_DEFAULT, TSK_FS_ATTR_ID_DEFAULT,
			(void*)cont_ptr, fs_meta->size)) {
			free(cont_ptr);

			fs_meta->attr_state = TSK_FS_META_ATTR_ERROR;
			return 1;
		}
		fs_meta->attr_state = TSK_FS_META_ATTR_STUDIED;
		
    }
    else if (fs_meta->type != TSK_FS_META_TYPE_DIR)
    {
        int itr = fs_meta->size + 10;

        for (int i = 2; i < itr; i++) {
            if(cont_ptr[i] == 0)
                break;
            if (f2fs_make_data_run(fs_info, fs_attr, fs_meta, (i-2), cont_ptr[i], 1))
                return 1;
        }
        
    }
    
    fs_meta->attr_state = TSK_FS_META_ATTR_STUDIED;
    return 0;

}

//Have to recheck
uint8_t f2fs_fscheck(TSK_FS_INFO * fs, FILE * HFile)
{
    return -1;
}
//Have to recheck
TSK_FS_BLOCK_FLAG_ENUM f2fs_block_getflags(TSK_FS_INFO * a_fs, TSK_DADDR_T a_addr)
{
    int flags = 0;

    if (a_addr == 0)
        return TSK_FS_BLOCK_FLAG_CONT | TSK_FS_BLOCK_FLAG_ALLOC;
    
    flags = TSK_FS_BLOCK_FLAG_ALLOC;
    flags |= TSK_FS_BLOCK_FLAG_CONT;

    return (TSK_FS_BLOCK_FLAG_ENUM)flags;
}

uint8_t f2fs_fsstat(TSK_FS_INFO * fs, FILE * hFile)
{
	unsigned int i;
	const char *tmptypename;
    F2FS_INFO * f2fs = (F2FS_INFO *) fs;
    f2fs_sb *sb = f2fs->fs;
    f2fs_checkpoint *cp = f2fs->fs_cp;

        tsk_error_reset();
        printf("\nFILE SYSTEM INFORMATION\n");
        printf("--------------------------------------------\n");
        if(tsk_getu32(fs->endian, sb->magic) == F2FS_FS_MAGIC)
        tmptypename = "F2FS";
        tsk_fprintf(hFile, "File System Type : %s\n", tmptypename);
	tsk_fprintf(hFile, "\n\nMETADATA INFORMATION\n");
        tsk_fprintf(hFile, "--------------------------------------------\n");
        tsk_fprintf(hFile, "mounted time : %" PRIu64 "\n", tsk_getu64(fs->endian, cp->elapsed_time));
        tsk_fprintf(hFile, "major version : %" PRIu16 "\n", tsk_getu16(fs->endian, sb->major_ver));
        tsk_fprintf(hFile, "minor version : %" PRIu16 "\n", tsk_getu16(fs->endian, sb->minor_ver));
        tsk_fprintf(hFile, "UUID : %lld\n", sb->uuid);
        tsk_fprintf(hFile, "\n");
        tsk_fprintf(hFile, "checkpoint blockaddress : %" PRIu32 "\n", tsk_getu32(fs->endian, sb->cp_blkaddr));
        tsk_fprintf(hFile, "SIT blockaddress : %" PRIu32 "\n", tsk_getu32(fs->endian, sb->sit_blkaddr));
        tsk_fprintf(hFile, "NAT blockaddress : %" PRIu32 "\n", tsk_getu32(fs->endian, sb->nat_blkaddr));
        tsk_fprintf(hFile, "SSA blockaddress : %" PRIu32 "\n", tsk_getu32(fs->endian, sb->ssa_blkaddr));
        tsk_fprintf(hFile, "Main blockaddress : %" PRIu32 "\n", tsk_getu32(fs->endian, sb->main_blkaddr));
        tsk_fprintf(hFile, "\n");
        tsk_fprintf(hFile, "root inode : %" PRIu32 "\n", tsk_getu32(fs->endian, sb->root_ino));
        tsk_fprintf(hFile, "node inode : %" PRIu32 "\n", tsk_getu32(fs->endian, sb->node_ino));
        tsk_fprintf(hFile, "meta inode : %" PRIu32 "\n", tsk_getu32(fs->endian, sb->meta_ino));
	tsk_fprintf(hFile, "\n");
        tsk_fprintf(hFile, "checksum : %" PRIu32 "\n", tsk_getu32(fs->endian, sb->crc));
        tsk_fprintf(hFile, "free segment : %" PRIu32 "\n", tsk_getu32(fs->endian, cp->free_segment_count));
        tsk_fprintf(hFile, "user block : %" PRIu32 "\n", tsk_getu32(fs->endian, cp->user_block_count));
        tsk_fprintf(hFile, "\n");
	tsk_fprintf(hFile, "valid block : %" PRIu32 "\n", tsk_getu32(fs->endian, cp->valid_block_count));
        tsk_fprintf(hFile, "valid node : %" PRIu32 "\n", tsk_getu32(fs->endian, cp->valid_node_count));
        tsk_fprintf(hFile, "valid inode : %" PRIu32 "\n", tsk_getu32(fs->endian, cp->valid_inode_count));
        tsk_fprintf(hFile, "\n\nITEM SIZE INFORMATION\n");
        tsk_fprintf(hFile, "--------------------------------------------\n");
        tsk_fprintf(hFile, "sector size : 2^%" PRIu32 "\n", tsk_getu32(fs->endian, sb->log_sectorsize));
        tsk_fprintf(hFile, "sector per block : 2^%" PRIu32 "\n", tsk_getu32(fs->endian, sb->log_sectors_per_block));
        tsk_fprintf(hFile, "block size : 2^%" PRIu32 "\n", tsk_getu32(fs->endian, sb->log_blocksize));
        tsk_fprintf(hFile, "blocks per segment : 2^%" PRIu32 "\n", tsk_getu32(fs->endian, sb->log_blocks_per_seg));
        tsk_fprintf(hFile, "segments per section : %" PRIu32 "\n", tsk_getu32(fs->endian, sb->segs_per_sec));
        tsk_fprintf(hFile, "section per zone : %" PRIu32 "\n", tsk_getu32(fs->endian, sb->secs_per_zone));
        return -1;
}

uint8_t f2fs_istat(TSK_FS_INFO * fs, TSK_FS_ISTAT_FLAG_ENUM flags, FILE * hFile, TSK_INUM_T inum,
            TSK_DADDR_T numblock, int32_t sec_skew)
{
    return -1;
}

void f2fs_close(TSK_FS_INFO * fs)
{
    F2FS_INFO * f2fs = (F2FS_INFO *) fs;

    fs->tag = 0;
    free(f2fs->fs);
    tsk_deinit_lock(&f2fs->lock);
    tsk_fs_free(fs);
    return;
}
//have to recheck



TSK_FS_INFO * f2fs_open(TSK_IMG_INFO * img_info, TSK_OFF_T offset,
	TSK_FS_TYPE_ENUM ftype, uint8_t test)
{
	F2FS_INFO *f2fs;
	unsigned int len;
	TSK_FS_INFO *fs;
	ssize_t cnt;
	// clean up any error messages that are lying around
	tsk_error_reset();

	if (TSK_FS_TYPE_ISF2FS(ftype) == 0) {
		tsk_error_reset();
		tsk_error_set_errno(TSK_ERR_FS_ARG);
		tsk_error_set_errstr("Invalid FS Type in f2fs_open");
		return NULL;
	}

	if (img_info->sector_size == 0) {
		tsk_error_reset();
		tsk_error_set_errno(TSK_ERR_FS_ARG);
		tsk_error_set_errstr("f2fs_open: sector size is 0");
		return NULL;
	}


	if ((f2fs = (F2FS_INFO *)tsk_fs_malloc(sizeof(*f2fs))) == NULL)
		return NULL;

	fs = &(f2fs->fs_info);

	fs->ftype = ftype;
	fs->flags = 0;
	fs->img_info = img_info;
	fs->offset = offset;
	fs->tag = TSK_FS_INFO_TAG;

	//read the superblock
	len = sizeof(f2fs_sb);
	if ((f2fs->fs = (f2fs_sb *)tsk_malloc(len)) == NULL) {
		fs->tag = 0;
		tsk_fs_free((TSK_FS_INFO *)f2fs);
		return NULL;
	}

	cnt = tsk_fs_read(fs, F2FS_SUPER_OFFSET, (char *)f2fs->fs, len);
	if (cnt != len) {
		if (cnt >= 0) {
			tsk_error_reset();
			tsk_error_set_errno(TSK_ERR_FS_READ);
		}
		tsk_error_set_errstr2("f2fs_open: superblock");
		fs->tag = 0;
		free(f2fs->fs);
		tsk_fs_free((TSK_FS_INFO *)f2fs);
		return NULL;
	}
	//verify image

	if (tsk_fs_guessu32(fs, f2fs->fs->magic, F2FS_FS_MAGIC)) {
		fs->tag = 0;
		free(f2fs->fs);
		tsk_fs_free((TSK_FS_INFO *)f2fs);
		tsk_error_reset();
		tsk_error_set_errno(TSK_ERR_FS_MAGIC);
		tsk_error_set_errstr("not an F2FS file system (magic)");
		if (tsk_verbose)
			fprintf(stderr, "f2fs_open: invalid magic\n");
		return NULL;
	}

	f2fs->cp_offset = tsk_getu32(fs->endian, f2fs->fs->cp_blkaddr) * F2FS_BLKSIZE;
	f2fs->nat_offset = tsk_getu32(fs->endian, f2fs->fs->nat_blkaddr) * F2FS_BLKSIZE;
	f2fs->nat_inode_index = tsk_getu32(fs->endian, f2fs->fs->root_ino);
	//read checkpoint

	len = sizeof(f2fs_checkpoint);
	if ((f2fs->fs_cp = (f2fs_checkpoint *)tsk_malloc(len)) == NULL) {
		fs->tag = 0;
		tsk_fs_free((TSK_FS_INFO *)f2fs);
		return NULL;
	}

	cnt = tsk_fs_read(fs, f2fs->cp_offset, (char *)f2fs->fs_cp, len);
	if (cnt != len) {
		if (cnt >= 0) {
			tsk_error_reset();
			tsk_error_set_errno(TSK_ERR_FS_READ);
		}
		tsk_error_set_errstr2("f2fs_open: checkpoint");
		fs->tag = 0;
		free(f2fs->fs_cp);
		tsk_fs_free((TSK_FS_INFO *)f2fs);
		return NULL;
	}

	//read nat *read only one block of nat
	len = F2FS_BLKSIZE;
	if ((f2fs->fs_nat = (f2fs_nat_block *)tsk_malloc(len)) == NULL) {
		fs->tag = 0;
		tsk_fs_free((TSK_FS_INFO *)f2fs);
		return NULL;
	}

	/*
	 * Calculate the meta data info
	 */
	fs->inum_count = tsk_getu32(fs->endian, f2fs->fs_cp->valid_node_count) -3;
	fs->last_inum = fs->inum_count;
	fs->first_inum = tsk_getu32(fs->endian, f2fs->fs->node_ino);
	fs->root_inum = tsk_getu32(fs->endian, f2fs->fs->root_ino) + 1;


	int rdcnt = 0;
    int targetcnt = 0x600;
	
	f2fs_nat_entry * tmp;

	if ((f2fs->fs_nat->entries = (f2fs_nat_entry *)tsk_malloc(F2FS_BLKSIZE*targetcnt)) == NULL) {
		fs->tag = 0;
		tsk_fs_free((TSK_FS_INFO *)f2fs);
		return NULL;
	}
	if ((tmp = (f2fs_nat_entry *)tsk_malloc(F2FS_BLKSIZE)) == NULL) {
		fs->tag = 0;
		tsk_fs_free((TSK_FS_INFO *)f2fs);
		return NULL;
	}

	memset(f2fs->fs_nat->entries, 0, F2FS_BLKSIZE*targetcnt);
    memset(tmp, 0, F2FS_BLKSIZE);

	for (int j = 0; j < targetcnt; j++) {

		cnt = tsk_fs_read(fs, f2fs->nat_offset + (F2FS_BLKSIZE*j), (char *)tmp, F2FS_BLKSIZE-1);
		if (cnt != F2FS_BLKSIZE-1) {
			if (cnt >= 0) {
				tsk_error_reset();
				tsk_error_set_errno(TSK_ERR_FS_READ);
			}
			tsk_error_set_errstr2("f2fs_open: nat");
			fs->tag = 0;
			free(f2fs->fs_nat);
			tsk_fs_free((TSK_FS_INFO *)f2fs);
			return NULL;
		}

        uint8_t tmptmp[8];
		memcpy(tmptmp, tmp, 8);

		memcpy((uint8_t*)f2fs->fs_nat->entries + ((F2FS_BLKSIZE-1)*j), tmp, F2FS_BLKSIZE-1);
        memset(tmp, 0, F2FS_BLKSIZE - 1);
	}

    if (fs->inum_count < 10) {
        fs->tag = 0;
        free(f2fs->fs);
        tsk_fs_free((TSK_FS_INFO *)f2fs);
        tsk_error_reset();
        tsk_error_set_errno(TSK_ERR_FS_MAGIC);
        tsk_error_set_errstr("Not an F2FS file system (inum count)");
        if (tsk_verbose)
            fprintf(stderr, "f2fs_open: two few inodes\n");
        return NULL;
    }
    /* Set the size of the inode, but default to our data structure
     * size if it is larger */
    f2fs->inode_size = F2FS_BLKSIZE;
    if (f2fs->inode_size < sizeof(f2fs_inode)) {
        if (tsk_verbose)
            tsk_fprintf(stderr, "SB inode size is small");
    }
    //calcul the blk info
    fs->dev_bsize = img_info->sector_size;
    fs->first_block = 0;
    fs->block_count = (tsk_getu64(fs->endian, f2fs->fs_cp->user_block_count));
    fs->block_size = F2FS_BLKSIZE;
    fs->last_block_act = fs->last_block = fs->block_count - 1;


    fs->inode_walk = f2fs_inode_walk;
    fs->block_walk = f2fs_block_walk;
    fs->block_getflags = f2fs_block_getflags;
    fs->get_default_attr_type = tsk_fs_unix_get_default_attr_type;
    fs->load_attrs = f2fs_load_attrs;
    fs->file_add_meta = f2fs_inode_lookup;
    fs->dir_open_meta = f2fs_dir_open_meta; 
    fs->fsstat = f2fs_fsstat;
    fs->fscheck = f2fs_fscheck;
    fs->istat = f2fs_istat;
    fs->name_cmp = tsk_fs_unix_name_cmp;
    fs->close = f2fs_close;
    
    tsk_init_lock(&f2fs->lock);
	
	free(tmp);

    return(fs);
}
