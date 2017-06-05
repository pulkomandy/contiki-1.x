/*
 * Copyright 2017, Adrien Destugues, pulkomandy@pulkomandy.tk
 * Distributed under terms of the MIT license.
 */


#include "cfs-fatfs.h"

#include "cfs.h"
#include "cfs-service.h"

#include "fatfs/ff.h"

#include <stdlib.h>

static int  s_open(const char *n, int f);
static void s_close(int f);
static int  s_read(int f, char *b, unsigned int l);
static int  s_write(int f, char *b, unsigned int l);
static int  s_opendir(struct cfs_dir *p, const char *n);
static int  s_readdir(struct cfs_dir *p, struct cfs_dirent *e);
static int  s_closedir(struct cfs_dir *p);

static const struct cfs_service_interface interface =
  {
    CFS_SERVICE_VERSION,
    s_open,
    s_close,
    s_read,
    s_write,
    f_opendir,
    s_readdir,
    f_closedir
  };

EK_EVENTHANDLER(eventhandler, ev, data);
EK_PROCESS(proc, CFS_SERVICE_NAME, EK_PRIO_NORMAL,
           eventhandler, NULL, (void *)&interface);
/*---------------------------------------------------------------------------*/
EK_PROCESS_INIT(cfs_fatfs_init, arg)
{
  arg_free(arg);
  ek_service_start(CFS_SERVICE_NAME, &proc);
}
/*---------------------------------------------------------------------------*/
EK_EVENTHANDLER(eventhandler, ev, data)
{
  switch(ev) {
  case EK_EVENT_INIT:
    break;
  case EK_EVENT_REQUEST_REPLACE:
    ek_replace((struct ek_proc *)data, &interface);
    break;
  case EK_EVENT_REQUEST_EXIT:
    ek_exit();
    break;
  }
}
/*---------------------------------------------------------------------------*/

static int s_open(const char* n, int f)
{
	int mode;
	if (f == CFS_READ)
		mode = FA_READ | FA_OPEN_EXISTING;
	else
		mode = FA_WRITE | FA_CREATE_ALWAYS;
	FIL* ptr = malloc(sizeof(FIL));
	FRESULT r = f_open(ptr, n, mode);

	if (r != FR_OK) {
		free(ptr);
		return -1;
	}
	return ptr;
}

static void
s_close(int f)
{
	f_close(f);
	free(f);
}

static int s_read(int f, char* b, unsigned int l)
{
	if (f == -1)
		return -1;

	int r = 0;
	f_read(f, b, l, &r);
	return r;
}

static int s_write(int f, char* b, unsigned int l)
{
	int r = 0;
	f_write(f, b, l, &r);
	return r;
}


static int s_readdir(struct cfs_dir *p, struct cfs_dirent *e)
{
	FILINFO info;
	f_readdir(p, &info);

	strncpy(e->name, info.fname, 13);
	e->size = info.fsize;

	return 0;
}
