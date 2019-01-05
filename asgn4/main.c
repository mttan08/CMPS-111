#define FUSE_USE_VERSION 26

#include <fuse.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>

#include "directory.h"
#include "superblock.h"
#include "fsbase.h"

static FileSystemBase fsb;

static int ops_getattr(const char *path, struct stat *stbuf)
{
    int res;
    memset(stbuf, 0, sizeof(struct stat));

    char *_path = malloc(strlen(path) + 1);
    strcpy(_path, path);

    printf("ops_getattr: path: %s\n", path);

    Directory dir;
    res = fsb_pathToDirectory(&fsb, path, NULL, &dir, NULL);
    if(res == 0){
        printf("ops_getattr: dir is: fileLength: %d, startBlock: %d\n",
               dir.fileLength, dir.startBlock);
        stbuf->st_ctime = dir.creTime;
        stbuf->st_atime = dir.accTime;
        stbuf->st_mtime = dir.modTime;
        stbuf->st_size = dir.fileLength;
        stbuf->st_blocks = dir.startBlock;
        stbuf->st_uid = getuid();
        stbuf->st_gid = getgid();

        mode_t mode = 0;
        mode |= dir.flags == 1? S_IFDIR | 0777 : S_IFREG | 0777;

        stbuf->st_mode = mode;
        return 0;
    }
    return -ENOENT;
}

static int ops_access(const char *path, int i)
{
    return 0;
}

static int ops_readdir(const char *path,
                       void *buf,
                       fuse_fill_dir_t filler,
                       off_t offset,
                       struct fuse_file_info *fi)
{
    printf("ops_readdir: path is: %s\n", path);

    (void) offset;
    (void) fi;
    Directory dir;
    DirectoryLocation dl;

    char *_path = malloc(strlen(path) + 1);
    strcpy(_path, path);

    int res = fsb_pathToDirectory(&fsb, _path, NULL, &dir, &dl);
    if(res != 0 || dir.flags != 1) {
        free(_path);
        return -ENOENT;
    }

    filler(buf, ".", NULL, 0);
    filler(buf, "..", NULL, 0);

    Directory *subDirs = malloc(dir.fileLength);
    res = fsb_readDir(&fsb, (char *)subDirs, &dl, &dir, 0, dir.fileLength);
    if (res != 0) return -ENOENT;

    int len = dir.fileLength/sizeof(Directory);
    for (int i = 0; i < len; ++i) {
        filler(buf, subDirs[i].fileName, NULL, 0);
    }

    free(subDirs);
    return 0;
}

static int ops_opendir(const char *path, struct fuse_file_info *fi)
{
    printf("opendir\n");
    //just always succeed, we don't care about "permissions"
    return 0;
}

static int ops_mkdir(const char *path, mode_t mode)
{
    printf("ops_mkdir: path: %s\n", path);

    Directory dir;
    return fsb_addDir(&fsb, path, 1, &dir, NULL);
}

static int ops_rmdir(const char *path)
{
    return fsb_removeFile(&fsb, path);
}

static int ops_open(const char *path , struct fuse_file_info *fi)
{
    //we just check that the file exists. Creation is filtered out before this point
    char *_path = malloc(strlen(path) + 1);
    strcpy(_path, path);

    Directory dir;

    int res = fsb_pathToDirectory(&fsb, _path, NULL, &dir, NULL);
    free(_path);
    if (res != 0) return -ENOENT;
    return 0;
}

static int ops_read(const char *path, char *buf, size_t size, off_t offset,
                    struct fuse_file_info *fi)
{
    printf("ops_read: path is: %s, size is: %ld, offset: %ld\n",
        path, size, offset);

    char *_path = malloc(strlen(path) + 1);
    strcpy(_path, path);

    Directory dir;
    DirectoryLocation dl;

    int res = fsb_pathToDirectory(&fsb, _path, NULL, &dir, &dl);
    free(_path);
    if (res != 0) return -ENOENT;

    res = fsb_readDir(&fsb, buf, &dl, &dir, offset, size);

    return res != 0? 0 : size;
}

static int ops_write(const char *path, const char *data, size_t size, off_t offset, struct fuse_file_info *fi)
{
    char *_path = malloc(strlen(path) + 1);
    strcpy(_path, path);

    Directory dir;
    DirectoryLocation dl;

    int res = fsb_pathToDirectory(&fsb, _path, NULL, &dir, &dl);
    free(_path);
    if (res != 0) return -ENOENT;

    res = fsb_writeDir(&fsb, data, &dl, &dir, offset, size);

    return res != 0? 0 : size;
}

static int ops_create(const char *path, mode_t mide, struct fuse_file_info *fi)
{
    printf("create\n");

    char *_path = malloc(strlen(path) + 1);
    strcpy(_path, path);

    Directory dir;
    DirectoryLocation dl;

    int res = fsb_pathToDirectory(&fsb, _path, NULL, &dir, &dl);
    free(_path);

    if (res == 0) {
        if (dir.flags == 1) {
            //can't overwrite a directory
            return -ENOENT;
        } else {
            //delete old file
            res = fsb_removeFile(&fsb, path);
            if (res != 0) return -ENOENT;
        }
    }

    return fsb_addDir(&fsb, path, 0, &dir, NULL);
}

static int ops_statfs(const char *path, struct statvfs *stbuf)
{
	int res;
	printf("statfs\n");

	res = statvfs(path, stbuf);
	if (res == -ENOENT)
		return -errno;

	return 0;
}

static int ops_unlink(const char *path)
{
    return fsb_removeFile(&fsb, path);
}

static struct fuse_operations ops = {
    .access     = ops_access,
    .getattr    = ops_getattr,
    .readdir    = ops_readdir,
    .opendir    = ops_opendir,
    .mkdir      = ops_mkdir,
    .rmdir      = ops_rmdir,
    .open       = ops_open,
    .read       = ops_read,
    .write      = ops_write,
    .unlink     = ops_unlink,
    .create     = ops_create,
    .statfs     = ops_statfs,
};

int main(int argc, char *argv[])
{
    FILE *file = fopen("disk.img", "rb+");
    if (file != NULL)
        fsb_load(&fsb, file);
    else {
        file = fopen("disk.img", "wb+");
        fsb_init(&fsb, file, 1024, 1, 512);
    }
    return fuse_main(argc, argv, &ops, NULL);
}
