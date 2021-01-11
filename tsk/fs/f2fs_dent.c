#include <ctype.h>
#include "tsk_fs_i.h"
#include "tsk_f2fs.h"

static uint8_t
f2fs_dent_copy(F2FS_INFO * f2fs, 
    char *f2fs_dent, char *f2fs_name, TSK_FS_NAME * fs_name, int len)
{
    TSK_FS_INFO *fs = &(f2fs->fs_info);
    f2fs_dir_entry *dir = (f2fs_dir_entry *) f2fs_dent;

    if (len >= fs_name->name_size) {
        tsk_error_reset();
        tsk_error_set_errno(TSK_ERR_FS_ARG);
        tsk_error_set_errstr
            ("f2fs_dent_copy: Name Space too Small %d %" PRIuSIZE "",
            tsk_getu16(fs->endian, dir->name_len), fs_name->name_size);
        return 1;
    }
    
    strncpy(fs_name->name, f2fs_name, len);
    fs_name->name[len] = '\0';

    switch (dir->file_type) {
        case F2FS_DE_REG:
            fs_name->type = TSK_FS_NAME_TYPE_REG;
            break;
        case F2FS_DE_DIR:
            fs_name->type = TSK_FS_NAME_TYPE_DIR;
            break;
        case F2FS_DE_CHR:
            fs_name->type = TSK_FS_NAME_TYPE_CHR;
            break;
        case F2FS_DE_BLK:
            fs_name->type = TSK_FS_NAME_TYPE_BLK;
            break;
        case F2FS_DE_FIFO:
            fs_name->type = TSK_FS_NAME_TYPE_FIFO;
            break;
        case F2FS_DE_SOCK:
            fs_name->type = TSK_FS_NAME_TYPE_SOCK;
            break;
        case F2FS_DE_LNK:
            fs_name->type = TSK_FS_NAME_TYPE_LNK;
            break;
        case F2FS_DE_UNKNOWN:
        default:
            fs_name->type = TSK_FS_NAME_TYPE_UNDEF;
            break;
    }
    fs_name->flags = 0;

    return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/*
 * @param a_is_del Set to 1 if block is from a deleted directory.
 */
static TSK_RETVAL_ENUM
f2fs_dent_parse_block(F2FS_INFO * f2fs, TSK_FS_DIR * a_fs_dir,
    uint8_t a_is_del, TSK_LIST ** list_seen, uint8_t *buf, int len)
{
    TSK_FS_INFO *fs = &(f2fs->fs_info);
    int remainder;
    uint32_t inode;
    uint32_t ha;
    uint8_t v;
    f2fs_dir_entry *dirp;
    TSK_FS_NAME *fs_name;
    unsigned int namelen;
	char * name;

    if ((fs_name = tsk_fs_name_alloc(F2FS_NAME_LEN + 1, 0)) == NULL)
        return TSK_ERR;

	dirp = (f2fs_dir_entry *)tsk_malloc(sizeof(f2fs_dir_entry) * ((F2FS_DENTRY_MAX / 11)+2));
	name = (char *)tsk_malloc(255);
	int j = F2FS_DENTRY_MAX;
	
	for (int i = 0; i < 180; i++)
	{
		dirp[i].hash_code[0] = buf[i * 11];
		dirp[i].hash_code[1] = buf[(i * 11) + 1];
		dirp[i].hash_code[2] = buf[(i * 11) + 2];
		dirp[i].hash_code[3] = buf[(i * 11) + 3];
		dirp[i].ino[0] = buf[(i * 11) + 4];
		dirp[i].ino[1] = buf[(i * 11) + 5];
		dirp[i].ino[2] = buf[(i * 11) + 6];
		dirp[i].ino[3] = buf[(i * 11) + 7];
		dirp[i].name_len[0] = buf[(i * 11) + 8];
		dirp[i].name_len[1] = buf[(i * 11) + 9];
		dirp[i].file_type = buf[(i * 11) + 10];

		inode = tsk_getu32(fs->endian, dirp[i].ino);
		namelen = tsk_getu16(fs->endian, dirp[i].name_len);
		ha = tsk_getu32(fs->endian, dirp[i].hash_code);
		v = dirp[i].file_type;


		if (inode == 0 || namelen == 0)
			continue;

		fs_name->meta_addr = inode;
		int k = 0;
		while(1){
			name[k] = buf[j];
			k++;
			j++;

			if (k == namelen) {
				name[k] = '\0';
				break;
			}
		}
		remainder = namelen % 8;
		if (remainder != 0)
			j += (8 - remainder);
		if (f2fs_dent_copy(f2fs, &dirp[i], name, fs_name, namelen)) {
			free(name);
			free(dirp);
			tsk_fs_name_free(fs_name);
			return TSK_ERR;
		}

		fs_name->flags = TSK_FS_NAME_FLAG_ALLOC;

		if (tsk_fs_dir_add(a_fs_dir, fs_name)) {
			tsk_fs_name_free(fs_name);
			return TSK_ERR;
		}
	}
	free(dirp);
	free(name);
    tsk_fs_name_free(fs_name);
    return TSK_OK;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
TSK_RETVAL_ENUM
f2fs_dir_open_meta(TSK_FS_INFO * a_fs, TSK_FS_DIR ** a_fs_dir,
    TSK_INUM_T a_addr)
{
    F2FS_INFO *f2fs = (F2FS_INFO *) a_fs;
    uint8_t *dirbuf;
    TSK_OFF_T size;
    TSK_FS_DIR *fs_dir;
    TSK_LIST *list_seen = NULL;

    /* If we get corruption in one of the blocks, then continue processing.
     * retval_final will change when corruption is detected.  Errors are
     * returned immediately. */
    TSK_RETVAL_ENUM retval_tmp;
    TSK_RETVAL_ENUM retval_final = TSK_OK;
    if (a_fs_dir == NULL) {
        tsk_error_reset();
        tsk_error_set_errno(TSK_ERR_FS_ARG);
        tsk_error_set_errstr
            ("f2fs_dir_open_meta: NULL fs_attr argument given");
        return TSK_ERR;
    }

    if (tsk_verbose) {
        tsk_fprintf(stderr,
            "f2fs_dir_open_meta: Processing directory %" PRIuINUM
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

    if ((fs_dir->fs_file =
            tsk_fs_file_open_meta(a_fs, NULL, a_addr)) == NULL) {
        printf("open_meta\n");
        tsk_error_reset();
        tsk_error_errstr2_concat("- f2fs_dir_open_meta");
        return TSK_COR;
    }

    ssize_t len = sizeof(f2fs->fs_dentry->dentry) + sizeof(f2fs->fs_dentry->filename);

    dirbuf = fs_dir->fs_file->meta->content_ptr;

    retval_tmp =
        f2fs_dent_parse_block(f2fs, fs_dir,
        (fs_dir->fs_file->meta->
            flags & TSK_FS_META_FLAG_UNALLOC) ? 1 : 0, &list_seen,
        dirbuf, len);

    
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


