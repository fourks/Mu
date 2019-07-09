#ifndef _PXA260_DMA_H_
#define _PXA260_DMA_H_

#include "pxa260_mem.h"
#include "pxa260_CPU.h"
#include "pxa260_IC.h"

/*
	PXA260 OS DMA controller

*/

#define PXA260_DMA_BASE		0x40000000UL
#define PXA260_DMA_SIZE		0x00001000UL

typedef struct{
	
	UInt32 DAR;	//descriptor address register
	UInt32 SAR;	//source address register
	UInt32 TAR;	//target address register
	UInt32 CR;	//command register
	UInt32 CSR;	//control and status register
	
}Pxa255dmaChannel;

typedef struct{

	Pxa255ic* ic;
	ArmMem* mem;
	
	UInt16 DINT;
	Pxa255dmaChannel channels[16];
	UInt8 CMR[40];			//channel map registers	[  we store lower 8 bits only :-)  ]
	
}Pxa255dma;



Boolean pxa260dmaInit(Pxa255dma* gpio, ArmMem* physMem, Pxa255ic* ic);

#endif
