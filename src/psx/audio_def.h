#pragma once

#define XA_LENGTH(x) (((u64)(x) * 75) / 100 * IO_SECT_SIZE) //Centiseconds to sectors in bytes (w)

typedef struct
{
	XA_File file;
	u32 length;
} XA_TrackDef;

static const XA_TrackDef xa_tracks[] = {
	{XA_Menu, XA_LENGTH(14700)},
	{XA_Menu, XA_LENGTH(9200)},
	{XA_Menu, XA_LENGTH(3840)},

	{XA_CupheadA, XA_LENGTH(12800)},
	{XA_CupheadA, XA_LENGTH(15400)},
	{XA_CupheadB, XA_LENGTH(17900)},
};

static const char *xa_paths[] = {
	"\\SONGS\\MENU.MUS;1",
	"\\SONGS\\CUPA.MUS;1",
	"\\SONGS\\CUPB.MUS;1",
	NULL,
};
