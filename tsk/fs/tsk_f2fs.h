#ifndef _TSK_EXT2FS_H
#define _TSK_EXT2FS_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

extern TSK_RETVAL_ENUM f2fs_dir_open_meta(TSK_FS_INFO * a_fs, TSK_FS_DIR ** a_fs_dir, TSK_INUM_T a_addr);


#define F2FS_IN_FMT  0170000
#define F2FS_IN_SOCK 0140000
#define F2FS_IN_LNK  0120000
#define F2FS_IN_REG  0100000
#define F2FS_IN_BLK  0060000
#define F2FS_IN_DIR  0040000
#define F2FS_IN_CHR  0020000
#define F2FS_IN_FIFO  0010000

#define F2FS_IN_ISUID   0004000
#define F2FS_IN_ISGID   0002000
#define F2FS_IN_ISVTX   0001000
#define F2FS_IN_IRUSR   0000400
#define F2FS_IN_IWUSR   0000200
#define F2FS_IN_IXUSR   0000100
#define F2FS_IN_IRGRP   0000040
#define F2FS_IN_IWGRP   0000020
#define F2FS_IN_IXGRP   0000010
#define F2FS_IN_IROTH   0000004
#define F2FS_IN_IWOTH   0000002
#define F2FS_IN_IXOTH   0000001

#define F2FS_DE_UNKNOWN         0
#define F2FS_DE_REG        1
#define F2FS_DE_DIR             2
#define F2FS_DE_CHR          3
#define F2FS_DE_BLK          4
#define F2FS_DE_FIFO            5
#define F2FS_DE_SOCK            6
#define F2FS_DE_LNK         7
#define F2FS_DE_MAX             8

#define F2FS_FS_MAGIC			0xf2f52010
#define F2FS_SUPER_OFFSET		1024	/* byte-size offset */
#define F2FS_MIN_LOG_SECTOR_SIZE	9	/* 9 bits for 512 bytes */
#define F2FS_MAX_LOG_SECTOR_SIZE	12	/* 12 bits for 4096 bytes */
#define F2FS_LOG_SECTORS_PER_BLOCK	3	/* log number for sector/blk */
#define F2FS_BLKSIZE			0x1000	/* support only 4KB block */
#define F2FS_BLKSIZE_BITS		12	/* bits for F2FS_BLKSIZE */
#define F2FS_MAX_EXTENSION		64	/* # of extension entries */
#define F2FS_EXTENSION_LEN		8	/* max size of extension */
#define F2FS_BLK_ALIGN(x)	(((x) + F2FS_BLKSIZE - 1) >> F2FS_BLKSIZE_BITS)


#define NULL_ADDR		((block_t)0)	/* used as block_t addresses */
#define NEW_ADDR		((block_t)-1)	/* used as block_t addresses */

#define F2FS_BYTES_TO_BLK(bytes)	((bytes) >> F2FS_BLKSIZE_BITS)
#define F2FS_BLK_TO_BYTES(blk)		((blk) << F2FS_BLKSIZE_BITS)

/* 0, 1(node nid), 2(meta nid) are reserved node id */
#define F2FS_RESERVED_NODE_NUM		3

#define F2FS_MAX_QUOTAS		3

#define F2FS_IO_SIZE(sbi)	(1 << F2FS_OPTION(sbi).write_io_size_bits) /* Blocks */
#define F2FS_IO_SIZE_KB(sbi)	(1 << (F2FS_OPTION(sbi).write_io_size_bits + 2)) /* KB */
#define F2FS_IO_SIZE_BYTES(sbi)	(1 << (F2FS_OPTION(sbi).write_io_size_bits + 12)) /* B */
#define F2FS_IO_SIZE_BITS(sbi)	(F2FS_OPTION(sbi).write_io_size_bits) /* power of 2 */
#define F2FS_IO_SIZE_MASK(sbi)	(F2FS_IO_SIZE(sbi) - 1)

/* This flag is used by node and meta inodes, and by recovery */
#define GFP_F2FS_ZERO		(GFP_NOFS | __GFP_ZERO)

#define MAX_ACTIVE_LOGS	16
#define MAX_ACTIVE_NODE_LOGS	8
#define MAX_ACTIVE_DATA_LOGS	8

#define F2FS_VERSION_LEN	256
#define F2FS_MAX_VOLUME_NAME		512
#define F2FS_MAX_PATH_LEN		64
#define F2FS_MAX_DEVICES		8

#define F2FS_DENTRY_START 0x1A0
#define F2FS_DENTRY_FILENAME 0x96C

typedef struct f2fs_device {
	uint8_t path[F2FS_MAX_PATH_LEN];
	uint8_t total_segments[4];
}f2fs_device;

typedef struct {
	uint8_t magic[4];			/* Magic Number */
	uint8_t major_ver[2];		/* Major Version */
	uint8_t minor_ver[2];		/* Minor Version */
	uint8_t log_sectorsize[4];		/* log2 sector size in bytes */
	uint8_t log_sectors_per_block[4];	/* log2 # of sectors per block */
	uint8_t log_blocksize[4];		/* log2 block size in bytes */
	uint8_t log_blocks_per_seg[4];	/* log2 # of blocks per segment */
	uint8_t segs_per_sec[4];		/* # of segments per section */
	uint8_t secs_per_zone[4];		/* # of sections per zone */
	uint8_t checksum_offset[4];		/* checksum offset inside super block */
	uint8_t block_count[8];		/* total # of user blocks */
	uint8_t section_count[4];		/* total # of sections */
	uint8_t segment_count[4];		/* total # of segments */
	uint8_t segment_count_ckpt[4];	/* # of segments for checkpoint */
	uint8_t segment_count_sit[4];	/* # of segments for SIT */
	uint8_t segment_count_nat[4];	/* # of segments for NAT */
	uint8_t segment_count_ssa[4];	/* # of segments for SSA */
	uint8_t segment_count_main[4];	/* # of segments for main area */
	uint8_t segment0_blkaddr[4];	/* start block address of segment 0 */
	uint8_t cp_blkaddr[4];		/* start block address of checkpoint */
	uint8_t sit_blkaddr[4];		/* start block address of SIT */
	uint8_t nat_blkaddr[4];		/* start block address of NAT */
	uint8_t ssa_blkaddr[4];		/* start block address of SSA */
	uint8_t main_blkaddr[4];		/* start block address of main area */
	uint8_t root_ino[4];		/* root inode number */
	uint8_t node_ino[4];		/* node inode number */
	uint8_t meta_ino[4];		/* meta inode number */
	uint8_t uuid[16];			/* 128-bit uuid for volume */
	uint16_t volume_name[F2FS_MAX_VOLUME_NAME];	/* volume name */
	uint8_t extension_count[4];		/* # of extensions below */
	uint8_t extension_list[F2FS_MAX_EXTENSION][F2FS_EXTENSION_LEN];/* extension array */
	uint8_t cp_payload[4];
	uint8_t version[F2FS_VERSION_LEN];	/* the kernel version */
	uint8_t init_version[F2FS_VERSION_LEN];	/* the initial kernel version */
	uint8_t feature[4];			/* defined features */
	uint8_t encryption_level;		/* versioning level for encryption */
	uint8_t encrypt_pw_salt[16];	/* Salt used for string2key algorithm */
	f2fs_device devs[F2FS_MAX_DEVICES];	/* device list */
	uint32_t qf_ino[F2FS_MAX_QUOTAS];	/* quota inode numbers */
	uint8_t hot_ext_count;		/* # of hot file extension */
	uint8_t reserved[310];		/* valid reserved region */
	uint8_t crc[4];			/* checksum of superblock */
} f2fs_sb;

/*
 * For checkpoint
 */
#define CP_DISABLED_QUICK_FLAG		0x00002000
#define CP_DISABLED_FLAG		0x00001000
#define CP_QUOTA_NEED_FSCK_FLAG		0x00000800
#define CP_LARGE_NAT_BITMAP_FLAG	0x00000400
#define CP_NOCRC_RECOVERY_FLAG	0x00000200
#define CP_TRIMMED_FLAG		0x00000100
#define CP_NAT_BITS_FLAG	0x00000080
#define CP_CRC_RECOVERY_FLAG	0x00000040
#define CP_FASTBOOT_FLAG	0x00000020
#define CP_FSCK_FLAG		0x00000010
#define CP_ERROR_FLAG		0x00000008
#define CP_COMPACT_SUM_FLAG	0x00000004
#define CP_ORPHAN_PRESENT_FLAG	0x00000002
#define CP_UMOUNT_FLAG		0x00000001

#define F2FS_CP_PACKS		2	/* # of checkpoint packs */

typedef struct f2fs_checkpoint {
	uint8_t checkpoint_ver[8];		/* checkpoint block version number */
	uint8_t user_block_count[8];	/* # of user blocks */
	uint8_t valid_block_count[8];	/* # of valid blocks in main area */
	uint8_t rsvd_segment_count[4];	/* # of reserved segments for gc */
	uint8_t overprov_segment_count[4];	/* # of overprovision segments */
	uint8_t free_segment_count[4];	/* # of free segments in main area */

	/* information of current node segments */
	uint32_t cur_node_segno[MAX_ACTIVE_NODE_LOGS];
	uint16_t cur_node_blkoff[MAX_ACTIVE_NODE_LOGS];
	/* information of current data segments */
	uint32_t cur_data_segno[MAX_ACTIVE_DATA_LOGS];
	uint16_t cur_data_blkoff[MAX_ACTIVE_DATA_LOGS];
	uint8_t ckpt_flags[4];		/* Flags : umount and journal_present */
	uint8_t cp_pack_total_block_count[4];	/* total # of one cp pack */
	uint8_t cp_pack_start_sum[4];	/* start block number of data summary */
	uint8_t valid_node_count[4];	/* Total number of valid nodes */
	uint8_t valid_inode_count[4];	/* Total number of valid inodes */
	uint8_t next_free_nid[4];		/* Next free node number */
	uint8_t sit_ver_bitmap_bytesize[4];	/* Default value 64 */
	uint8_t nat_ver_bitmap_bytesize[4]; /* Default value 256 */
	uint8_t checksum_offset[4];		/* checksum offset inside cp block */
	uint8_t elapsed_time[8];		/* mounted time */
	/* allocation type of current segment */
	unsigned char alloc_type[MAX_ACTIVE_LOGS];

	/* SIT and NAT version bitmap */
	unsigned char sit_nat_version_bitmap[1];
}f2fs_checkpoint;


typedef struct f2fs_extent {
	uint8_t fofs[4];		/* start file offset of the extent */
	uint8_t blk[4];		/* start block address of the extent */
	uint8_t len[4];		/* length of the extent */
}f2fs_extent;

#define F2FS_NAME_LEN		255
/* 200 bytes for inline xattrs by default */
#define DEFAULT_INLINE_XATTR_ADDRS	50
#define DEF_ADDRS_PER_INODE	923	/* Address Pointers in an Inode */
#define CUR_ADDRS_PER_INODE(inode)	(DEF_ADDRS_PER_INODE - \
					get_extra_isize(inode))
#define DEF_NIDS_PER_INODE	5	/* Node IDs in an Inode */
#define ADDRS_PER_INODE(inode)	addrs_per_inode(inode)
#define DEF_ADDRS_PER_BLOCK	1018	/* Address Pointers in a Direct Block */
#define ADDRS_PER_BLOCK(inode)	addrs_per_block(inode)
#define NIDS_PER_BLOCK		1018	/* Node IDs in an Indirect Block */

#define ADDRS_PER_PAGE(page, inode)	\
	(IS_INODE(page) ? ADDRS_PER_INODE(inode) : ADDRS_PER_BLOCK(inode))

#define	NODE_DIR1_BLOCK		(DEF_ADDRS_PER_INODE + 1)
#define	NODE_DIR2_BLOCK		(DEF_ADDRS_PER_INODE + 2)
#define	NODE_IND1_BLOCK		(DEF_ADDRS_PER_INODE + 3)
#define	NODE_IND2_BLOCK		(DEF_ADDRS_PER_INODE + 4)
#define	NODE_DIND_BLOCK		(DEF_ADDRS_PER_INODE + 5)

#define F2FS_INLINE_XATTR	0x01	/* file inline xattr flag */
#define F2FS_INLINE_DATA	0x02	/* file inline data flag */
#define F2FS_INLINE_DENTRY	0x04	/* file inline dentry flag */
#define F2FS_DATA_EXIST		0x08	/* file inline data exist flag */
#define F2FS_INLINE_DOTS	0x10	/* file having implicit dot dentries */
#define F2FS_EXTRA_ATTR		0x20	/* file having extra attribute */
#define F2FS_PIN_FILE		0x40	/* file should not be gced */

typedef struct node_footer {
	uint8_t nid[4];		/* node id */
	uint8_t ino[4];		/* inode number */
	uint8_t flag[4];		/* include cold/fsync/dentry marks and offset */
	uint8_t cp_ver[8];		/* checkpoint version */
	uint8_t next_blkaddr[4];	/* next node page block address */
}node_footer;

#define F2FS_DENTRY_MAX 0x7BC
#define F2FS_DENTRY_FN_ARY 205

typedef struct f2fs_dir_entry {
	uint8_t hash_code[4];	/* hash code of file name */
	uint8_t ino[4];		/* inode number */
	uint8_t name_len[2];	/* length of file name */
	uint8_t file_type;		/* file type */
}f2fs_dir_entry;

typedef struct f2fs_dentry_block {
    uint8_t reserved[56];  //bitmap
    f2fs_dir_entry dentry[F2FS_DENTRY_MAX / 11];
    uint8_t reserved2[16];
    uint8_t filename[F2FS_DENTRY_FN_ARY][8];
}f2fs_dentry_block;

typedef struct _i_nid {
	uint8_t addr[5][4];
} f2fs_i_nid;

typedef struct f2fs_inode {
	uint8_t i_mode[2];			/* file mode */
	uint8_t i_advise[1];			/* file hints */
	uint8_t i_inline[1];			/* file inline flags */
	uint8_t i_uid[4];			/* user ID */
	uint8_t i_gid[4];			/* group ID */
	uint8_t i_links[4];			/* links count */
	uint8_t i_size[8];			/* file size in bytes */
	uint8_t i_blocks[8];		/* file size in blocks */
	uint8_t i_atime[8];			/* access time */
	uint8_t i_ctime[8];			/* change time */
	uint8_t i_mtime[8];			/* modification time */
	uint8_t i_atime_nsec[4];		/* access time in nano scale */
	uint8_t i_ctime_nsec[4];		/* change time in nano scale */
	uint8_t i_mtime_nsec[4];		/* modification time in nano scale */
	uint8_t i_generation[4];		/* file version (for NFS) */
	union {
		uint8_t i_current_depth[4];	/* only for directory depth */
		uint8_t i_gc_failures[2];	/*
					 * # of gc failures on pinned file.
					 * only for regular files.
					 */
	};
	uint8_t i_xattr_nid[4];		/* nid to save xattr */
	uint8_t i_flags[4];			/* file attributes */
	uint8_t i_pino[4];			/* parent inode number */
	uint8_t i_namelen[4];		/* file name length */
	uint8_t i_name[F2FS_NAME_LEN];	/* file name for SPOR */
	uint8_t i_dir_level;		/* dentry_level for large dir */

	f2fs_extent i_ext;	/* caching a largest extent */

    union {
		struct {
			uint8_t i_extra_isize[2];	/* extra inode attribute size */
			uint8_t i_inline_xattr_size[2];	/* inline xattr size, unit: 4 bytes */
			uint8_t i_projid[4];	/* project id */
			uint8_t i_inode_checksum[4];/* inode meta checksum */
			uint8_t i_crtime[8];	/* creation time */
			uint8_t i_crtime_nsec[4];	/* creation time in nano scale */
			uint32_t i_extra_end[0];	/* for attribute size calculation */
		};
		uint8_t i_addr[DEF_ADDRS_PER_INODE][4];	/* Pointers to data blocks */
    	f2fs_dentry_block i_dentry;
		uint8_t i_reldata[DEF_ADDRS_PER_INODE*4];
	};
	f2fs_i_nid i_nid;	/* direct(2), indirect(2),
						double_indirect(1) node id */
    struct node_footer i_footer;
}f2fs_inode;



typedef struct direct_node {
	uint8_t addr[DEF_ADDRS_PER_BLOCK][4];	/* array of data block address */
    node_footer i_footer;
}direct_node;

typedef struct indirect_node {
	uint8_t nid[NIDS_PER_BLOCK][4];	/* array of data block address */
    struct node_footer i_footer;
}indirect_node;

#define OFFSET_BIT_MASK		(0x07)	/* (0x01 << OFFSET_BIT_SHIFT) - 1 */

typedef struct f2fs_node {
	/* can be one of three types: inode, direct, and indirect types */
	union {
		f2fs_inode i;
		direct_node dn;
		indirect_node in;
	};
	struct node_footer footer;
}f2fs_node;




#define NAT_ENTRY_PER_BLOCK (F2FS_BLKSIZE / 9)

typedef struct f2fs_nat_entry {
	uint8_t version;		
	uint8_t ino[4];		
	uint8_t block_addr[4];
} f2fs_nat_entry;

typedef struct f2fs_nat_block {
	f2fs_nat_entry *entries;
} f2fs_nat_block;


typedef struct {
        TSK_FS_INFO fs_info;    /* super class */
        f2fs_sb *fs;          /* super block */
		f2fs_checkpoint *fs_cp; /*checkpoint block*/
        f2fs_nat_block *fs_nat;
		
        f2fs_dentry_block *fs_dentry;
        int nat_inode_index;
		TSK_OFF_T cp_offset;
		TSK_OFF_T nat_offset;  //nat offset
		uint8_t tmp_addr[DEF_ADDRS_PER_INODE][4];
        uint16_t inode_size;    /* size of each inode */
        TSK_DADDR_T first_data_block;

        tsk_lock_t lock;
} F2FS_INFO;

#ifdef __cplusplus
}
#endif
#endif