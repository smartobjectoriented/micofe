/*
 * Copyright (C) 2014-2025 Daniel Rossier <daniel.rossier@heig-vd.ch>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#ifndef UAPI_SOO_H
#define UAPI_SOO_H

#include <stdint.h>

#define MAX_S3C_DOMAINS	5

/*
 * ME states:
 * - S3C_state_stopped:		Capsule is stopped (right after start or later)
 * - S3C_state_living:		ME is full-functional and activated (all frontend devices are consistent)
 * - S3C_state_suspended:	ME is suspended before migrating. This state is maintained for the resident ME instance
 * - S3C_state_hibernate:	ME is in a state of hibernate snapshot
 * - S3C_state_resuming:         ME ready to perform resuming (after recovering)
 * - S3C_state_awakened:         ME is just being awakened
 * - S3C_state_terminated:	ME has been terminated (by a shutdown)
 * - S3C_state_dead:		ME does not exist
 */
typedef enum {
	S3C_state_stopped,
	S3C_state_living,
	S3C_state_suspended,
	S3C_state_hibernate,
	S3C_state_resuming,
	S3C_state_awakened,
	S3C_state_killed,
	S3C_state_terminated,
	S3C_state_dead
} S3C_state_t;

/* Keep information about slot availability
 * FREE:	the slot is available (no ME)
 * BUSY:	the slot is allocated a ME
 */
typedef enum { S3C_SLOT_FREE, S3C_SLOT_BUSY } S3C_slotState_t;

/* ME ID related information */
#define S3C_NAME_SIZE 40
#define S3C_SHORTDESC_SIZE 1024

/*
 * Definition of ME ID information used by functions which need
 * to get a list of running MEs with their information.
 */
typedef struct {
	uint32_t slotID;
	S3C_state_t state;

	uint64_t spid;

	char name[S3C_NAME_SIZE];
	char shortdesc[S3C_SHORTDESC_SIZE];
} S3C_id_t;

/*
 * IOCTL commands for migration.
 * This part is shared between the kernel and user spaces.
 */

/*
 * IOCTL codes
 */

#define AGENCY_IOCTL_READ_SNAPSHOT		_IOWR('S', 1, agency_ioctl_args_t)
#define AGENCY_IOCTL_WRITE_SNAPSHOT		_IOW('S', 2, agency_ioctl_args_t)
#define AGENCY_IOCTL_SHUTDOWN   		_IOW('S', 3, agency_ioctl_args_t)
#define AGENCY_IOCTL_INJECT_CAPSULE     	_IOWR('S', 4, agency_ioctl_args_t)
#define AGENCY_IOCTL_START_CAPSULE              _IOWR('S', 5, agency_ioctl_args_t)
#define AGENCY_IOCTL_GET_S3C_ID			_IOWR('S', 6, agency_ioctl_args_t)
#define AGENCY_IOCTL_GET_S3C_ID_ARRAY		_IOR('S', 7, agency_ioctl_args_t)

/* struct agency_ioctl_args used in IOCTLs */
typedef struct agency_ioctl_args {
	void	*buffer; /* IN/OUT */
	int	slotID;
	unsigned capsuleID;
	long	value;   /* IN/OUT */
} agency_ioctl_args_t;

#endif /* UAPI_SOO_H */
