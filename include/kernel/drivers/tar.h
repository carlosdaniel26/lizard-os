#ifndef TAR_H
#define TAR_H

#include <stdint.h>

typedef struct TAR
{
	char file_name[100];			/**   0: 	File name (100 bytes) */
	uint64_t file_mode;			 /** 100: 	File mode (8 bytes) */
	uint64_t owner_id;			  /** 108: 	Owner's numeric user ID (8 bytes) */
	uint64_t group_id;			  /** 116: 	Group's numeric user ID (8 bytes) */
	uint8_t file_size[12];		  /** 124: 	File size in bytes (octal base, 12 bytes) */
	uint8_t last_change[12];		/** 136: 	Last modification time (octal base, 12 bytes) */
	uint64_t checksum_header_record;/** 148: 	Checksum for header record (8 bytes) */
	uint8_t type_flag;			  /** 156: 	Type flag (1 byte) */
	char linked_file_name[100];	 /** 157: 	Name of linked file (100 bytes) */
	char ustar_indicator[6];		/** 257: 	UStar indicator "ustar" then NUL (6 bytes) */
	char ustar_version[2];		  /** 263: 	UStar version "00" (2 bytes) */
	char owner_user_name[32];	   /** 265: 	Owner user name (32 bytes) */
	char owner_group_name[32];	  /** 297: 	Owner group name (32 bytes) */
	uint64_t device_major_number;   /** 329: 	Device major number (8 bytes) */
	uint64_t device_minor_number;   /** 337: 	Device minor number (8 bytes) */
	char filename_prefix[155];	  /** 345:	Filename prefix (155 bytes) */
} TAR;

#endif
