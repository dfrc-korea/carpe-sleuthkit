/*
 ** tsk_recover
 ** The Sleuth Kit
 **
 ** Brian Carrier [carrier <at> sleuthkit [dot] org]
 ** Copyright (c) 2010-2011 Brian Carrier.  All Rights reserved
 **
 ** This software is distributed under the Common Public License 1.0
 **
 */

#include "tsk/tsk_tools_i.h"
#include "tsk/auto/tsk_case_db.h"
#include <locale.h>
#include <sys/stat.h>
#include <errno.h>


static TSK_TCHAR *progname;

static void
usage()
{
	TFPRINTF(stderr,
		_TSK_T
		("usage: %s [-F recover, loaddb, fsstat]\n"),
		progname);

	exit(1);

}

#ifdef TSK_WIN32
#include <windows.h>
#include "shlobj.h"
#endif


class CarpeTsk :public TskAuto {
public:
	explicit CarpeTsk(TSK_TCHAR * a_base_dir);
	virtual TSK_RETVAL_ENUM processFile(TSK_FS_FILE * fs_file, const char *path);
	virtual TSK_FILTER_ENUM filterVol(const TSK_VS_PART_INFO * vs_part);
	virtual TSK_FILTER_ENUM filterFs(TSK_FS_INFO * fs_info);
	uint8_t openFs(TSK_OFF_T a_soffset, TSK_FS_TYPE_ENUM fstype, TSK_POOL_TYPE_ENUM pooltype, TSK_DADDR_T pvol_block);
	uint8_t findFiles(TSK_INUM_T a_dirInum);
	uint8_t handleError();

private:
	TSK_TCHAR * m_base_dir;
	uint8_t writeFile(TSK_FS_FILE * a_fs_file, const char *a_path);
	char m_vsName[FILENAME_MAX];
	bool m_writeVolumeDir;
	int m_fileCount;
	TSK_FS_INFO * m_fs_info;
};

CarpeTsk::CarpeTsk(TSK_TCHAR * a_base_dir)
{
	m_base_dir = a_base_dir;
#ifdef TSK_WIN32
	m_vsName[0] = L'\0';
#else
	m_vsName[0] = '\0';
#endif
	m_writeVolumeDir = false;
	m_fileCount = 0;
}

// Print errors as they are encountered
uint8_t CarpeTsk::handleError()
{
	fprintf(stderr, "%s", tsk_error_get());
	return 0;
}

/** \internal
 * Callback used to walk file content and write the results to the recovery file.
 */
static TSK_WALK_RET_ENUM
file_walk_cb(TSK_FS_FILE * a_fs_file, TSK_OFF_T a_off,
	TSK_DADDR_T a_addr, char *a_buf, size_t a_len,
	TSK_FS_BLOCK_FLAG_ENUM a_flags, void *a_ptr)
{
	//write to the file
#ifdef TSK_WIN32
	DWORD written = 0;
	if (!WriteFile((HANDLE)a_ptr, a_buf, (DWORD)a_len, &written, NULL)) {
		fprintf(stderr, "Error writing file content\n");
		return TSK_WALK_ERROR;
	}

#else
	FILE *hFile = (FILE *)a_ptr;
	if (fwrite(a_buf, a_len, 1, hFile) != 1) {
		fprintf(stderr, "Error writing file content\n");
		return TSK_WALK_ERROR;
	}
#endif
	return TSK_WALK_CONT;
}


/**
 * @returns 1 on error.
 */
uint8_t CarpeTsk::writeFile(TSK_FS_FILE * a_fs_file, const char *a_path)
{

#ifdef TSK_WIN32
	/* Step 1 is to make the full path in UTF-16 and create the
	 * needed directories. */

	 // combine the volume name and path
	char path8[FILENAME_MAX];
	strncpy(path8, m_vsName, FILENAME_MAX);
	strncat(path8, a_path, FILENAME_MAX - strlen(path8));
	size_t ilen = strlen(path8);

	// clean up any control characters
	for (size_t i = 0; i < ilen; i++) {
		if (TSK_IS_CNTRL(path8[i]))
			path8[i] = '^';
	}

	//convert path from utf8 to utf16
	wchar_t path16[FILENAME_MAX];
	UTF8 *utf8 = (UTF8 *)path8;
	UTF16 *utf16 = (UTF16 *)path16;
	TSKConversionResult
		retVal =
		tsk_UTF8toUTF16((const UTF8 **)&utf8, &utf8[ilen], &utf16,
			&utf16[FILENAME_MAX], TSKlenientConversion);

	if (retVal != TSKconversionOK) {
		fprintf(stderr, "Error Converting file path");
		return 1;
	}
	*utf16 = '\0';

	//combine the target directory with volume name and path
	//wchar_t path16full[FILENAME_MAX];
	wchar_t path16full[FILENAME_MAX + 1];
	path16full[FILENAME_MAX] = L'\0'; // Try this
	wcsncpy(path16full, (wchar_t *)m_base_dir, FILENAME_MAX);
	wcsncat(path16full, L"\\", FILENAME_MAX - wcslen(path16full));
	wcsncat(path16full, path16, FILENAME_MAX - wcslen(path16full));

	//build up directory structure
	size_t
		len = wcslen((const wchar_t *)path16full);
	for (size_t i = 0; i < len; i++) {
		if (path16full[i] == L'/')
			path16full[i] = L'\\';
		if (((i > 0) && (path16full[i] == L'\\') && (path16full[i - 1] != L'\\'))
			|| ((path16full[i] != L'\\') && (i == len - 1))) {
			uint8_t
				replaced = 0;
			if (path16full[i] == L'\\') {
				path16full[i] = L'\0';
				replaced = 1;
			}
			BOOL
				result = CreateDirectoryW((LPCTSTR)path16full, NULL);
			if (result == FALSE) {
				if (GetLastError() == ERROR_PATH_NOT_FOUND) {
					fprintf(stderr, "Error Creating Directory (%S)", path16full);
					return 1;
				}
			}
			if (replaced)
				path16full[i] = L'\\';
		}
	}

	//fix the end of the path so that the file name can be appended
	if (path16full[len - 1] != L'\\')
		path16full[len] = L'\\';

	//do name mangling
	char name8[FILENAME_MAX];
	strncpy(name8, a_fs_file->name->name, FILENAME_MAX);
	for (int i = 0; name8[i] != '\0'; i++) {
		if (TSK_IS_CNTRL(name8[i]))
			name8[i] = '^';
	}

	//convert file name from utf8 to utf16
	wchar_t name16[FILENAME_MAX];

	ilen = strlen(name8);
	utf8 = (UTF8 *)name8;
	utf16 = (UTF16 *)name16;

	retVal = tsk_UTF8toUTF16((const UTF8 **)&utf8, &utf8[ilen],
		&utf16, &utf16[FILENAME_MAX], TSKlenientConversion);
	*utf16 = '\0';

	if (retVal != TSKconversionOK) {
		fprintf(stderr, "Error Converting file name to UTF-16");
		return 1;
	}

	//append the file name onto the path
	wcsncat(path16full, name16, FILENAME_MAX - wcslen(path16full));

	//create the file
	HANDLE
		handle =
		CreateFileW((LPCTSTR)path16full, GENERIC_WRITE, 0, NULL, OPEN_ALWAYS,
			FILE_ATTRIBUTE_NORMAL, NULL);
	if (handle == INVALID_HANDLE_VALUE) {
		fprintf(stderr, "Error Creating File (%S)", path16full);
		return 1;
	}

	//try to write to the file
	if (tsk_fs_file_walk(a_fs_file, (TSK_FS_FILE_WALK_FLAG_ENUM)0,
		file_walk_cb, handle)) {
		fprintf(stderr, "Error writing file %S\n", path16full);
		tsk_error_print(stderr);
		CloseHandle(handle);
		return 1;
	}

	CloseHandle(handle);

#else
	struct stat
		statds;
	char
		fbuf[PATH_MAX];
	FILE *
		hFile;

	snprintf(fbuf, PATH_MAX, "%s/%s/%s", (char *)m_base_dir, m_vsName,
		a_path);

	// clean up any control characters in path
	for (size_t i = 0; i < strlen(fbuf); i++) {
		if (TSK_IS_CNTRL(fbuf[i]))
			fbuf[i] = '^';
	}

	// see if the directory already exists. Create, if not.
	if (0 != lstat(fbuf, &statds)) {
		size_t
			len = strlen(fbuf);
		for (size_t i = 0; i < len; i++) {
			if (((i > 0) && (fbuf[i] == '/') && (fbuf[i - 1] != '/'))
				|| ((fbuf[i] != '/') && (i == len - 1))) {
				uint8_t
					replaced = 0;

				if (fbuf[i] == '/') {
					fbuf[i] = '\0';
					replaced = 1;
				}
				if (0 != lstat(fbuf, &statds)) {
					if (mkdir(fbuf, 0775)) {
						fprintf(stderr,
							"Error making directory (%s) (%x)\n", fbuf,
							errno);
						return 1;
					}
				}
				if (replaced)
					fbuf[i] = '/';
			}
		}
	}

	if (fbuf[strlen(fbuf) - 1] != '/')
		strncat(fbuf, "/", PATH_MAX - strlen(fbuf));

	strncat(fbuf, a_fs_file->name->name, PATH_MAX - strlen(fbuf));

	//do name mangling of the file name that was just added
	for (int i = strlen(fbuf) - 1; fbuf[i] != '/'; i--) {
		if (TSK_IS_CNTRL(fbuf[i]))
			fbuf[i] = '^';
	}


	// open the file
	if ((hFile = fopen(fbuf, "w+")) == NULL) {
		fprintf(stderr, "Error opening file for writing (%s)\n", fbuf);
		return 1;
	}

	if (tsk_fs_file_walk(a_fs_file, (TSK_FS_FILE_WALK_FLAG_ENUM)0,
		file_walk_cb, hFile)) {
		fprintf(stderr, "Error writing file: %s\n", fbuf);
		tsk_error_print(stderr);
		fclose(hFile);
		return 1;
	}

	fclose(hFile);

#endif

	m_fileCount++;
	if (tsk_verbose)
		tsk_fprintf(stderr, "Recovered file %s%s (%" PRIuINUM ")\n",
			a_path, a_fs_file->name->name, a_fs_file->name->meta_addr);

	return 0;
}


TSK_RETVAL_ENUM CarpeTsk::processFile(TSK_FS_FILE * fs_file, const char *path)
{
	// skip a bunch of the files that we don't want to write
	if (isDotDir(fs_file))
		return TSK_OK;
	else if (isDir(fs_file))
		return TSK_OK;
	else if ((isNtfsSystemFiles(fs_file, path)) || (isFATSystemFiles(fs_file)))
		return TSK_OK;
	else if ((fs_file->meta == NULL) || (fs_file->meta->size == 0))
		return TSK_OK;

	writeFile(fs_file, path);
	return TSK_OK;
}


TSK_FILTER_ENUM
CarpeTsk::filterVol(const TSK_VS_PART_INFO * /*vs_part*/)
{
	/* Set the flag to show that we have a volume system so that we
	 * make a volume directory. */
	m_writeVolumeDir = true;
	return TSK_FILTER_CONT;
}

TSK_FILTER_ENUM
CarpeTsk::filterFs(TSK_FS_INFO * fs_info)
{
	// make a volume directory if we analyzing a volume system
	if (m_writeVolumeDir) {
		snprintf(m_vsName, FILENAME_MAX, "vol_%" PRIdOFF "/",
			fs_info->offset / m_img_info->sector_size);
	}

	return TSK_FILTER_CONT;
}

uint8_t
CarpeTsk::openFs(TSK_OFF_T a_soffset, TSK_FS_TYPE_ENUM fstype, TSK_POOL_TYPE_ENUM pooltype, TSK_DADDR_T pvol_block)
{
	if (pvol_block == 0) {
		if ((m_fs_info = tsk_fs_open_img_decrypt(m_img_info, a_soffset * m_img_info->sector_size,
			fstype, "")) == NULL) {
			tsk_error_print(stderr);
			if (tsk_error_get_errno() == TSK_ERR_FS_UNSUPTYPE)
				tsk_fs_type_print(stderr);
			return TSK_ERR;
		}
	}
	else {
		const TSK_POOL_INFO *pool = tsk_pool_open_img_sing(m_img_info, a_soffset * m_img_info->sector_size, pooltype);
		if (pool == NULL) {
			tsk_error_print(stderr);
			if (tsk_error_get_errno() == TSK_ERR_FS_UNSUPTYPE)
				tsk_pool_type_print(stderr);
			return TSK_ERR;
		}

		m_img_info = pool->get_img_info(pool, pvol_block);
		if ((m_fs_info = tsk_fs_open_img_decrypt(m_img_info, a_soffset * m_img_info->sector_size, fstype, "")) == NULL) {
			tsk_error_print(stderr);
			if (tsk_error_get_errno() == TSK_ERR_FS_UNSUPTYPE)
				tsk_fs_type_print(stderr);
			return TSK_ERR;
		}
	}
	return TSK_OK;
}

uint8_t
CarpeTsk::findFiles(TSK_INUM_T a_dirInum)
{
	uint8_t retval;
	if (a_dirInum)
		retval = findFilesInFs(m_fs_info, a_dirInum);
	else
		retval = findFilesInFs(m_fs_info);

	printf("Files Recovered: %d\n", m_fileCount);
	return retval;
}

int
main(int argc, char **argv1)
{
	TSK_IMG_TYPE_ENUM imgtype = TSK_IMG_TYPE_DETECT;
	TSK_FS_TYPE_ENUM fstype = TSK_FS_TYPE_DETECT;
	int ch;
	TSK_TCHAR **argv;
	unsigned int ssize = 0;
	TSK_OFF_T soffset = 0;
	TSK_TCHAR *cp;
	TSK_FS_DIR_WALK_FLAG_ENUM walkflag = TSK_FS_DIR_WALK_FLAG_UNALLOC;
	TSK_INUM_T dirInum = 0;
	TSK_TCHAR *database = NULL;
	bool bLoadFlag = false;
	bool bRecoverFlag = false;
	bool bFsstatFlag = false;
	bool blkMapFlag = true;   // true if we are going to write the block map
	bool createDbFlag = true; // true if we are going to create a new database
	bool calcHash = false;
	TSK_IMG_INFO *img;
	TSK_OFF_T imgaddr = 0;
	TSK_FS_INFO *fs;
	uint8_t type = 0;
	TSK_POOL_TYPE_ENUM pooltype = TSK_POOL_TYPE_DETECT;
	TSK_OFF_T pvol_block = 0;
	int num;
	FILE *fp;

#ifdef TSK_WIN32
	// On Windows, get the wide arguments (mingw doesn't support wmain)
	argv = CommandLineToArgvW(GetCommandLineW(), &argc);
	if (argv == NULL) {
		fprintf(stderr, "Error getting wide arguments\n");
		exit(1);
	}
#else
	argv = (TSK_TCHAR **)argv1;
#endif

	progname = argv[0];
	setlocale(LC_ALL, "");


	if (argc == 1) {
		usage();
		exit(0);
	}

	ch = GETOPT(argc, argv, _TSK_T("F:ab:d:hi:kvVz:"));
	switch(ch) {
		case _TSK_T('?'):
		default:
			usage();
			break;
		case _TSK_T('F'):
			if (TSTRCMP(OPTARG, _TSK_T("recover")) == 0) {
				bRecoverFlag = true;
			}
			else if (TSTRCMP(OPTARG, _TSK_T("loaddb")) == 0) {
				bLoadFlag = true;
			}
			else if (TSTRCMP(OPTARG, _TSK_T("fsstat")) == 0) {
				bFsstatFlag = true;
			}

			break;
	}


	if (bLoadFlag) {
	// loaddb
		while ((ch = GETOPT(argc, argv, _TSK_T("ab:d:hi:kvVz:"))) > 0) {
			switch (ch) {
				case _TSK_T('?'):
				default:
					TFPRINTF(stderr, _TSK_T("Invalid argument: %s\n"),
						argv[OPTIND]);
					usage();

				case _TSK_T('a'):
					createDbFlag = false;
					break;

				case _TSK_T('b'):
					ssize = (unsigned int)TSTRTOUL(OPTARG, &cp, 0);
					if (*cp || *cp == *OPTARG || ssize < 1) {
						TFPRINTF(stderr,
							_TSK_T
							("invalid argument: sector size must be positive: %s\n"),
							OPTARG);
						usage();
					}
					break;

				case _TSK_T('i'):
					if (TSTRCMP(OPTARG, _TSK_T("list")) == 0) {
						tsk_img_type_print(stderr);
						exit(1);
					}
					imgtype = tsk_img_type_toid(OPTARG);
					if (imgtype == TSK_IMG_TYPE_UNSUPP) {
						TFPRINTF(stderr, _TSK_T("Unsupported image type: %s\n"),
							OPTARG);
						usage();
					}
					break;

				case _TSK_T('k'):
					blkMapFlag = false;
					break;

				case _TSK_T('h'):
					calcHash = true;
					break;

				case _TSK_T('d'):
					database = OPTARG;
					break;

				case _TSK_T('v'):
					tsk_verbose++;
					break;

				case _TSK_T('V'):
					tsk_version_print(stdout);
					exit(0);

				case _TSK_T('z'):
					TSK_TCHAR envstr[32];
					TSNPRINTF(envstr, 32, _TSK_T("TZ=%s"), OPTARG);
					if (0 != TPUTENV(envstr)) {
						tsk_fprintf(stderr, "error setting environment");
						exit(1);
					}
					TZSET();
					break;
			}
		}

		/* We need at least one more argument */
		if (OPTIND >= argc) {
			tsk_fprintf(stderr, "Missing image names\n");
			usage();
		}

		TSK_TCHAR buff[1024];

		if (database == NULL) {
			if (createDbFlag == false) {
				fprintf(stderr, "Error: -a requires that database be specified with -d\n");
				usage();
			}
			TSNPRINTF(buff, 1024, _TSK_T("%s.db"), argv[OPTIND]);
			database = buff;
		}

		//CarpeTsk.setFileFilterFlags(TSK_FS_DIR_WALK_FLAG_UNALLOC);

		TskCaseDb * tskCase;

		if (createDbFlag) {
			tskCase = TskCaseDb::newDb(database);
		}
		else {
			tskCase = TskCaseDb::openDb(database);
		}

		if (tskCase == NULL) {
			tsk_error_print(stderr);
			exit(1);
		}

		TskAutoDb *autoDb = tskCase->initAddImage();
		autoDb->createBlockMap(blkMapFlag);
		autoDb->hashFiles(calcHash);
		autoDb->setAddUnallocSpace(true);

		if (autoDb->startAddImage(argc - OPTIND, &argv[OPTIND], imgtype, ssize)) {
			std::vector<TskAuto::error_record> errors = autoDb->getErrorList();
			for (size_t i = 0; i < errors.size(); i++) {
				fprintf(stderr, "Error: %s\n", TskAuto::errorRecordToString(errors[i]).c_str());
			}
		}

		if (autoDb->commitAddImage() == -1) {
			tsk_error_print(stderr);
			exit(1);
		}
		TFPRINTF(stdout, _TSK_T("Database stored at: %s\n"), database);

		autoDb->closeImage();
		delete tskCase;
		delete autoDb;
	}
	// recover
	else if (bRecoverFlag) {
		while ((ch = GETOPT(argc, argv, _TSK_T("ab:B:d:ef:i:o:P:vV"))) > 0) {
			switch (ch) {
				case _TSK_T('?'):
				default:
					TFPRINTF(stderr, _TSK_T("Invalid argument: %s\n"),
						argv[OPTIND]);
					usage();

				case _TSK_T('a'):
					walkflag = TSK_FS_DIR_WALK_FLAG_ALLOC;
					break;

				case _TSK_T('b'):
					ssize = (unsigned int)TSTRTOUL(OPTARG, &cp, 0);
					if (*cp || *cp == *OPTARG || ssize < 1) {
						TFPRINTF(stderr,
							_TSK_T
							("invalid argument: sector size must be positive: %s\n"),
							OPTARG);
						usage();
					}
					break;

				case _TSK_T('d'):
					if (tsk_fs_parse_inum(OPTARG, &dirInum, NULL, NULL, NULL, NULL)) {
						TFPRINTF(stderr,
							_TSK_T("invalid argument for directory inode: %s\n"),
							OPTARG);
						usage();
					}
					break;

				case _TSK_T('e'):
					walkflag =
						(TSK_FS_DIR_WALK_FLAG_ENUM)(TSK_FS_DIR_WALK_FLAG_UNALLOC |
							TSK_FS_DIR_WALK_FLAG_ALLOC);
					break;

				case _TSK_T('f'):

					if (TSTRCMP(OPTARG, _TSK_T("list")) == 0) {
						tsk_fs_type_print(stderr);
						exit(1);
					}
					fstype = tsk_fs_type_toid(OPTARG);
					if (fstype == TSK_FS_TYPE_UNSUPP) {
						TFPRINTF(stderr,
							_TSK_T("Unsupported file system type: %s\n"), OPTARG);
						usage();
					}
					if (fstype == TSK_FS_TYPE_APFS) {
						const auto img = tsk_img_open(1, &argv[OPTIND], imgtype, ssize);
						if (img == nullptr) {
							tsk_error_print(stderr);
							exit(1);
						}
						if ((imgaddr * img->sector_size) >= img->size) {
							tsk_fprintf(stderr,
								"Sector offset supplied is larger than disk image (maximum: %"
								PRIu64 ")\n", img->size / img->sector_size);
							exit(1);
						}

						const auto pool = tsk_pool_open_img_sing(img, imgaddr * img->sector_size, pooltype);
						if (pool == nullptr) {
							tsk_error_print(stderr);
							if (tsk_error_get_errno() == TSK_ERR_FS_UNSUPTYPE)
								tsk_pool_type_print(stderr);
							img->close(img);
							exit(1);
						}
						fp = fopen("pstat", "w+");

						if (type) {
							tsk_printf("%s\n", tsk_pool_type_toname(pool->ctype));
						}
						else {
							if (pool->poolstat(pool, fp)) {
								tsk_error_print(stderr);
								pool->close(pool);
								img->close(img);
								exit(1);
							}
						}
						pool->close(pool);
						img->close(img);
						fclose(fp);

						fp = fopen("pstat", "r");
			//			char * line = NULL;
						char line[1000];
						size_t line_len = 0;
						ssize_t read;
						for (int i = 0; i < 23; i++) {
							//read = getline(&line, &line_len, fp);
							fgets(line, 1000, fp);
						}
						char* p = strrchr(line, ' ');

						char* q = (char *)malloc(20);
						for (int i = 0; i <strlen(p) -1; i++) {
							q[i] = p[i+1];
						}
						pvol_block = atoi(q);
						fclose(fp);
					}

					break;


				case _TSK_T('i'):
					if (TSTRCMP(OPTARG, _TSK_T("list")) == 0) {
						tsk_img_type_print(stderr);
						exit(1);
					}
					imgtype = tsk_img_type_toid(OPTARG);
					if (imgtype == TSK_IMG_TYPE_UNSUPP) {
						TFPRINTF(stderr, _TSK_T("Unsupported image type: %s\n"),
							OPTARG);
						usage();
					}
					break;

				case _TSK_T('o'):
					if ((soffset = tsk_parse_offset(OPTARG)) == -1) {
						tsk_error_print(stderr);
						usage();
					}
					break;

				case _TSK_T('P'):
					if (TSTRCMP(OPTARG, _TSK_T("list")) == 0) {
						tsk_pool_type_print(stderr);
						exit(1);
					}
					pooltype = tsk_pool_type_toid(OPTARG);
					if (pooltype == TSK_POOL_TYPE_UNSUPP) {
						TFPRINTF(stderr,
							_TSK_T("Unsupported pool container type: %s\n"), OPTARG);
						usage();
					}
					break;

				case _TSK_T('B'):
					if ((pvol_block = tsk_parse_offset(OPTARG)) == -1) {
						tsk_error_print(stderr);
						exit(1);
					}
					break;

				case _TSK_T('v'):
					tsk_verbose++;
					break;

				case _TSK_T('V'):
					tsk_version_print(stdout);
					exit(0);
			}
		}
		walkflag =
			(TSK_FS_DIR_WALK_FLAG_ENUM)(TSK_FS_DIR_WALK_FLAG_UNALLOC |
				TSK_FS_DIR_WALK_FLAG_ALLOC);

		/* We need at least one more argument */
		if (OPTIND + 1 >= argc) {
			tsk_fprintf(stderr,
				"Missing output directory and/or image name\n");
			usage();
		}

		CarpeTsk carpeTsk(argv[argc - 1]);

		carpeTsk.setFileFilterFlags(walkflag);
		if (carpeTsk.openImage(argc - OPTIND - 1, &argv[OPTIND], imgtype,
			ssize)) {
			tsk_error_print(stderr);
			exit(1);
		}

		if (carpeTsk.openFs(soffset, fstype, pooltype, (TSK_DADDR_T)pvol_block)) {
			// Errors were already logged
			exit(1);
		}

		if (carpeTsk.findFiles(dirInum)) {
			// errors were already logged
			exit(1);
		}
	}
	// fsstat
	else if (bFsstatFlag) {
		while ((ch = GETOPT(argc, argv, _TSK_T("b:f:i:o:tvV"))) > 0) {
			switch (ch) {
				case _TSK_T('?'):
				default:
					TFPRINTF(stderr, _TSK_T("Invalid argument: %s\n"),
						argv[OPTIND]);
					usage();
				case _TSK_T('b'):
					ssize = (unsigned int)TSTRTOUL(OPTARG, &cp, 0);
					if (*cp || *cp == *OPTARG || ssize < 1) {
						TFPRINTF(stderr,
							_TSK_T
							("invalid argument: sector size must be positive: %s\n"),
							OPTARG);
						usage();
					}
					break;
				case _TSK_T('f'):
					if (TSTRCMP(OPTARG, _TSK_T("list")) == 0) {
						tsk_fs_type_print(stderr);
						exit(1);
					}
					fstype = tsk_fs_type_toid(OPTARG);
					if (fstype == TSK_FS_TYPE_UNSUPP) {
						TFPRINTF(stderr,
							_TSK_T("Unsupported file system type: %s\n"), OPTARG);
						usage();
					}
					break;

				case _TSK_T('i'):
					if (TSTRCMP(OPTARG, _TSK_T("list")) == 0) {
						tsk_img_type_print(stderr);
						exit(1);
					}
					imgtype = tsk_img_type_toid(OPTARG);
					if (imgtype == TSK_IMG_TYPE_UNSUPP) {
						TFPRINTF(stderr, _TSK_T("Unsupported image type: %s\n"),
							OPTARG);
						usage();
					}
					break;

				case _TSK_T('o'):
					if ((imgaddr = tsk_parse_offset(OPTARG)) == -1) {
						tsk_error_print(stderr);
						exit(1);
					}
					break;

				case _TSK_T('t'):
					type = 1;
					break;

				case _TSK_T('v'):
					tsk_verbose++;
					break;

				case _TSK_T('V'):
					tsk_version_print(stdout);
					exit(0);
			}
		}

		/* We need at least one more argument */
		if (OPTIND >= argc) {
			tsk_fprintf(stderr, "Missing image name\n");
			usage();
		}

		if ((img =
			tsk_img_open(argc - OPTIND, &argv[OPTIND], imgtype,
				ssize)) == NULL) {
			tsk_error_print(stderr);
			exit(1);
		}
		if ((imgaddr * img->sector_size) >= img->size) {
			tsk_fprintf(stderr,
				"Sector offset supplied is larger than disk image (maximum: %"
				PRIu64 ")\n", img->size / img->sector_size);
			exit(1);
		}

		if ((fs = tsk_fs_open_img(img, imgaddr * img->sector_size, fstype)) == NULL) {
			tsk_error_print(stderr);
			if (tsk_error_get_errno() == TSK_ERR_FS_UNSUPTYPE)
				tsk_fs_type_print(stderr);
			img->close(img);
			exit(1);
		}

		if (type) {
			tsk_printf("%s\n", tsk_fs_type_toname(fs->ftype));
		}
		else {
			if (fs->fsstat(fs, stdout)) {
				tsk_error_print(stderr);
				fs->close(fs);
				img->close(img);
				exit(1);
			}
		}

		fs->close(fs);
		img->close(img);
	}

	exit(0);
}
