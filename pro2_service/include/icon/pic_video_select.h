#ifndef _PIC_VIDEO_SELECT_H_
#define _PIC_VIDEO_SELECT_H_


#define PIC_VIDEO_LIVE_ITEM_MAX 10

typedef struct stPicVideoCfg {
	const char* 	pItemName;								/* 设置项的名称 */
	int				iItemMaxVal;							/* 设置项可取的最大值 */
	int  			iCurVal;								/* 当前的值,(根据当前值来选择对应的图标) */
	ICON_POS		stPos;
    ACTION_INFO     stAction;
	const u8 * 		stLightIcon[PIC_VIDEO_LIVE_ITEM_MAX];		/* 选中时的图标列表 */
	const u8 * 		stNorIcon[PIC_VIDEO_LIVE_ITEM_MAX];		/* 未选中时的图标列表 */
} PicVideoCfg;


#endif /* _PIC_VIDEO_SELECT_H_ */