#define _FILE_OFFSET_BITS 64
// for O_TMPFILE
#define _GNU_SOURCE

#include <fcntl.h>
#include <linux/fs.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <err.h>
#include <error.h>
#include <libgen.h>
#include <unistd.h>
#include <inttypes.h>

static void usage(FILE *fp)
{
	fprintf(fp, "Usage: ficlone <source> <destination> <start> [<end> [<target_offset>]]\n");
}

uint64_t read64(const char *str)
{
	uint64_t rv;
	if (0 >= sscanf(str, "%" SCNd64, &rv)) {
		error(1, 0, "not a valid number: %s", str);
	}
	return rv;
}

int main(int argc, char *argv[])
{
	if (argc == 2 && (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0)) {
		usage(stdout);
		return 0;
	}
	if (argc < 4) {
		usage(stderr);
		return 1;
	}
	const char *src = argv[1];
	int src_fd = open(src, O_RDONLY);
	if (src_fd < 0) {
		err(1, "open(\"%s\", ...)", argv[1]);
	}
	const char *dst = argv[2];
	char *dst2 = strdup(dst);
	if (dst2 == NULL) {
		err(1, "strdup()");
	}
	const char *dst_dir = dirname(dst2);
	int dst_fd = open(dst_dir, O_TMPFILE | O_RDWR, S_IRUSR | S_IWUSR);
	if (dst_fd < 0) {
		err(1, "open(\"%s\", O_TMPFILE...)", dst_dir);
	}
	free(dst2);
	dst_dir = dst2 = NULL;
	char dst_fd_path[40];
	int n = sprintf(dst_fd_path, "/proc/self/fd/%d", dst_fd);
	if (n < 0) {
		err(1, "sprintf()");
	}
	uint64_t start, end = 0, target_offset = 0;
	start = read64(argv[3]);
	if (argc >= 5) {
		end = read64(argv[4]);
		if (argc >= 6) {
			target_offset = read64(argv[5]);
		}
	}
	struct file_clone_range range = {
		src_fd,
		start,
		end,
		target_offset,
	};
	int rc = ioctl(dst_fd, FICLONERANGE, &range);
	if (rc < 0) {
		err(1, "ioctl(..., FICLONERANGE, ...)");
	}
	rc = linkat(AT_FDCWD, dst_fd_path, AT_FDCWD, dst, AT_SYMLINK_FOLLOW);
	if (rc < 0) {
		err(1, "linkat(..., \"%s\", ...)", dst);
	}
	return 0;
}
