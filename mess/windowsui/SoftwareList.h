#ifndef SOFTWARELIST_H
#define SOFTWARELIST_H

#include <tchar.h>
#include "SmartListView.h"

#ifdef UNDER_CE
#define HAS_IDLING	0
#define HAS_CRC		0
#else
#define HAS_IDLING	1
#define HAS_CRC		1
#endif

typedef struct {
    int type;
    const char *ext;
#if HAS_CRC
	UINT32 (*partialcrc)(const unsigned char *buf, unsigned int size);
#endif
} mess_image_type;

/* SoftwareListView Class calls */
LPCTSTR SoftwareList_GetText(struct SmartListView *pListView, int nRow, int nColumn);
BOOL SoftwareList_ItemChanged(struct SmartListView *pListView, BOOL bWasSelected, BOOL bNowSelected, int nRow);
BOOL SoftwareList_IsItemSelected(struct SmartListView *pListView, int nItem);

#if HAS_IDLING
BOOL SoftwareList_CanIdle(struct SmartListView *pListView);
void SoftwareList_Idle(struct SmartListView *pListView);
#endif

/* External calls */
void SetupImageTypes(int nDriver, mess_image_type *types, int count, BOOL bZip, int type);
void FillSoftwareList(struct SmartListView *pSoftwareListView, int nGame, int nBasePaths, LPCSTR *plpBasePaths, LPCSTR lpExtraPath);
int MessLookupByFilename(const TCHAR *filename);
int MessImageCount(void);
void MessIntroduceItem(struct SmartListView *pListView, const char *filename, mess_image_type *imagetypes);
int GetImageType(int nItem);
LPCTSTR GetImageName(int nItem);
LPCTSTR GetImageFullName(int nItem);

#ifdef MAME_DEBUG
void MessTestsFlex(struct SmartListView *pListView);
#endif

#endif /* SOFTWARELIST_H */
