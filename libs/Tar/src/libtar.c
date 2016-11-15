/*
**  Copyright 1998-2003 University of Illinois Board of Trustees
**  Copyright 1998-2003 Mark D. Roth
**  All rights reserved.
**
**  libtar.c - demo driver program for libtar
**
**  Mark D. Roth <roth@uiuc.edu>
**  Campus Information Technologies and Educational Services
**  University of Illinois at Urbana-Champaign
*/

//#include <config.h>
#include <libtar.h>

#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
//#include <sys/param.h>
#include <stdlib.h>

#ifdef DEBUG
# include <signal.h>
#endif

//#include <compat.h>


int verbose = 0;
int use_gnu = 0;


/*int
create(char *tarfile, char *rootdir, libtar_list_t *l)
{
	TAR *t;
	char *pathname;
	char buf[MAXPATHLEN];
	libtar_listptr_t lp;

	if (tar_open(&t, tarfile,
		     NULL,
		     O_WRONLY | O_CREAT, 0644,
		     (verbose ? TAR_VERBOSE : 0)
		     | (use_gnu ? TAR_GNU : 0)) == -1)
	{
		fprintf(stderr, "tar_open(): %s\n", strerror(errno));
		return -1;
	}

	libtar_listptr_reset(&lp);
	while (libtar_list_next(l, &lp) != 0)
	{
		pathname = (char *)libtar_listptr_data(&lp);
		if (pathname[0] != '/' && rootdir != NULL)
			snprintf(buf, sizeof(buf), "%s/%s", rootdir, pathname);
		else
			strlcpy(buf, pathname, sizeof(buf));
		if (tar_append_tree(t, buf, pathname) != 0)
		{
			fprintf(stderr,
				"tar_append_tree(\"%s\", \"%s\"): %s\n", buf,
				pathname, strerror(errno));
			tar_close(t);
			return -1;
		}
	}

	if (tar_append_eof(t) != 0)
	{
		fprintf(stderr, "tar_append_eof(): %s\n", strerror(errno));
		tar_close(t);
		return -1;
	}

	if (tar_close(t) != 0)
	{
		fprintf(stderr, "tar_close(): %s\n", strerror(errno));
		return -1;
	}

	return 0;
}*/


int
libtar_list(const char *tarfile)
{
	TAR *t;
	int i;

	if (tar_open(&t, tarfile,
		     NULL,
             O_RDONLY, 0,
		     (verbose ? TAR_VERBOSE : 0)
		     | (use_gnu ? TAR_GNU : 0)) == -1)
	{
#ifdef DEBUG
        fprintf(stderr, "tar_open(): %s\n", strerror(errno));
#endif
		return -1;
	}

	while ((i = th_read(t)) == 0)
	{
        //th_print_long_ls(t);
#ifdef DEBUG
        th_print(t);
#endif
		if (TH_ISREG(t) && tar_skip_regfile(t) != 0)
		{
#ifdef DEBUG
            fprintf(stderr, "tar_skip_regfile(): %s\n",
                strerror(errno));
#endif
			return -1;
		}
	}

#ifdef DEBUG
	printf("th_read() returned %d\n", i);
	printf("EOF mark encountered after %ld bytes\n",
	       lseek(t->fd, 0, SEEK_CUR)
	       );
#endif

	if (tar_close(t) != 0)
	{
#ifdef DEBUG
        fprintf(stderr, "tar_close(): %s\n", strerror(errno));
#endif
		return -1;
	}

	return 0;
}


int
libtar_extract(const char *tarfile, const char *rootdir)
{
	TAR *t;

#ifdef DEBUG
	puts("opening tarfile...");
#endif
	if (tar_open(&t, tarfile,
		     NULL,
		     O_RDONLY, 0,
		     (verbose ? TAR_VERBOSE : 0)
		     | (use_gnu ? TAR_GNU : 0)) == -1)
	{
#ifdef DEBUG
		fprintf(stderr, "tar_open(): %s\n", strerror(errno));
#endif
		return -1;
	}

#ifdef DEBUG
	puts("extracting tarfile...");
#endif
	if (tar_extract_all(t, rootdir) != 0)
	{
#ifdef DEBUG
		fprintf(stderr, "tar_extract_all(): %s\n", strerror(errno));
#endif
		tar_close(t);
		return -1;
	}

#ifdef DEBUG
	puts("closing tarfile...");
#endif
	if (tar_close(t) != 0)
	{
#ifdef DEBUG
		fprintf(stderr, "tar_close(): %s\n", strerror(errno));
#endif
		return -1;
	}

	return 0;
}


