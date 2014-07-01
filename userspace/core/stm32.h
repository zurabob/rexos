/*
    RExOS - embedded RTOS
    Copyright (c) 2011-2014, Alexey Kramarenko
    All rights reserved.
*/

#ifndef STM32_H
#define STM32_H

//---------------------------------------------------------------------------- STM32 F1 ----------------------------------------------------------------------------------------------------------
#if defined(STM32F100C4) || defined(STM32F100C6) || defined(STM32F100C8) || defined(STM32F100CB) \
 || defined(STM32F100R4) || defined(STM32F100R6) || defined(STM32F100R8) || defined(STM32F100RB) || defined(STM32F100RC) || defined(STM32F100RD) || defined(STM32F100RE) \
 || defined(STM32F100V8) || defined(STM32F100VB) || defined(STM32F100VC) || defined(STM32F100VD) || defined(STM32F100VE) \
 || defined(STM32F100ZC) || defined(STM32F100ZD) || defined(STM32F100ZE)
#define STM32F100
#endif

#if defined(STM32F101C4) || defined(STM32F101C6) || defined(STM32F101C8) || defined(STM32F101CB) \
 || defined(STM32F101R4) || defined(STM32F101R6) || defined(STM32F101R8) || defined(STM32F101RB) || defined(STM32F101RC) || defined(STM32F101RD) || defined(STM32F101RE) \
 || defined(STM32F101RF) || defined(STM32F101RG) \
 || defined(STM32F101T4) || defined(STM32F101T6) || defined(STM32F101T8) || defined(STM32F101TB) \
 || defined(STM32F101V8) || defined(STM32F101VB) || defined(STM32F101VC) || defined(STM32F101VD) || defined(STM32F101VE) || defined(STM32F101VF) || defined(STM32F101VG)  \
 || defined(STM32F101ZC) || defined(STM32F101ZD) || defined(STM32F101ZE) || defined(STM32F101ZG)
#define STM32F101
#endif

#if defined(STM32F102C4) || defined(STM32F102C6) || defined(STM32F102C8) || defined(STM32F102CB) \
 || defined(STM32F102R4) || defined(STM32F102R6) || defined(STM32F102R8) || defined(STM32F102RB)
#define STM32F102
#endif

#if defined(STM32F103C4) || defined(STM32F103C6) || defined(STM32F103C8) || defined(STM32F103CB) \
 || defined(STM32F103R4) || defined(STM32F103R6) || defined(STM32F103R8) || defined(STM32F103RB) || defined(STM32F103RC) || defined(STM32F103RD) || defined(STM32F103RE) \
 || defined(STM32F103RF) || defined(STM32F103RG) \
 || defined(STM32F103T4) || defined(STM32F103T6) || defined(STM32F103T8) || defined(STM32F103TB) \
 || defined(STM32F103V8) || defined(STM32F103VB) || defined(STM32F103VC) || defined(STM32F103VD) || defined(STM32F103VE) || defined(STM32F103VF) || defined(STM32F103VG)  \
 || defined(STM32F103ZC) || defined(STM32F103ZD) || defined(STM32F103ZE) || defined(STM32F103ZF) || defined(STM32F103ZG)
#define STM32F103
#endif

#if defined(STM32F105R8) || defined(STM32F105RB) || defined(STM32F105RC) \
 || defined(STM32F105V8) || defined(STM32F105VB) || defined(STM32F105VC)
#define STM32F105
#endif

#if defined(STM32F107RB) || defined(STM32F107RC) \
 || defined(STM32F107VB) || defined(STM32F107VC)
#define STM32F107
#endif

#if defined(STM32F100C4) || defined(STM32F100R4)
#define FLASH_SIZE        0x4000
#define STM32F10X_LD_VL
#endif //16k LD_VL

#if defined(STM32F101C4) || defined(STM32F101R4) || defined(STM32F101T4) || defined(STM32F102C4) || defined(STM32F102R4) || defined(STM32F103C4) || defined(STM32F103R4) || defined(STM32F103T4)
#define FLASH_SIZE        0x4000
#define STM32F10X_LD
#endif //16k LD

#if defined(STM32F100C6) || defined(STM32F100R6)
#define FLASH_SIZE        0x8000
#define STM32F10X_LD_VL
#endif //32k LD_VL

#if defined(STM32F101C6) || defined(STM32F101R6) || defined(STM32F101T6) || defined(STM32F102C6) || defined(STM32F102R6) || defined(STM32F103C6) || defined(STM32F103R6) || defined(STM32F103T6)
#define FLASH_SIZE        0x8000
#define STM32F10X_LD
#endif //32k LD

#if defined(STM32F100C8) || defined(STM32F100R8) || defined(STM32F100V8)
#define FLASH_SIZE        0x10000
#define STM32F10X_MD_VL
#endif //64k MD_VL

#if defined(STM32F101C8) || defined(STM32F101R8) || defined(STM32F101T8) || defined(STM32F101V8) || defined(STM32F102C8) || defined(STM32F102R8) || defined(STM32F103C8) || defined(STM32F103R8) \
 || defined(STM32F103T8) || defined(STM32F103V8)
#define FLASH_SIZE        0x10000
#define STM32F10X_MD
#endif //64k MD

#if defined(STM32F105R8) || defined(STM32F105V8)
#define FLASH_SIZE        0x10000
#define STM32F10X_CL
#endif //64k CL

#if defined(STM32F100CB) || defined(STM32F100RB) || defined(STM32F100VB)
#define FLASH_SIZE        0x20000
#define STM32F10X_MD_VL
#endif //128k MD_VL

#if defined(STM32F101CB) || defined(STM32F101RB) || defined(STM32F101TB) || defined(STM32F101VB) || defined(STM32F102CB) || defined(STM32F102RB) || defined(STM32F103CB) || defined(STM32F103RB) \
 || defined(STM32F103TB) || defined(STM32F103VB)
#define FLASH_SIZE        0x20000
#define STM32F10X_MD
#endif //128k MD

#if defined(STM32F105RB) || defined(STM32F105VB) || defined(STM32F107RB) || defined(STM32F107VB)
#define FLASH_SIZE        0x20000
#define STM32F10X_CL
#endif //128k CL

#if defined(STM32F100RC) || defined(STM32F100VC) || defined(STM32F100ZC)
#define FLASH_SIZE        0x40000
#define STM32F10X_HD_VL
#endif //256k HD_VL

#if defined(STM32F101RC) || defined(STM32F101VC) || defined(STM32F101ZC) || defined(STM32F103RC) || defined(STM32F103VC) || defined(STM32F103ZC)
#define FLASH_SIZE        0x40000
#define STM32F10X_HD
#endif //256k HD

#if defined(STM32F105RC) || defined(STM32F105VC) || defined(STM32F107RC) || defined(STM32F107VC)
#define FLASH_SIZE        0x40000
#define STM32F10X_CL
#endif //256k CL

#if defined(STM32F100RD) || defined(STM32F100VD) || defined(STM32F100ZD)
#define FLASH_SIZE        0x60000
#define STM32F10X_HD_VL
#endif //384k HD_VL

#if defined(STM32F101RD) || defined(STM32F101VD) || defined(STM32F101ZD) || defined(STM32F103RD) || defined(STM32F103VD) || defined(STM32F103ZD)
#define FLASH_SIZE        0x60000
#define STM32F10X_HD
#endif //384k HD

#if defined(STM32F100RE) || defined(STM32F100VE) || defined(STM32F100ZE)
#define FLASH_SIZE        0x80000
#define STM32F10X_HD_VL
#endif //512k HD_VL

#if defined(STM32F101RE) || defined(STM32F101VE) || defined(STM32F101ZE) || defined(STM32F103RE) || defined(STM32F103VE) || defined(STM32F103ZE)
#define FLASH_SIZE        0x80000
#define STM32F10X_HD
#endif //512k HD

#if defined(STM32F101RF) || defined(STM32F101VF) || defined(STM32F103RF) || defined(STM32F103VF) || defined(STM32F103ZF)
#define FLASH_SIZE        0xC0000
#define STM32F10X_XL
#endif //768k XL

#if defined(STM32F101RG) || defined(STM32F101VG) || defined(STM32F101ZG) || defined(STM32F103RG) || defined(STM32F103VG) || defined(STM32F103ZG)
#define FLASH_SIZE        0x100000
#define STM32F10X_XL
#endif //1m XL

#if defined(STM32F100)
#if defined(STM32F10X_LD_VL)
#define SRAM_SIZE        0x1000
#elif defined(STM32F10X_MD_VL)
#define SRAM_SIZE        0x2000
#elif (FLASH_SIZE == 0x40000)
#define SRAM_SIZE        0x6000
#elif defined(STM32F10X_HD_VL)
#define SRAM_SIZE        0x8000
#endif //STM32F100

#elif defined(STM32F101) || defined(STM32F102)
#if (FLASH_SIZE == 0x4000)
#define SRAM_SIZE        0x1000
#elif (FLASH_SIZE == 0x8000)
#define SRAM_SIZE        0x1800
#elif (FLASH_SIZE == 0x10000)
#define SRAM_SIZE        0x2800
#elif (FLASH_SIZE == 0x20000)
#define SRAM_SIZE        0x4000
#elif (FLASH_SIZE == 0x40000)
#define SRAM_SIZE        0x8000
#elif defined(STM32F10X_HD)
#define SRAM_SIZE        0xC000
#elif defined(STM32F10X_XL)
#define SRAM_SIZE        0x14000
#endif //STM32F101 || STM32F102

#elif defined(STM32F103)
#if (FLASH_SIZE == 0x4000)
#define SRAM_SIZE        0x1800
#elif (FLASH_SIZE == 0x8000)
#define SRAM_SIZE        0x2800
#elif defined(STM32F10X_MD)
#define SRAM_SIZE        0x5000
#elif (FLASH_SIZE == 0x40000)
#define SRAM_SIZE        0xC000
#elif defined(STM32F10X_HD)
#define SRAM_SIZE        0x10000
#elif defined(STM32F10X_XL)
#define SRAM_SIZE        0x18000
#endif //STM32F103

#elif defined(STM32F105) || defined(STM32F107)
#define SRAM_SIZE        0x10000

#endif  //SRAM_SIZE

#if defined(STM32F100) || defined(STM32F101) || defined(STM32F102) || defined(STM32F103) || defined(STM32F105) || defined(STM32F107)
#define STM32F1
#endif

//---------------------------------------------------------------------------- stm 32 F2 ----------------------------------------------------------------------------------------------------------
#if defined(STM32F205RB) || defined(STM32F205RC) || defined(STM32F205RE) || defined(STM32F205RF) || defined(STM32F205RG) || defined(STM32F205VG) || defined(STM32F205ZG) || defined(STM32F205VF) \
 || defined(STM32F205ZF) || defined(STM32F205VE) || defined(STM32F205ZE) || defined(STM32F205VC) || defined(STM32F205ZC) || defined(STM32F205VB)

#define STM32F205
#endif

#if defined(STM32F207VC) || defined(STM32F207VE) || defined(STM32F207VF) || defined(STM32F207VG) || defined(STM32F207ZC) || defined(STM32F207ZE) || defined(STM32F207ZF) || defined(STM32F207ZG) \
    || defined(STM32F207IC) || defined(STM32F207IE) || defined(STM32F207IF) || defined(STM32F207IG)
#define STM32F207
#endif

#if defined(STM32F215RE) || defined(STM32F215RG) || defined(STM32F215VG) || defined(STM32F215ZG) || defined(STM32F215VE) || defined(STM32F215ZE)
#define STM32F215
#endif

#if defined(STM32F217VE) || defined(STM32F217VG) || defined(STM32F217ZE) || defined(STM32F217ZG) || defined(STM32F217IE) || defined(STM32F217IG)
#define STM32F217
#endif

#if defined(STM32F205) || defined(STM32F207) || defined(STM32F215) || defined(STM32F217)
#define STM32F2

#if defined(STM32F205RB) || defined(STM32F205VB)
#define FLASH_SIZE        0x20000
#endif //128K

#if defined(STM32F205RC) || defined(STM32F205VC) || defined(STM32F205ZC) || defined(STM32F207VC) || defined(STM32F207ZC) || defined(STM32F207IC)
#define FLASH_SIZE        0x40000
#endif //256K

#if defined(STM32F205RE) || defined(STM32F205VE) || defined(STM32F205ZE) || defined(STM32F215RE) || defined(STM32F215VE) || defined(STM32F215ZE) || defined(STM32F207VE) || defined(STM32F207ZE) \
    || defined(STM32F207IE)  || defined(STM32F217VE) || defined(STM32F217ZE) || defined(STM32F207IE)
#define FLASH_SIZE        0x80000
#endif //512K

#if defined(STM32F205RF) || defined(STM32F205VF) || defined(STM32F205ZF) || defined(STM32F207VF) || defined(STM32F207ZF) || defined(STM32F207IF)
#define FLASH_SIZE        0xc0000
#endif //768K

#if defined(STM32F205RG) || defined(STM32F205VG) || defined(STM32F205ZG) || defined(STM32F215RG) || defined(STM32F215VG) || defined(STM32F215ZG) || defined(STM32F207VG) || defined(STM32F207ZG) \
    || defined(STM32F207IG)  || defined(STM32F217VG) || defined(STM32F217ZG) || defined(STM32F207IG)
#define FLASH_SIZE        0x100000
#endif //1M

#if defined(STM32F215) || defined(STM32F207) || defined(STM32F217) || (FLASH_SIZE > 0x40000)
#define SRAM_SIZE        0x20000
#elif (FLASH_SIZE==0x20000)
#define SRAM_SIZE        0x10000
#elif (FLASH_SIZE==0x40000)
#define SRAM_SIZE        0x18000
#endif
#endif //defined(STM32F205) || defined(STM32F207) || defined(STM32F215) || defined(STM32F217)

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
#if defined(STM32F1) || defined(STM32F2) || defined(STM32F4)
#define STM32
#ifndef FLASH_BASE
#define FLASH_BASE                0x08000000
#endif
#ifndef CORTEX_M3
#define CORTEX_M3
#endif
#endif //STM32F1 || STM32F2 || STM32F4

#endif //STM32_H