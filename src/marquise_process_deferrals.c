#include <stdio.h>
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>
#include <string.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <unistd.h>

#include "macros.h"
#include "marquise.h"
#include "defer.h"

#define bail_if( assertion, action) do {                                 \
fail_if( assertion                                                       \
	, { action }; return;                                       \
	, "marquise_retrieve_from_file failed: %s", strerror( errno ) ); \
} while( 0 )

void process_defer_file(char *path) 
{
	int fd = open(path, O_RDONLY);
	bail_if(fd == -1,);
	if (flock(fd, LOCK_EX | LOCK_NB) == -1) {
		if (errno == EWOULDBLOCK) {
			fprintf(stderr, "%s is locked, skipping.\n", path);
			goto defer_file_cleanup;
		}
		fprintf(stderr, "Could not lock %s: %s\n", path, strerror(errno));
		goto defer_file_cleanup;
	}
	struct stat buf;
	if (fstat(fd, &buf) != 0) {
		fprintf(stderr, "Could not stat %s: %s\n", path, strerror(errno));
		goto defer_file_cleanup;
	}
	if (buf.st_size == 0) {
		printf("Deleting zero-size %s\n", path);
		unlink(path);
	}
	// Of nonzero size and not locked, so we retry it
	printf("Retrying %s...\n", path);
	FILE *fi = fdopen(fd, "r");
		
defer_file_cleanup:
	flock(fd, LOCK_UN);
	close(fd);
}

int main(int argc, char **argv) 
{
	if (argc < 2) {
		fprintf(stderr, "Usage: %s <defer_dir>\n", argv[0]);
		exit(1);
	}
	int template_len = strlen(deferral_file_template);
	// We want to cut off the random suffix at the end. 
	size_t prefix_len = template_len - 6;
	char *defer_prefix = malloc(template_len);
	// Same with the / at the start.
	strncpy(defer_prefix, &deferral_file_template[1], prefix_len);
	defer_prefix[prefix_len-1] = '\0';
	size_t defer_dir_len = strlen(argv[1]);
	DIR *defer_dir = opendir(argv[1]);
	if (defer_dir == NULL) {
		fprintf(stderr, "Could not open deferral directory %s: %s\n", argv[1], strerror(errno));
		exit(2);
	}
	struct dirent *entry = NULL;
	for (;;) {
		errno = 0;
		entry = readdir(defer_dir);
		if (errno) {
			fprintf(stderr, "Error reading directory entry: %s\n", strerror(errno));
			break;
		}
		if (entry == NULL) {
			break; // eof
		}
		size_t name_len = strlen(entry->d_name);
		if (strncmp(entry->d_name, defer_prefix, prefix_len-1) != 0) {
			continue;
		}
		char *path = malloc(name_len * defer_dir_len + 2);
		strcpy(path, argv[1]);
		path[defer_dir_len] = '/';
		path[defer_dir_len + 1] = '\0';
		strncat(path, entry->d_name, name_len);
		process_defer_file(path);
		free(path);
	}
		
	free(defer_prefix);
	return 0;
}
