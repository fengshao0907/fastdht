/**
* Copyright (C) 2008 Happy Fish / YuQing
*
* FastDFS may be copied only under the terms of the GNU General
* Public License V3, which may be found in the FastDFS source kit.
* Please visit the FastDFS Home Page http://www.csource.org/ for more detail.
**/

//fdht_types.h

#ifndef _FDHT_TYPES_H
#define _FDHT_TYPES_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "fdht_define.h"

#ifdef __cplusplus
extern "C" {
#endif

#define FDHT_MAX_NAMESPACE_LEN	 64
#define FDHT_MAX_OBJECT_ID_LEN	128
#define FDHT_MAX_SUB_KEY_LEN	128
#define FDHT_FULL_KEY_SEPERATOR	'\x1'

#define FDHT_EXPIRES_NEVER	 0  //never timeout
#define FDHT_EXPIRES_NONE	-1  //invalid timeout, should ignore

#define FDHT_MAX_FULL_KEY_LEN    (FDHT_MAX_NAMESPACE_LEN + 1 + \
			FDHT_MAX_OBJECT_ID_LEN + 1 + FDHT_MAX_SUB_KEY_LEN)

#define FDHT_PACK_FULL_KEY(namespace_len, pNameSpace, obj_id_len, pObjectId, \
			key_len, key, full_key, full_key_len, p) \
	p = full_key; \
	if (namespace_len > 0) \
	{ \
		memcpy(p, pNameSpace, namespace_len); \
		p += namespace_len; \
	} \
	*p++ = FDHT_FULL_KEY_SEPERATOR; /*field seperator*/  \
	if (obj_id_len > 0) \
	{ \
		memcpy(p, pObjectId, obj_id_len); \
		p += obj_id_len; \
	} \
	*p++ = FDHT_FULL_KEY_SEPERATOR; /*field seperator*/  \
	memcpy(p, key, key_len); \
	p += key_len; \
	full_key_len = p - full_key;
	

typedef struct
{
	int sock;
	int port;
	char ip_addr[IP_ADDRESS_SIZE];
} FDHTServerInfo;

typedef struct
{
	char ip_addr[IP_ADDRESS_SIZE];
	bool sync_old_done;
	int port;
	int sync_req_count;    //sync req count
	int64_t update_count;  //runtime var
} FDHTGroupServer;

typedef struct {
	uint64_t total_set_count;
	uint64_t success_set_count;
	uint64_t total_inc_count;
	uint64_t success_inc_count;
	uint64_t total_delete_count;
	uint64_t success_delete_count;
	uint64_t total_get_count;
	uint64_t success_get_count;
} FDHTServerStat;

typedef struct
{
	FDHTServerInfo *servers;
	int count;  //server count
} ServerArray;

typedef struct
{
	ServerArray *groups;
	int count;  //group count
} GroupArray;

#ifdef __cplusplus
}
#endif

#endif
