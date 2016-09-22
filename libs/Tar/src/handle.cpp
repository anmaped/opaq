/*
**  Copyright 1998-2003 University of Illinois Board of Trustees
**  Copyright 1998-2003 Mark D. Roth
**  All rights reserved.
**
**  handle.c - libtar code for initializing a TAR handle
**
**  Mark D. Roth <roth@uiuc.edu>
**  Campus Information Technologies and Educational Services
**  University of Illinois at Urbana-Champaign
*/

#include <internal.h>

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>

#include <Arduino.h>
#include <FS.h>

#define PACKAGE_VERSION "Arduinolibtar0"

const char libtar_version[] = PACKAGE_VERSION;

extern "C"
{
    int __open(const char * filename, int oflags, ...)
    {
        File *x = new File();

        Serial.println((int)x);

        *x = SPIFFS.open(filename, (oflags & O_CREAT)? "w" : "r");

        if (!(*x))
        {
            delete x;
            return -1;
        }

        return (int)x;
    }

    int __close(int tag)
    {
        File *x = (File *)tag;

        Serial.println(tag);

        x->close();
        delete x;

        return 0;
    }

    ssize_t __read(int tag, void * array, size_t size)
    {
        File *x = (File *)tag;
        return x->read((uint8_t*)array, size);
    }

    ssize_t __write(int tag, const void * array, size_t size)
    {
        File *x = (File *)tag;
        return x->write((uint8_t*)array, size);
    }

}

static tartype_t default_type = { __open, __close, __read, __write };

static int
tar_init(TAR **t, const char *pathname, tartype_t *type,
	 int oflags, int mode, int options)
{
	if ((oflags & O_ACCMODE) == O_RDWR)
	{
		errno = EINVAL;
		return -1;
	}

	*t = (TAR *)calloc(1, sizeof(TAR));
	if (*t == NULL)
		return -1;

    (*t)->pathname = (char *)pathname;
	(*t)->options = options;
    (*t)->type = (type ? type : &default_type);
	(*t)->oflags = oflags;

    /*
	if ((oflags & O_ACCMODE) == O_RDONLY)
		(*t)->h = libtar_hash_new(256,
					  (libtar_hashfunc_t)path_hashfunc);
	else
		(*t)->h = libtar_hash_new(16, (libtar_hashfunc_t)dev_hash);

	if ((*t)->h == NULL)
	{
		free(*t);
		return -1;
	}
    */

	return 0;
}


/* open a new tarfile handle */
int
tar_open(TAR **t, const char *pathname, tartype_t *type,
	 int oflags, int mode, int options)
{
	if (tar_init(t, pathname, type, oflags, mode, options) == -1)
		return -1;

	if ((options & TAR_NOOVERWRITE) && (oflags & O_CREAT))
		oflags |= O_EXCL;

#ifdef O_BINARY
	oflags |= O_BINARY;
#endif

	(*t)->fd = (*((*t)->type->openfunc))(pathname, oflags, mode);
	if ((*t)->fd == -1)
	{
        //libtar_hash_free((*t)->h, NULL);
		free(*t);
		return -1;
	}

	return 0;
}


int
tar_fdopen(TAR **t, int fd, const char *pathname, tartype_t *type,
	   int oflags, int mode, int options)
{
	if (tar_init(t, pathname, type, oflags, mode, options) == -1)
		return -1;

	(*t)->fd = fd;
	return 0;
}


int
tar_fd(TAR *t)
{
	return t->fd;
}


/* close tarfile handle */
int
tar_close(TAR *t)
{
	int i;

	i = (*(t->type->closefunc))(t->fd);

    /*if (t->h != NULL)
		libtar_hash_free(t->h, ((t->oflags & O_ACCMODE) == O_RDONLY
					? free
					: (libtar_freefunc_t)tar_dev_free));
                    */
	if (t->th_pathname != NULL)
		free(t->th_pathname);
	free(t);

	return i;
}


