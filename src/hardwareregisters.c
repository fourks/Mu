#include <stdio.h>
#include <string.h>

#include <boolean.h>

#include "emulator.h"
#include "hardwareRegisterNames.h"
#include "m68k/m68k.h"


static inline unsigned int registerArrayRead8(unsigned int address){
   return palmReg[address];
}

static inline unsigned int registerArrayRead16(unsigned int address){
   return palmReg[address] << 8 | palmReg[address + 1];
}

static inline unsigned int registerArrayRead32(unsigned int address){
   return palmReg[address] << 24 | palmReg[address + 1] << 16 | palmReg[address + 2] << 8 | palmReg[address + 3];
}

static inline void registerArrayWrite8(unsigned int address, unsigned int value){
   palmReg[address] = value;
}

static inline void registerArrayWrite16(unsigned int address, unsigned int value){
   palmReg[address] = value >> 8;
   palmReg[address + 1] = value & 0xFF;
}

static inline void registerArrayWrite32(unsigned int address, unsigned int value){
   palmReg[address] = value >> 24;
   palmReg[address + 1] = (value >> 16) & 0xFF;
   palmReg[address + 2] = (value >> 8) & 0xFF;
   palmReg[address + 3] = value & 0xFF;
}


void printUnknownHwAccess(unsigned int address, unsigned int value, unsigned int size, bool isWrite){
   if(isWrite){
      printf("Cpu Wrote %d bits of 0x%08X to register 0x%04X.\n", size, value, address);
   }
   else{
      printf("Cpu Read %d bits from register 0x%04X.\n", size, address);
   }
}

void setPllfsr16(unsigned int value){
   if(!(registerArrayRead16(PLLFSR) & 0x4000)){
      //frequency protect bit not set
      registerArrayWrite16(PLLFSR, value & 0x4FFF);
      uint32_t prescaler1 = (registerArrayRead16(PLLCR) & 0x0080) ? 2 : 1;
      uint32_t p = value & 0x00FF;
      uint32_t q = (value & 0x0F00) >> 8;
      uint32_t newCrystalCycles = 2 * (14 * (p + 1) + q + 1) / prescaler1;
      uint32_t newFrequency = newCrystalCycles * CRYSTAL_FREQUENCY;
      printf("New CPU frequency of:%d cycles per second.\n", newFrequency);
      printf("New clk32 cycle count of :%d.\n", newCrystalCycles);
      
      palmCpuFrequency = newFrequency;
   }
}

void setPllcr(unsigned int value){
   //values that matter are disable pll, prescaler 1 and possibly wakeselect
   registerArrayWrite16(PLLCR, value & 0x3FBB);
   uint16_t pllfsr = registerArrayRead16(PLLFSR);
   uint32_t prescaler1 = (value & 0x0080) ? 2 : 1;
   uint32_t p = pllfsr & 0x00FF;
   uint32_t q = (pllfsr & 0x0F00) >> 8;
   uint32_t newCrystalCycles = 2 * (14 * (p + 1) + q + 1) / prescaler1;
   uint32_t newFrequency = newCrystalCycles * CRYSTAL_FREQUENCY;
   printf("New CPU frequency of:%d cycles per second.\n", newFrequency);
   printf("New clk32 cycle count of :%d.\n", newCrystalCycles);
   
   palmCpuFrequency = newFrequency;
   
   if(value & 0x0008){
      //the pll is disabled, the cpu is off, end execution now
      m68k_end_timeslice();
   }
}

void rtcAddSecond(){
   if(registerArrayRead16(RTCCTL) & 0x0080){
      //rtc enable bit set
      uint32_t newRtcTime;
      uint32_t oldRtcTime = registerArrayRead32(RTCTIME);
      uint32_t hours = oldRtcTime >> 24;
      uint32_t minutes = (oldRtcTime >> 16) & 0x0000003F;
      uint32_t seconds = oldRtcTime & 0x0000003F;
      
      seconds++;
      if(seconds >= 60){
         minutes++;
         seconds = 0;
         if(minutes >= 60){
            hours++;
            minutes = 0;
            if(hours >= 24){
               hours = 0;
               uint16_t days = registerArrayRead16(DAYR);
               days++;
               registerArrayWrite16(DAYR, days & 0x01FF);
            }
         }
      }
      
      newRtcTime = seconds & 0x0000003F;
      newRtcTime |= minutes << 16;
      newRtcTime |= hours << 24;
      registerArrayWrite32(RTCTIME, newRtcTime);
   }
}

void toggleClk32(){
   //more will be added here for timers
   registerArrayWrite16(PLLFSR, registerArrayRead16(PLLFSR) ^ 0x8000);
}

bool cpuIsOn(){
   return registerArrayRead16(PLLCR) & 0x0008;
}


unsigned int getHwRegister8(unsigned int address){
   switch(address){
      //select between gpio or special function
      case PBSEL:
      case PCSEL:
      case PDSEL:
      case PESEL:
      case PFSEL:
      case PGSEL:
      case PJSEL:
      case PKSEL:
      case PMSEL:
         
      //pull up/down enable
      case PAPUEN:
      case PBPUEN:
      case PCPDEN:
      case PDPUEN:
      case PEPUEN:
      case PFPUEN:
      case PGPUEN:
      case PJPUEN:
      case PKPUEN:
      case PMPUEN:
         //simple read, no actions needed
         //PGPUEN, PGSEL PMSEL and PMPUEN lack the top 2 bits but that is handled on write
         //PDSEL lacks the bottom 4 bits but that is handled on write
         return registerArrayRead8(address);
         
      default:
         printUnknownHwAccess(address, 0, 8, false);
         return registerArrayRead8(address);
   }
}

unsigned int getHwRegister16(unsigned int address){
   switch(address){
         
      case PLLCR:
      case PLLFSR:
      case SDCTRL:
         //simple read, no actions needed
         return registerArrayRead16(address);
         
      default:
         printUnknownHwAccess(address, 0, 16, false);
         return registerArrayRead16(address);
   }
}

unsigned int getHwRegister32(unsigned int address){
   switch(address){
         
      case RTCTIME:
      case IDR:
         //simple read, no actions needed
         return registerArrayRead32(address);
         
      default:
         printUnknownHwAccess(address, 0, 32, false);
         return registerArrayRead32(address);
   }
}


void setHwRegister8(unsigned int address, unsigned int value){
   switch(address){
      case PDSEL:
         //write without the bottom 4 bits
         registerArrayWrite8(address, value & 0xF0);
         break;
         
      case PGSEL:
      case PMSEL:
      case PGPUEN:
      case PMPUEN:
         //write without the top 2 bits
         registerArrayWrite8(address, value & 0x3F);
         break;
         
      //select between gpio or special function
      case PBSEL:
      case PCSEL:
      case PESEL:
      case PFSEL:
      case PJSEL:
      case PKSEL:
      
      //pull up/down enable
      case PAPUEN:
      case PBPUEN:
      case PCPDEN:
      case PDPUEN:
      case PEPUEN:
      case PFPUEN:
      case PJPUEN:
      case PKPUEN:
         //simple write, no actions needed
         registerArrayWrite8(address, value);
         break;
         
      default:
         printUnknownHwAccess(address, value, 8, true);
         registerArrayWrite8(address, value);
         break;
   }
}

void setHwRegister16(unsigned int address, unsigned int value){
   switch(address){
         
      case PLLFSR:
         setPllfsr16(value);
         break;
         
      case SDCTRL:
         //missing bits 13, 9, 8 and 7
         registerArrayWrite16(address, value & 0xDC7F);
         break;
         
      default:
         printUnknownHwAccess(address, value, 16, true);
         registerArrayWrite16(address, value);
         break;
   }
}

void setHwRegister32(unsigned int address, unsigned int value){
   switch(address){
      case RTCTIME:
         registerArrayWrite32(address, value & 0x1F3F003F);
         break;
         
      case IDR:
         //invalid write, do nothing
         break;
         
      case LSSA:
         //simple write, no actions needed
         registerArrayWrite32(address, value);
         break;
      
      default:
         printUnknownHwAccess(address, value, 32, true);
         registerArrayWrite32(address, value);
         break;
   }
}


void resetHwRegisters(){
   memset(palmReg, 0x00, REG_SIZE);
   
   //system control
   registerArrayWrite8(SCR, 0x1C);
   
   //cpu id
   registerArrayWrite32(IDR, 0x56000000);
   
   //i/o drive control //probably unused
   registerArrayWrite16(IODCR, 0x1FFF);
   
   //chip selects
   registerArrayWrite16(CSA, 0x00B0);
   registerArrayWrite16(CSD, 0x0200);
   registerArrayWrite16(EMUCS, 0x0060);
   
   //phase lock loop
   registerArrayWrite16(PLLCR, 0x24B3);
   registerArrayWrite16(PLLFSR, 0x0347);
   
   //power control
   registerArrayWrite8(PCTLR, 0x1F);
   
   //cpu interrupts
   registerArrayWrite32(IMR, 0x00FFFFFF);
   registerArrayWrite16(ILCR, 0x6533);
   
   //gpio ports
   registerArrayWrite8(PADATA, 0xFF);
   registerArrayWrite8(PAPUEN, 0xFF);
   
   registerArrayWrite8(PBDATA, 0xFF);
   registerArrayWrite8(PBPUEN, 0xFF);
   registerArrayWrite8(PBSEL, 0xFF);
   
   registerArrayWrite8(PCPDEN, 0xFF);
   registerArrayWrite8(PCSEL, 0xFF);
   
   registerArrayWrite8(PDDATA, 0xFF);
   registerArrayWrite8(PDPUEN, 0xFF);
   registerArrayWrite8(PDSEL, 0xF0);
   
   registerArrayWrite8(PEDATA, 0xFF);
   registerArrayWrite8(PEPUEN, 0xFF);
   registerArrayWrite8(PESEL, 0xFF);
   
   registerArrayWrite8(PFDATA, 0xFF);
   registerArrayWrite8(PFPUEN, 0xFF);
   registerArrayWrite8(PFSEL, 0x87);
   
   registerArrayWrite8(PGDATA, 0x3F);
   registerArrayWrite8(PGPUEN, 0x3D);
   registerArrayWrite8(PGSEL, 0x08);
   
   registerArrayWrite8(PJDATA, 0xFF);
   registerArrayWrite8(PJPUEN, 0xFF);
   registerArrayWrite8(PJSEL, 0xEF);
   
   registerArrayWrite8(PKDATA, 0x0F);
   registerArrayWrite8(PKPUEN, 0xFF);
   registerArrayWrite8(PKSEL, 0xFF);
   
   registerArrayWrite8(PMDATA, 0x20);
   registerArrayWrite8(PMPUEN, 0x3F);
   registerArrayWrite8(PMSEL, 0x3F);
   
   //pulse width modulation control
   registerArrayWrite16(PWMC1, 0x0020);
   registerArrayWrite8(PWMP1, 0xFE);
   
   //timers
   registerArrayWrite16(TCMP1, 0xFFFF);
   registerArrayWrite16(TCMP2, 0xFFFF);
   
   //serial i/o
   registerArrayWrite16(UBAUD1, 0x003F);
   registerArrayWrite16(UBAUD2, 0x003F);
   registerArrayWrite16(HMARK, 0x0102);
   
   //lcd control registers, unused since the sed1376 is present
   registerArrayWrite8(LVPW, 0xFF);
   registerArrayWrite16(LXMAX, 0x03F0);
   registerArrayWrite16(LYMAX, 0x01FF);
   registerArrayWrite16(LCWCH, 0x0101);
   registerArrayWrite8(LBLKC, 0x7F);
   registerArrayWrite8(LRRA, 0xFF);
   registerArrayWrite8(LGPMR, 0x84);
   registerArrayWrite8(DMACR, 0x62);
   
   //realtime clock
   //RTCTIME is not changed on reset
   registerArrayWrite16(WATCHDOG, 0x0001);
   registerArrayWrite16(RTCCTL, 0x0080);//conflicting size in datasheet, it says its 8 bit but provides 16 bit values
   registerArrayWrite16(STPWCH, 0x003F);//conflicting size in datasheet, it says its 8 bit but provides 16 bit values
   //DAYR is not changed on reset
   
   //sdram control, unused since ram refresh is unemulated
   registerArrayWrite16(SDCTRL, 0x003C);
}

void setRtc(uint32_t days, uint32_t hours, uint32_t minutes, uint32_t seconds){
   uint32_t rtcTime;
   rtcTime = seconds & 0x0000003F;
   rtcTime |= (minutes << 16) & 0x003F0000;
   rtcTime |= (hours << 24) & 0x1F000000;
   registerArrayWrite32(RTCTIME, rtcTime);
   registerArrayWrite16(DAYR, days & 0x01FF);
}