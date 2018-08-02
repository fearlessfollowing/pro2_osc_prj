#ifndef _PIC_VIDEO_SELECT_H_
#define _PIC_VIDEO_SELECT_H_


#define PIC_VIDEO_LIVE_ITEM_MAX 10


/*
 * 带RAW项
 * - 8K|3D|OF|RAW
 * - 8K|3D|RAW
 * - 8K|RAW
 * - AEB3,5,7,9|RAW
 * - BURST|RAW
 * - CUSTOMER
 */

const u8 pic8K3DOFRAWLight_78X16[] = {
	/* 图像	   78x16	*/
	0x00,0x00,0xFC,0x9C,0x6C,0x6C,0x6C,0x9C,0xFC,0x0C,0x7C,0xBC,0xDC,0xEC,0xFC,0x04,
	0xFC,0xEC,0x6C,0x6C,0x6C,0x9C,0xFC,0x0C,0xEC,0xEC,0xEC,0xEC,0xDC,0x3C,0xFC,0x04,
	0xFC,0x3C,0xDC,0xEC,0xEC,0xEC,0xDC,0x3C,0xFC,0x0C,0x6C,0x6C,0x6C,0xEC,0xFC,0x04,
	0xFC,0x0C,0xEC,0xEC,0xEC,0x1C,0xFC,0xFC,0xFC,0xFC,0x3C,0xCC,0x3C,0xFC,0xFC,0xFC,
	0x8C,0x7C,0xFC,0x7C,0x8C,0x7C,0xFC,0x7C,0x8C,0xFC,0xFC,0xFC,0xFC,0xFC,0x00,0x00,
	0x3F,0x38,0x37,0x37,0x37,0x38,0x3F,0x30,0x3E,0x3D,0x3B,0x37,0x3F,0x20,0x3F,0x37,
	0x37,0x37,0x37,0x38,0x3F,0x30,0x37,0x37,0x37,0x37,0x3B,0x3C,0x3F,0x20,0x3F,0x3C,
	0x3B,0x37,0x37,0x37,0x3B,0x3C,0x3F,0x30,0x3F,0x3F,0x3F,0x3F,0x3F,0x20,0x3F,0x30,
	0x3E,0x3E,0x3C,0x3B,0x37,0x3F,0x33,0x3C,0x3D,0x3D,0x3D,0x3C,0x33,0x3F,0x3F,0x3C,
	0x33,0x3C,0x3F,0x3C,0x33,0x3C,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,
};

const u8 pic8K3DOF_RAW_Nor_78x16[] = {
	0x00,0x00,0x00,0x60,0x90,0x90,0x90,0x60,0x00,0xF0,0x80,0x40,0x20,0x10,0x00,0xF8,
	0x00,0x10,0x90,0x90,0x90,0x60,0x00,0xF0,0x10,0x10,0x10,0x10,0x20,0xC0,0x00,0xF8,
	0x00,0xC0,0x20,0x10,0x10,0x10,0x20,0xC0,0x00,0xF0,0x90,0x90,0x90,0x10,0x00,0xF8,
	0x00,0xF0,0x10,0x10,0x10,0xE0,0x00,0x00,0x00,0x00,0xC0,0x30,0xC0,0x00,0x00,0x00,
	0x70,0x80,0x00,0x80,0x70,0x80,0x00,0x80,0x70,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x07,0x08,0x08,0x08,0x07,0x00,0x0F,0x01,0x02,0x04,0x08,0x00,0x1F,0x00,0x08,
	0x08,0x08,0x08,0x07,0x00,0x0F,0x08,0x08,0x08,0x08,0x04,0x03,0x00,0x1F,0x00,0x03,
	0x04,0x08,0x08,0x08,0x04,0x03,0x00,0x0F,0x00,0x00,0x00,0x00,0x00,0x1F,0x00,0x0F,
	0x01,0x01,0x03,0x04,0x08,0x00,0x0C,0x03,0x02,0x02,0x02,0x03,0x0C,0x00,0x00,0x03,
	0x0C,0x03,0x00,0x03,0x0C,0x03,0x00,0x00,0x00,0x00,0x00,0x00,
};

const u8 pic8K3DRAWLight_78X16[] = {
	/* 图像	   78x16	*/
	0x00,0x00,0xFC,0x9C,0x6C,0x6C,0x6C,0x9C,0xFC,0x0C,0x7C,0xBC,0xDC,0xEC,0xFC,0x04,
	0xFC,0x3C,0xDC,0xEC,0xEC,0xEC,0xDC,0x3C,0xFC,0x0C,0x6C,0x6C,0x6C,0xEC,0xFC,0x04,
	0xFC,0x0C,0xEC,0xEC,0xEC,0x1C,0xFC,0xFC,0xFC,0xFC,0x3C,0xCC,0x3C,0xFC,0xFC,0xFC,
	0x8C,0x7C,0xFC,0x7C,0x8C,0x7C,0xFC,0x7C,0x8C,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,
	0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0x00,0x00,
	0x3F,0x38,0x37,0x37,0x37,0x38,0x3F,0x30,0x3E,0x3D,0x3B,0x37,0x3F,0x20,0x3F,0x3C,
	0x3B,0x37,0x37,0x37,0x3B,0x3C,0x3F,0x30,0x3F,0x3F,0x3F,0x3F,0x3F,0x20,0x3F,0x30,
	0x3E,0x3E,0x3C,0x3B,0x37,0x3F,0x33,0x3C,0x3D,0x3D,0x3D,0x3C,0x33,0x3F,0x3F,0x3C,
	0x33,0x3C,0x3F,0x3C,0x33,0x3C,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,
	0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,
};



const u8 pic8K3DRAWNor_78X16[] = {
	/* 图像	   78x16	*/
	0x00,0x00,0x00,0x60,0x90,0x90,0x90,0x60,0x00,0xF0,0x80,0x40,0x20,0x10,0x00,0xF8,
	0x00,0xC0,0x20,0x10,0x10,0x10,0x20,0xC0,0x00,0xF0,0x90,0x90,0x90,0x10,0x00,0xF8,
	0x00,0xF0,0x10,0x10,0x10,0xE0,0x00,0x00,0x00,0x00,0xC0,0x30,0xC0,0x00,0x00,0x00,
	0x70,0x80,0x00,0x80,0x70,0x80,0x00,0x80,0x70,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x07,0x08,0x08,0x08,0x07,0x00,0x0F,0x01,0x02,0x04,0x08,0x00,0x1F,0x00,0x03,
	0x04,0x08,0x08,0x08,0x04,0x03,0x00,0x0F,0x00,0x00,0x00,0x00,0x00,0x1F,0x00,0x0F,
	0x01,0x01,0x03,0x04,0x08,0x00,0x0C,0x03,0x02,0x02,0x02,0x03,0x0C,0x00,0x00,0x03,
	0x0C,0x03,0x00,0x03,0x0C,0x03,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
};

const u8 pic8KRAWLight_78X16[] = {
	/* 图像	   78x16	*/
	0x00,0x00,0xFC,0x9C,0x6C,0x6C,0x6C,0x9C,0xFC,0x0C,0x7C,0xBC,0xDC,0xEC,0xFC,0x04,
	0xFC,0x0C,0xEC,0xEC,0xEC,0x1C,0xFC,0xFC,0xFC,0xFC,0x3C,0xCC,0x3C,0xFC,0xFC,0xFC,
	0x8C,0x7C,0xFC,0x7C,0x8C,0x7C,0xFC,0x7C,0x8C,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,
	0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,
	0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0x00,0x00,
	0x3F,0x38,0x37,0x37,0x37,0x38,0x3F,0x30,0x3E,0x3D,0x3B,0x37,0x3F,0x20,0x3F,0x30,
	0x3E,0x3E,0x3C,0x3B,0x37,0x3F,0x33,0x3C,0x3D,0x3D,0x3D,0x3C,0x33,0x3F,0x3F,0x3C,
	0x33,0x3C,0x3F,0x3C,0x33,0x3C,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,
	0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,
	0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,
};


const u8 pic8KRAWNor_78X16[] = {
	/* 图像	   78x16	*/
	0x00,0x00,0x00,0x60,0x90,0x90,0x90,0x60,0x00,0xF0,0x80,0x40,0x20,0x10,0x00,0xF8,
	0x00,0xF0,0x10,0x10,0x10,0xE0,0x00,0x00,0x00,0x00,0xC0,0x30,0xC0,0x00,0x00,0x00,
	0x70,0x80,0x00,0x80,0x70,0x80,0x00,0x80,0x70,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x07,0x08,0x08,0x08,0x07,0x00,0x0F,0x01,0x02,0x04,0x08,0x00,0x1F,0x00,0x0F,
	0x01,0x01,0x03,0x04,0x08,0x00,0x0C,0x03,0x02,0x02,0x02,0x03,0x0C,0x00,0x00,0x03,
	0x0C,0x03,0x00,0x03,0x0C,0x03,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
};

const u8 picAEB3_RAW_Light_78X16[] = {
	/* 图像	   78x16	*/
	0x00,0x00,0xFC,0xFC,0xFC,0x3C,0xCC,0x3C,0xFC,0xFC,0xFC,0x0C,0x6C,0x6C,0x6C,0x6C,
	0xFC,0x0C,0x6C,0x6C,0x6C,0x6C,0x9C,0xFC,0xFC,0xEC,0x6C,0x6C,0x6C,0x9C,0xFC,0x04,
	0xFC,0x0C,0xEC,0xEC,0xEC,0x1C,0xFC,0xFC,0xFC,0xFC,0x3C,0xCC,0x3C,0xFC,0xFC,0xFC,
	0x8C,0x7C,0xFC,0x7C,0x8C,0x7C,0xFC,0x7C,0x8C,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,
	0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0x00,0x00,
	0x3F,0x33,0x3C,0x3D,0x3D,0x3D,0x3C,0x33,0x3F,0x30,0x37,0x37,0x37,0x37,0x3F,0x30,
	0x37,0x37,0x37,0x37,0x38,0x3F,0x3F,0x37,0x37,0x37,0x37,0x38,0x3F,0x20,0x3F,0x30,
	0x3E,0x3E,0x3C,0x3B,0x37,0x3F,0x33,0x3C,0x3D,0x3D,0x3D,0x3C,0x33,0x3F,0x3F,0x3C,
	0x33,0x3C,0x3F,0x3C,0x33,0x3C,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,
	0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,
};


const u8 picAEB3_RAW_Nor_78X16[] = {
	/* 图像	   78x16	*/
	0x00,0x00,0x00,0x00,0x00,0xC0,0x30,0xC0,0x00,0x00,0x00,0xF0,0x90,0x90,0x90,0x90,
	0x00,0xF0,0x90,0x90,0x90,0x90,0x60,0x00,0x00,0x10,0x90,0x90,0x90,0x60,0x00,0xF8,
	0x00,0xF0,0x10,0x10,0x10,0xE0,0x00,0x00,0x00,0x00,0xC0,0x30,0xC0,0x00,0x00,0x00,
	0x70,0x80,0x00,0x80,0x70,0x80,0x00,0x80,0x70,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x0C,0x03,0x02,0x02,0x02,0x03,0x0C,0x00,0x0F,0x08,0x08,0x08,0x08,0x00,0x0F,
	0x08,0x08,0x08,0x08,0x07,0x00,0x00,0x08,0x08,0x08,0x08,0x07,0x00,0x1F,0x00,0x0F,
	0x01,0x01,0x03,0x04,0x08,0x00,0x0C,0x03,0x02,0x02,0x02,0x03,0x0C,0x00,0x00,0x03,
	0x0C,0x03,0x00,0x03,0x0C,0x03,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
};


const u8 picAEB5_RAW_Light_78X16[] = {
	/* 图像	   78x16	*/
	0x00,0x00,0xFC,0xFC,0xFC,0x3C,0xCC,0x3C,0xFC,0xFC,0xFC,0x0C,0x6C,0x6C,0x6C,0x6C,
	0xFC,0x0C,0x6C,0x6C,0x6C,0x6C,0x9C,0xFC,0xFC,0x0C,0x6C,0x6C,0x6C,0xFC,0xFC,0x04,
	0xFC,0x0C,0xEC,0xEC,0xEC,0x1C,0xFC,0xFC,0xFC,0xFC,0x3C,0xCC,0x3C,0xFC,0xFC,0xFC,
	0x8C,0x7C,0xFC,0x7C,0x8C,0x7C,0xFC,0x7C,0x8C,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,
	0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0x00,0x00,
	0x3F,0x33,0x3C,0x3D,0x3D,0x3D,0x3C,0x33,0x3F,0x30,0x37,0x37,0x37,0x37,0x3F,0x30,
	0x37,0x37,0x37,0x37,0x38,0x3F,0x3F,0x37,0x37,0x37,0x37,0x38,0x3F,0x20,0x3F,0x30,
	0x3E,0x3E,0x3C,0x3B,0x37,0x3F,0x33,0x3C,0x3D,0x3D,0x3D,0x3C,0x33,0x3F,0x3F,0x3C,
	0x33,0x3C,0x3F,0x3C,0x33,0x3C,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,
	0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,
};


const u8 picAEB5_RAW_Nor_78X16[] = {
	/* 图像	   78x16	*/
	0x00,0x00,0x00,0x00,0x00,0xC0,0x30,0xC0,0x00,0x00,0x00,0xF0,0x90,0x90,0x90,0x90,
	0x00,0xF0,0x90,0x90,0x90,0x90,0x60,0x00,0x00,0xF0,0x90,0x90,0x90,0x00,0x00,0xF8,
	0x00,0xF0,0x10,0x10,0x10,0xE0,0x00,0x00,0x00,0x00,0xC0,0x30,0xC0,0x00,0x00,0x00,
	0x70,0x80,0x00,0x80,0x70,0x80,0x00,0x80,0x70,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x0C,0x03,0x02,0x02,0x02,0x03,0x0C,0x00,0x0F,0x08,0x08,0x08,0x08,0x00,0x0F,
	0x08,0x08,0x08,0x08,0x07,0x00,0x00,0x08,0x08,0x08,0x08,0x07,0x00,0x1F,0x00,0x0F,
	0x01,0x01,0x03,0x04,0x08,0x00,0x0C,0x03,0x02,0x02,0x02,0x03,0x0C,0x00,0x00,0x03,
	0x0C,0x03,0x00,0x03,0x0C,0x03,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
};


const u8 picAEB7_RAW_Light_78X16[] = {
	/* 图像	   78x16	*/
	0x00,0x00,0xFC,0xFC,0xFC,0x3C,0xCC,0x3C,0xFC,0xFC,0xFC,0x0C,0x6C,0x6C,0x6C,0x6C,
	0xFC,0x0C,0x6C,0x6C,0x6C,0x6C,0x9C,0xFC,0xFC,0xEC,0xEC,0xEC,0x6C,0x8C,0xFC,0x04,
	0xFC,0x0C,0xEC,0xEC,0xEC,0x1C,0xFC,0xFC,0xFC,0xFC,0x3C,0xCC,0x3C,0xFC,0xFC,0xFC,
	0x8C,0x7C,0xFC,0x7C,0x8C,0x7C,0xFC,0x7C,0x8C,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,
	0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0x00,0x00,
	0x3F,0x33,0x3C,0x3D,0x3D,0x3D,0x3C,0x33,0x3F,0x30,0x37,0x37,0x37,0x37,0x3F,0x30,
	0x37,0x37,0x37,0x37,0x38,0x3F,0x3F,0x3F,0x3F,0x31,0x3E,0x3F,0x3F,0x20,0x3F,0x30,
	0x3E,0x3E,0x3C,0x3B,0x37,0x3F,0x33,0x3C,0x3D,0x3D,0x3D,0x3C,0x33,0x3F,0x3F,0x3C,
	0x33,0x3C,0x3F,0x3C,0x33,0x3C,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,
	0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,
};


const u8 picAEB7_RAW_Nor_78X16[] = {
	/* 图像	   78x16	*/
	0x00,0x00,0x00,0x00,0x00,0xC0,0x30,0xC0,0x00,0x00,0x00,0xF0,0x90,0x90,0x90,0x90,
	0x00,0xF0,0x90,0x90,0x90,0x90,0x60,0x00,0x00,0x10,0x10,0x10,0x90,0x70,0x00,0xF8,
	0x00,0xF0,0x10,0x10,0x10,0xE0,0x00,0x00,0x00,0x00,0xC0,0x30,0xC0,0x00,0x00,0x00,
	0x70,0x80,0x00,0x80,0x70,0x80,0x00,0x80,0x70,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x0C,0x03,0x02,0x02,0x02,0x03,0x0C,0x00,0x0F,0x08,0x08,0x08,0x08,0x00,0x0F,
	0x08,0x08,0x08,0x08,0x07,0x00,0x00,0x00,0x00,0x0E,0x01,0x00,0x00,0x1F,0x00,0x0F,
	0x01,0x01,0x03,0x04,0x08,0x00,0x0C,0x03,0x02,0x02,0x02,0x03,0x0C,0x00,0x00,0x03,
	0x0C,0x03,0x00,0x03,0x0C,0x03,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
};


const u8 picAEB9_RAW_Light_78X16[] = {
	/* 图像	   78x16	*/
	0x00,0x00,0xFC,0xFC,0xFC,0x3C,0xCC,0x3C,0xFC,0xFC,0xFC,0x0C,0x6C,0x6C,0x6C,0x6C,
	0xFC,0x0C,0x6C,0x6C,0x6C,0x6C,0x9C,0xFC,0xFC,0x1C,0xEC,0xEC,0xEC,0x1C,0xFC,0x04,
	0xFC,0x0C,0xEC,0xEC,0xEC,0x1C,0xFC,0xFC,0xFC,0xFC,0x3C,0xCC,0x3C,0xFC,0xFC,0xFC,
	0x8C,0x7C,0xFC,0x7C,0x8C,0x7C,0xFC,0x7C,0x8C,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,
	0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0x00,0x00,
	0x3F,0x33,0x3C,0x3D,0x3D,0x3D,0x3C,0x33,0x3F,0x30,0x37,0x37,0x37,0x37,0x3F,0x30,
	0x37,0x37,0x37,0x37,0x38,0x3F,0x3F,0x3B,0x36,0x36,0x36,0x38,0x3F,0x20,0x3F,0x30,
	0x3E,0x3E,0x3C,0x3B,0x37,0x3F,0x33,0x3C,0x3D,0x3D,0x3D,0x3C,0x33,0x3F,0x3F,0x3C,
	0x33,0x3C,0x3F,0x3C,0x33,0x3C,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,
	0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,
};


const u8 picAEB9_RAW_Nor_78X16[] = {
	/* 图像	   78x16	*/
	0x00,0x00,0x00,0x00,0x00,0xC0,0x30,0xC0,0x00,0x00,0x00,0xF0,0x90,0x90,0x90,0x90,
	0x00,0xF0,0x90,0x90,0x90,0x90,0x60,0x00,0x00,0xE0,0x10,0x10,0x10,0xE0,0x00,0xF8,
	0x00,0xF0,0x10,0x10,0x10,0xE0,0x00,0x00,0x00,0x00,0xC0,0x30,0xC0,0x00,0x00,0x00,
	0x70,0x80,0x00,0x80,0x70,0x80,0x00,0x80,0x70,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x0C,0x03,0x02,0x02,0x02,0x03,0x0C,0x00,0x0F,0x08,0x08,0x08,0x08,0x00,0x0F,
	0x08,0x08,0x08,0x08,0x07,0x00,0x00,0x04,0x09,0x09,0x09,0x07,0x00,0x1F,0x00,0x0F,
	0x01,0x01,0x03,0x04,0x08,0x00,0x0C,0x03,0x02,0x02,0x02,0x03,0x0C,0x00,0x00,0x03,
	0x0C,0x03,0x00,0x03,0x0C,0x03,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
};


const u8 picBurstRAWLight_78x16[] = {
	/* 图像	   78x16	*/
	0x00,0x00,0xFC,0x0C,0x6C,0x6C,0x6C,0x6C,0x9C,0xFC,0x3C,0xFC,0xFC,0xFC,0x3C,0xFC,
	0x3C,0xBC,0xBC,0xFC,0x7C,0xBC,0xBC,0xBC,0x7C,0xFC,0x0C,0xBC,0xBC,0xFC,0x04,0xFC,
	0x0C,0xEC,0xEC,0xEC,0x1C,0xFC,0xFC,0xFC,0xFC,0x3C,0xCC,0x3C,0xFC,0xFC,0xFC,0x8C,
	0x7C,0xFC,0x7C,0x8C,0x7C,0xFC,0x7C,0x8C,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,
	0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0x00,0x00,
	0x3F,0x30,0x37,0x37,0x37,0x37,0x38,0x3F,0x38,0x37,0x37,0x37,0x30,0x3F,0x30,0x3F,
	0x3F,0x3F,0x3B,0x36,0x36,0x35,0x3B,0x3F,0x38,0x37,0x37,0x3F,0x20,0x3F,0x30,0x3E,
	0x3E,0x3C,0x3B,0x37,0x3F,0x33,0x3C,0x3D,0x3D,0x3D,0x3C,0x33,0x3F,0x3F,0x3C,0x33,
	0x3C,0x3F,0x3C,0x33,0x3C,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,
	0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,
};


const u8 picBurstRAWNor_78x16[] = {
	/* 图像	   78x16	*/
	0x00,0x00,0x00,0xF0,0x90,0x90,0x90,0x90,0x60,0x00,0xC0,0x00,0x00,0x00,0xC0,0x00,
	0xC0,0x40,0x40,0x00,0x80,0x40,0x40,0x40,0x80,0x00,0xF0,0x40,0x40,0x00,0xF8,0x00,
	0xF0,0x10,0x10,0x10,0xE0,0x00,0x00,0x00,0x00,0xC0,0x30,0xC0,0x00,0x00,0x00,0x70,
	0x80,0x00,0x80,0x70,0x80,0x00,0x80,0x70,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x0F,0x08,0x08,0x08,0x08,0x07,0x00,0x07,0x08,0x08,0x08,0x0F,0x00,0x0F,0x00,
	0x00,0x00,0x04,0x09,0x09,0x0A,0x04,0x00,0x07,0x08,0x08,0x00,0x1F,0x00,0x0F,0x01,
	0x01,0x03,0x04,0x08,0x00,0x0C,0x03,0x02,0x02,0x02,0x03,0x0C,0x00,0x00,0x03,0x0C,
	0x03,0x00,0x03,0x0C,0x03,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
};



/*
 * 不带RAW项
 * - 8K|3D|OF
 * - 8K|3D
 * - 8K
 * - AEB3,5,7,9
 * - BURST
 * - CUSTOMER
 */


const u8 pic8K3DOFLight_78X16[] = {
	0x00,0x00,0xFC,0x9C,0x6C,0x6C,0x6C,0x9C,0xFC,0x0C,0x7C,0xBC,0xDC,0xEC,0xFC,0x04,
	0xFC,0xEC,0x6C,0x6C,0x6C,0x9C,0xFC,0x0C,0xEC,0xEC,0xEC,0xEC,0xDC,0x3C,0xFC,0x04,
	0xFC,0x3C,0xDC,0xEC,0xEC,0xEC,0xDC,0x3C,0xFC,0x0C,0x6C,0x6C,0x6C,0xEC,0xFC,0xFC,
	0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,
	0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0x00,0x00,
	0x3F,0x38,0x37,0x37,0x37,0x38,0x3F,0x30,0x3E,0x3D,0x3B,0x37,0x3F,0x20,0x3F,0x37,
	0x37,0x37,0x37,0x38,0x3F,0x30,0x37,0x37,0x37,0x37,0x3B,0x3C,0x3F,0x20,0x3F,0x3C,
	0x3B,0x37,0x37,0x37,0x3B,0x3C,0x3F,0x30,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,
	0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,
	0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,
};


const u8 pic8K3DOFNor_78X16[] = {
	0x00,0x00,0x00,0x60,0x90,0x90,0x90,0x60,0x00,0xF0,0x80,0x40,0x20,0x10,0x00,0xF8,
	0x00,0x10,0x90,0x90,0x90,0x60,0x00,0xF0,0x10,0x10,0x10,0x10,0x20,0xC0,0x00,0xF8,
	0x00,0xC0,0x20,0x10,0x10,0x10,0x20,0xC0,0x00,0xF0,0x90,0x90,0x90,0x10,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x07,0x08,0x08,0x08,0x07,0x00,0x0F,0x01,0x02,0x04,0x08,0x00,0x1F,0x00,0x08,
	0x08,0x08,0x08,0x07,0x00,0x0F,0x08,0x08,0x08,0x08,0x04,0x03,0x00,0x1F,0x00,0x03,
	0x04,0x08,0x08,0x08,0x04,0x03,0x00,0x0F,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
};


const u8 pic8K3DLight_78x16[] = {
	0x00,0x00,0xFC,0x9C,0x6C,0x6C,0x6C,0x9C,0xFC,0x0C,0x7C,0xBC,0xDC,0xEC,0xFC,0x04,
	0xFC,0x3C,0xDC,0xEC,0xEC,0xEC,0xDC,0x3C,0xFC,0x0C,0x6C,0x6C,0x6C,0xEC,0xFC,0xFC,
	0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,
	0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,
	0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0x00,0x00,
	0x3F,0x38,0x37,0x37,0x37,0x38,0x3F,0x30,0x3E,0x3D,0x3B,0x37,0x3F,0x20,0x3F,0x3C,
	0x3B,0x37,0x37,0x37,0x3B,0x3C,0x3F,0x30,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,
	0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,
	0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,
	0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,	
};

const u8 pic8K3DNor_78x16[] = {
	0x00,0x00,0x00,0x60,0x90,0x90,0x90,0x60,0x00,0xF0,0x80,0x40,0x20,0x10,0x00,0xF8,
	0x00,0xC0,0x20,0x10,0x10,0x10,0x20,0xC0,0x00,0xF0,0x90,0x90,0x90,0x10,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x07,0x08,0x08,0x08,0x07,0x00,0x0F,0x01,0x02,0x04,0x08,0x00,0x1F,0x00,0x03,
	0x04,0x08,0x08,0x08,0x04,0x03,0x00,0x0F,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
};


const u8 pic8KLight_78x16[] = {
	0x00,0x00,0xFC,0x9C,0x6C,0x6C,0x6C,0x9C,0xFC,0x0C,0x7C,0xBC,0xDC,0xEC,0xFC,0xFC,
	0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,
	0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,
	0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,
	0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0x00,0x00,
	0x3F,0x38,0x37,0x37,0x37,0x38,0x3F,0x30,0x3E,0x3D,0x3B,0x37,0x3F,0x3F,0x3F,0x3F,
	0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,
	0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,
	0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,
	0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,	
};

const u8 pic8KNor_78x16[] = {
	0x00,0x00,0x00,0x60,0x90,0x90,0x90,0x60,0x00,0xF0,0x80,0x40,0x20,0x10,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x07,0x08,0x08,0x08,0x07,0x00,0x0F,0x01,0x02,0x04,0x08,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
};


const u8 picAEB3Light_78X16[] = {
	/* 图像	   78x16	*/
	0x00,0x00,0xFC,0xFC,0xFC,0x3C,0xCC,0x3C,0xFC,0xFC,0xFC,0x0C,0x6C,0x6C,0x6C,0x6C,
	0xFC,0x0C,0x6C,0x6C,0x6C,0x6C,0x9C,0xFC,0xFC,0xEC,0x6C,0x6C,0x6C,0x9C,0xFC,0xFC,
	0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,
	0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,
	0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0x00,0x00,
	0x3F,0x33,0x3C,0x3D,0x3D,0x3D,0x3C,0x33,0x3F,0x30,0x37,0x37,0x37,0x37,0x3F,0x30,
	0x37,0x37,0x37,0x37,0x38,0x3F,0x3F,0x37,0x37,0x37,0x37,0x38,0x3F,0x3F,0x3F,0x3F,
	0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,
	0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,
	0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,
};


const u8 picAEB3Nor_78X16[] = {
	/* 图像	   78x16	*/
	0x00,0x00,0x00,0x00,0x00,0xC0,0x30,0xC0,0x00,0x00,0x00,0xF0,0x90,0x90,0x90,0x90,
	0x00,0xF0,0x90,0x90,0x90,0x90,0x60,0x00,0x00,0x10,0x90,0x90,0x90,0x60,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x0C,0x03,0x02,0x02,0x02,0x03,0x0C,0x00,0x0F,0x08,0x08,0x08,0x08,0x00,0x0F,
	0x08,0x08,0x08,0x08,0x07,0x00,0x00,0x08,0x08,0x08,0x08,0x07,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
};

const u8 picAEB5Light_78X16[] = {
	/* 图像	   78x16	*/
	0x00,0x00,0xFC,0xFC,0xFC,0x3C,0xCC,0x3C,0xFC,0xFC,0xFC,0x0C,0x6C,0x6C,0x6C,0x6C,
	0xFC,0x0C,0x6C,0x6C,0x6C,0x6C,0x9C,0xFC,0xFC,0x0C,0x6C,0x6C,0x6C,0xFC,0xFC,0xFC,
	0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,
	0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,
	0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0x00,0x00,
	0x3F,0x33,0x3C,0x3D,0x3D,0x3D,0x3C,0x33,0x3F,0x30,0x37,0x37,0x37,0x37,0x3F,0x30,
	0x37,0x37,0x37,0x37,0x38,0x3F,0x3F,0x37,0x37,0x37,0x37,0x38,0x3F,0x3F,0x3F,0x3F,
	0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,
	0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,
	0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,
};


const u8 picAEB5Nor_78X16[] = {
	/* 图像	   78x16	*/
	0x00,0x00,0x00,0x00,0x00,0xC0,0x30,0xC0,0x00,0x00,0x00,0xF0,0x90,0x90,0x90,0x90,
	0x00,0xF0,0x90,0x90,0x90,0x90,0x60,0x00,0x00,0xF0,0x90,0x90,0x90,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x0C,0x03,0x02,0x02,0x02,0x03,0x0C,0x00,0x0F,0x08,0x08,0x08,0x08,0x00,0x0F,
	0x08,0x08,0x08,0x08,0x07,0x00,0x00,0x08,0x08,0x08,0x08,0x07,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
};


const u8 picAEB7Light_78X16[] = {
	/* 图像	   78x16	*/
	0x00,0x00,0xFC,0xFC,0xFC,0x3C,0xCC,0x3C,0xFC,0xFC,0xFC,0x0C,0x6C,0x6C,0x6C,0x6C,
	0xFC,0x0C,0x6C,0x6C,0x6C,0x6C,0x9C,0xFC,0xFC,0xEC,0xEC,0xEC,0x6C,0x8C,0xFC,0xFC,
	0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,
	0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,
	0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0x00,0x00,
	0x3F,0x33,0x3C,0x3D,0x3D,0x3D,0x3C,0x33,0x3F,0x30,0x37,0x37,0x37,0x37,0x3F,0x30,
	0x37,0x37,0x37,0x37,0x38,0x3F,0x3F,0x3F,0x3F,0x31,0x3E,0x3F,0x3F,0x3F,0x3F,0x3F,
	0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,
	0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,
	0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,
};


const u8 picAEB7Nor_78X16[] = {
	/* 图像	   78x16	*/
	0x00,0x00,0x00,0x00,0x00,0xC0,0x30,0xC0,0x00,0x00,0x00,0xF0,0x90,0x90,0x90,0x90,
	0x00,0xF0,0x90,0x90,0x90,0x90,0x60,0x00,0x00,0x10,0x10,0x10,0x90,0x70,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x0C,0x03,0x02,0x02,0x02,0x03,0x0C,0x00,0x0F,0x08,0x08,0x08,0x08,0x00,0x0F,
	0x08,0x08,0x08,0x08,0x07,0x00,0x00,0x00,0x00,0x0E,0x01,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
};



const u8 picAEB9Light_78X16[] = {
	/* 图像	   78x16	*/
	0x00,0x00,0xFC,0xFC,0xFC,0x3C,0xCC,0x3C,0xFC,0xFC,0xFC,0x0C,0x6C,0x6C,0x6C,0x6C,
	0xFC,0x0C,0x6C,0x6C,0x6C,0x6C,0x9C,0xFC,0xFC,0x1C,0xEC,0xEC,0xEC,0x1C,0xFC,0xFC,
	0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,
	0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,
	0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0xFC,0x00,0x00,
	0x3F,0x33,0x3C,0x3D,0x3D,0x3D,0x3C,0x33,0x3F,0x30,0x37,0x37,0x37,0x37,0x3F,0x30,
	0x37,0x37,0x37,0x37,0x38,0x3F,0x3F,0x3B,0x36,0x36,0x36,0x38,0x3F,0x3F,0x3F,0x3F,
	0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,
	0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,
	0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,
};


const u8 picAEB9Nor_78X16[] = {
	/* 图像	   78x16	*/
	0x00,0x00,0x00,0x00,0x00,0xC0,0x30,0xC0,0x00,0x00,0x00,0xF0,0x90,0x90,0x90,0x90,
	0x00,0xF0,0x90,0x90,0x90,0x90,0x60,0x00,0x00,0xE0,0x10,0x10,0x10,0xE0,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x0C,0x03,0x02,0x02,0x02,0x03,0x0C,0x00,0x0F,0x08,0x08,0x08,0x08,0x00,0x0F,
	0x08,0x08,0x08,0x08,0x07,0x00,0x00,0x04,0x09,0x09,0x09,0x07,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
};


const u8 picBurstLight_78x16[] = {
	0x00, 0x00, 0xfc, 0x0c, 0x6c, 0x6c, 0x6c, 0x6c, 0x9c, 0xfc, 0x3c, 0xfc, 0xfc, 0xfc, 0x3c, 0xfc,
	0x3c, 0xbc, 0xbc, 0xfc, 0x7c, 0xbc, 0xbc, 0xbc, 0x7c, 0xfc, 0x0c, 0xbc, 0xbc, 0xfc, 0xfc, 0xfc,
	0xfc, 0xfc, 0xfc, 0xfc, 0xfc, 0xfc, 0xfc, 0xfc, 0xfc, 0xfc, 0xfc, 0xfc, 0xfc, 0xfc, 0xfc, 0xfc,
	0xfc, 0xfc, 0xfc, 0xfc, 0xfc, 0xfc, 0xfc, 0xfc, 0xfc, 0xfc, 0xfc, 0xfc, 0xfc, 0xfc, 0xfc, 0xfc,
	0xfc, 0xfc, 0xfc, 0xfc, 0xfc, 0xfc, 0xfc, 0xfc, 0xfc, 0xfc, 0xfc, 0xfc, 0xfc, 0xfc, 0x00, 0x00,
	0x3f, 0x30, 0x37, 0x37, 0x37, 0x37, 0x38, 0x3f, 0x38, 0x37, 0x37, 0x37, 0x30, 0x3f, 0x30, 0x3f,
	0x3f, 0x3f, 0x3b, 0x36, 0x36, 0x35, 0x3b, 0x3f, 0x38, 0x37, 0x37, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f,
	0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f,
	0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f,
	0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f,	
};

const u8 picBurstNor_78x16[] = {
	0x00, 0x00, 0x00, 0xf0, 0x90, 0x90, 0x90, 0x90, 0x60, 0x00, 0xc0, 0x00, 0x00, 0x00, 0xc0, 0x00,
	0xc0, 0x40, 0x40, 0x00, 0x80, 0x40, 0x40, 0x40, 0x80, 0x00, 0xf0, 0x40, 0x40, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x0f, 0x08, 0x08, 0x08, 0x08, 0x07, 0x00, 0x07, 0x08, 0x08, 0x08, 0x0f, 0x00, 0x0f, 0x00,
	0x00, 0x00, 0x04, 0x09, 0x09, 0x0a, 0x04, 0x00, 0x07, 0x08, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};


const u8 picCustmLight_78x16[ ] =  {
	0x00, 0x00, 0xfc, 0x7c, 0xbc, 0xbc, 0xbc, 0xfc, 0x3c, 0xfc, 0xfc, 0xfc, 0x3c, 0xfc, 0x7c, 0xbc,
	0xbc, 0xbc, 0x7c, 0xfc, 0x0c, 0xbc, 0xbc, 0xfc, 0x7c, 0xbc, 0xbc, 0xbc, 0x7c, 0xfc, 0x3c, 0xbc,
	0xbc, 0x7c, 0xbc, 0xbc, 0x7c, 0xfc, 0x2c, 0xfc, 0xbc, 0xbc, 0xbc, 0xbc, 0x3c, 0xfc, 0x7c, 0xbc,
	0xbc, 0xbc, 0x7c, 0xfc, 0xfc, 0xfc, 0xfc, 0xfc, 0xfc, 0xfc, 0xfc, 0xfc, 0xfc, 0xfc, 0xfc, 0xfc,
	0xfc, 0xfc, 0xfc, 0xfc, 0xfc, 0xfc, 0xfc, 0xfc, 0xfc, 0xfc, 0xfc, 0xfc, 0xfc, 0xfc, 0x00, 0x00,
	0x3f, 0x38, 0x37, 0x37, 0x37, 0x3f, 0x38, 0x37, 0x37, 0x37, 0x30, 0x3f, 0x3b, 0x36, 0x36, 0x35,
	0x3b, 0x3f, 0x38, 0x37, 0x37, 0x3f, 0x38, 0x37, 0x37, 0x37, 0x38, 0x3f, 0x30, 0x3f, 0x3f, 0x30,
	0x3f, 0x3f, 0x30, 0x3f, 0x30, 0x3f, 0x37, 0x33, 0x35, 0x36, 0x37, 0x3f, 0x38, 0x36, 0x36, 0x36,
	0x36, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f,
	0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 
};

const u8  picCustmNor_78x16[ ] =  {
	0x00, 0x00, 0x00, 0x80, 0x40, 0x40, 0x40, 0x00, 0xc0, 0x00, 0x00, 0x00, 0xc0, 0x00, 0x80, 0x40,
	0x40, 0x40, 0x80, 0x00, 0xf0, 0x40, 0x40, 0x00, 0x80, 0x40, 0x40, 0x40, 0x80, 0x00, 0xc0, 0x40,
	0x40, 0x80, 0x40, 0x40, 0x80, 0x00, 0xd0, 0x00, 0x40, 0x40, 0x40, 0x40, 0xc0, 0x00, 0x80, 0x40,
	0x40, 0x40, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x07, 0x08, 0x08, 0x08, 0x00, 0x07, 0x08, 0x08, 0x08, 0x0f, 0x00, 0x04, 0x09, 0x09, 0x0a,
	0x04, 0x00, 0x07, 0x08, 0x08, 0x00, 0x07, 0x08, 0x08, 0x08, 0x07, 0x00, 0x0f, 0x00, 0x00, 0x0f,
	0x00, 0x00, 0x0f, 0x00, 0x0f, 0x00, 0x08, 0x0c, 0x0a, 0x09, 0x08, 0x00, 0x07, 0x09, 0x09, 0x09,
	0x09, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
};



struct stIconPos; 
struct _action_info_;

typedef struct stPicVideoCfg {
	const char* 			pItemName;								/* 设置项的名称 */
	int						iItemMaxVal;							/* 设置项可取的最大值 */
	int  					iCurVal;								/* 当前的值,(根据当前值来选择对应的图标) */
	int						iRawStorageRatio;	/* 使能RAW时的存储比例 */					
	struct stIconPos		stPos;
    struct _action_info_*   pStAction;
	const u8 * 				stLightIcon[PIC_VIDEO_LIVE_ITEM_MAX];	/* 选中时的图标列表 */
	const u8 * 				stNorIcon[PIC_VIDEO_LIVE_ITEM_MAX];		/* 未选中时的图标列表 */
} PicVideoCfg;


/*
 * 8K|3D|OF ICON_INFO
 */
static ACTION_INFO pic8K3DOFDefault = {
	MODE_3D,		/* 拼接模式 */
	40,				/* 每张照片的大小: 36M, 180M */
	0,				/* 该值现在无效,由photodelay控制 */
	{	/* Origin */
		EN_JPEG,	
		SAVE_DEF,
		4000,
		3000,
		0,			/* 原片的存储位置: 0: nvidia; 1: module; 2: both */
		{}
	},
	{	/* Stitch */
		EN_JPEG,
		STITCH_OPTICAL_FLOW,
		7680,
		7680,
		{},
	}
};



/*
 * 8K|3D
 */
static ACTION_INFO pic8K3DDefault = {
	MODE_PANO,		/* 拼接模式 */
	30,				/* 每张照片的大小: 30M, 170M */
	0,				/* 该值现在无效,由photodelay控制 */
	{	/* Origin */
		EN_JPEG,	
		SAVE_DEF,
		4000,
		3000,
		0,			/* 原片的存储位置: 0: nvidia; 1: module; 2: both */
		{}
	},
	{	/* Stitch */
		EN_JPEG,
		STITCH_OPTICAL_FLOW,
		7680,
		7680,
		{},
	}
};


/*
 * 8K
 */
static ACTION_INFO pic8KDefault = {
	MODE_PANO,		/* 拼接模式 */
	25,				/* 每张照片的大小: 25M, 160M */
	0,				/* 该值现在无效,由photodelay控制 */
	{	/* Origin */
		EN_JPEG,	
		SAVE_DEF,
		4000,
		3000,
		0,			/* 原片的存储位置: 0: nvidia; 1: module; 2: both */
		{}
	},
	{	/* Stitch */
		EN_JPEG,
		STITCH_OPTICAL_FLOW,
		7680,
		3840,
		{},
	}
};


/*
 * AEB
 */
static ACTION_INFO picAebDefault = {
	MODE_PANO,				/* 拼接模式 */
	30,						/* 每张照片的大小: 40M, 80M, 120M, 160M */
	0,						/* 该值现在无效,由photodelay控制 */
	{	/* Origin */
		EN_JPEG,	
		SAVE_DEF,
		4000,
		3000,
		0,					/* 原片的存储位置: 0: nvidia; 1: module; 2: both */
		{3, -64, 64, 0}		/* AEB:3, 5, 7, 9 */
	},
	{	/* Stitch */
		EN_JPEG,
		STITCH_OFF,
		7680,
		3840,
		{},
	}
};


/*
 * Burst
 */
static ACTION_INFO picBurstDefault = {
	MODE_PANO,		/* 拼接模式 */
	150,			/* 每张照片的大小 */
	0,				/* 该值现在无效,由photodelay控制 */
	{	/* Origin */
		EN_JPEG,	
		SAVE_DEF,
		4000,
		3000,
		0,			/* 原片的存储位置: 0: nvidia; 1: module; 2: both */
		{0,0,0,10}
	},
	{	/* Stitch */
		EN_JPEG,
		STITCH_OFF,
		7680,
		3840,
		{},
	}
};


/*
 * Customer
 */
static ACTION_INFO picCustomerDefault = {
	MODE_PANO,		/* 拼接模式 */
	30,				/* 每张照片的大小 */
	0,				/* 该值现在无效,由photodelay控制 */
	{	/* Origin */
		EN_JPEG,	
		SAVE_DEF,
		4000,
		3000,
		0,			/* 原片的存储位置: 0: nvidia; 1: module; 2: both */
	},
	{	/* Stitch */
		EN_JPEG,
		STITCH_OPTICAL_FLOW,
		7680,
		3840,
		{},
	}
};


PicVideoCfg pic8K_3D_OF = {
	TAKE_PIC_MODE_8K_3D_OF,		// pItemName
	1,							// iItemMaxVal
	0,							// iCurVal
	5,							// 5倍
	{0},						// stPos
	&pic8K3DOFDefault,			/* 默认值,如果由配置文件可以在初始化时使用配置文件的数据替换 */
	{	/* 选中时的图标列表 */
		pic8K3DOFLight_78X16,
		pic8K3DOFRAWLight_78X16,
	},
	{	/* 未选中时的图标列表 */
		pic8K3DOFNor_78X16,
		pic8K3DOF_RAW_Nor_78x16,
	}
};

PicVideoCfg pic8K_3D = {
	TAKE_PIC_MODE_8K_3D,		// pItemName
	1,							// iItemMaxVal
	0,							// iCurVal
	5,							// 5倍	
	{0},						// stPos
	&pic8K3DDefault,
	{	/* 选中时的图标列表 */
		pic8K3DLight_78x16,
		pic8K3DRAWLight_78X16,
	},
	{	/* 未选中时的图标列表 */
		pic8K3DNor_78x16,
		pic8K3DRAWNor_78X16
	}
};


PicVideoCfg pic8K = {
	TAKE_PIC_MODE_8K,		// pItemName
	1,							// iItemMaxVal
	0,							// iCurVal
	5,							// 5倍	
	{0},						// stPos
	&pic8KDefault,
	{	/* 选中时的图标列表 */
		pic8KLight_78x16,
		pic8KRAWLight_78X16,
	},
	{	/* 未选中时的图标列表 */
		pic8KNor_78x16,
		pic8KRAWNor_78X16
	}
};


PicVideoCfg picAEB = {
	TAKE_PIC_MODE_AEB,			// pItemName
	7,							// iItemMaxVal
	0,							// iCurVal
	10,							// 10倍
	{0},						// stPos
	&picAebDefault,
	{	/* 选中时的图标列表 */
		picAEB3Light_78X16,
		picAEB5Light_78X16,
		picAEB7Light_78X16,
		picAEB9Light_78X16,
		picAEB3_RAW_Light_78X16,
		picAEB5_RAW_Light_78X16,
		picAEB7_RAW_Light_78X16,
		picAEB9_RAW_Light_78X16,
	},
	{	/* 未选中时的图标列表 */
		picAEB3Nor_78X16,
		picAEB5Nor_78X16,
		picAEB7Nor_78X16,
		picAEB9Nor_78X16,
		picAEB3_RAW_Nor_78X16,
		picAEB5_RAW_Nor_78X16,
		picAEB7_RAW_Nor_78X16,
		picAEB9_RAW_Nor_78X16,
	}
};


PicVideoCfg picBurst = {
	TAKE_PIC_MODE_BURST,		// pItemName
	1,							// iItemMaxVal
	0,							// iCurVal
	10,							// 10倍	
	{0},						// stPos
	&picBurstDefault,
	{	/* 选中时的图标列表 */
		picBurstLight_78x16,
		picBurstRAWLight_78x16,
	},
	{	/* 未选中时的图标列表 */
		picBurstNor_78x16,
		picBurstRAWNor_78x16,
	}
};


PicVideoCfg picCustomer = {
	TAKE_PIC_MODE_CUSTOMER,		// pItemName
	0,							// iItemMaxVal
	0,							// iCurVal
	10,
	{0},						// stPos
	&picCustomerDefault,
	{	/* 选中时的图标列表 */
		picCustmLight_78x16,
	},
	{	/* 未选中时的图标列表 */
		picCustmNor_78x16,
	}
};


/*
 * 系统含有SD卡/USB硬件及TF卡时支持的拍照模式（目前只支持该种模式）
 */
PicVideoCfg* gPicAllModeCfgList[] = {
	&pic8K_3D_OF,
	&pic8K_3D,
	&pic8K,
	&picAEB,
	&picBurst,
	&picCustomer,
};


/*
 * 系统仅含SD卡/USB移动硬盘时支持的拍照规格
 */
PicVideoCfg* gPicSdModeCfgList[] = {
	&pic8K_3D_OF,
	&pic8K_3D,
	&pic8K,
	&picAEB,
	&picBurst,
	&picCustomer,
};


/*
 * 系统仅含TF卡时支持的拍照规格
 */
PicVideoCfg* gPicTFModeCfgList[] = {
	&pic8K_3D_OF,
	&pic8K_3D,
	&pic8K,
	&picAEB,
	&picBurst,
	&picCustomer,
};


#endif /* _PIC_VIDEO_SELECT_H_ */