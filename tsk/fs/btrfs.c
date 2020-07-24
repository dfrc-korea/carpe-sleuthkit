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
 *\file btrfs.c
 * Contains the internal TSK Btrfs file system functions.
 */

#include "tsk_btrfs.h"

static uint8_t
btrfs_dinode_load(BTRFS_INFO * btrfs, TSK_INUM_T dino_inum,
	btrfs_inode_item * dino_buf)
{
	TSK_OFF_T addr;
	ssize_t cnt;
	TSK_FS_INFO *fs = &(btrfs->fs_info);

	/*
	 * Sanity check.
	 * Use last_num-1 to account for virtual Orphan directory in last_inum.
	 */
	if ((dino_inum < fs->first_inum) || (dino_inum > fs->last_inum - 1)) {
		tsk_error_reset();
		tsk_error_set_errno(TSK_ERR_FS_INODE_NUM);
		tsk_error_set_errstr("ext2fs_dinode_load: address: %" PRIuINUM,
			dino_inum);
		return 1;
	}

	if (dino_buf == NULL) {
		tsk_error_reset();
		tsk_error_set_errno(TSK_ERR_FS_ARG);
		tsk_error_set_errstr("ext2fs_dinode_load: dino_buf is NULL");
		return 1;
	}
	
	addr = btrfs_seek_fs_leaf(btrfs, dino_inum, BTRFS_INODE_ITEM_KEY);

	cnt = tsk_fs_read(fs, addr, (char *)dino_buf, (size_t)btrfs->inode_size);
	ssize_t len = btrfs->inode_size;
	if (cnt != len) {
		if (cnt >= 0) {
			tsk_error_reset();
			tsk_error_set_errno(TSK_ERR_FS_READ);
		}
		tsk_error_set_errstr2("btrfs_dinode_load: Inode %" PRIuINUM
			" from %" PRIuOFF, dino_inum, addr);
		return 1;
	}
	return 0;
}


static uint8_t
btrfs_dinode_copy(BTRFS_INFO * btrfs, TSK_FS_META * fs_meta,
	TSK_INUM_T inum, const btrfs_inode_item * dino_buf)
{
	TSK_FS_INFO *fs = (TSK_FS_INFO *)& btrfs->fs_info;
	TSK_OFF_T extent_item_offset;
	int cnt;
	
	uint64_t * addr_ptr = (uint64_t *)fs_meta->content_ptr;

	if (dino_buf == NULL) {
		tsk_error_reset();
		tsk_error_set_errno(TSK_ERR_FS_ARG);
		tsk_error_set_errstr("btrfs_dinode_copy: dino_buf is NULL");
		return 1;
	}

	fs_meta->attr_state = TSK_FS_META_ATTR_EMPTY;
	if (fs_meta->attr) {
		tsk_fs_attrlist_markunused(fs_meta->attr);
	}

	// set the type
	switch (tsk_getu32(fs->endian, dino_buf->mode) & BTRFS_IN_FMT) {
		case BTRFS_IN_REG:
			fs_meta->type = TSK_FS_META_TYPE_REG;
			break;
		case BTRFS_IN_DIR:
			fs_meta->type = TSK_FS_META_TYPE_DIR;
			break;
		case BTRFS_IN_SOCK:
			fs_meta->type = TSK_FS_META_TYPE_SOCK;
			break;
		case BTRFS_IN_LNK:
			fs_meta->type = TSK_FS_META_TYPE_LNK;
			break;
		case BTRFS_IN_BLK:
			fs_meta->type = TSK_FS_META_TYPE_BLK;
			break;
		case BTRFS_IN_CHR:
			fs_meta->type = TSK_FS_META_TYPE_CHR;
			break;
		case BTRFS_IN_FIFO:
			fs_meta->type = TSK_FS_META_TYPE_FIFO;
			break;
		default:
			fs_meta->type = TSK_FS_META_TYPE_UNDEF;
			break;
	}
	// set the mode
	fs_meta->mode = 0;
	if (tsk_getu32(fs->endian, dino_buf->mode) & BTRFS_IN_ISUID)
		fs_meta->mode |= TSK_FS_META_MODE_ISUID;
	if (tsk_getu32(fs->endian, dino_buf->mode) & BTRFS_IN_ISGID)
		fs_meta->mode |= TSK_FS_META_MODE_ISGID;
	if (tsk_getu32(fs->endian, dino_buf->mode) & BTRFS_IN_ISVTX)
		fs_meta->mode |= TSK_FS_META_MODE_ISVTX;

	if (tsk_getu32(fs->endian, dino_buf->mode) & BTRFS_IN_IRUSR)
		fs_meta->mode |= TSK_FS_META_MODE_IRUSR;
	if (tsk_getu32(fs->endian, dino_buf->mode) & BTRFS_IN_IWUSR)
		fs_meta->mode |= TSK_FS_META_MODE_IWUSR;
	if (tsk_getu32(fs->endian, dino_buf->mode) & BTRFS_IN_IXUSR)
		fs_meta->mode |= TSK_FS_META_MODE_IXUSR;

	if (tsk_getu32(fs->endian, dino_buf->mode) & BTRFS_IN_IRGRP)
		fs_meta->mode |= TSK_FS_META_MODE_IRGRP;
	if (tsk_getu32(fs->endian, dino_buf->mode) & BTRFS_IN_IWGRP)
		fs_meta->mode |= TSK_FS_META_MODE_IWGRP;
	if (tsk_getu32(fs->endian, dino_buf->mode) & BTRFS_IN_IXGRP)
		fs_meta->mode |= TSK_FS_META_MODE_IXGRP;

	if (tsk_getu32(fs->endian, dino_buf->mode) & BTRFS_IN_IROTH)
		fs_meta->mode |= TSK_FS_META_MODE_IROTH;
	if (tsk_getu32(fs->endian, dino_buf->mode) & BTRFS_IN_IWOTH)
		fs_meta->mode |= TSK_FS_META_MODE_IWOTH;
	if (tsk_getu32(fs->endian, dino_buf->mode) & BTRFS_IN_IXOTH)
		fs_meta->mode |= TSK_FS_META_MODE_IXOTH;

	fs_meta->nlink = tsk_getu32(fs->endian, dino_buf->nlink);
	fs_meta->size = tsk_getu64(fs->endian, dino_buf->size);

	fs_meta->addr = inum;

	fs_meta->uid = tsk_getu32(fs->endian, dino_buf->uid);
	fs_meta->gid = tsk_getu32(fs->endian, dino_buf->gid);

	fs_meta->mtime = tsk_getu64(fs->endian, dino_buf->mtime.sec);
	fs_meta->atime = tsk_getu64(fs->endian, dino_buf->atime.sec);
	fs_meta->ctime = tsk_getu64(fs->endian, dino_buf->ctime.sec);
	fs_meta->crtime = tsk_getu64(fs->endian, dino_buf->otime.sec);
	fs_meta->mtime_nano = tsk_getu32(fs->endian, dino_buf->mtime.nsec);
	fs_meta->atime_nano = tsk_getu32(fs->endian, dino_buf->atime.nsec);
	fs_meta->ctime_nano = tsk_getu32(fs->endian, dino_buf->ctime.nsec);
	fs_meta->crtime_nano = tsk_getu32(fs->endian, dino_buf->otime.nsec);
	
	fs_meta->seq = tsk_getu64(fs->endian, dino_buf->sequence);
	
	if (fs_meta->link) {
		free(fs_meta->link);
		fs_meta->link = NULL;
	}

	if (fs_meta->type == TSK_FS_META_TYPE_DIR) {
		btrfs_leaf * fs_leaf;
		int num_stripes;

		int i, j;
		int tmp = -1;

		//	num_stripes = btrfs->num_stripes;
		num_stripes = 1;

		for (i = 0; i < num_stripes; i++) {
			// trick to find fast
			for (j = 0; j < btrfs->fs_leaf_num[i]; j++) {
				fs_leaf = btrfs->fs_leaf[i][j];
				if (tsk_getu64(fs->endian, fs_leaf->items[0].key.objectid) <= inum) {
					if (tsk_getu64(fs->endian, fs_leaf->items[0].key.objectid) == inum) {
						tmp = j;
						break;
					}
					continue;
				}
				else {
					tmp = j - 1;
					break;
				}
			}
			if (tmp == -1)
				tmp = btrfs->fs_leaf_num[i] - 1;
		}

		addr_ptr[0] = btrfs->fs_leaf_phy_addr[i - 1][tmp];
	}
	else {
		// calc extent item offset
		extent_item_offset = btrfs_seek_fs_leaf(btrfs, inum, BTRFS_EXTENT_DATA_KEY);


		
		// read extent item
		if (extent_item_offset != 0x01) {
			char * buf = (char *)btrfs->tmp_extent_item;
			cnt = tsk_fs_read(fs, extent_item_offset, (char *)buf, BTRFS_FILE_EXTENT_LEN);
			if (cnt != BTRFS_FILE_EXTENT_LEN) {
				if (tsk_verbose) {
					fprintf(stderr, "invalid extent item read size, cnt: %d\n", cnt);
				}
				return -1;
			}
			addr_ptr[0] = btrfs_calc_phyAddr(fs, btrfs, btrfs->tmp_extent_item->disk_bytenr, 0);
		} else {
			fs_meta->size = 0x00;
			addr_ptr[0] = 0x01;
		}

	}
	fs_meta->content_type = TSK_FS_META_CONTENT_TYPE_DEFAULT;

	return 0;
}



/* btrfs_inode_lookup - lookup inode, external interface
 *
 * Returns 1 on error and 0 on success
 *
 */

static uint8_t
btrfs_inode_lookup(TSK_FS_INFO * fs, TSK_FS_FILE * a_fs_file,
	TSK_INUM_T inum)
{
	BTRFS_INFO *btrfs = (BTRFS_INFO *)fs;
	btrfs_inode_item *dino_buf = NULL;
	unsigned int size = 0;

	if (a_fs_file == NULL) {
		tsk_error_set_errno(TSK_ERR_FS_ARG);
		tsk_error_set_errstr("btrfs_inode_lookup: fs_file is NULL");
		return 1;
	}

	if (a_fs_file->meta == NULL) {
		if ((a_fs_file->meta =
			tsk_fs_meta_alloc(BTRFS_FILE_CONTENT_LEN)) == NULL)
			return 1;
	}
	else {
		tsk_fs_meta_reset(a_fs_file->meta);
	}

	// see if they are looking for the special "orphans" directory
	TSK_OFF_T addr = btrfs_seek_fs_leaf(btrfs, inum, BTRFS_INODE_ITEM_KEY);
	if (addr == 1) {
		if (tsk_fs_dir_make_orphan_dir_meta(fs, a_fs_file->meta))
			return 1;
		else
			return 0;
	}

	size =
		btrfs->inode_size >
		sizeof(btrfs_inode_item) ? btrfs->inode_size : sizeof(btrfs_inode_item);

	if ((dino_buf = (btrfs_inode_item *)tsk_malloc(size)) == NULL) {
		return 1;
	}

	if (btrfs_dinode_load(btrfs, inum, dino_buf)) {
		free(dino_buf);
		return 1;
	}

	if (btrfs_dinode_copy(btrfs, a_fs_file->meta, inum, dino_buf)) {
		free(dino_buf);
		return 1;
	}

	free(dino_buf);

	return 0;
}



/* btrfs_inode_walk - inode iterator
 *
 * flags used: TSK_FS_META_FLAG_USED, TSK_FS_META_FLAG_UNUSED,
 *  TSK_FS_META_FLAG_ALLOC, TSK_FS_META_FLAG_UNALLOC, TSK_FS_META_FLAG_ORPHAN
 *
 *  Return 1 on error and 0 on success
*/

uint8_t
btrfs_inode_walk(TSK_FS_INFO * fs, TSK_INUM_T start_inum,
	TSK_INUM_T end_inum, TSK_FS_META_FLAG_ENUM flags,
	TSK_FS_META_WALK_CB a_action, void *a_ptr)
{
	char *myname = "btrfs_inode_walk";
	TSK_FS_FILE *fs_file;
	btrfs_inode_item *dino_buf = NULL;

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

	/* If ORPHAN is wanted, then make sure that the flags are correct */
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
			("- btrfs_inode_walk: identifying inodes allocated by file names");
			return 1;
		}

	}


	if ((fs_file = tsk_fs_file_alloc(fs)) == NULL)
		return 1;
	

	/*
	 * Cleanup.
	 */
	tsk_fs_file_close(fs_file);
	free(dino_buf);

	return -1;
}



TSK_FS_BLOCK_FLAG_ENUM
btrfs_block_getflags(TSK_FS_INFO * a_fs, TSK_DADDR_T a_addr)
{
	return -1;
}



static uint8_t
btrfs_fscheck(TSK_FS_INFO * fs, FILE * hFile)
{
	return -1;
}


/** \internal
 * Add a single extent -- that is, a single data ran -- to the file data attribute.
 * @return 0 on success, 1 on error.
 */
static TSK_OFF_T
btrfs_make_data_run_extent_data(TSK_FS_INFO * fs_info, TSK_FS_ATTR * fs_attr, btrfs_run_data *brd)
{
	TSK_FS_ATTR_RUN *data_run;

	if ((data_run = tsk_fs_attr_run_alloc()) == NULL)
		return 1;

	data_run->offset = 0;
	data_run->addr = (brd->data_addr / fs_info->block_size);
	data_run->len = brd->data_len;


	if (tsk_fs_attr_add_run(fs_info, fs_attr, data_run)) {
		return 1;
	}

	return 0;
}



/**
 * \internal
 * Loads attribute for Ext4 Extents-based storage method.
 * @param fs_file File system to analyze
 * @returns 0 on success, 1 otherwise
 */
static uint8_t
btrfs_load_attrs_extent_data(TSK_FS_FILE *fs_file)
{
	TSK_FS_META *fs_meta = fs_file->meta;
	TSK_FS_INFO *fs_info = fs_file->fs_info;
	TSK_OFF_T length = 0;
	TSK_FS_ATTR *fs_attr;
	btrfs_run_data * brd;

	brd = (btrfs_run_data *)fs_meta->content_ptr;


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

	if (TSK_FS_TYPE_ISBTRFS(fs_info->ftype) == 0) {
		tsk_error_set_errno(TSK_ERR_FS_INODE_COR);
		tsk_error_set_errstr
		("btrfs_load_attr: Called with non-ExtX file system: %x",
			fs_info->ftype);
		return 1;
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

	// make file
	if (brd->data_addr == 0x01) {
		brd->data_addr = 0x00;
		brd->data_len = 0x00;
	} else {
		brd->data_len = length;
	}

	btrfs_make_data_run_extent_data(fs_info, fs_attr, brd);
	
	fs_meta->attr_state = TSK_FS_META_ATTR_STUDIED;
		
	return 0;
}

static uint8_t
btrfs_load_attrs(TSK_FS_FILE * fs_file)
{
	/* EXT4 extents-based storage is dealt with differently than
	* the traditional pointer lists. */
	if (fs_file->meta->content_type == TSK_FS_META_CONTENT_TYPE_DEFAULT) {
		return btrfs_load_attrs_extent_data(fs_file);
	}
	else {
		fprintf(stderr, "content_type = unknown content type\n");
	}

	return 0;
}




typedef struct {
	FILE *hFile;
	int idx;
} BTRFS_PRINT_ADDR;




static uint8_t
btrfs_istat(TSK_FS_INFO * fs, TSK_FS_ISTAT_FLAG_ENUM istat_flags, FILE * hFile, TSK_INUM_T inum,
	TSK_DADDR_T numblock, int32_t sec_skew)
{
	return -1;
}

/* btrfs_close - close an btrfs file system */
static void
btrfs_close(TSK_FS_INFO * fs)
{
	BTRFS_INFO *btrfs = (BTRFS_INFO *)fs;

	fs->tag = 0;
	free(btrfs->fs);
	free(btrfs->root_tree);
	free(btrfs->root_phy_addr);
	free(btrfs->chunk_tree);
	free(btrfs->fs_tree);
	free(btrfs->fs_phy_addr);
	free(btrfs->fs_leaf);
	free(btrfs->fs_leaf_phy_addr);
	free(btrfs->fs_leaf_num);

	tsk_deinit_lock(&btrfs->lock);

	tsk_fs_free(fs);
}


//fsstat
uint8_t btrfs_fsstat(TSK_FS_INFO * fs, FILE * hFile)
{
	uint i;
	const char *tmptypename;

	BTRFS_INFO * btrfs = (BTRFS_INFO *)fs;
	btrfs_super_block *sb = btrfs->fs;

	tsk_error_reset();
	tsk_fprintf(hFile, "\nFILE SYSTEM INFORMATION\n");
	tsk_fprintf(hFile, "--------------------------------------------\n");

	if (tsk_getu64(fs->endian, sb->magic) == BTRFS_MAGIC)
		tmptypename = "BTRFS";

	tsk_fprintf(hFile, "File System Type : %s\n", tmptypename);
	tsk_fprintf(hFile, "FSID : 0x");
	for (i = 0; i < 16; i++)
		tsk_fprintf(hFile, "%X", sb->fsid[i]);
	tsk_fprintf(hFile, "\n");

	if (tsk_getu32(fs->endian, sb->compat_flags)) {
		tsk_fprintf(hFile, "compat flags: %" PRIu64 "\n", tsk_getu64(fs->endian, sb->compat_flags));
	}
	if (tsk_getu32(fs->endian, sb->compat_ro_flags)) {
		tsk_fprintf(hFile, "compat ro flags: %" PRIu64 "\n", tsk_getu64(fs->endian, sb->compat_ro_flags));
	}
	if (tsk_getu32(fs->endian, sb->incompat_flags)) {
		tsk_fprintf(hFile, "incompat flags: %" PRIu64 "\n", tsk_getu64(fs->endian, sb->incompat_flags));
	}

	tsk_fprintf(hFile, "\n\nMETADATA INFORMATION\n");
	tsk_fprintf(hFile, "--------------------------------------------\n");
	tsk_fprintf(hFile, "Checksum : 0x");
	for (i = 0; i < 32; i++)
		tsk_fprintf(hFile, "%X", sb->csum[i]);
	tsk_fprintf(hFile, "\n"); 
	tsk_fprintf(hFile, "Generation : %" PRIu64 "\n", tsk_getu64(fs->endian, sb->generation));
	tsk_fprintf(hFile, "Root Logical Address : %" PRIu64 "\n", tsk_getu64(fs->endian, sb->root));
	tsk_fprintf(hFile, "Chunk Logical Address : %" PRIu64 "\n", tsk_getu64(fs->endian, sb->chunk_root));
	tsk_fprintf(hFile, "Log Logical Address : %" PRIu64 "\n", tsk_getu64(fs->endian, sb->log_root));
	tsk_fprintf(hFile, "Root level : %" PRIu8 "\n", sb->root_level);
	tsk_fprintf(hFile, "Chunk level : %" PRIu8 "\n", sb->chunk_root_level);
	tsk_fprintf(hFile, "Log level : %" PRIu8 "\n", sb->log_root_level);
	tsk_fprintf(hFile, "\n");
	tsk_fprintf(hFile, "total bytes : %" PRIu64 "\n", tsk_getu64(fs->endian, sb->total_bytes));
	tsk_fprintf(hFile, "bytes used : %" PRIu64 "\n", tsk_getu64(fs->endian, sb->bytes_used));
	tsk_fprintf(hFile, "root directory objectid : %" PRIu64 "\n", tsk_getu64(fs->endian, sb->root_dir_objectid));
	tsk_fprintf(hFile, "number of devices : %" PRIu64 "\n", tsk_getu64(fs->endian, sb->num_devices));
	tsk_fprintf(hFile, "checksum type : %" PRIu16 "\n", tsk_getu16(fs->endian, sb->csum_type));
	tsk_fprintf(hFile, "\n");
	tsk_fprintf(hFile, "UUID Tree Generation : %" PRIu64 "\n", tsk_getu64(fs->endian, sb->uuid_tree_generation));
	tsk_fprintf(hFile, "Chunk Root Generation : %" PRIu64 "\n", tsk_getu64(fs->endian, sb->chunk_root_generation));
	tsk_fprintf(hFile, "Cache Generation : %" PRIu64 "\n", tsk_getu64(fs->endian, sb->cache_generation));
	tsk_fprintf(hFile, "\n\nITEM SIZE INFORMATION\n");
	tsk_fprintf(hFile, "--------------------------------------------\n");
	tsk_fprintf(hFile, "sector size : %" PRIu32 "\n", tsk_getu32(fs->endian, sb->sectorsize));
	tsk_fprintf(hFile, "node size : %" PRIu32 "\n", tsk_getu32(fs->endian, sb->nodesize));
	tsk_fprintf(hFile, "leaf size : %" PRIu32 "\n", tsk_getu32(fs->endian, sb->__unused_leafsize));
	tsk_fprintf(hFile, "stripe size : %" PRIu32 "\n", tsk_getu32(fs->endian, sb->stripesize));
	tsk_fprintf(hFile, "system chunk array size : %" PRIu32 "\n", tsk_getu32(fs->endian, sb->sys_chunk_array_size));
	tsk_fprintf(hFile, "\n");



	return -1;
}


uint8_t btrfs_block_walk(TSK_FS_INFO * fs, TSK_DADDR_T start, TSK_DADDR_T end,
	TSK_FS_BLOCK_WALK_FLAG_ENUM flags, TSK_FS_BLOCK_WALK_CB cb, void *ptr)
{
	return -1;
}

/**
 * \internal
 * Open part of a disk image as a BTRFS/3 file system.
 *
 * @param img_info Disk image to analyze
 * @param offset Byte offset where file system starts
 * @param ftype Specific type of file system
 * @param test NOT USED
 * @returns NULL on error or if data is not an BTRFS/3 file system
 */
TSK_FS_INFO *
btrfs_open(TSK_IMG_INFO * img_info, TSK_OFF_T offset,
	TSK_FS_TYPE_ENUM ftype, uint8_t test)
{
	BTRFS_INFO *btrfs;
	unsigned int len;
	TSK_FS_INFO *fs;
	ssize_t cnt;

	// clean up any error messages that are lying around
	tsk_error_reset();

	if (TSK_FS_TYPE_ISBTRFS(ftype) == 0) {
		tsk_error_reset();
		tsk_error_set_errno(TSK_ERR_FS_ARG);
		tsk_error_set_errstr("Invalid FS Type inbtrfs_open");
		return NULL;
	}

	if (img_info->sector_size == 0) {
		tsk_error_reset();
		tsk_error_set_errno(TSK_ERR_FS_ARG);
		tsk_error_set_errstr("Btrfs_open: sector size is 0");
		return NULL;
	}

	if ((btrfs = (BTRFS_INFO *)tsk_fs_malloc(sizeof(*btrfs))) == NULL)
		return NULL;

	fs = &(btrfs->fs_info);

	fs->ftype = ftype;
	fs->flags = 0;
	fs->img_info = img_info;
	fs->offset = offset;
	fs->tag = TSK_FS_INFO_TAG;

	/*
	 * Read the superblock.
	 */
	len = sizeof(btrfs_super_block);
	if ((btrfs->fs = (btrfs_super_block *)tsk_malloc(len)) == NULL) {
		fs->tag = 0;
		tsk_fs_free((TSK_FS_INFO *)btrfs);
		return NULL;
	}

	if (BTRFS_SUPER_INFO_OFFSET + len >= fs->img_info->size) {
		tsk_error_reset();
		tsk_error_set_errno(TSK_ERR_IMG_SEEK);

		tsk_error_set_errstr2("btrfs_open: Image is too small");
		fs->tag = 0;
		free(btrfs->fs);
		tsk_fs_free((TSK_FS_INFO *)btrfs);
		return NULL;
	}

	cnt = tsk_fs_read(fs, BTRFS_SUPER_INFO_OFFSET, (char *)btrfs->fs, len);
	if (cnt != len) {
		if (cnt >= 0) {
			tsk_error_reset();
			tsk_error_set_errno(TSK_ERR_FS_READ);
		}
		tsk_error_set_errstr2("btrfs_open: superblock");
		fs->tag = 0;
		free(btrfs->fs);
		tsk_fs_free((TSK_FS_INFO *)btrfs);
		return NULL;
	}

	if (tsk_fs_guessu64(fs, btrfs->fs->bytenr, BTRFS_SUPER_INFO_OFFSET)) {
		fs->tag = 0;
			free(btrfs->fs);
			tsk_fs_free((TSK_FS_INFO *)btrfs);
			tsk_error_reset();
			tsk_error_set_errno(TSK_ERR_FS_UNSUPTYPE);
			tsk_error_set_errstr("not an Btrfs file system (bytenr)");
			if (tsk_verbose)
				fprintf(stderr, "btrfs_open: invalid bytenr\n");
			return NULL;
	}

	if (!tsk_fs_guessu64(fs, btrfs->fs->num_devices, (uint64_t)0)) {
		fs->tag = 0;
			free(btrfs->fs);
			tsk_fs_free((TSK_FS_INFO *)btrfs);
			tsk_error_reset();
			tsk_error_set_errno(TSK_ERR_FS_UNSUPTYPE);
			tsk_error_set_errstr("not an Btrfs file system (num_devices)");
			if (tsk_verbose)
				fprintf(stderr, "btrfs_open: invalid devices\n");
			return NULL;
	}

	if (tsk_fs_guessu64(fs, btrfs->fs->magic, BTRFS_MAGIC)) {
		fs->tag = 0;
		free(btrfs->fs);
		tsk_fs_free((TSK_FS_INFO *)btrfs);
		tsk_error_reset();
		tsk_error_set_errno(TSK_ERR_FS_MAGIC);
		tsk_error_set_errstr("not an Btrfs file system (magic)");
		if (tsk_verbose)
			fprintf(stderr, "btrfs_open: invalid magic\n");
		return NULL;
	}
	
	fs->first_inum = 256;
	fs->root_inum = 256;
	fs->dev_bsize = img_info->sector_size;
	fs->first_block = 0;
	fs->block_size = 0x1000;
	btrfs->inode_size = 160;

	if ((tsk_getu64(fs->endian, btrfs->fs->total_bytes) % fs->block_size) > 0)
		fs->block_count = (TSK_DADDR_T)(tsk_getu64(fs->endian, btrfs->fs->total_bytes)/fs->block_size) + 1;
	else 
		fs->block_count = (TSK_DADDR_T)(tsk_getu64(fs->endian, btrfs->fs->total_bytes) / fs->block_size);
	fs->last_block = fs->block_count;
	fs->last_block_act = fs->block_count - 1;

	// initializing chunk tree
	btrfs_init_chunk(btrfs);
	
	// initializing root tree
	btrfs_init_root(btrfs);

	// initializing root tree
	btrfs_init_fs(btrfs);

	// initializing root leaf
	btrfs_get_fs_leaf(btrfs);
	fs->last_inum = btrfs_get_last_inum(btrfs);
	
	if ((fs->block_size == 0) || (fs->block_size % 512)) {
		fs->tag = 0;
		free(btrfs->fs);
		tsk_fs_free((TSK_FS_INFO *)btrfs);
		tsk_error_reset();
		tsk_error_set_errno(TSK_ERR_FS_MAGIC);
		tsk_error_set_errstr("Not an btrfs file system (block size)");
		if (tsk_verbose)
			fprintf(stderr, "btrfs_open : invalid block size\n");
		return NULL;
	}


	// determine the last block we have in this image
	if ((TSK_DADDR_T)((img_info->size - offset) / fs->block_size) <
		fs->block_count)
		fs->last_block_act =
		(img_info->size - offset) / fs->block_size - 1;


	/* Volume ID */
	for (fs->fs_id_used = 0; fs->fs_id_used < 16; fs->fs_id_used++) {
		fs->fs_id[fs->fs_id_used] = btrfs->fs->fsid[fs->fs_id_used];
	}
	/* Set the generic function pointers */
	fs->inode_walk = btrfs_inode_walk;
	fs->block_walk = btrfs_block_walk;
	fs->block_getflags = btrfs_block_getflags;

	fs->get_default_attr_type = tsk_fs_unix_get_default_attr_type;
	//fs->load_attrs = tsk_fs_unix_make_data_run;
	fs->load_attrs = btrfs_load_attrs;

	fs->file_add_meta = btrfs_inode_lookup;
	fs->dir_open_meta = btrfs_dir_open_meta;
	fs->fsstat = btrfs_fsstat;
	fs->fscheck = btrfs_fscheck;
	fs->istat = btrfs_istat;
	fs->name_cmp = tsk_fs_unix_name_cmp;
	fs->close = btrfs_close;

	tsk_init_lock(&btrfs->lock);

	return fs;
}

