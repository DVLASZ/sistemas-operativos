#include <string.h>

#include "mbr.h"

const char * mbr_partition_types[256] = {
	"Empty", //00
	"FAT12", //01
	"XENIX root", //02
	"XENIX usr", //03
	"FAT16 (less than 65.536 sectors)", //04
	"Extended partition", //05
	"FAT16B (65.535 or more sectors)", //06
	"IFS/HPTS/NTFS/exFAT", //07
	"QNX/AIX boot/Multidrive", //08
	"AIX data | QNX | Coherent FS/OS-9 RBF", //09
	"OS/2 Boot Manager/Coherent swap", //0A
	"FAT32 CHS", //0B
	"FAT32 LBA", //0C
	"Unused 0x0D", // 0D
	"FAT16B LBA", //0E
	"Extended partition - LBA", //0F
	"Unused 0x10", //10
	"Logical sectored FAT16 or FAT12/Hidden FAT12", //11
	"Config | Recovery | Hibernation | Diagnostics | Service", //12
	"Unused 0x13", //13
	"Logical Sectored FAT12 | FAT16 | Hidden FAT16 | Omega", // 14
	"Hidden extended CHS | SWAP", //15
	"Hidden FAT16B", //16
	"Hidden IFS | HPFS | NTFS | exFAT", //17
	"AST Zero Volt", //18
	"Willowtech Photon coS", // 19
	"Unused 0x1A", //1A
	"Hidden FAT32", //1B
	"Hidden FAT32 with LBA | ASUS Recovery", //1C
	"Unused 0x1D", //1D
	"Hidden FAT16 with LBA", //1E
	"Hidden extended with LBA", // 1F
	"Windows Mobile Update | Willowsoft OFS1", //20
	"HP Volume Expansion | FSo2", //21
	"Oxygen Extended Partition Table", //22
	"Windows Mobile Boot XIP", //23
	"Logical sectored FAT12/FAT16", //24
	"Unused 0x25", //25
	"Unused 0x26", //26
	"WRE | Rescue | RouterBoot", //27
	"Unused 0x28", //28
	"Unused 0x29", //29
	"AtheOS file system | Reserved", //2A
	"SyllableSecure", //2B
	"Unused 0x2C", //2C
	"Unused 0x2D", //2D
	"Unused 0x2E", //2E
	"Unused 0x2F", //2F
	"Personal CP/M-86", //30
	"Microsoft/IBM Reserved", //31
	"Unused 0x32", //32
	"Microsoft/IBM Reserved", //33
	"Microsoft/IBM Reserved", //34
	"JFS (OS/2)", //35
	"Microsoft/IBM Reserved", //36
	"Unused 0x37", //37
	"THEOS 3.2", //38
	"Plan 9/THEOS v.4 spanned", //39
	"THEOS v.4 spanned", //3A
	"THEOS v.4 extended", //3B
	"PqRP", //3C
	"Hidden Netware", //3D
	"Unused 0x3E", //3E
	"Unused 0x3F", //3F
	"PICK R83/Venix 80286", //40
	"Personal RISC Boot | Linux | Minix | PPC PReP", //41
	"SFS/Old Linux swap | Dynamic extended - Microsoft", //42
	"Old Linux Native", //43
	"GoBack Norton | WildFire | Adaptec | Roxio", //44
	"Priam | Boot-US | EUMEL | ELAN", //45
	"EUMEL | ELAN", //46
	"EUMEL | ELAN", //47
	"EUMEL | ELAN", //48
	"Unused 0x49", //49
	"Aquila | ALFS | THIN", //4A
	"Unused 0x4B", //4B
	"AoS", //4C
	"Primary QNX POSIX volume", //4D
	"Secondary QNX POSIX volume", //4E
	"Tertiary QNX | ETH Oberon boot", //4F
	"ETH Oberon alternative/Lynx RTOS", //50
	"Novell | Disk Manager 504", //51
	"CP/M-80", //52
	"Disk Manager Auxiliary 3", //53
	"Dynamic Drive Overlay", //54
	"EZ-Drive | Maxtor/MaxBlast", //55
	"Logical sectored FAT12 | FAT16", //56
	"VNDI Partition", //57
	"Unused 0x58", //58
	"yocFS", //59
	"Unused 0x5A", //5A
	"Unused 0x5B", //5B
	"Priam EDisk", //5C
	"Unused 0x5D", //5D
	"Unused 0x5E", //5E
	"Unused 0x5F", //5F
	"Unused 0x60", //60
	"Hidden FAT12", //61
	"Unused 0x62", //62
	"Old GNU | Hurd with UFS | Hidden read-only FAT12", //63
	"Hidden FAT16 | NetWare File System 286 | PC-ARMOUR", //64
	"NetWare File System 386", //65
	"Storage Management Services (SMS) | Hidden read-only FAT16", //66
	"Wolf Mountain cluster", //67
	"Netware 0x68", //68
	"Novell Storage Services (SNS)", //69
	"Unused 0x6A", //6A
	"Unused 0x6B", //6B
	"BSD Slice", //6C
	"Unused 0x6D", //6D
	"Unused 0x6E", //6E
	"Unused 0x6F", //6F
	"DiskSecure Multiboot", //70
	"Microsoft/IBM Reserved", //71
	"APTI Alternative FAT32 (CHS,SFN)/V7/x86", //72
	"Microsoft/IBM Reserved", //73
	"Hidden FAT16B", //74
	"IBM PC/IX", //75
	"Hidden read-only FAT16B", //76
	"VNDI/M2FS/M2CS", //77
	"XOSL bootloader filesystem", //78
	"APTI Alternative FAT16 (CHS,SFN)", //79
	"APTI Alternative FAT16 (LBA,SFN)", //7A
	"APTI Alternative FAT16B (LBA,SFN)", //7B
	"APTI Alternative FAT32 (LBA,SFN)", //7C
	"APTI Alternative FAT32 (CHS,SFN)", //7D
	"Level 2 cache", //7E
	"Reserved", //7F
	"MINIX file system (old)", //80
	"MINIX file system", //81
	"Linux SWAP space | GNU | HURD/Solaris x86", //82
	"Linux | GNU/Hurd", //83
	"APM Hibernation | Hidden C: (FAT16) | Rapid Start hibernation data", //84
	"Linux extended", //85
	"Fault-tolerant FAT16B | Linux RAID superblock", //86
	"Fault-tolerant HPFS | NTFS mirrored volume set", //87
	"Linux plaintext partition table", //88
	"Unused 0x89", //89
	"AirBoot", //8A
	"Legacy fault-tolerant FAT32 mirrored volume set", //8B
	"Legacy fault-tolerant FAT32 mirrored volume set", //8C
	"Hidden FAT12", //8D
	"Linux LVM", //8E
	"Unused 0x8F", //8F
	"Hidden FAT16", //90
	"Hidden extended partition with CHS addressing", //91
	"Hidden FAT16B", //92
	"Amoeba native file system/Hidden linux file system", //93
	"Amoeba bad block table", //94
	"EXOPC native", //95
	"ISO-9660 file system", //96
	"Hidden FAT12", //97
	"Hidden FAT32/Service partition (bootable FAT)", //98
	"Early Unix", //99
	"Hidden FAT16", //9A
	"Hidden extended partition with LBA", //9B
	"Unused 0x9C", //9C
	"Unused 0x9D", //9D
	"ForthOS", //9E
	"BSD/OS 3.0+", //9F
	"Diagnostics partition/Hibernate partition", //A0
	"Hibernate partition", //A1
	"Hard Processor System (HPS) ARM preloader", //A2
	"Unused 0xA3", //A3
	"Unused 0xA4", //A4
	"BSD slice", //A5
	"OpenBSD slice", //A6
	"NextSTEP", //A7
	"Apple Darwin/Mac OS X UFS", //A8
	"NetBSD slice", //A9
	"Olivetti MS-DOS FAT12", //AA
	"Apple Darwin | GO!", //AB
	"Apple RAID, Mac OS X boot", //AC
	"ADFS | Filecore format", //AD
	"ShagOS Filesystem", //AE
	"HFS | HFS+", //AF
	"Boot-Star dummy partition", //B0
	"QNX Neutrino power-safe", //B1
	"QNX Neutrino power-safe filesystem", //B2
	"QNX Neutrino power-safe filesystem", //B3
	"HP Volume Expansion", //B4
	"HP Volume Expansion", //B5
	"Corrupted fault-tolerant FAT16B", //B6
	"BSDI native file system/Corrupted fault-tolerant HPFS/NTFS", //B7
	"BSDI swap", //B8
	"Unused 0xB9", //B9
	"Unused 0xBA", //BA
	"PTS BootWizard 4/OS Selector 5", //BB
	"Corrupted fault-tolerant FAT32 mirrored volume", //BC
	"BonnyDOS/286", //BD
	"Solaris 8 boot", //BE
	"Solaris x86", //BF
	"Secured FAT partition", //C0
	"Secured FAT12 partition", //C1
	"Hidden Linux native file system", //C2
	"Hidden Linux swap", //C3
	"Secured FAT16", //C4
	"Secured extended partition with CHS", //C5
	"Secured FAT16B", //C6
	"Syrinx boot", //C7
	"Reserved for DR-DOS", //C8
	"Reserved for DR-DOS", //C9
	"Reserved for DR-DOS", //CA
	"Secured FAT32 | Corrupted fault-tolerant FAT32", //CB
	"Secured FAT32 | Corrupted fault-tolerant FAT32", //CC
	"Memory dump/openSUSE ISOHybrid", //CD
	"Secured FAT16B", //CE
	"Secured extended partition with LBA", //CF
	"Secured FAT32 partition", //D0
	"Secured FAT12", //D1
	"Unused 0xD2", //D2
	"Unused 0xD3", //D3
	"Secured FAT16", //D4
	"Secured extended partition with CHS addressing", //D5
	"Secured FAT16B", //D6
	"Unused 0xD7", //D7
	"CP/M-86", //D8
	"Unused 0xD9", //D9
	"Non-filesystem data", //DA
	"CP/M-86 | FAT32 restore partition (DSR)", //DB
	"Unused 0xDC", //DC
	"Hidden memory dump", //DD
	"FAT16 utility | diagnostic partition", //DE
	"DG/UX virtual disk manager", //DF
	"ST AVFS", //E0
	"FAT12", //E1
	"Unused 0xE2", //E2
	"Read-only FAT12", //E3
	"FAT16", //E4
	"Logical sectored FAT12 or FAT16", //E5
	"Read-only FAT16", //E6
	"Unused 0xE7", //E7
	"Linux Unified Key Setup", //E8
	"Unused 0xE9", //E9
	"Unused 0xEA", //EA
	"BFS", //EB
	"SkyFS", //EC
	"EDC Loader", //ED
	"GPT Protective MBR", //EE
	"EFI system partition", //EF
	"PA-RISC Linux boot loader", //F0
	"Unused 0xF1", //F1
	"Logical sectored FAT12/FAT16", //F2
	"Unused 0xF3", //F3
	"FAT16B | Single volume partition for NGF", //F4
	"MD0-MD9 multi volume partition", //F5
	"Read-only FAT16B", //F6
	"EFAT | Solid State file system", //F7
	"Protective partition for the area containing system firmware", //F8
	"pCache ext2 | ext3 persistent cache", //F9
	"Unused 0xFA", //FA
	"VMware VMFS file system partition", //FB
	"VMware swap | VMKCORE kernel", //FC
	"Linux RAID superblock", //FD
	"PS/2 IML partition | PS/2 recovery partition | Old Linux LVM", //FE
	"XENIX bad block table", //FF
};

int is_mbr(mbr * boot_record) {
	/* TODO verificar si el bootsector leido es valido. */
	return 0;
}


void mbr_partition_type(unsigned char type, char buf[TYPE_NAME_LEN]) {
	/* TODO rellenar el en buffer el nombre textual del tipo de particion */
}
