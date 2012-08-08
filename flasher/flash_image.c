/*
 * Copyright (C) 2008 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "cutils/log.h"
#include "flashutils.h"
#include "roots.h"
#include "mtdutils.h"
#include <sys/stat.h>
#include <string.h>
#include <sys/vfs.h>
#include <libgen.h>
#include "unyaffs.h"

static int unyaffs_wrapper(const char* backup_file_image, const char* backup_path, int callback) {
    /*char tmp[PATH_MAX];
    sprintf(tmp, "cd %s ; unyaffs %s ; exit $?", backup_path, backup_file_image);
    FILE *fp = __popen(tmp, "r");
    if (fp == NULL) {
        printf("Unable to execute unyaffs.\n");
        return -1;
    }

    while (fgets(tmp, PATH_MAX, fp) != NULL) {
        tmp[PATH_MAX - 1] = NULL;
    }

    return __pclose(fp);*/
    return unyaffs(backup_file_image, backup_path, NULL);
}

typedef int (*nandroid_restore_handler)(const char* backup_file_image, const char* backup_path, int callback);

static void ensure_directory(const char* dir) {
    char tmp[PATH_MAX];
    sprintf(tmp, "mkdir -p %s ; chmod 777 %s", dir, dir);
    __system(tmp);
}


int format_device(const char *device, const char *path, const char *fs_type) {
    Volume* v = volume_for_path(path);
    if (v == NULL) {
        // silent failure for sd-ext
        if (strcmp(path, "/sd-ext") == 0)
            return -1;
        printf("unknown volume \"%s\"\n", path);
        return -1;
    }
    if (strcmp(fs_type, "ramdisk") == 0) {
        // you can't format the ramdisk.
        printf("can't format_volume \"%s\"", path);
        return -1;
    }

    if (ensure_path_unmounted(path) != 0) {
        printf("format_volume failed to unmount \"%s\"\n", v->mount_point);
        return -1;
    }

    if (strcmp(fs_type, "yaffs2") == 0 || strcmp(fs_type, "mtd") == 0) {
        mtd_scan_partitions();
        const MtdPartition* partition = mtd_find_partition_by_name(device);
        if (partition == NULL) {
            printf("format_volume: no MTD partition \"%s\"\n", device);
            return -1;
        }
        MtdWriteContext *write = mtd_write_partition(partition);
        if (write == NULL) {
            printf("format_volume: can't open MTD \"%s\"\n", device);
            return -1;
        } else if (mtd_erase_blocks(write, -1) == (off_t) -1) {
            printf("format_volume: can't erase MTD \"%s\"\n", device);
            mtd_write_close(write);
            return -1;
        } else if (mtd_write_close(write)) {
            printf("format_volume: can't close MTD \"%s\"\n",device);
            return -1;
        }
        return 0;
    }
    return -1;
}

int nandroid_restore_partition_extended(const char* backup_path, const char* mount_point, int umount_when_finished) {
    int ret = 0;
    char* name = basename(mount_point);

    nandroid_restore_handler restore_handler = NULL;
    const char *filesystems[] = { "yaffs2", "ext2", "ext3", "ext4", "vfat", "rfs", NULL };
    const char* backup_filesystem = NULL;
    Volume *vol = volume_for_path(mount_point);
    const char *device = NULL;
    if (vol != NULL)
        device = vol->device;

    char tmp[PATH_MAX];
    sprintf(tmp, "%s", backup_path, name);
    struct stat file_info;
    if (0 != (ret = statfs(tmp, &file_info))) {
        // can't find the backup, it may be the new backup format?
        // iterate through the backup types
        printf("couldn't find default\n");
        char *filesystem;
        int i = 0;
        while ((filesystem = filesystems[i]) != NULL) {
            sprintf(tmp, "%s/%s.%s.img", backup_path, name, filesystem);
            if (0 == (ret = statfs(tmp, &file_info))) {
                backup_filesystem = filesystem;
                restore_handler = unyaffs_wrapper;
                break;
            }
            i++;
        }

        if (backup_filesystem == NULL || restore_handler == NULL) {
            printf("%s.img not found. Skipping restore of %s.\n", name, mount_point);
            return 0;
        }
        else {
            printf("Found new backup image: %s\n", tmp);
        }
    }

    ensure_directory(mount_point);

    int callback = 1;
    
    if (ensure_path_unmounted(mount_point) != 0 ) {
       printf("Failed to unmount %s\n", mount_point);
       return -1;
    }   

    printf("Flashing %s...\n", name);
    if (backup_filesystem == NULL) {
        if (0 != (ret = format_volume(mount_point))) {
            printf("Error while formatting %s!\n", mount_point);
            return ret;
        }
    }
    else if (0 != (ret = format_device(device, mount_point, backup_filesystem))) {
        printf("Error while formatting %s!\n", mount_point);
        return ret;
    }

    if (0 != (ret = ensure_path_mounted(mount_point))) {
        printf("Can't mount %s!\n", mount_point);
        return ret;
    }

    if (restore_handler == NULL) {
        //printf("Error finding an appropriate restore handler.\n");
        restore_handler = unyaffs_wrapper;
        //return -2;
    }
    if (0 != (ret = restore_handler(tmp, mount_point, callback))) {
        printf("Error while restoring %s!\n", mount_point);
        return ret;
    }

    ensure_path_unmounted(mount_point);
    
    return 0;
}

int main(int argc, char **argv)
{
    if (argc != 3) {
        fprintf(stderr, "usage: %s partition file.img\n", argv[0]);
        return 2;
    }
    //int ret = restore_raw_partition(NULL, argv[1], argv[2]);
    char tmp[PATH_MAX];
    sprintf(tmp, "/%s", argv[1]);
    load_volume_table();
    int ret = nandroid_restore_partition_extended(argv[2], tmp,1);
    if (ret != 0)
        fprintf(stderr, "failed with error: %d\n", ret);
    return ret;
}

