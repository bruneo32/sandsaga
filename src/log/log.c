#include "log.h"

#include <stdarg.h>
#include <stdlib.h>

static const char *m_fout_filename = "sandsaga.out.log";
static const char *m_ferr_filename = "sandsaga.err.log";

static FILE *m_fout = NULL;
static FILE *m_ferr = NULL;

static void F_logout_deinit() {
	if (m_fout)
		fclose(m_fout);
}

static void F_logerr_deinit() {
	if (m_ferr)
		fclose(m_ferr);
}

void loginfo(const char *fmt, ...) {
	if (!m_fout) {
		m_fout = fopen(m_fout_filename, "w");
		atexit(F_logout_deinit);
	}

	va_list args;
	va_start(args, fmt);
	vfprintf(m_fout, fmt, args);
	fprintf(stdout, fmt, args); /* Show in terminal too */
	va_end(args);
}

void logerr(const char *fmt, ...) {
	if (!m_ferr) {
		m_ferr = fopen(m_ferr_filename, "w");
		atexit(F_logerr_deinit);
	}

	va_list args;
	va_start(args, fmt);
	vfprintf(m_ferr, fmt, args);
	fprintf(stderr, fmt, args); /* Show in terminal too */
	va_end(args);
}