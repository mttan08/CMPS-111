#include "fsbase.h"

#include <assert.h>
#include <string.h>
#include <stdlib.h>

int splitPath(const char *path, char *parentDir, char *name)
{
    size_t length = strlen(path);
    memcpy(parentDir, path, length + 1);
    char *lastSlash = strrchr(parentDir, '/');
    //should be at least a leading slash
    if (lastSlash == NULL) return -1;

    //slash is the last char
    if (lastSlash[1] == 0) return -1;

    ++lastSlash;

    size_t endLength = strlen(lastSlash);
    memcpy(name, lastSlash, endLength + 1);
    lastSlash[0] = 0;
}

void fsb_load(FileSystemBase *fsb, FILE *storageFile)
{
    fsb->storageFile = storageFile;
    fseek(storageFile, 0, SEEK_SET);
    int result = fread(&fsb->superBlock, sizeof(SuperBlock), 1, storageFile);
    assert(result == 1);
}

void fsb_init(FileSystemBase *fsb,
              FILE *storageFile,
              unsigned int totalBlocks,
              unsigned int faBlocks,
              unsigned int blockSize)
{
    fsb->storageFile = storageFile;
    fseek(storageFile, 0, SEEK_SET);

    sb_init(&fsb->superBlock, totalBlocks, faBlocks, blockSize, faBlocks + 1);

    int result = fwrite(&fsb->superBlock, sizeof(SuperBlock), 1, storageFile);
    assert(result == 1);

    fseek(storageFile, blockSize, SEEK_SET);
    int val = -1;

    //Value for superblock
    result = fwrite(&val, 4, 1, storageFile);
    assert(result == 1);

    //Value for FAT blocks
    for (int i = 1; i < faBlocks; ++i) {
        val = i + 1;
        result = fwrite(&val, 4, 1, storageFile);
        assert(result == 1);
    }

    //Value for final FAT block
    val = -2;
    result = fwrite(&val, 4, 1, storageFile);
    assert(result == 1);

    //Value for root dir block, and start of root dir block.
    val = -2;
    result = fwrite(&val, 4, 1, storageFile);
    assert(result == 1);
    result = fwrite(&val, 4, 1, storageFile);
    assert(result == 1);

    //Value for the rest of the blocks
    val = 0;
    for (int i = 0; i < totalBlocks - faBlocks - 3; ++i) {
        result = fwrite(&val, 4, 1, storageFile);
        assert(result == 1);
    }

    //write root Dir
    Directory rootDir;
    dir_init(&rootDir, "", 0, faBlocks + 2, 1);
    result = fsb_write(fsb, (char *)&rootDir, faBlocks + 1, 0, sizeof(Directory));
    assert(result == 0);
}

Directory rootDirTmp;

int fsb_pathToDirectory(FileSystemBase *fsb,
                        char *path,
                        Directory *baseDir,
                        Directory *dir,
                        DirectoryLocation *dl)
{
    if (baseDir == NULL) {
        int result = fsb_read(fsb, (char *)&rootDirTmp, fsb->superBlock.rootDirStart, 0, sizeof(Directory));
        if (result != 0) return -1;
        baseDir = &rootDirTmp;
        if (dl != NULL) {
            dl->block = fsb->superBlock.rootDirStart;
            dl->index = 0;
        }
    }

    if (path == NULL) {
        memcpy(dir, baseDir, sizeof(Directory));
        return 0;
    }

    size_t pathLen = strlen(path);
    if (pathLen == 0) return -1;
    if (path[0] == '/') {
        if (pathLen == 1) {
            memcpy(dir, baseDir, sizeof(Directory));
            return 0;
        }
        ++path;
        --pathLen;
    }

    char *slash = memchr(path, '/', pathLen);
    if (slash != NULL) {
        //the current word in the path best be a directory...
        if (!(baseDir->flags | 1)) return -1;//Is a folder
        slash[0] = 0;
        ++slash;
        if (slash[0] == 0) slash = NULL;
    }

    unsigned int subDirCnt = baseDir->fileLength / sizeof(Directory);
    Directory *subDirs = malloc(baseDir->fileLength);
    int result = fsb_read(fsb, (char *)subDirs, baseDir->startBlock, 0, baseDir->fileLength);
    if (result != 0) return -1;

    for (int i = 0; i < subDirCnt; ++i) {
        if (strcmp(path, subDirs[i].fileName) != 0) continue;

        //We have found the correct directory
        Directory newBase = subDirs[i];
        free(subDirs);
        if (dl != NULL) {
            dl->block = baseDir->startBlock;
            dl->index = i;
        }
        return fsb_pathToDirectory(fsb, slash, &newBase, dir, dl);
    }

    return -1;
}

//Does a read from an inputed Directory location, dl. dir can be NULL
//dest must have at least numBytes avalible
int fsb_readDir(FileSystemBase *fsb,
                char *dest,
                DirectoryLocation *dl,
                Directory *dir,
                unsigned int offset,
                unsigned int numBytes)
{
    int result;

    //stack directory incase dir == NULL;
    Directory _dir;
    if (dir == NULL) {
        result = fsb_read(fsb, (char *)&_dir, dl->block, dl->index * sizeof(Directory), sizeof(Directory));
        if (result != 0) return -1;
        dir = &_dir;
    }

    //do the read
    result = fsb_read(fsb, dest, dir->startBlock, offset, numBytes);
    if (result != 0) return -1;

    //write an update of the directory
    dir_updateAccessTime(dir);
    result = fsb_write(fsb, (char *)dir, dl->block, dl->index * sizeof(Directory), sizeof(Directory));
    return result;
}

int fsb_writeDir(FileSystemBase *fsb,
                 char *source,
                 DirectoryLocation *dl,
                 Directory *dir,
                 unsigned int offset,
                 unsigned int numBytes)
{
    int result;

    //stack directory incase dir == NULL;
    Directory _dir;
    if (dir == NULL) {
        result = fsb_read(fsb, (char *)&_dir, dl->block, dl->index * sizeof(Directory), sizeof(Directory));
        if (result != 0) return -1;
        dir = &_dir;
    }

    //do the read
    result = fsb_write(fsb, source, dir->startBlock, offset, numBytes);
    if (result != 0) return -1;

    //write an update of the directory
    dir_updateModificationTime(dir);
    if (offset + numBytes > dir->fileLength)
        dir->fileLength = offset + numBytes;
    result = fsb_write(fsb, (char *)dir, dl->block, dl->index * sizeof(Directory), sizeof(Directory));
    return result;
}

int fsb_read(FileSystemBase *fsb,
             char *dest,
             unsigned int startBlock,
             unsigned int startOffset,
             unsigned int numBytes)
{
    SuperBlock *sb = &fsb->superBlock;

    //we go ahead and load the whole FAT
    int fat[4 * sb->totalBlocks];
    fseek(fsb->storageFile, sb->blockSize, SEEK_SET);
    int result = fread(fat, 4, sb->totalBlocks, fsb->storageFile);
    if (result != sb->totalBlocks) return -1;
    //first get to start position. startOffset can be greater than blockSize, so some traversal
    //is needed.
    while (startOffset > sb->blockSize) {
        if (startBlock <= 0) return -1;
        startBlock = fat[startBlock];
        if (startBlock >= 0 && startBlock != -2) return -1;

        startOffset -= sb->blockSize;
    }

    //we are at the right block, now we start reading into dest
    while (1) {
        unsigned int readPos = startBlock * sb->blockSize + startOffset;
        unsigned bytesLeftInBlock = sb->blockSize - startOffset;
        startOffset = 0;

        unsigned int readAmount;
        if (bytesLeftInBlock < numBytes) {
            readAmount = bytesLeftInBlock;
        } else {
            readAmount = numBytes;
        }

        numBytes -= readAmount;

        fseek(fsb->storageFile, readPos, SEEK_SET);
        result = fread(dest, readAmount, 1, fsb->storageFile);
        if (result != 1) return -1;

        dest += readAmount;

        if (numBytes != 0) {
            int next = fat[startBlock];
            if (next <= 0) return -1;
            startBlock = next;
        } else {
            break;
        }
    }

    return 0;
}

int fsb_write(FileSystemBase *fsb,
              char *source,
              unsigned int startBlock,
              unsigned int startOffset,
              unsigned int numBytes)
{
    SuperBlock *sb = &fsb->superBlock;

    //we go ahead and load the whole FAT
    int fat[4 * sb->totalBlocks];
    fseek(fsb->storageFile, sb->blockSize, SEEK_SET);
    int result = fread(fat, 4, sb->totalBlocks, fsb->storageFile);
    if (result != sb->totalBlocks) return -1;
    //first get to start position. startOffset can be greater than blockSize, so some traversal
    //is needed.
    while (startOffset > sb->blockSize) {
        int next = fat[startBlock];
        if (next == -1 || next == 0) return -1;
        //allocate more space for the file TODO update file's directory
        if (next == -2) {
            result = fsb_expandBlock(fsb, startBlock, fat);
            if (result != 0 || fat[startBlock] != -2) return -1;
            next = fat[startBlock];
        }

        startBlock = next;
        startOffset -= sb->blockSize;
    }

    //we are at the right block, now we start writing from source
    while (1) {
        unsigned int writePos = startBlock * sb->blockSize + startOffset;
        unsigned bytesLeftInBlock = sb->blockSize - startOffset;
        startOffset = 0;

        unsigned int writeAmount;
        if (bytesLeftInBlock < numBytes) {
            writeAmount = bytesLeftInBlock;
        } else {
            writeAmount = numBytes;
        }

        numBytes -= writeAmount;

        fseek(fsb->storageFile, writePos, SEEK_SET);
        result = fwrite(source, writeAmount, 1, fsb->storageFile);
        if (result != 1) return -1;
        source += writeAmount;

        if (numBytes != 0) {
            int next = fat[startBlock];

            if (next == -1 || next == 0) return -1;
            if (next == -2) {
                result = fsb_expandBlock(fsb, startBlock, fat);
                if (result != 0) return -1;
                next = fat[startBlock];
            }

            startBlock = next;
        } else {
            break;
        }
    }

    return 0;
}

int fsb_addDir(FileSystemBase *fsb,
               const char *fullPath,
               int isFolder,
               Directory *dir,
               DirectoryLocation *dl)
{
    Directory parentDir;
    DirectoryLocation parentDirLoc;
    int result;

    char path[100];
    char name[100];

    result = splitPath(fullPath, path, name);
    if (result == -1) return -1;

    result = fsb_pathToDirectory(fsb, path, NULL, &parentDir, &parentDirLoc);
    if (result != 0) return -1;

    //make the new directory
    dir_init(dir, name, 0, fsb_findFreeBlock(fsb, NULL), isFolder);

    //write the new directory
    result = fsb_write(fsb, (char *)dir, parentDir.startBlock, parentDir.fileLength, sizeof(Directory));
    if (result != 0) return -1;

    //set dl
    if (dl != NULL) {
        dl->block = parentDir.startBlock;
        dl->index = parentDir.fileLength / sizeof(Directory);
    }

    //update parentDir to reflect the addition
    parentDir.fileLength += sizeof(Directory);
    dir_updateModificationTime(&parentDir);

    //write parentDir back
    result = fsb_write(fsb, (char *)&parentDir, parentDirLoc.block, parentDirLoc.index * sizeof(Directory), sizeof(Directory));
    if (result != 0) return -1;
    return 0;
}

int fsb_removeFile(FileSystemBase *fsb, const char *path)
{
    int result;

    char parentPath[100];
    char toRemoveName[100];

    result = splitPath(path, parentPath, toRemoveName);
    if (result == -1) return -1;

    Directory parentDir;
    DirectoryLocation parentDL;
    result = fsb_pathToDirectory(fsb, parentPath, NULL, &parentDir, &parentDL);
    if (result != 0) return -1;

    Directory dir;
    DirectoryLocation dl;
    result = fsb_pathToDirectory(fsb, toRemoveName, &parentDir, &dir, &dl);
    if (result != 0) return -1;

    //can't remove non-empty folder
    if ((dir.flags & 1) && dir.fileLength != 0) return -1;

    //free all the blocks of the file
    result = fsb_freeChain(fsb, dir.startBlock, NULL);
    if (result != 0) return -1;

    //need to remove the directory listing, and move other directories to fill the slot.
    unsigned int totalDirs = parentDir.fileLength / sizeof(Directory);
    //if this was the last entry, don't need to do anything.
    if (dl.index != totalDirs - 1) {
        //we read the last entry
        Directory lastDir;
        result = fsb_read(fsb, (char *)&lastDir, parentDir.startBlock, (totalDirs - 1) * sizeof(Directory), sizeof(Directory));
        if (result != 0) return -1;
        //write it into the removed dir's slot
        result = fsb_write(fsb, (char *)&lastDir, parentDir.startBlock, dl.index * sizeof(Directory), sizeof(Directory));
        if (result != 0) return -1;
    }
    //update parent dir;
    parentDir.fileLength -= sizeof(Directory);
    dir_updateModificationTime(&parentDir);

    return fsb_write(fsb, (char *)&parentDir, parentDL.block, parentDL.index * sizeof(Directory), sizeof(Directory));
}

int fsb_freeChain(FileSystemBase *fsb, unsigned int startBlock, int *fat)
{
    int shouldFree = (fat == NULL);
    if (shouldFree) {
        SuperBlock *sb = &fsb->superBlock;
        fat = malloc(4 * sb->totalBlocks);
        fseek(fsb->storageFile, sb->blockSize, SEEK_SET);
        int result = fread(fat, 4, sb->totalBlocks, fsb->storageFile);
        if (result != sb->totalBlocks) {
            free(fat);
            return -1;
        }
    }

    int next = fat[startBlock];
    fat[startBlock] = 0;
    int result = fsb_write(fsb, (char*) &fat[startBlock], 1, startBlock * 4, 4);
    if (result != 0) return -1;
    if (next > 0) {
        result = fsb_freeChain(fsb, next, fat);
        if (shouldFree) free(fat);
        return result;
    }

    return 0;
}

unsigned int fsb_findFreeBlock(FileSystemBase *fsb, int *fat)
{
    SuperBlock *sb = &fsb->superBlock;
    int shouldFree = (fat == NULL);

    if (fat == NULL) {
        fat = malloc(4 * sb->totalBlocks);

        fseek(fsb->storageFile, sb->blockSize, SEEK_SET);
        int result = fread(fat, 4, sb->totalBlocks, fsb->storageFile);
        if (result != sb->totalBlocks) {
            free(fat);
            return -1;
        }
    }

    for (int i = 0; i < sb->totalBlocks; ++i) {
        if (fat[i] != 0) continue;

        fat[i] = -2;
        fseek(fsb->storageFile, sb->blockSize + i * 4, SEEK_SET);
        int result = fwrite(&fat[i], 4, 1, fsb->storageFile);
        if (result != 1) return -1;

        if (shouldFree) free(fat);
        return i;
    }

    if (shouldFree) free(fat);
    return -1;
}

int fsb_expandBlock(FileSystemBase *fsb, int block, int *fat)
{
    if (block <= 0) return -1;

    int newBlock = fsb_findFreeBlock(fsb, fat);
    if (newBlock == -1) return -1;

    if (fat != NULL)
        fat[block] = newBlock;

    fseek(fsb->storageFile, fsb->superBlock.blockSize + block * 4, SEEK_SET);
    int result = fwrite(&newBlock, 4, 1, fsb->storageFile);
    if (result != 1) return -1;
    return 0;
}
