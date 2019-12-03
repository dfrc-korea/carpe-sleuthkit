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

#ifndef _TSK_BTRFS_H
#define _TSK_BTRFS_H

#include <stdbool.h>
#include "tsk_fs_i.h"

#ifdef __cplusplus
extern "C" {
#endif


extern TSK_RETVAL_ENUM
btrfs_dir_open_meta(TSK_FS_INFO * a_fs, TSK_FS_DIR ** a_fs_dir,
	TSK_INUM_T a_addr);

/* MODE */
#define BTRFS_IN_ISUID   0004000
#define BTRFS_IN_ISGID   0002000
#define BTRFS_IN_ISVTX   0001000
#define BTRFS_IN_IRUSR   0000400
#define BTRFS_IN_IWUSR   0000200
#define BTRFS_IN_IXUSR   0000100
#define BTRFS_IN_IRGRP   0000040
#define BTRFS_IN_IWGRP   0000020
#define BTRFS_IN_IXGRP   0000010
#define BTRFS_IN_IROTH   0000004
#define BTRFS_IN_IWOTH   0000002
#define BTRFS_IN_IXOTH   0000001


#define BTRFS_MAGIC 0x4D5F53665248425FULL /* ascii _BHRfS_M, no null */
#define BTRFS_SUPER_INFO_OFFSET (64 * 1024)
#define BTRFS_SUPER_INFO_SIZE 16384
#define BTRFS_MAX_LEAF_SIZE 16384
#define BTRFS_BLOCK_SHIFT 14
#define BTRFS_BLOCK_SIZE  (1 << BTRFS_BLOCK_SHIFT)
#define BTRFS_MAX_NUM_STRIPE 0x20
#define BTRFS_FILE_CONTENT_LEN 16384

#define BTRFS_MAXNAMLEN 255
/*
* Fake signature for an unfinalized filesystem, which only has barebone tree
* structures (normally 6 near empty trees, on SINGLE meta/sys temporary chunks)
*
* ascii !BHRfS_M, no null
*/
#define BTRFS_MAGIC_TEMPORARY 0x4D5F536652484221ULL

#define BTRFS_MAX_MIRRORS 3

#define BTRFS_MAX_LEVEL 8

/* holds pointers to all of the tree roots */
#define BTRFS_ROOT_TREE_OBJECTID 1ULL

/* stores information about which extents are in use, and reference counts */
#define BTRFS_EXTENT_TREE_OBJECTID 2ULL

/*
 * chunk tree stores translations from logical -> physical block numbering
 * the super block points to the chunk tree
 */
#define BTRFS_CHUNK_TREE_OBJECTID 3ULL

 /*
  * stores information about which areas of a given device are in use.
  * one per device.  The tree of tree roots points to the device tree
  */
#define BTRFS_DEV_TREE_OBJECTID 4ULL

  /* one per subvolume, storing files and directories */
#define BTRFS_FS_TREE_OBJECTID 5ULL

/* directory objectid inside the root tree */
#define BTRFS_ROOT_TREE_DIR_OBJECTID 6ULL
/* holds checksums of all the data extents */
#define BTRFS_CSUM_TREE_OBJECTID 7ULL
#define BTRFS_QUOTA_TREE_OBJECTID 8ULL

/* for storing items that use the BTRFS_UUID_KEY* */
#define BTRFS_UUID_TREE_OBJECTID 9ULL

/* tracks free space in block groups. */
#define BTRFS_FREE_SPACE_TREE_OBJECTID 10ULL

/* device stats in the device tree */
#define BTRFS_DEV_STATS_OBJECTID 0ULL

/* for storing balance parameters in the root tree */
#define BTRFS_BALANCE_OBJECTID -4ULL

/* orphan objectid for tracking unlinked/truncated files */
#define BTRFS_ORPHAN_OBJECTID -5ULL

/* does write ahead logging to speed up fsyncs */
#define BTRFS_TREE_LOG_OBJECTID -6ULL
#define BTRFS_TREE_LOG_FIXUP_OBJECTID -7ULL

/* space balancing */
#define BTRFS_TREE_RELOC_OBJECTID -8ULL
#define BTRFS_DATA_RELOC_TREE_OBJECTID -9ULL

/*
 * extent checksums all have this objectid
 * this allows them to share the logging tree
 * for fsyncs
 */
#define BTRFS_EXTENT_CSUM_OBJECTID -10ULL

 /* For storing free space cache */
#define BTRFS_FREE_SPACE_OBJECTID -11ULL

/*
 * The inode number assigned to the special inode for storing
 * free ino cache
 */
#define BTRFS_FREE_INO_OBJECTID -12ULL

 /* dummy objectid represents multiple objectids */
#define BTRFS_MULTIPLE_OBJECTIDS -255ULL

/*
 * All files have objectids in this range.
 */
#define BTRFS_FIRST_FREE_OBJECTID 256ULL
#define BTRFS_LAST_FREE_OBJECTID -256ULL
#define BTRFS_FIRST_CHUNK_TREE_OBJECTID 256ULL



 /*
  * the device items go into the chunk tree.  The key is in the form
  * [ 1 BTRFS_DEV_ITEM_KEY device_id ]
  */
#define BTRFS_DEV_ITEMS_OBJECTID 1ULL

#define BTRFS_EMPTY_SUBVOL_DIR_OBJECTID 2ULL

  /*
   * the max metadata block size.  This limit is somewhat artificial,
   * but the memmove costs go through the roof for larger blocks.
   */
#define BTRFS_MAX_METADATA_BLOCKSIZE 65536

   /*
	* we can actually store much bigger names, but lets not confuse the rest
	* of linux
	*/
#define BTRFS_NAME_LEN 255

	/*
	 * Theoretical limit is larger, but we keep this down to a sane
	 * value. That should limit greatly the possibility of collisions on
	 * inode ref items.
	 */
#define	BTRFS_LINK_MAX	65535U

	 /* 32 bytes in various csum fields */
#define BTRFS_CSUM_SIZE 32
#define BTRFS_FSID_SIZE 16

/* csum types */
#define BTRFS_CSUM_TYPE_CRC32	0

#define BTRFS_EMPTY_DIR_SIZE 0

#define BTRFS_IN_FMT  0170000
#define BTRFS_IN_SOCK 0140000
#define BTRFS_IN_LNK  0120000
#define BTRFS_IN_REG  0100000
#define BTRFS_IN_BLK  0060000
#define BTRFS_IN_DIR  0040000
#define BTRFS_IN_CHR  0020000
#define BTRFS_IN_FIFO  0010000

#define BTRFS_FT_UNKNOWN	0
#define BTRFS_FT_REG_FILE	1
#define BTRFS_FT_DIR		2
#define BTRFS_FT_CHRDEV		3
#define BTRFS_FT_BLKDEV		4
#define BTRFS_FT_FIFO		5
#define BTRFS_FT_SOCK		6
#define BTRFS_FT_SYMLINK	7
#define BTRFS_FT_XATTR		8

#define BTRFS_DE_UNKNOWN	0
#define BTRFS_DE_REG		1
#define BTRFS_DE_DIR		2
#define BTRFS_DE_CHR		3
#define BTRFS_DE_BLK		4
#define BTRFS_DE_FIFO		5
#define BTRFS_DE_SOCK       6
#define BTRFS_DE_LNK		7
#define BTRFS_DE_MAX		8

#define BTRFS_ROOT_SUBVOL_RDONLY	(1ULL << 0)

/*
	* the key defines the order in the tree, and so it also defines (optimal)
	* block layout.  objectid corresponds to the inode number.  The flags
	* tells us things about the object, and is a kind of stream selector.
	* so for a given inode, keys with flags of 1 might refer to the inode
	* data, flags of 2 may point to file data in the btree and flags == 3
	* may point to extents.
	*
	* offset is the starting byte offset for this key in the stream.
	*
	* btrfs_disk_key is in disk byte order.  typedef struct _btrfs_key is always
	* in cpu native order.  Otherwise they are identical and their sizes
	* should be the same (ie both packed)
	*/

typedef enum _btrfs_dev_stat_values {
		BTRFS_DEV_STAT_WRITE_ERRS, BTRFS_DEV_STAT_READ_ERRS, BTRFS_DEV_STAT_FLUSH_ERRS, BTRFS_DEV_STAT_CORRUPTION_ERRS,
		BTRFS_DEV_STAT_GENERATION_ERRS, BTRFS_DEV_STAT_VALUES_MAX
} btrfs_dev_stat_values;


typedef struct _btrfs_disk_key {
	uint8_t objectid[8];
	uint8_t type;
	uint8_t offset[8];
} btrfs_disk_key;

typedef struct _btrfs_key {
	uint8_t objectid[8];
	uint8_t type;
	uint8_t offset[8];
} btrfs_key;


#define BTRFS_UUID_SIZE 16

typedef struct _btrfs_dev_item {
	/* the internal btrfs device id */
	uint8_t devid[8];

	/* size of the device */
	uint8_t total_bytes[8];

	/* bytes used */
	uint8_t bytes_used[8];

	/* optimal io alignment for this device */
	uint8_t io_align[4];

	/* optimal io width for this device */
	uint8_t io_width[4];

	/* minimal io size for this device */
	uint8_t sector_size[4];

	/* type and info about this device */
	uint8_t type[8];

	/* expected generation for this device */
	uint8_t generation[8];

	/*
		* starting byte of this partition on the device,
		* to allow for stripe alignment in the future
		*/
	uint8_t start_offset[8];

	/* grouping information for allocation decisions */
	uint8_t dev_group[4];

	/* seek speed 0-100 where 100 is fastest */
	uint8_t seek_speed;

	/* bandwidth 0-100 where 100 is fastest */
	uint8_t bandwidth;

	/* btrfs generated uuid for this device */
	uint8_t uuid[BTRFS_UUID_SIZE];

	/* uuid of FS who owns this device */
	uint8_t fsid[BTRFS_UUID_SIZE];
} btrfs_dev_item;



typedef struct _btrfs_stripe {
	uint8_t devid[8];
	uint8_t offset[8];
	uint8_t dev_uuid[BTRFS_UUID_SIZE];
} btrfs_stripe;



typedef struct _btrfs_chunk {
	/* size of this chunk in bytes */
	uint8_t length[8];

	/* objectid of the root referencing this chunk */
	uint8_t owner[8];

	uint8_t stripe_len[8];
	uint8_t type[8];

	/* optimal io alignment for this chunk */
	uint8_t io_align[4];

	/* optimal io width for this chunk */
	uint8_t io_width[4];

	/* minimal io size for this chunk */
	uint8_t sector_size[4];

	/* 2^16 stripes is quite a lot, a second limit is the size of a single
		* item in the btree
		*/
	uint8_t num_stripes[2];

	/* sub stripes only matter for raid10 */
	uint8_t sub_stripes[2];

	btrfs_stripe stripe[BTRFS_MAX_NUM_STRIPE];
	/* additional stripes go here */
} btrfs_chunk;



#define BTRFS_FREE_SPACE_EXTENT	1
#define BTRFS_FREE_SPACE_BITMAP	2



#define BTRFS_HEADER_FLAG_WRITTEN		(1ULL << 0)
#define BTRFS_HEADER_FLAG_RELOC			(1ULL << 1)
#define BTRFS_SUPER_FLAG_SEEDING		(1ULL << 32)
#define BTRFS_SUPER_FLAG_METADUMP		(1ULL << 33)
#define BTRFS_SUPER_FLAG_METADUMP_V2		(1ULL << 34)
#define BTRFS_SUPER_FLAG_CHANGING_FSID		(1ULL << 35)
#define BTRFS_SUPER_FLAG_CHANGING_FSID_V2	(1ULL << 36)

#define BTRFS_BACKREF_REV_MAX		256
#define BTRFS_BACKREF_REV_SHIFT		56
#define BTRFS_BACKREF_REV_MASK		(((uint8_t)BTRFS_BACKREF_REV_MAX - 1) << \
					 BTRFS_BACKREF_REV_SHIFT)

#define BTRFS_OLD_BACKREF_REV		0
#define BTRFS_MIXED_BACKREF_REV		1

/*
	* every tree block (leaf or node) starts with this header.
	*/
typedef struct _btrfs_header {
	/* these first four must match the super block */
	uint8_t csum[BTRFS_CSUM_SIZE];
	uint8_t fsid[BTRFS_FSID_SIZE]; /* FS specific uuid */
	uint8_t bytenr[8]; /* which block this node is supposed to live in */
	uint8_t flags[8];

	/* allowed to be different from the super from here on down */
	uint8_t chunk_tree_uuid[BTRFS_UUID_SIZE];
	uint8_t generation[8];
	uint8_t owner[8];
	uint8_t nritems[4];
	uint8_t level;
} btrfs_header;

#define __BTRFS_LEAF_DATA_SIZE(bs) ((bs) - sizeof(struct btrfs_header))
#define BTRFS_LEAF_DATA_SIZE(fs_info) \
				(__BTRFS_LEAF_DATA_SIZE(fs_info->nodesize))

/*
* this is a very generous portion of the super block, giving us
* room to translate 14 chunks with 3 stripes each.
*/
#define BTRFS_SYSTEM_CHUNK_ARRAY_SIZE 2048
#define BTRFS_LABEL_SIZE 256

/*
* just in case we somehow lose the roots and are not able to mount,
* we store an array of the roots from previous transactions
* in the super.
*/
#define BTRFS_NUM_BACKUP_ROOTS 4

typedef struct _btrfs_root_backup {
	uint8_t tree_root[8];
	uint8_t tree_root_gen[8];
	
	uint8_t chunk_root[8];
	uint8_t chunk_root_gen[8];

	uint8_t extent_root[8];
	uint8_t extent_root_gen[8];

	uint8_t fs_root[8];
	uint8_t fs_root_gen[8];

	uint8_t dev_root[8];
	uint8_t dev_root_gen[8];

	uint8_t csum_root[8];
	uint8_t csum_root_gen[8];

	uint8_t total_bytes[8];
	uint8_t bytes_used[8];
	uint8_t num_devices[8];

	/* future */
	uint8_t unsed_64[32];

	uint8_t tree_root_level;
	uint8_t chunk_root_level;
	uint8_t extent_root_level;
	uint8_t fs_root_level;
	uint8_t dev_root_level;
	uint8_t csum_root_level;
	/* future and to align */
	uint8_t unused_8[10];
} btrfs_root_backup;

/*
	* the super block basically lists the main trees of the FS
	* it currently lacks any block count etc etc
	*/
typedef struct _btrfs_super_block {
	uint8_t csum[BTRFS_CSUM_SIZE];
	/* the first 3 fields must match struct btrfs_header */
	uint8_t fsid[BTRFS_FSID_SIZE];    /* FS specific uuid */
	uint8_t bytenr[8]; /* this block number */
	uint8_t flags[8];

	/* allowed to be different from the btrfs_header from here own down */
	uint8_t magic[8];
	uint8_t generation[8];
	uint8_t root[8];
	uint8_t chunk_root[8];
	uint8_t log_root[8];

	/* this will help find the new super based on the log root */
	uint8_t log_root_transid[8];
	uint8_t total_bytes[8];
	uint8_t bytes_used[8];
	uint8_t root_dir_objectid[8];
	uint8_t num_devices[8];
	uint8_t sectorsize[4];
	uint8_t nodesize[4];
	/* Unused and must be equal to nodesize */
	uint8_t __unused_leafsize[4];
	uint8_t stripesize[4];
	uint8_t sys_chunk_array_size[4];
	uint8_t chunk_root_generation[8];
	uint8_t compat_flags[8];
	uint8_t compat_ro_flags[8];
	uint8_t incompat_flags[8];
	uint8_t csum_type[2];
	uint8_t root_level;
	uint8_t chunk_root_level;
	uint8_t log_root_level;
	btrfs_dev_item dev_item;

	char label[BTRFS_LABEL_SIZE];

	uint8_t cache_generation[8];
	uint8_t uuid_tree_generation[8];

	uint8_t metadata_uuid[BTRFS_FSID_SIZE];
	/* future expansion */
	uint8_t reserved[224];
	uint8_t sys_chunk_array[BTRFS_SYSTEM_CHUNK_ARRAY_SIZE];
	btrfs_root_backup super_roots[BTRFS_NUM_BACKUP_ROOTS];
} btrfs_super_block;

/*
	* Compat flags that we support.  If any incompat flags are set other than the
	* ones specified below then we will fail to mount
	*/
#define BTRFS_FEATURE_COMPAT_RO_FREE_SPACE_TREE	(1ULL << 0)
	/*
	* Older kernels on big-endian systems produced broken free space tree bitmaps,
	* and btrfs-progs also used to corrupt the free space tree. If this bit is
	* clear, then the free space tree cannot be trusted. btrfs-progs can also
	* intentionally clear this bit to ask the kernel to rebuild the free space
	* tree.
	*/
#define BTRFS_FEATURE_COMPAT_RO_FREE_SPACE_TREE_VALID	(1ULL << 1)

#define BTRFS_FEATURE_INCOMPAT_MIXED_BACKREF	(1ULL << 0)
#define BTRFS_FEATURE_INCOMPAT_DEFAULT_SUBVOL	(1ULL << 1)
#define BTRFS_FEATURE_INCOMPAT_MIXED_GROUPS	(1ULL << 2)
#define BTRFS_FEATURE_INCOMPAT_COMPRESS_LZO	(1ULL << 3)
#define BTRFS_FEATURE_INCOMPAT_COMPRESS_ZSTD	(1ULL << 4)

	  /*
	   * older kernels tried to do bigger metadata blocks, but the
	   * code was pretty buggy.  Lets not let them try anymore.
	   */
#define BTRFS_FEATURE_INCOMPAT_BIG_METADATA     (1ULL << 5)
#define BTRFS_FEATURE_INCOMPAT_EXTENDED_IREF	(1ULL << 6)
#define BTRFS_FEATURE_INCOMPAT_RAID56		(1ULL << 7)
#define BTRFS_FEATURE_INCOMPAT_SKINNY_METADATA	(1ULL << 8)
#define BTRFS_FEATURE_INCOMPAT_NO_HOLES		(1ULL << 9)
#define BTRFS_FEATURE_INCOMPAT_METADATA_UUID    (1ULL << 10)

#define BTRFS_FEATURE_COMPAT_SUPP		0ULL

	   /*
		* The FREE_SPACE_TREE and FREE_SPACE_TREE_VALID compat_ro bits must not be
		* added here until read-write support for the free space tree is implemented in
		* btrfs-progs.
		*/
#define BTRFS_FEATURE_COMPAT_RO_SUPP			\
	(BTRFS_FEATURE_COMPAT_RO_FREE_SPACE_TREE |	\
	 BTRFS_FEATURE_COMPAT_RO_FREE_SPACE_TREE_VALID)

#define BTRFS_FEATURE_INCOMPAT_SUPP			\
	(BTRFS_FEATURE_INCOMPAT_MIXED_BACKREF |		\
	 BTRFS_FEATURE_INCOMPAT_DEFAULT_SUBVOL |	\
	 BTRFS_FEATURE_INCOMPAT_COMPRESS_LZO |		\
	 BTRFS_FEATURE_INCOMPAT_COMPRESS_ZSTD |		\
	 BTRFS_FEATURE_INCOMPAT_BIG_METADATA |		\
	 BTRFS_FEATURE_INCOMPAT_EXTENDED_IREF |		\
	 BTRFS_FEATURE_INCOMPAT_RAID56 |		\
	 BTRFS_FEATURE_INCOMPAT_MIXED_GROUPS |		\
	 BTRFS_FEATURE_INCOMPAT_SKINNY_METADATA |	\
	 BTRFS_FEATURE_INCOMPAT_NO_HOLES |		\
	 BTRFS_FEATURE_INCOMPAT_METADATA_UUID)

/*
* A leaf is full of items. offset and size tell us where to find
* the item in the leaf (relative to the start of the data area)
*/
typedef struct _btrfs_item {
	btrfs_disk_key key;
	uint8_t offset[4];
	uint8_t size[4];
} btrfs_item;

/*
	* leaves have an item area and a data area:
	* [item0, item1....itemN] [free space] [dataN...data1, data0]
	*
	* The data is separate from the items to get the keys closer together
	* during searches.
	*/
typedef struct _btrfs_leaf {
	btrfs_header header;
	btrfs_item items[];
} btrfs_leaf;

/*
	* all non-leaf blocks are nodes, they hold only keys and pointers to
	* other blocks
	*/
typedef struct _btrfs_key_ptr {
	btrfs_disk_key key;
	uint8_t blockptr[8];
	uint8_t generation[8];
} btrfs_key_ptr;

typedef struct _btrfs_node {
	btrfs_header header;
	btrfs_key_ptr ptrs[];
} btrfs_node;

/*
* items in the extent btree are used to record the objectid of the
* owner of the block and the number of references
*/

typedef struct _btrfs_extent_item {
	uint8_t refs[8];
	uint8_t generation[8];
	uint8_t flags[8];
} btrfs_extent_item;

typedef struct _btrfs_extent_item_v0 {
	uint8_t refs[4];
} btrfs_extent_item_v0;

#define BTRFS_MAX_EXTENT_ITEM_SIZE(r) \
			((BTRFS_LEAF_DATA_SIZE(r->fs_info) >> 4) - \
					sizeof(btrfs_item))
#define BTRFS_MAX_EXTENT_SIZE		SZ_128M

#define BTRFS_EXTENT_FLAG_DATA		(1ULL << 0)
#define BTRFS_EXTENT_FLAG_TREE_BLOCK	(1ULL << 1)

/* following flags only apply to tree blocks */

/* use full backrefs for extent pointers in the block*/
#define BTRFS_BLOCK_FLAG_FULL_BACKREF	(1ULL << 8)

typedef struct _btrfs_tree_block_info {
	btrfs_disk_key key;
	uint8_t level;
} btrfs_tree_block_info;


typedef struct _btrfs_extent_data_ref {
	uint8_t root[8];
	uint8_t objectid[8];
	uint8_t offset[8];
	uint8_t count[4];
} btrfs_extent_data_ref;

typedef struct _btrfs_shared_data_ref {
	uint8_t count[4];
} btrfs_shared_data_ref;

typedef struct _btrfs_extent_inline_ref {
	uint8_t type;
	uint8_t offset[8];
} btrfs_extent_inline_ref;

typedef struct _btrfs_extent_ref_v0 {
	uint8_t root[8];
	uint8_t generation[8];
	uint8_t objectid[8];
	uint8_t count[4];
} btrfs_extent_ref_v0;

/* dev extents record free space on individual devices.  The owner
	* field points back to the chunk allocation mapping tree that allocated
	* the extent.  The chunk tree uuid field is a way to double check the owner
	*/
typedef struct _btrfs_dev_extent {
	uint8_t chunk_tree[8];
	uint8_t chunk_objectid[8];
	uint8_t chunk_offset[8];
	uint8_t length[8];
	uint8_t chunk_tree_uuid[BTRFS_UUID_SIZE];
} btrfs_dev_extent;

typedef struct _btrfs_inode_ref {
	uint8_t index[8];
	uint8_t name_len[2];
	/* name goes here */
} btrfs_inode_ref;

typedef struct _btrfs_inode_extref {
	uint8_t parent_objectid[8];
	uint8_t index[8];
	uint8_t name_len[2];
	__uint8_t   name[0]; /* name goes here */
} btrfs_inode_extref;

typedef struct _btrfs_timespec {
	uint8_t sec[8];
	uint8_t nsec[4];
} btrfs_timespec;

typedef enum {
	BTRFS_COMPRESS_NONE = 0,
	BTRFS_COMPRESS_ZLIB = 1,
	BTRFS_COMPRESS_LZO = 2,
	BTRFS_COMPRESS_ZSTD = 3,
	BTRFS_COMPRESS_TYPES = 3,
	BTRFS_COMPRESS_LAST = 4,
} btrfs_compression_type;

/* we don't understand any encryption methods right now */
typedef enum {
	BTRFS_ENCRYPTION_NONE = 0,
	BTRFS_ENCRYPTION_LAST = 1,
} btrfs_encryption_type;

enum btrfs_tree_block_status {
	BTRFS_TREE_BLOCK_CLEAN,
	BTRFS_TREE_BLOCK_INVALID_NRITEMS,
	BTRFS_TREE_BLOCK_INVALID_PARENT_KEY,
	BTRFS_TREE_BLOCK_BAD_KEY_ORDER,
	BTRFS_TREE_BLOCK_INVALID_LEVEL,
	BTRFS_TREE_BLOCK_INVALID_FREE_SPACE,
	BTRFS_TREE_BLOCK_INVALID_OFFSETS,
};

typedef struct _btrfs_inode_item {
	/* nfs style generation number */
	uint8_t generation[8];
	/* transid that last touched this inode */
	uint8_t transid[8];
	uint8_t size[8];
	uint8_t nbytes[8];
	uint8_t block_group[8];
	uint8_t nlink[4];
	uint8_t uid[4];
	uint8_t gid[4];
	uint8_t mode[4];
	uint8_t rdev[8];
	uint8_t flags[8];

	/* modification sequence number for NFS */
	uint8_t sequence[8];

	/*
		* a little future expansion, for more than this we can
		* just grow the inode item and version it
		*/
	uint8_t reserved[32];
	btrfs_timespec atime;
	btrfs_timespec ctime;
	btrfs_timespec mtime;
	btrfs_timespec otime;
} btrfs_inode_item;

typedef struct _btrfs_dir_log_item {
	uint8_t end[8];
} btrfs_dir_log_item;


typedef struct _btrfs_dir_item {
	btrfs_disk_key location;
	uint8_t transid[8];
	uint8_t data_len[2];
	uint8_t name_len[2];
	uint8_t type;
	char name[BTRFS_MAXNAMLEN];
} btrfs_dir_item;

typedef struct _btrfs_root_item_v0 {
	btrfs_inode_item inode;
	uint8_t generation[8];
	uint8_t root_dirid[8];
	uint8_t bytenr[8];
	uint8_t byte_limit[8];
	uint8_t bytes_used[8];
	uint8_t last_snapshot[8];
	uint8_t flags[8];
	uint8_t refs[4];
	btrfs_disk_key drop_progress;
	uint8_t drop_level;
	uint8_t level;
} btrfs_root_item_v0;

typedef struct _btrfs_root_item {
	btrfs_inode_item inode;
	uint8_t generation[8];
	uint8_t root_dirid[8];
	uint8_t bytenr[8];
	uint8_t byte_limit[8];
	uint8_t bytes_used[8];
	uint8_t last_snapshot[8];
	uint8_t flags[8];
	uint8_t refs[4];
	btrfs_disk_key drop_progress;
	uint8_t drop_level;
	uint8_t level;

	/*
		* The following fields appear after subvol_uuids+subvol_times
		* were introduced.
		*/

		/*
		* This generation number is used to test if the new fields are valid
		* and up to date while reading the root item. Every time the root item
		* is written out, the "generation" field is copied into this field. If
		* anyone ever mounted the fs with an older kernel, we will have
		* mismatching generation values here and thus must invalidate the
		* new fields. See btrfs_update_root and btrfs_find_last_root for
		* details.
		* the offset of generation_v2 is also used as the start for the memset
		* when invalidating the fields.
		*/
	uint8_t generation_v2[8];
	uint8_t uuid[BTRFS_UUID_SIZE];
	uint8_t parent_uuid[BTRFS_UUID_SIZE];
	uint8_t received_uuid[BTRFS_UUID_SIZE];
	uint8_t ctransid[8]; /* updated when an inode changes */
	uint8_t otransid[8]; /* trans when created */
	uint8_t stransid[8]; /* trans when sent. non-zero for received subvol */
	uint8_t rtransid[8]; /* trans when received. non-zero for received subvol */
	btrfs_timespec ctime;
	btrfs_timespec otime;
	btrfs_timespec stime;
	btrfs_timespec rtime;
	uint8_t reserved[64]; /* for future */
} btrfs_root_item;



#define BTRFS_FILE_EXTENT_INLINE 0
#define BTRFS_FILE_EXTENT_REG 1
#define BTRFS_FILE_EXTENT_PREALLOC 2
#define BTRFS_FILE_EXTENT_LEN 0x35

typedef struct _btrfs_file_extent_item {
	/*
		* transaction id that created this extent
		*/
	uint8_t generation[8];
	/*
		* max number of bytes to hold this extent in ram
		* when we split a compressed extent we can't know how big
		* each of the resulting pieces will be.  So, this is
		* an upper limit on the size of the extent in ram instead of
		* an exact limit.
		*/
	uint8_t ram_bytes[8];

	/*
		* 32 bits for the various ways we might encode the data,
		* including compression and encryption.  If any of these
		* are set to something a given disk format doesn't understand
		* it is treated like an incompat flag for reading and writing,
		* but not for stat.
		*/
	uint8_t compression;
	uint8_t encryption;
	uint8_t other_encoding[2]; /* spare for later use */

	/* are we inline data or a real extent? */
	uint8_t type;

	/*
		* disk space consumed by the extent, checksum blocks are included
		* in these numbers
		*/
	uint8_t disk_bytenr[8];
	uint8_t disk_num_bytes[8];
	/*
		* the logical offset in file blocks (no csums)
		* this extent record is for.  This allows a file extent to point
		* into the middle of an existing extent on disk, sharing it
		* between two snapshots (useful if some bytes in the middle of the
		* extent have changed
		*/
	uint8_t offset[8];
	/*
		* the logical number of file blocks (no csums included)
		*/
	uint8_t num_bytes[8];

} btrfs_file_extent_item;

typedef struct _btrfs_dev_stats_item {
	/*
		* grow this item typedef struct _at the end for future enhancements and keep
		* the existing values unchanged
		*/
	uint8_t values[BTRFS_DEV_STAT_VALUES_MAX*8];
} btrfs_dev_stats_item;

typedef struct _btrfs_csum_item {
	uint8_t csum;
} btrfs_csum_item;

typedef struct _btrfs_run_data {
	uint64_t data_addr;
	uint64_t data_len;
} btrfs_run_data;
/*
	* We don't want to overwrite 1M at the beginning of device, even though
	* there is our 1st superblock at 64k. Some possible reasons:
	*  - the first 64k blank is useful for some boot loader/manager
	*  - the first 1M could be scratched by buggy partitioner or somesuch
	*/
#define BTRFS_BLOCK_RESERVED_1M_FOR_SUPER	((uint8_t)SZ_1M)

	 /* tag for the radix tree of block groups in ram */
#define BTRFS_BLOCK_GROUP_DATA		(1ULL << 0)
#define BTRFS_BLOCK_GROUP_SYSTEM	(1ULL << 1)
#define BTRFS_BLOCK_GROUP_METADATA	(1ULL << 2)
#define BTRFS_BLOCK_GROUP_RAID0		(1ULL << 3)
#define BTRFS_BLOCK_GROUP_RAID1		(1ULL << 4)
#define BTRFS_BLOCK_GROUP_DUP		(1ULL << 5)
#define BTRFS_BLOCK_GROUP_RAID10	(1ULL << 6)
#define BTRFS_BLOCK_GROUP_RAID5    	(1ULL << 7)
#define BTRFS_BLOCK_GROUP_RAID6    	(1ULL << 8)
#define BTRFS_BLOCK_GROUP_RESERVED	BTRFS_AVAIL_ALLOC_BIT_SINGLE

enum btrfs_raid_types {
	BTRFS_RAID_RAID10,
	BTRFS_RAID_RAID1,
	BTRFS_RAID_DUP,
	BTRFS_RAID_RAID0,
	BTRFS_RAID_SINGLE,
	BTRFS_RAID_RAID5,
	BTRFS_RAID_RAID6,
	BTRFS_NR_RAID_TYPES
};

#define BTRFS_BLOCK_GROUP_TYPE_MASK	(BTRFS_BLOCK_GROUP_DATA |    \
					 BTRFS_BLOCK_GROUP_SYSTEM |  \
					 BTRFS_BLOCK_GROUP_METADATA)

#define BTRFS_BLOCK_GROUP_PROFILE_MASK	(BTRFS_BLOCK_GROUP_RAID0 |   \
					 BTRFS_BLOCK_GROUP_RAID1 |   \
					 BTRFS_BLOCK_GROUP_RAID5 |   \
					 BTRFS_BLOCK_GROUP_RAID6 |   \
					 BTRFS_BLOCK_GROUP_DUP |     \
					 BTRFS_BLOCK_GROUP_RAID10)

/* used in typedef struct _btrfs_balance_args fields */
#define BTRFS_AVAIL_ALLOC_BIT_SINGLE	(1ULL << 48)

/*
 * GLOBAL_RSV does not exist as a on-disk block group type and is used
 * internally for exporting info about global block reserve from space infos
 */
#define BTRFS_SPACE_INFO_GLOBAL_RSV    (1ULL << 49)



#define BTRFS_FREE_SPACE_USING_BITMAPS (1ULL << 0)




/*
 * Structure of an Btrfs file system handle.
 */
typedef struct _BTRFS_INFO {
	TSK_FS_INFO fs_info;    /* super class */
	btrfs_super_block *fs;          /* super block */

	tsk_lock_t lock;
	
	btrfs_leaf **root_tree;
	uint64_t *root_phy_addr;

	btrfs_leaf **chunk_tree;
	uint64_t *chunk_phy_addr;

	btrfs_node **fs_tree;
	uint64_t *fs_phy_addr;

	btrfs_leaf ***fs_leaf;
	uint64_t **fs_leaf_phy_addr;
	uint8_t *fs_leaf_num;

	uint16_t num_stripes;
	uint64_t inode_size;

	uint64_t tmp_dir_item_info[2][0x100];
	btrfs_dir_item tmp_dir_item[0x1000][0x500];

	uint8_t tmp_cnt;

	btrfs_file_extent_item tmp_extent_item[BTRFS_FILE_EXTENT_LEN];
} BTRFS_INFO;




/*
* inode items have the data typically returned from stat and store other
* info about object characteristics.  There is one for every file and dir in
* the FS
*/
#define BTRFS_INODE_ITEM_KEY		1
#define BTRFS_INODE_REF_KEY		12
#define BTRFS_INODE_EXTREF_KEY		13
#define BTRFS_XATTR_ITEM_KEY		24
#define BTRFS_ORPHAN_ITEM_KEY		48

#define BTRFS_DIR_LOG_ITEM_KEY  60
#define BTRFS_DIR_LOG_INDEX_KEY 72
/*
* dir items are the name -> inode pointers in a directory.  There is one
* for every name in a directory.
*/
#define BTRFS_DIR_ITEM_KEY	84
#define BTRFS_DIR_INDEX_KEY	96

/*
* extent data is for file data
*/
#define BTRFS_EXTENT_DATA_KEY	108

/*
* csum items have the checksums for data in the extents
*/
#define BTRFS_CSUM_ITEM_KEY	120
/*
	* extent csums are stored in a separate tree and hold csums for
	* an entire extent on disk.
	*/
#define BTRFS_EXTENT_CSUM_KEY	128

/*
* root items point to tree roots.  There are typically in the root
* tree used by the super block to find all the other trees
*/
#define BTRFS_ROOT_ITEM_KEY	132

/*
* root backrefs tie subvols and snapshots to the directory entries that
* reference them
*/
#define BTRFS_ROOT_BACKREF_KEY	144

/*
* root refs make a fast index for listing all of the snapshots and
* subvolumes referenced by a given root.  They point directly to the
* directory item in the root that references the subvol
*/
#define BTRFS_ROOT_REF_KEY	156

/*
* extent items are in the extent map tree.  These record which blocks
* are used, and how many references there are to each block
*/
#define BTRFS_EXTENT_ITEM_KEY	168

/*
* The same as the BTRFS_EXTENT_ITEM_KEY, except it's metadata we already know
* the length, so we save the level in key->offset instead of the length.
*/
#define BTRFS_METADATA_ITEM_KEY	169

#define BTRFS_TREE_BLOCK_REF_KEY	176

#define BTRFS_EXTENT_DATA_REF_KEY	178

/* old style extent backrefs */
#define BTRFS_EXTENT_REF_V0_KEY		180

#define BTRFS_SHARED_BLOCK_REF_KEY	182

#define BTRFS_SHARED_DATA_REF_KEY	184


/*
* block groups give us hints into the extent allocation trees.  Which
* blocks are free etc etc
*/
#define BTRFS_BLOCK_GROUP_ITEM_KEY 192

/*
* Every block group is represented in the free space tree by a free space info
* item, which stores some accounting information. It is keyed on
* (block_group_start, FREE_SPACE_INFO, block_group_length).
*/
#define BTRFS_FREE_SPACE_INFO_KEY 198

/*
* A free space extent tracks an extent of space that is free in a block group.
* It is keyed on (start, FREE_SPACE_EXTENT, length).
*/
#define BTRFS_FREE_SPACE_EXTENT_KEY 199

/*
* When a block group becomes very fragmented, we convert it to use bitmaps
* instead of extents. A free space bitmap is keyed on
* (start, FREE_SPACE_BITMAP, length); the corresponding item is a bitmap with
* (length / sectorsize) bits.
*/
#define BTRFS_FREE_SPACE_BITMAP_KEY 200

#define BTRFS_DEV_EXTENT_KEY	204
#define BTRFS_DEV_ITEM_KEY	216
#define BTRFS_CHUNK_ITEM_KEY	228

#define BTRFS_BALANCE_ITEM_KEY	248

/*
* quota groups
*/
#define BTRFS_QGROUP_STATUS_KEY		240
#define BTRFS_QGROUP_INFO_KEY		242
#define BTRFS_QGROUP_LIMIT_KEY		244
#define BTRFS_QGROUP_RELATION_KEY	246

/*
* Obsolete name, see BTRFS_TEMPORARY_ITEM_KEY.
*/
#define BTRFS_BALANCE_ITEM_KEY	248

/*
* The key type for tree items that are stored persistently, but do not need to
* exist for extended period of time. The items can exist in any tree.
*
* [subtype, BTRFS_TEMPORARY_ITEM_KEY, data]
*
* Existing items:
*
* - balance status item
*   (BTRFS_BALANCE_OBJECTID, BTRFS_TEMPORARY_ITEM_KEY, 0)
*/
#define BTRFS_TEMPORARY_ITEM_KEY	248

/*
* Obsolete name, see BTRFS_PERSISTENT_ITEM_KEY
*/
#define BTRFS_DEV_STATS_KEY		249

/*
* The key type for tree items that are stored persistently and usually exist
* for a long period, eg. filesystem lifetime. The item kinds can be status
* information, stats or preference values. The item can exist in any tree.
*
* [subtype, BTRFS_PERSISTENT_ITEM_KEY, data]
*
* Existing items:
*
* - device statistics, store IO stats in the device tree, one key for all
*   stats
*   (BTRFS_DEV_STATS_OBJECTID, BTRFS_DEV_STATS_KEY, 0)
*/
#define BTRFS_PERSISTENT_ITEM_KEY	249

/*
* Persistently stores the device replace state in the device tree.
* The key is built like this: (0, BTRFS_DEV_REPLACE_KEY, 0).
*/
#define BTRFS_DEV_REPLACE_KEY	250

/*
* Stores items that allow to quickly map UUIDs to something else.
* These items are part of the filesystem UUID tree.
* The key is built like this:
* (UUID_upper_64_bits, BTRFS_UUID_KEY*, UUID_lower_64_bits).
*/
#if BTRFS_UUID_SIZE != 16
#error "UUID items require BTRFS_UUID_SIZE == 16!"
#endif
#define BTRFS_UUID_KEY_SUBVOL	251	/* for UUIDs assigned to subvols */
#define BTRFS_UUID_KEY_RECEIVED_SUBVOL	252	/* for UUIDs assigned to
* received subvols */

/*
* string items are for debugging.  They just store a short string of
* data in the FS
*/
#define BTRFS_STRING_ITEM_KEY	253
/*
* Inode flags
*/
#define BTRFS_INODE_NODATASUM		(1 << 0)
#define BTRFS_INODE_NODATACOW		(1 << 1)
#define BTRFS_INODE_READONLY		(1 << 2)
#define BTRFS_INODE_NOCOMPRESS		(1 << 3)
#define BTRFS_INODE_PREALLOC		(1 << 4)
#define BTRFS_INODE_SYNC		(1 << 5)
#define BTRFS_INODE_IMMUTABLE		(1 << 6)
#define BTRFS_INODE_APPEND		(1 << 7)
#define BTRFS_INODE_NODUMP		(1 << 8)
#define BTRFS_INODE_NOATIME		(1 << 9)
#define BTRFS_INODE_DIRSYNC		(1 << 10)
#define BTRFS_INODE_COMPRESS		(1 << 11)


static inline uint64_t
btrfs_calc_phyAddr(TSK_FS_INFO * a_fs, BTRFS_INFO *btrfs, uint8_t *logical_addr, uint8_t current_stripe) {
	int i = 0;
	int len = 0x30;
	int size;
	int chunk_data_off;
	
	ssize_t cnt;
	
	uint64_t target_addr;
	uint64_t chunk_log_addr; 
	uint64_t chunk_phy_addr;
	uint64_t chunk_off;
	uint64_t target_phy_addr;

	btrfs_chunk *tmpbuf;
	
	if ((tmpbuf = (btrfs_chunk *)tsk_fs_malloc(sizeof(*tmpbuf))) == NULL)
		return TSK_ERR;


	if (tsk_fs_guessu64(a_fs, btrfs->chunk_tree[current_stripe]->header.bytenr, tsk_getu64(a_fs->endian, btrfs->fs->chunk_root))) {
		a_fs->tag = 0;
		free(btrfs->fs);
		tsk_fs_free((TSK_FS_INFO *)btrfs);

		if (tsk_verbose)
			fprintf(stderr, "btrfs_calc_physical_addr : invalid chunk tree\n");

		return TSK_ERR;
	}
	target_addr = tsk_getu64(a_fs->endian, logical_addr);

	while (true) {
		if (target_addr < tsk_getu64(a_fs->endian, btrfs->chunk_tree[current_stripe]->items[i].key.offset)){
			chunk_off = tsk_getu64(a_fs->endian, btrfs->chunk_tree[current_stripe]->items[i-1].offset);
			chunk_log_addr = tsk_getu64(a_fs->endian, btrfs->chunk_tree[current_stripe]->items[i-1].key.offset);
			size = tsk_getu32(a_fs->endian, btrfs->chunk_tree[current_stripe]->items[i-1].size);
			break;
		}
		else {
			if ((i != 0) && (tsk_getu64(a_fs->endian, btrfs->chunk_tree[current_stripe]->items[i].key.offset) == 0)) {
				chunk_off = tsk_getu64(a_fs->endian, btrfs->chunk_tree[current_stripe]->items[i-1].offset);
				chunk_log_addr = tsk_getu64(a_fs->endian, btrfs->chunk_tree[current_stripe]->items[i-1].key.offset);
				size = tsk_getu32(a_fs->endian, btrfs->chunk_tree[current_stripe]->items[i - 1].size);

				break;
			}
			i++;
		}
	}

	chunk_data_off = (btrfs->chunk_phy_addr[current_stripe]) + 0x65 + chunk_off;
	cnt = tsk_fs_read(a_fs, chunk_data_off, (char *)tmpbuf, len);
	if (cnt != len) {
		if (cnt >= 0) {
			tsk_error_reset();
			tsk_error_set_errno(TSK_ERR_FS_READ);
		}
		tsk_error_set_errstr2("btrfs_init_chunk : read chunk_item");
		a_fs->tag = 0;
		free(btrfs->fs);
		free(tmpbuf);
		tsk_fs_free((TSK_FS_INFO *)btrfs);
		return TSK_ERR;
	}

	cnt = tsk_fs_read(a_fs, chunk_data_off + 0x30, (char *)tmpbuf + 0x30, size - 0x30);
	if (cnt != (size-0x30)) {
		if (cnt >= 0) {
			tsk_error_reset();
			tsk_error_set_errno(TSK_ERR_FS_READ);
		}
		tsk_error_set_errstr2("btrfs_init_chunk : read chunk_item(stripe)");
		a_fs->tag = 0;
		free(btrfs->fs);
		free(tmpbuf);
		tsk_fs_free((TSK_FS_INFO *)btrfs);
		return TSK_ERR;
	}

	chunk_phy_addr = tsk_getu64(a_fs->endian, tmpbuf->stripe->offset);
	target_phy_addr = target_addr - chunk_log_addr + chunk_phy_addr;

	free(tmpbuf);

	return target_phy_addr;
}



static inline uint64_t btrfs_get_last_inum(BTRFS_INFO * btrfs) {
	TSK_FS_INFO *fs;

	btrfs_leaf * fs_leaf;
	uint64_t num_stripes;
	int fs_leaf_num;
	int i, j, k;
	int last_inum;

	fs = &(btrfs->fs_info);
	//	num_stripes = btrfs->num_stripes;
	num_stripes = 1;
	
	for (i = 0; i < num_stripes; i++) {
		fs_leaf_num = btrfs->fs_leaf_num[i];
		for (j = 0; j < fs_leaf_num; j++) {
			fs_leaf = btrfs->fs_leaf[i][j];

			for (k = 0; ; k++) {
				if (tsk_getu32(fs->endian, fs_leaf->items[k].offset) == 0) {
					break;
				}
				last_inum = tsk_getu64(fs->endian, fs_leaf->items[k].key.objectid);
			}
		}
	}
	return last_inum;
}


static inline TSK_RETVAL_ENUM btrfs_seek_dir_item(BTRFS_INFO * btrfs, TSK_INUM_T inum, uint8_t current_stripe, uint64_t target_addr) {
	TSK_FS_INFO *fs;

	btrfs_leaf * fs_leaf;
	uint64_t tmp;
	uint64_t current_phy_addr;

	int i;
	int cnt = 0;

	fs = &(btrfs->fs_info);


	for (i = 0; i < btrfs->fs_leaf_num[current_stripe]; i++) {
		if (btrfs->fs_leaf_phy_addr[current_stripe][i] == target_addr)
			break;
	}

	fs_leaf = btrfs->fs_leaf[current_stripe][i];
	current_phy_addr = btrfs->fs_leaf_phy_addr[current_stripe][i];
	
	cnt = 0;

	for (i = 0; ; i++) {
		if (tsk_getu32(fs->endian, fs_leaf->items[i].offset) == 0) {
			break;
		}
		if ((tsk_getu64(fs->endian, fs_leaf->items[i].key.objectid) == inum) && (fs_leaf->items[i].key.type == BTRFS_DIR_ITEM_KEY)) {
			tmp = current_phy_addr + 0x65 + tsk_getu32(fs->endian, fs_leaf->items[i].offset);
			btrfs->tmp_dir_item_info[0][cnt] = tmp;
			btrfs->tmp_dir_item_info[1][cnt] = tsk_getu32(fs->endian, fs_leaf->items[i].size);

			char * buf = (char *)btrfs->tmp_dir_item[cnt];
			int cnt2 = tsk_fs_read(fs, btrfs->tmp_dir_item_info[0][cnt], buf, btrfs->tmp_dir_item_info[1][cnt]);
			if (cnt2 != btrfs->tmp_dir_item_info[1][cnt]) {
				tsk_error_set_errstr2("btrfs_seek_dir_item : read dir item");
				fs->tag = 0;
				free(btrfs->fs);
				tsk_fs_free((TSK_FS_INFO *)btrfs);
				return TSK_ERR;
			}
			cnt++;
		}
	}

	btrfs->tmp_cnt = cnt;

	if (cnt == 0) {
		return TSK_ERR;
	}
	return TSK_OK;
}



static inline uint8_t btrfs_seek_inode_where_leaf(BTRFS_INFO * btrfs, TSK_INUM_T inum, uint8_t current_stripe) {
	TSK_FS_INFO *fs;
	btrfs_leaf * fs_leaf;
	btrfs_node * fs_tree;

	int i;
	int tmp = -1;

	fs = &(btrfs->fs_info);
//	num_stripes = btrfs->num_stripes;
	fs_tree = btrfs->fs_tree[current_stripe];

	// trick to find fast
	for (i = 0; i < btrfs->fs_leaf_num[current_stripe]; i++) {
		fs_leaf = btrfs->fs_leaf[current_stripe][i];

		if (tsk_getu64(fs->endian, fs_leaf->items[0].key.objectid) <= inum) {
			if (tsk_getu64(fs->endian, fs_leaf->items[0].key.objectid) == inum) {
				tmp = i;
				break;
			}
			continue;
		}
		else {
			tmp = i - 1;
			break;
		}
	}
	if (tmp == -1)
		tmp = btrfs->fs_leaf_num[current_stripe] - 1;

	fs_leaf = btrfs->fs_leaf[current_stripe][tmp];

	return tmp;
}


static inline uint64_t btrfs_seek_fs_leaf(BTRFS_INFO * btrfs, TSK_INUM_T inum, uint8_t type) {
	TSK_FS_INFO *fs;

	btrfs_node * fs_tree;
	btrfs_leaf * fs_leaf;
	uint64_t num_stripes;
	uint64_t target_phyAddr;

	int i, j, k;
	int tmp = -1;

	fs = &(btrfs->fs_info);
	//	num_stripes = btrfs->num_stripes;
	num_stripes = 1;
	for (i = 0; i < num_stripes; i++) {
		fs_tree = btrfs->fs_tree[i];

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

		fs_leaf = btrfs->fs_leaf[i][tmp];

		for (k = 0; ; k++) {
			if (tsk_getu32(fs->endian, fs_leaf->items[k].offset) == 0) {
				break;
			}
			if ((tsk_getu64(fs->endian, fs_leaf->items[k].key.objectid) == inum) && (fs_leaf->items[k].key.type == type)) {

				if ((type == BTRFS_EXTENT_DATA_KEY) && (tsk_getu32(fs->endian, fs_leaf->items[k].size) != BTRFS_FILE_EXTENT_LEN)) {
					return 0x01;
				}
				target_phyAddr = btrfs_calc_phyAddr(fs, btrfs, fs_tree[i].ptrs[tmp].blockptr, i);
				target_phyAddr = target_phyAddr + 0x65 + tsk_getu32(fs->endian, fs_leaf->items[k].offset);
				
				return target_phyAddr;
			}
		}
	}

	return TSK_ERR;
}





static inline TSK_RETVAL_ENUM btrfs_get_fs_leaf(BTRFS_INFO *btrfs) {
	TSK_FS_INFO *fs;

	btrfs_node * fs_tree;
	uint64_t num_stripes;
	uint64_t leaf_phyAddr;

	int i, j, tmp;
	int len;
	int cnt;

	fs = &(btrfs->fs_info);
	//	num_stripes = btrfs->num_stripes;
	num_stripes = 1;

	if ((btrfs->fs_leaf = (btrfs_leaf ***)tsk_fs_malloc(sizeof(btrfs_leaf **)*num_stripes)) == NULL)
		return TSK_ERR;
	if ((btrfs->fs_leaf_num = (uint8_t *)tsk_fs_malloc(sizeof(uint8_t)*num_stripes)) == NULL)
		return TSK_ERR;
	if ((btrfs->fs_leaf_phy_addr = (uint64_t **)tsk_fs_malloc(sizeof(uint64_t *)*num_stripes)) == NULL)
		return TSK_ERR;

	for (i = 0; i < num_stripes; i++) {
		fs_tree = btrfs->fs_tree[i];

		for (j = 0; ; j++) {
			if (tsk_getu64(fs->endian, fs_tree->ptrs[j].blockptr) == 0)
				break;
		}
		tmp = j;
		btrfs->fs_leaf_num[i] = tmp;
		if ((btrfs->fs_leaf[i] = (btrfs_leaf **)tsk_fs_malloc(sizeof(btrfs_leaf *)*j)) == NULL)
			return TSK_ERR;
		if ((btrfs->fs_leaf_phy_addr[i] = (uint64_t *)tsk_fs_malloc(sizeof(uint64_t)*j)) == NULL)
			return TSK_ERR;

		for (j = 0; j < tmp; j++) {

			if (tsk_getu64(fs->endian, fs_tree->ptrs[j].blockptr) == 0) {
				break;
			}
			len = 0x4000;
			if ((btrfs->fs_leaf[i][j] = (btrfs_leaf *)tsk_fs_malloc(len)) == NULL)
				return TSK_ERR;

			leaf_phyAddr = btrfs_calc_phyAddr(fs, btrfs, fs_tree->ptrs[j].blockptr, i);

			btrfs->fs_leaf_phy_addr[i][j] = leaf_phyAddr;
			len = 0x4000;
			cnt = tsk_fs_read(fs, leaf_phyAddr, (char *)btrfs->fs_leaf[i][j], len);
			if (cnt != len) {
				if (cnt >= 0) {
					tsk_error_reset();
					tsk_error_set_errno(TSK_ERR_FS_READ);
				}
				tsk_error_set_errstr2("btrfs_get_fs_leaf : read fs_leaf");
				fs->tag = 0;
				free(btrfs->fs);
				free(btrfs->fs_leaf);
				tsk_fs_free((TSK_FS_INFO *)btrfs);
				return TSK_ERR;
			}
		}

	}

	return TSK_OK;
}



static inline TSK_RETVAL_ENUM btrfs_init_fs(BTRFS_INFO *btrfs) {
	uint64_t fs_phyAddr;
	uint64_t num_stripes;
	uint64_t num_rootitem;

	btrfs_leaf * root_tree;
	btrfs_root_item *root_item;

	int root_data_off;
	int cnt;
	int i, j, objid;
	int len;

	TSK_FS_INFO *fs = &(btrfs->fs_info);

	//	num_stripes = btrfs->num_stripes;
	num_stripes = 1;

	if ((root_item = (btrfs_root_item *)tsk_fs_malloc(sizeof(*root_item))) == NULL)
		return TSK_ERR;
	if ((btrfs->fs_tree = (btrfs_node **)tsk_fs_malloc(sizeof(btrfs_node *)*num_stripes)) == NULL)
		return TSK_ERR;

	for (i = 0; i < num_stripes; i++) {
		root_tree = btrfs->root_tree[i];
		num_rootitem = (tsk_getu32(fs->endian, btrfs->fs->nodesize) - 0x65) / 0x21;

		for (j = 0; j < num_rootitem; j++) {
			if (tsk_getu64(fs->endian, root_tree->items[j].offset) == 0)
				break;

			objid = tsk_getu64(fs->endian, root_tree->items[j].key.objectid);

			if ((objid == 0x05 || objid >= 0x100) && (root_tree->items[j].key.type == 0x84)) {
				len = tsk_getu32(fs->endian, root_tree->items[j].size);
				root_data_off = btrfs->root_phy_addr[i] + 0x65 + tsk_getu64(fs->endian, root_tree->items[j].offset);
				cnt = tsk_fs_read(fs, root_data_off, (char *)root_item, len);
				if (cnt != len) {
					if (cnt >= 0) {
						tsk_error_reset();
						tsk_error_set_errno(TSK_ERR_FS_READ);
					}
					tsk_error_set_errstr2("btrfs_init_fs : read root");
					fs->tag = 0;
					free(btrfs->fs);
					free(root_item);
					tsk_fs_free((TSK_FS_INFO *)btrfs);
					return TSK_ERR;
				}

				if ((btrfs->fs_phy_addr = (uint64_t *)tsk_fs_malloc(sizeof(btrfs->fs_phy_addr))) == NULL)
					return TSK_ERR;

				fs_phyAddr = btrfs_calc_phyAddr(fs, btrfs, root_item->bytenr, i);
				btrfs->fs_phy_addr[i] = fs_phyAddr;

				len = tsk_getu32(fs->endian, btrfs->fs->nodesize);

				if ((btrfs->fs_tree[i] = (btrfs_node *)tsk_fs_malloc(len)) == NULL)
					return TSK_ERR;

				cnt = tsk_fs_read(fs, fs_phyAddr, (char *)btrfs->fs_tree[i], len);
				if (cnt != len) {
					if (cnt >= 0) {
						tsk_error_reset();
						tsk_error_set_errno(TSK_ERR_FS_READ);
					}
					tsk_error_set_errstr2("btrfs_init_fs : read root");
					fs->tag = 0;
					free(btrfs->fs);
					free(root_item);
					tsk_fs_free((TSK_FS_INFO *)btrfs);
					return TSK_ERR;
				}
			}
		}
	}

	free(root_item);
	return TSK_OK;
}




static inline TSK_RETVAL_ENUM btrfs_init_root(BTRFS_INFO *btrfs) {
	uint64_t root_phyAddr;
	uint64_t num_stripes;
	TSK_FS_INFO *fs;
	int i;
	int cnt;
	uint32_t len;

	fs = &(btrfs->fs_info);

	//	num_stripes = btrfs->num_stripes;
	num_stripes = 1;

	if ((btrfs->root_tree = (btrfs_leaf **)tsk_fs_malloc(sizeof(btrfs_leaf *)*num_stripes)) == NULL)
		return TSK_ERR;

	for (i = 0; i < num_stripes; i++) {
		root_phyAddr = btrfs_calc_phyAddr(fs, btrfs, btrfs->fs->root, i);

		if ((btrfs->root_phy_addr = (uint64_t *)tsk_fs_malloc(sizeof(btrfs->root_phy_addr))) == NULL)
			return TSK_ERR;
		btrfs->root_phy_addr[i] = root_phyAddr;

		len = tsk_getu32(fs->endian, btrfs->fs->nodesize);
		if ((btrfs->root_tree[i] = (btrfs_leaf *)tsk_fs_malloc(len)) == NULL)
			return TSK_ERR;

		cnt = tsk_fs_read(fs, btrfs->root_phy_addr[i], (char*)btrfs->root_tree[i], len);
		if (cnt != len) {
			if (cnt >= 0) {
				tsk_error_reset();
				tsk_error_set_errno(TSK_ERR_FS_READ);
			}
			tsk_error_set_errstr2("btrfs_init_root : read root");
			fs->tag = 0;
			free(btrfs->fs);
			tsk_fs_free((TSK_FS_INFO *)btrfs);
			return TSK_ERR;
		}
	}

	return TSK_OK;
}



static inline TSK_RETVAL_ENUM btrfs_init_chunk(BTRFS_INFO *btrfs) {
	unsigned int len;
	int i;
	ssize_t cnt;

	btrfs_super_block *btrfs_sb;
	btrfs_disk_key *sys_chunk_key;
	btrfs_chunk *sys_chunk;

	TSK_FS_INFO *fs = &(btrfs->fs_info);
	btrfs_sb = btrfs->fs;

	if ((sys_chunk_key = (btrfs_disk_key *)tsk_fs_malloc(sizeof(*sys_chunk_key))) == NULL)
		return TSK_ERR;
	if ((sys_chunk = (btrfs_chunk *)tsk_fs_malloc(sizeof(*sys_chunk))) == NULL)
		return TSK_ERR;
	len = sizeof(btrfs_disk_key);

	cnt = tsk_fs_read(fs, BTRFS_SUPER_INFO_OFFSET + 0x32b, (char *)sys_chunk_key, len);
	if (cnt != len) {
		if (cnt >= 0) {
			tsk_error_reset();
			tsk_error_set_errno(TSK_ERR_FS_READ);
		}
		tsk_error_set_errstr2("btrfs_init_chunk : read sys_chunk");
		fs->tag = 0;
		free(btrfs->fs);
		tsk_fs_free((TSK_FS_INFO *)btrfs);
		return TSK_ERR;
	}

	len = 0x30;
	cnt = tsk_fs_read(fs, BTRFS_SUPER_INFO_OFFSET + 0x33c, (char *)sys_chunk, len);
	if (cnt != len) {
		if (cnt >= 0) {
			tsk_error_reset();
			tsk_error_set_errno(TSK_ERR_FS_READ);
		}
		tsk_error_set_errstr2("btrfs_init_chunk : read sys_chunk");
		fs->tag = 0;
		free(btrfs->fs);
		tsk_fs_free((TSK_FS_INFO *)btrfs);
		return TSK_ERR;
	}

	int num_stripes = tsk_getu16(fs->endian, sys_chunk->num_stripes);

	uint64_t target_log;
	uint64_t chunk_log;
	uint64_t chunk_phy;
	uint64_t target_phyAddr;

	btrfs->num_stripes = num_stripes;

	if ((btrfs->chunk_tree = (btrfs_leaf **)tsk_fs_malloc(sizeof(btrfs_leaf *)*num_stripes)) == NULL)
		return TSK_ERR;
	if ((btrfs->chunk_phy_addr = (uint64_t *)tsk_fs_malloc(sizeof(btrfs->chunk_phy_addr))) == NULL)
		return TSK_ERR;

	for (i = 0; i < num_stripes; i++) {
		len = tsk_getu32(fs->endian, btrfs_sb->sys_chunk_array_size) - 0x11 - 0x30;

		cnt = tsk_fs_read(fs, BTRFS_SUPER_INFO_OFFSET + 0x36c + (i * sizeof(btrfs_stripe)), (char *)sys_chunk + 0x30 + (0x20 * i), len);
		if (cnt != len) {
			if (cnt >= 0) {
				tsk_error_reset();
				tsk_error_set_errno(TSK_ERR_FS_READ);
			}
			tsk_error_set_errstr2("btrfs_init_chunk : read sys_chunk");
			fs->tag = 0;
			free(btrfs->fs);
			tsk_fs_free((TSK_FS_INFO *)btrfs);
			free(sys_chunk_key);
			free(sys_chunk);
			return TSK_ERR;
		}

		target_log = tsk_getu64(fs->endian, btrfs_sb->chunk_root);
		chunk_log = tsk_getu64(fs->endian, sys_chunk_key->offset);
		chunk_phy = tsk_getu64(fs->endian, sys_chunk->stripe->offset);
		target_phyAddr = target_log - chunk_log + chunk_phy;

		btrfs->chunk_phy_addr[i] = target_phyAddr;
		len = tsk_getu32(fs->endian, btrfs->fs->nodesize);
		if ((btrfs->chunk_tree[i] = (btrfs_leaf *)tsk_fs_malloc(len)) == NULL)
			return TSK_ERR;

		cnt = tsk_fs_read(fs, target_phyAddr, (char *)btrfs->chunk_tree[i], len);
		if (cnt != len) {
			if (cnt >= 0) {
				tsk_error_reset();
				tsk_error_set_errno(TSK_ERR_FS_READ);
			}

			tsk_error_set_errstr2("btrfs_init_chunk : read chunk_tree");
			fs->tag = 0;
			free(btrfs->fs);
			tsk_fs_free((TSK_FS_INFO *)btrfs);
			free(sys_chunk_key);
			free(sys_chunk);
			return TSK_ERR;
		}

		cnt = tsk_fs_read(fs, target_phyAddr, (char *)btrfs->chunk_tree[i], len);
		if (cnt < len)
			len = cnt;
		if (cnt != len) {
			if (cnt >= 0) {
				tsk_error_reset();
				tsk_error_set_errno(TSK_ERR_FS_READ);
			}

			tsk_error_set_errstr2("btrfs_init_chunk : read chunk_tree");
			fs->tag = 0;
			free(btrfs->fs);
			tsk_fs_free((TSK_FS_INFO *)btrfs);
			free(sys_chunk_key);
			free(sys_chunk);
			return TSK_ERR;
		}
	}

	free(sys_chunk_key);
	free(sys_chunk);

	return TSK_OK;
}

#endif