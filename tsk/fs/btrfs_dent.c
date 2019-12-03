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

#include "tsk_fs_i.h"
#include "tsk_btrfs.h"

static int files_found = 0;
static int folders_found = 0;


static uint8_t
btrfs_dent_copy(BTRFS_INFO * btrfs,
	btrfs_dir_item* dir_item, TSK_FS_NAME *fs_name)
{
	TSK_FS_INFO *fs = &(btrfs->fs_info);

	/* BTRFS does not null terminate */
	int namelen = tsk_getu16(fs->endian, dir_item->name_len);
	if (namelen >= fs_name->name_size) {
		tsk_error_reset();
		tsk_error_set_errno(TSK_ERR_FS_ARG);
		tsk_error_set_errstr
		("btrfs_dent_copy: Name Space too Small %d %" PRIuSIZE "",
			namelen, fs_name->name_size);
		return 1;
	}

	/* Copy and Null Terminate */
	strncpy(fs_name->name, dir_item->name, namelen);
	fs_name->name[namelen] = '\0';

	fs_name->type = TSK_FS_NAME_TYPE_UNDEF;
	fs_name->meta_addr = tsk_getu64(fs->endian, dir_item->location.objectid);

	switch (dir_item->type) {
		case BTRFS_DE_REG:
			fs_name->type = TSK_FS_NAME_TYPE_REG;
			break;
		case BTRFS_DE_DIR:
			fs_name->type = TSK_FS_NAME_TYPE_DIR;
			break;
		case BTRFS_DE_CHR:
			fs_name->type = TSK_FS_NAME_TYPE_CHR;
			break;
		case BTRFS_DE_BLK:
			fs_name->type = TSK_FS_NAME_TYPE_BLK;
			break;
		case BTRFS_DE_FIFO:
			fs_name->type = TSK_FS_NAME_TYPE_FIFO;
			break;
		case BTRFS_DE_SOCK:
			fs_name->type = TSK_FS_NAME_TYPE_SOCK;
			break;
		case BTRFS_DE_LNK:
			fs_name->type = TSK_FS_NAME_TYPE_LNK;
			break;
		case BTRFS_DE_UNKNOWN:
		default:
			fs_name->type = TSK_FS_NAME_TYPE_UNDEF;
			break;
	}

	fs_name->flags = 0;

	return 0;
}


static TSK_RETVAL_ENUM
btrfs_dent_parse_leaf(BTRFS_INFO * btrfs, TSK_FS_DIR * a_fs_dir, uint8_t a_is_del, TSK_LIST ** list_seen, uint64_t *leaf_addr)
{
	TSK_FS_INFO *fs = &(btrfs->fs_info);
	uint64_t inode;
	TSK_INUM_T inum;
	TSK_FS_NAME *fs_name;

	inum = a_fs_dir->addr;
	int idx, cnt, len;

	if ((fs_name = tsk_fs_name_alloc(BTRFS_MAXNAMLEN + 1, 0)) == NULL)
		return TSK_ERR;

	/* update each time by the actual length instead of the
	 ** recorded length so we can view the deleted entries
	 */
	
	if ((leaf_addr[0] != 0x01) && (leaf_addr[0] == 0x00)) {
		int leaf_num = btrfs_seek_inode_where_leaf(btrfs, inum, 0);
		leaf_addr[0] = btrfs->fs_leaf_phy_addr[0][leaf_num];
	}

	btrfs_seek_dir_item(btrfs, inum, 0, leaf_addr[0]);
	
	for (idx = 0; idx < btrfs->tmp_cnt; idx++) {
		unsigned int namelen;
		
		btrfs_dir_item* dir_item = btrfs->tmp_dir_item[idx];

		inode = tsk_getu64(fs->endian, dir_item->location.objectid);
		namelen = tsk_getu16(fs->endian, dir_item->name_len);

		/*
		 ** Check if we may have a valid directory entry.  If we don't,
		 ** then increment to the next word and try again.
		 */
		if ((inode > fs->last_inum) || (namelen > BTRFS_MAXNAMLEN) || (namelen == 0))
			continue;

		if (btrfs_dent_copy(btrfs, dir_item, fs_name)) {
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

static TSK_RETVAL_ENUM
btrfs_dent_parse(BTRFS_INFO * btrfs, TSK_FS_DIR * a_fs_dir, uint8_t a_is_del, TSK_LIST ** list_seen, char *buf, TSK_OFF_T offset)
{
	return -1;
}

/** \internal
* Process a directory and load up FS_DIR with the entries. If a pointer to
* an already allocated FS_DIR structure is given, it will be cleared.  If no existing
* FS_DIR structure is passed (i.e. NULL), then a new one will be created. If the return
* value is error or corruption, then the FS_DIR structure could
* have entries (depending on when the error occurred).
*
* @param a_fs File system to analyze
* @param a_fs_dir Pointer to FS_DIR pointer. Can contain an already allocated
* structure or a new structure.
* @param a_addr Address of directory to process.
* @returns error, corruption, ok etc.
*/
TSK_RETVAL_ENUM
btrfs_dir_open_meta(TSK_FS_INFO * a_fs, TSK_FS_DIR ** a_fs_dir,
	TSK_INUM_T a_addr)
{
	BTRFS_INFO *btrfs = (BTRFS_INFO *)a_fs;
	char *dirbuf;
	TSK_OFF_T size;
	TSK_FS_DIR *fs_dir;
	TSK_LIST *list_seen = NULL;

	/* If we get corruption in one of the blocks, then continue processing.
	 * retval_final will change when corruption is detected.  Errors are
	 * returned immediately. */
	TSK_RETVAL_ENUM retval_tmp;
	TSK_RETVAL_ENUM retval_final = TSK_OK;

	if (a_addr < a_fs->first_inum || a_addr > a_fs->last_inum) {
		tsk_error_reset();
		tsk_error_set_errno(TSK_ERR_FS_WALK_RNG);
		tsk_error_set_errstr("btrfs_dir_open_meta: inode value: %"
			PRIuINUM "\n", a_addr);
		return TSK_ERR;
	}
	else if (a_fs_dir == NULL) {
		tsk_error_reset();
		tsk_error_set_errno(TSK_ERR_FS_ARG);
		tsk_error_set_errstr
		("btrfs_dir_open_meta: NULL fs_attr argument given");
		return TSK_ERR;
	}

	if (tsk_verbose) {
		tsk_fprintf(stderr,
			"btrfs_dir_open_meta: Processing directory %" PRIuINUM
			"\n", a_addr);
	}

	fs_dir = *a_fs_dir;
	if (fs_dir) {
		tsk_fs_dir_reset(fs_dir);
		fs_dir->addr = a_addr;
	}
	else {
		if ((*a_fs_dir = fs_dir =
			tsk_fs_dir_alloc(a_fs, a_addr, 128)) == NULL) {
			return TSK_ERR;
		}
	}

	//  handle the orphan directory if its contents were requested
	if (a_addr == TSK_FS_ORPHANDIR_INUM(a_fs)) {
		return tsk_fs_dir_find_orphans(a_fs, fs_dir);
	}
	else {
	}

	if ((fs_dir->fs_file =
		tsk_fs_file_open_meta(a_fs, NULL, a_addr)) == NULL) {
		tsk_error_reset();
		tsk_error_errstr2_concat("- btrfs_dir_open_meta");
		return TSK_COR;
	}

	retval_tmp =
		btrfs_dent_parse_leaf(btrfs, fs_dir,
		(fs_dir->fs_file->meta->
			flags & TSK_FS_META_FLAG_UNALLOC) ? 1 : 0, &list_seen,
			fs_dir->fs_file->meta->content_ptr);

	if (retval_tmp == TSK_ERR) {
		retval_final = TSK_ERR;
	}
	else if (retval_tmp == TSK_COR) {
		retval_final = TSK_COR;
	}

	return retval_final;
}