#include <glib.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include "../structs.h"
#include "../defer.h"
#include "../macros.h"

typedef struct {
	deferral_file *df;
} fixture;

void setup(fixture * f, gconstpointer td)
{
	f->df = marquise_deferral_file_new();
}

void teardown(fixture * f, gconstpointer td)
{
	marquise_deferral_file_close(f->df);
	marquise_deferral_file_free(f->df);
}

void setup_envvar(fixture *f, gconstpointer td)
{
	const char *tmpl = "/tmp/marquise_defer_test";
	mkdir(tmpl, 0777);
	setenv("LIBMARQUISE_DEFERRAL_DIR", tmpl, 1);
	f->df = marquise_deferral_file_new();
}

void teardown_envvar(fixture *f, gconstpointer td)
{
	marquise_deferral_file_close(f->df);
	marquise_deferral_file_free(f->df);
	rmdir("/tmp/marquise_defer_test");
}

void defer_then_read(fixture * f, gconstpointer td)
{
	// two test bursts, we expect it to behave as a LIFO stack
	data_burst *first = malloc(sizeof(data_burst));
	first->data = malloc(6);
	memcpy(first->data, "first", 6);
	first->length = 6;

	data_burst *last = malloc(sizeof(data_burst));
	last->data = malloc(5);
	memcpy(first->data, "last", 5);
	last->length = 5;

	marquise_defer_to_file(f->df, first);
	marquise_defer_to_file(f->df, last);

	data_burst *first_retrieved = marquise_retrieve_from_file(f->df);
	g_assert_cmpuint(last->length, ==, first_retrieved->length);
	g_assert_cmpstr((char *)last->data, ==, (char *)first_retrieved->data);

	data_burst *last_retrieved = marquise_retrieve_from_file(f->df);
	g_assert_cmpuint(first->length, ==, last_retrieved->length);
	g_assert_cmpstr((char *)first->data, ==, (char *)last_retrieved->data);

	data_burst *nonexistent = marquise_retrieve_from_file(f->df);
	g_assert(!nonexistent);

	free_databurst(first);
	free_databurst(last);
	free_databurst(first_retrieved);
	free_databurst(last_retrieved);
}

void defer_unlink_then_read(fixture * f, gconstpointer td)
{
	data_burst *first = malloc(sizeof(data_burst));
	first->data = malloc(6);
	memcpy(first->data, "first", 6);
	first->length = 6;

	marquise_defer_to_file(f->df, first);
	unlink(f->df->path);
	data_burst *nonexistent = marquise_retrieve_from_file(f->df);
	g_assert(!nonexistent);

	free_databurst(first);
}

void unlink_defer_then_read(fixture * f, gconstpointer td)
{
	data_burst *first = malloc(sizeof(data_burst));
	first->data = malloc(6);
	memcpy(first->data, "first", 6);
	first->length = 6;

	unlink(f->df->path);
	marquise_defer_to_file(f->df, first);
	data_burst *first_retrieved = marquise_retrieve_from_file(f->df);
	g_assert(first_retrieved);
	g_assert_cmpuint(first->length, ==, first_retrieved->length);
	g_assert_cmpstr((char *)first->data, ==, (char *)first_retrieved->data);

	free_databurst(first);
	free_databurst(first_retrieved);
}

void set_defer_dir(fixture *f, gconstpointer td)
{
	const char *tmpl = "/tmp/marquise_defer_test";
	int d = strncmp(f->df->path, tmpl, strlen(tmpl));
	g_assert_cmpint(d, ==, 0);
}

int main(int argc, char **argv)
{
	g_test_init(&argc, &argv, NULL);
	g_test_add("/defer_file/defer_then", fixture, NULL, setup,
		   defer_then_read, teardown);
	g_test_add("/defer_file/defer_unlink_then_read", fixture, NULL, setup,
		   defer_unlink_then_read, teardown);
	g_test_add("/defer_file/unlink_defer_then_read", fixture, NULL, setup,
		   unlink_defer_then_read, teardown);
	g_test_add("/defer_file/set_defer_dir", fixture, NULL, setup_envvar,
		   set_defer_dir, teardown_envvar);
	return g_test_run();
}
