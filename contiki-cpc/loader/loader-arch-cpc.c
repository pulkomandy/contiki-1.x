#include "loader.h"
#include "rel.h"
#include "log.h"
#include <stddef.h>
#include <malloc.h>

extern void *progend;

struct prg_hdr {
	char *relocatedata;
	char arch[8];
	char version[8];
	char initfunc[1];
};

unsigned char loader_arch_load(const char *name, char *arg)
{
	char *loadaddr;	
	struct prg_hdr *prghdr;
	int length;

	/* get length of file */
	length = get_file_length(name);
	if (length==0)
		return LOADER_ERR_OPEN;

	/* allocate memory */
	loadaddr = malloc(length);
	if (loadaddr==NULL)
		return LOADER_ERR_MEM;
	
	/* load the file */
	load_file(name,loadaddr);

	prghdr = (struct prg_hdr *)loadaddr;

	/* relocate it */
	relocate(prghdr->relocatedata,loadaddr);

	((void (*)(char *))prghdr->initfunc)(arg);

	return LOADER_OK;
}


static void relocate_dsc(struct dsc* data, int loadaddr)
{
	data->description += loadaddr;
	data->prgname += loadaddr;
	data->icon = (struct ctk_icon*)(((char*)data->icon)+ loadaddr);

	data->icon->title += loadaddr;
#if CTK_CONF_ICON_BITMAPS
	data->icon->bitmap += loadaddr;
#endif
#if CTK_CONF_ICON_TEXTMAPS
	data->icon->textmap += loadaddr;
#endif
}


struct dsc *loader_arch_load_dsc(const char *name)
{
	char *loadaddr;
	struct dsc *dschdr;
	int length;

	/* get length of file */
	length = get_file_length(name);
	if (length==0)
		return NULL;

	/* allocate memory */
	loadaddr = malloc(length);
	if (loadaddr==NULL)
		return NULL;

	/* load the file */
	load_file(name, loadaddr);
	
	dschdr = (struct dsc *)loadaddr;
	/* relocate it */
	relocate_dsc(dschdr, (int)loadaddr);

	return dschdr;
}

void loader_arch_free(void *loadaddr)
{
	/* free module */
	/* we're given the start of 'arch' member of the prg_hdr,
	calculate the real start address and then free the block */
	void *header = (void *)((char *)loadaddr - offsetof(struct prg_hdr,arch));
	free(header);
}

void loader_arch_free_dsc(struct dsc *dscdata)
{
	free(dscdata);
}

