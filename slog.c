#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>

#include "slog.h"

#if defined(_WIN32) || defined(_WIN64)
#define vsnprintf _vsnprintf
#define vsprintf _vsprintf
#endif

#define MAX_PRINT_LEN	2048

//#define SLOG_TO_FILE
#define SLOG_FILE	"/tmp/slog.txt"
//static FILE *g_log_file=NULL;

#define DEFAULT_MODULE_NAME	"SLOG"

enSLOG_LEVEL debuglevel_slog = SL_WARNING;


static int neednl;
static FILE *fmsg = NULL;
static FILE *ffmsg = NULL;

static const char *levels[] = {
    "CRIT", "ERROR", "WARNING", "INFO",
    "DEBUG", "DEBUG2"
};

static const int log_color[SL_ALL]= {34,31,33,35,32,36,};

static fSLOG_HOOK vlog_default, *cb = vlog_default;

static void vlog_default(const char *module, int level, const char *format, va_list vl) {
	char str[MAX_PRINT_LEN]="";
	
	vsnprintf(str, MAX_PRINT_LEN-1, format, vl);

	if (module == NULL) {
		module = DEFAULT_MODULE_NAME;
	}

	/*
	* Set debug Level for Every Module
	* Fix Me
	*/

	/* Filter out 'no-name' */
	if ( debuglevel_slog<SL_ALL && strstr(str, "no-name" ) != NULL )
		return;


#ifdef SLOG_TO_FILE
	if(g_log_file == NULL){
		g_log_file=fopen(SLOG_FILE,"wb+");
		if(g_log_file==NULL){
			printf("open file failed\n");
			return NULL;
		}
		printf("create file:%s success",SLOG_FILE);
	}
	if ( !fmsg ) fmsg = g_log_file;
#else
	if ( !fmsg ) fmsg = stderr;
#endif

	if ( level <= debuglevel_slog) {
		if (neednl) {
			putc('\n', fmsg);
			neednl = 0;
		}
		fprintf(fmsg, "\033[1;%dm%s::%s:\033[0m %s", log_color[level], module,levels[level], str);
#ifdef _DEBUG
		fflush(fmsg);
#endif
	}
}


static void vlog_file_default(const char *module, int level, const char *format, va_list vl) {
	char str[MAX_PRINT_LEN]="";
	
	vsnprintf(str, MAX_PRINT_LEN-1, format, vl);

	if (module == NULL) {
		module = DEFAULT_MODULE_NAME;
	}

	/* Filter out 'no-name' */
	if ( debuglevel_slog<SL_ALL && strstr(str, "no-name" ) != NULL )
		return;

	if(ffmsg == NULL){
		ffmsg = stderr;
	}

	if ( level <= debuglevel_slog ) {
		if (neednl) {
			putc('\n', ffmsg);
			neednl = 0;
		}
		fprintf(ffmsg, "\033[1;%dm%s::%s:\033[0m %s", log_color[level], module,levels[level], str);
		fflush(ffmsg);
	}
}

void slog_set_output(FILE *file) 
{
	fmsg = file;
}

void slog_set_level(enSLOG_LEVEL level) 
{
	debuglevel_slog = level;
}

enSLOG_LEVEL slog_GetLevel() 
{
	return debuglevel_slog;
}

void slog_set_callback(fSLOG_HOOK *cbp) 
{
	cb = cbp;
}


void slog(const char *module, int level, const char *format, ...) 
{
	va_list args;
	va_start(args, format);
	cb(module, level, format, args);
	va_end(args);
}

void flog(const char *module, int level, const char *format, ...) 
{
	va_list args;
	va_start(args, format);
	vlog_file_default(module, level, format, args);
	va_end(args);
}


void slog_assert(int expression, const char *module, int level, const char *format, ...)
{
	if (!(expression)) {
		va_list args;
		va_start(args, format);
		cb(module, level, format, args);
		va_end(args);
		assert(expression);
	}
}

static const char hexdig[] = "0123456789abcdef";

void slog_hex(int level, const unsigned char *data, unsigned long len) 
{
	unsigned long i;
	char line[50], *ptr;

	if ( level > debuglevel_slog )
		return;

	ptr = line;

	for (i=0; i<len; i++) {
		*ptr++ = hexdig[0x0f & (data[i] >> 4)];
		*ptr++ = hexdig[0x0f & data[i]];
		if ((i & 0x0f) == 0x0f) {
		*ptr = '\0';
		ptr = line;
		slog(NULL, level, "%s", line);
		} else {
		*ptr++ = ' ';
		}
	}
	if (i & 0x0f) {
		*ptr = '\0';
		slog(NULL, level, "%s", line);
	}
}

void slog_hexstring(int level, const unsigned char *data, unsigned long len) 
{
#define BP_OFFSET 9
#define BP_GRAPH 60
#define BP_LEN	80
	char	line[BP_LEN];
	unsigned long i;

	if ( !data || level > debuglevel_slog )
		return;

	/* in case len is zero */
	line[0] = '\0';

	for ( i = 0 ; i < len ; i++ ) {
		int n = i % 16;
		unsigned off;

		if ( !n ) {
			if ( i ) slog(NULL, level, "%s", line );
			memset( line, ' ', sizeof(line)-2 );
			line[sizeof(line)-2] = '\0';

			off = i % 0x0ffffU;

			line[2] = hexdig[0x0f & (off >> 12)];
			line[3] = hexdig[0x0f & (off >>  8)];
			line[4] = hexdig[0x0f & (off >>  4)];
			line[5] = hexdig[0x0f & off];
			line[6] = ':';
		}

		off = BP_OFFSET + n*3 + ((n >= 8)?1:0);
		line[off] = hexdig[0x0f & ( data[i] >> 4 )];
		line[off+1] = hexdig[0x0f & data[i]];

		off = BP_GRAPH + n + ((n >= 8)?1:0);

		if ( isprint( data[i] )) {
			line[BP_GRAPH + n] = data[i];
		} else {
			line[BP_GRAPH + n] = '.';
		}
	}

	slog(NULL, level, "%s", line );
}

/* These should only be used by apps, never by the library itself */
void slog_printf(const char *format, ...) 
{
	char str[MAX_PRINT_LEN]="";
	int len;
	va_list args;
	va_start(args, format);
	len = vsnprintf(str, MAX_PRINT_LEN-1, format, args);
	va_end(args);

	if ( debuglevel_slog==SL_CRIT)
		return;

	if ( !fmsg ) fmsg = stderr;

	if (neednl) {
		putc('\n', fmsg);
		neednl = 0;
	}

	if (len > MAX_PRINT_LEN-1)
		len = MAX_PRINT_LEN-1;
	fprintf(fmsg, "%s", str);
	if (str[len-1] == '\n')
		fflush(fmsg);
}

void slog_status(const char *format, ...) 
{
	char str[MAX_PRINT_LEN]="";
	va_list args;
	va_start(args, format);
	vsnprintf(str, MAX_PRINT_LEN-1, format, args);
	va_end(args);

	if ( debuglevel_slog==SL_CRIT)
		return;

	if ( !fmsg ) fmsg = stderr;

	fprintf(fmsg, "%s", str);
	fflush(fmsg);
	neednl = 1;
}

void flog_set_output(FILE *file)
{
	if (ffmsg == NULL) {
		ffmsg = file;
	} else {
		if (ffmsg != stderr) {
			printf("FLOG is busy!!!!\n");
		} else {
			ffmsg = file;
		}
	}
}

void flog_close(void)
{
	if (ffmsg && (ffmsg != stderr)) {
		fclose(ffmsg);
		ffmsg = NULL;
		printf("FLOG close now!!!!\n");
	}
}

