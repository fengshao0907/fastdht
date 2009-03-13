/**
* Copyright (C) 2008 Happy Fish / YuQing
*
* FastDFS may be copied only under the terms of the GNU General
* Public License V3, which may be found in the FastDFS source kit.
* Please visit the FastDFS Home Page http://www.csource.org/ for more detail.
**/

#include "hash.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

static unsigned int prime_array[] = {
    1,              /* 0 */
    3,              /* 1 */
    17,             /* 2 */
    37,             /* 3 */
    79,             /* 4 */
    163,            /* 5 */
    331,            /* 6 */
    673,            /* 7 */
    1361,           /* 8 */
    2729,           /* 9 */
    5471,           /* 10 */
    10949,          /* 11 */
    21911,          /* 12 */
    43853,          /* 13 */
    87719,          /* 14 */
    175447,         /* 15 */
    350899,         /* 16 */
    701819,         /* 17 */
    1403641,        /* 18 */
    2807303,        /* 19 */
    5614657,        /* 20 */
    11229331,       /* 21 */
    22458671,       /* 22 */
    44917381,       /* 23 */
    89834777,       /* 24 */
    179669557,      /* 25 */
    359339171,      /* 26 */
    718678369,      /* 27 */
    1437356741,     /* 28 */
    2147483647      /* 29 (largest signed int prime) */
};

#define PRIME_ARRAY_SIZE  30

int _hash_alloc_buckets(HashArray *pHash)
{
	ChainList *plist;
	ChainList *list_end;

	pHash->items=(ChainList *)malloc(sizeof(ChainList)*(*pHash->capacity));
	if (pHash->items == NULL)
	{
		return ENOMEM;
	}

	list_end = pHash->items + (*pHash->capacity);
	for (plist=pHash->items; plist!=list_end; plist++)
	{
		chain_init(plist, CHAIN_TYPE_APPEND, free, NULL);
	}

	return 0;
}

int hash_init(HashArray *pHash, HashFunc hash_func, \
		const unsigned int capacity, const double load_factor)
{
	unsigned int *pprime;
	unsigned int *prime_end;
	int result;

	if (pHash == NULL || hash_func == NULL)
	{
		return EINVAL;
	}

	memset(pHash, 0, sizeof(HashArray));
	prime_end = prime_array + PRIME_ARRAY_SIZE;
	for (pprime = prime_array; pprime!=prime_end; pprime++)
	{
		if (*pprime > capacity)
		{
			pHash->capacity = pprime;
			break;
		}
	}

	if (pHash->capacity == NULL)
	{
		return EINVAL;
	}

	if ((result=_hash_alloc_buckets(pHash)) != 0)
	{
		return result;
	}

	pHash->hash_func = hash_func;

	if (load_factor >= 0.10 && load_factor <= 1.00)
	{
		pHash->load_factor = load_factor;
	}
	else
	{
		pHash->load_factor = 0.50;
	}

	return 0;
}

void hash_destroy(HashArray *pHash)
{
	ChainList *plist;
	ChainList *list_end;

	if (pHash == NULL || pHash->items == NULL)
	{
		return;
	}

	list_end = pHash->items + (*pHash->capacity);
	for (plist=pHash->items; plist!=list_end; plist++)
	{
		chain_destroy(plist);
	}

	free(pHash->items);
	pHash->items = NULL;
	if (pHash->is_malloc_capacity)
	{
		free(pHash->capacity);
		pHash->capacity = NULL;
		pHash->is_malloc_capacity = false;
	}
}

void hash_stat_print(HashArray *pHash)
{
#define STAT_MAX_NUM  17
	ChainList *plist;
	ChainList *list_end;
	int totalLength;
	//ChainNode *pnode;
	//HashData *hash_data;
	int stats[STAT_MAX_NUM];
	int last;
	int index;
	int i;
	int max_length;
	int list_count;
	int bucket_used;

	if (pHash == NULL || pHash->items == NULL)
	{
		return;
	}

	memset(stats, 0, sizeof(stats));
	last = STAT_MAX_NUM - 1;
	list_end = pHash->items + (*pHash->capacity);
	max_length = 0;
	bucket_used = 0;
	for (plist=pHash->items; plist!=list_end; plist++)
	{
		list_count = chain_count(plist);
		if (list_count == 0)
		{
			continue;
		}

		bucket_used++;
		index = list_count - 1;
		if (index > last)
		{
			index = last;
		}
		stats[index]++;

		if (list_count > max_length)
		{
			max_length = list_count;
		}

		/*
		pnode = plist->head;
		while (pnode != NULL)
		{
			pnode = pnode->next;
		}
		*/
	}

	/*
	printf("collision stat:\n");
	for (i=0; i<last; i++)
	{
		if (stats[i] > 0) printf("%d: %d\n", i+1, stats[i]);
	}
	if (stats[i] > 0) printf(">=%d: %d\n", i+1, stats[i]);
	*/

	totalLength = 0;
	for (i=0; i<STAT_MAX_NUM; i++)
	{
		if (stats[i] > 0) totalLength += (i+1) * stats[i];
	}
	printf("capacity: %d, item_count=%d, bucket_used: %d, " \
		"avg length: %.4f, max length: %d, bucket / item = %.2f%%\n", 
               *pHash->capacity, pHash->item_count, bucket_used,
               bucket_used > 0 ? (double)totalLength / (double)bucket_used:0.00,
               max_length, (double)bucket_used*100.00/(double)*pHash->capacity);
}

static int _rehash1(HashArray *pHash, const int old_capacity, \
		unsigned int *new_capacity)
{
	ChainList *old_items;
	ChainList *plist;
	ChainList *list_end;
	ChainNode *pnode;
	HashData *hash_data;
	ChainList *pNewList;
	int result;

	old_items = pHash->items;
	pHash->capacity = new_capacity;
	if ((result=_hash_alloc_buckets(pHash)) != 0)
	{
		pHash->items = old_items;
		return result;
	}

	//printf("old: %d, new: %d\n", old_capacity, *pHash->capacity);
	list_end = old_items + old_capacity;
	for (plist=old_items; plist!=list_end; plist++)
	{
		pnode = plist->head;
		while (pnode != NULL)
		{
			hash_data = (HashData *) pnode->data;
			pNewList = pHash->items + (hash_data->hash_code % \
				(*pHash->capacity));

			//success to add node
			if (addNode(pNewList, hash_data) == 0)
			{
			}

			pnode = pnode->next;
		}

		plist->freeDataFunc = NULL;
		chain_destroy(plist);
	}

	free(old_items);
	return 0;
}

static int _rehash(HashArray *pHash)
{
	int result;
	unsigned int *pOldCapacity;

	pOldCapacity = pHash->capacity;
	if (pHash->is_malloc_capacity)
	{
		unsigned int *pprime;
		unsigned int *prime_end;

		pHash->capacity = NULL;

		prime_end = prime_array + PRIME_ARRAY_SIZE;
		for (pprime = prime_array; pprime!=prime_end; pprime++)
		{
			if (*pprime > *pOldCapacity)
			{
				pHash->capacity = pprime;
				break;
			}
		}
	}
	else
	{
		pHash->capacity++;
	}

	if ((result=_rehash1(pHash, *pOldCapacity, pHash->capacity)) != 0)
	{
		pHash->capacity = pOldCapacity;  //rollback
	}
	else
	{
		if (pHash->is_malloc_capacity)
		{
			free(pOldCapacity);
			pHash->is_malloc_capacity = false;
		}
	}

	/*printf("rehash, old_capacity=%d, new_capacity=%d\n", \
		old_capacity, *pHash->capacity);
	*/
	return result;
}

int _hash_conflict_count(HashArray *pHash)
{
	ChainList *plist;
	ChainList *list_end;
	ChainNode *pnode;
	ChainNode *pSubNode;
	int conflicted;
	int conflict_count;

	if (pHash == NULL || pHash->items == NULL)
	{
		return 0;
	}

	list_end = pHash->items + (*pHash->capacity);
	conflict_count = 0;
	for (plist=pHash->items; plist!=list_end; plist++)
	{
		if (plist->head == NULL || plist->head->next == NULL)
		{
			continue;
		}

		conflicted = 0;
		pnode = plist->head;
		while (pnode != NULL)
		{
			pSubNode = pnode->next;
			while (pSubNode != NULL)
			{
				if (((HashData *)pnode->data)->hash_code != \
					((HashData *)pSubNode->data)->hash_code)
				{
					conflicted = 1;
					break;
				}

				pSubNode = pSubNode->next;
			}

			if (conflicted)
			{
				break;
			}

			pnode = pnode->next;
		}

		conflict_count += conflicted;
	}

	return conflict_count;
}

int hash_best_op(HashArray *pHash, const int suggest_capacity)
{
	int old_capacity;
	int conflict_count;
	unsigned int *new_capacity;
	int result;

	if ((conflict_count=_hash_conflict_count(pHash)) == 0)
	{
		return 0;
	}

	old_capacity = *pHash->capacity;
	new_capacity = (unsigned int *)malloc(sizeof(unsigned int));
	if (new_capacity == NULL)
	{
		return -ENOMEM;
	}

	if ((suggest_capacity > 2) && (suggest_capacity >= pHash->item_count))
	{
		*new_capacity = suggest_capacity - 2;
		if (*new_capacity % 2 == 0)
		{
			++(*new_capacity);
		}
	}
	else
	{
		*new_capacity = 2 * (pHash->item_count - 1) + 1;
	}

	do
	{
		do
		{
			*new_capacity += 2;
		} while ((*new_capacity % 3 == 0) || (*new_capacity % 5 == 0) \
			 || (*new_capacity % 7 == 0));

		if ((result=_rehash1(pHash, old_capacity, new_capacity)) != 0)
		{
			pHash->is_malloc_capacity = \
					(pHash->capacity == new_capacity);
			*pHash->capacity = old_capacity;
			return -1 * result;
		}

		old_capacity = *new_capacity;
		/*printf("rehash, conflict_count=%d, old_capacity=%d, " \
			"new_capacity=%d\n", conflict_count, \
			old_capacity, *new_capacity);
		*/
	} while ((conflict_count=_hash_conflict_count(pHash)) > 0);

	pHash->is_malloc_capacity = true;

	//hash_stat_print(pHash);
	return 1;
}

HashData *_chain_find_entry(ChainList *plist, const void *key, \
		const int key_len, const unsigned int hash_code)
{
	ChainNode *pnode;
	HashData *hash_data;

	pnode = plist->head;
	while (pnode != NULL)
	{
		hash_data = (HashData *)pnode->data;
		if (key_len == hash_data->key_len && \
			memcmp(key, hash_data->key, key_len) == 0)
		{
			return hash_data;
		}

		pnode = pnode->next;
	}

	return NULL;
}

void *hash_find(HashArray *pHash, const void *key, const int key_len)
{
	unsigned int hash_code;
	ChainList *plist;
	HashData *hash_data;

	if (pHash == NULL || key == NULL || key_len < 0)
	{
		return NULL;
	}

	hash_code = pHash->hash_func(key, key_len);
	plist = pHash->items + (hash_code % (*pHash->capacity));

	hash_data = _chain_find_entry(plist, key, key_len, hash_code);
	if (hash_data != NULL)
	{
		return hash_data->value;
	}
	else
	{
		return NULL;
	}
}

int hash_insert(HashArray *pHash, const void *key, const int key_len, \
		void *value)
{
	unsigned int hash_code;
	ChainList *plist;
	HashData *hash_data;
	char *pBuff;
	int result;

	if (pHash == NULL || key == NULL || key_len < 0)
	{
		return -EINVAL;
	}

	hash_code = pHash->hash_func(key, key_len);
	plist = pHash->items + (hash_code % (*pHash->capacity));
	hash_data = _chain_find_entry(plist, key, key_len, hash_code);
	if (hash_data != NULL)
	{
		hash_data->value = value;
		return 0; 
	}

	pBuff = (char *)malloc(sizeof(HashData) + key_len);
	if (pBuff == NULL)
	{
		return -ENOMEM;
	}

	hash_data = (HashData *)pBuff;
	hash_data->key = pBuff + sizeof(HashData);

	hash_data->key_len = key_len;
	memcpy(hash_data->key, key, key_len);
	hash_data->hash_code = hash_code;
	hash_data->value = value;

	if ((result=addNode(plist, hash_data)) != 0) //fail to add node
	{
		free(hash_data);
		return -1 * result;
	}

	pHash->item_count++;

	if ((double)pHash->item_count / (double)*pHash->capacity >= \
		pHash->load_factor)
	{
		_rehash(pHash);
	}

	return 1;
}

int hash_delete(HashArray *pHash, const void *key, const int key_len)
{
	unsigned int hash_code;
	ChainList *plist;
	ChainNode *previous;
	ChainNode *pnode;
	HashData *hash_data;

	if (pHash == NULL || key == NULL || key_len < 0)
	{
		return -EINVAL;
	}

	hash_code = pHash->hash_func(key, key_len);
	plist = pHash->items + (hash_code % (*pHash->capacity));

	previous = NULL;
	pnode = plist->head;
	while (pnode != NULL)
	{
		hash_data = (HashData *)pnode->data;
		if (key_len == hash_data->key_len && \
			memcmp(key, hash_data->key, key_len) == 0)
		{
			deleteNodeEx(plist, previous, pnode);
			pHash->item_count--;
			return 1;
		}

		previous = pnode;
		pnode = pnode->next;
	}

	return 0;
}

void hash_walk(HashArray *pHash, HashWalkFunc walkFunc, void *args)
{
	ChainList *plist;
	ChainList *list_end;
	ChainNode *pnode;
	HashData *hash_data;
	int index;

	if (pHash == NULL || pHash->items == NULL || walkFunc == NULL)
	{
		return;
	}

	index = 0;
	list_end = pHash->items + (*pHash->capacity);
	for (plist=pHash->items; plist!=list_end; plist++)
	{
		pnode = plist->head;
		while (pnode != NULL)
		{
			hash_data = (HashData *) pnode->data;
			walkFunc(index, hash_data, args);

			pnode = pnode->next;
			index++;
		}
	}
}

// RS Hash Function
int RSHash(const void *key, const int key_len)
{
    unsigned char *pKey;
    unsigned char *pEnd;
    int a = 63689;
    int hash = 0;

    pEnd = (unsigned char *)key + key_len;
    for (pKey = (unsigned char *)key; pKey != pEnd; pKey++)
    {
        hash = hash * a + (*pKey);
        a *= 378551;
    }

    return hash;
} 
 
#define JS_HASH_FUNC(init_value) \
    unsigned char *pKey; \
    unsigned char *pEnd; \
    int hash; \
 \
    hash = init_value; \
    pEnd = (unsigned char *)key + key_len; \
    for (pKey = (unsigned char *)key; pKey != pEnd; pKey++) \
    { \
        hash ^= ((hash << 5) + (*pKey) + (hash >> 2)); \
    } \
 \
    return hash; \


// JS Hash Function
int JSHash(const void *key, const int key_len)
{
	JS_HASH_FUNC(1315423911)
}
 
int JSHash_ex(const void *key, const int key_len, \
	const int init_value)
{
	JS_HASH_FUNC(init_value)
}
 
#define PJW_HASH_FUNC(init_value) \
    unsigned char *pKey; \
    unsigned char *pEnd; \
    int BitsInUnignedInt = (int)(sizeof(int) * 8); \
    int ThreeQuarters    = (int)((BitsInUnignedInt * 3) / 4);\
    int OneEighth        = (int)(BitsInUnignedInt / 8); \
 \
    int HighBits         = (int)(0xFFFFFFFF) << \
				(BitsInUnignedInt - OneEighth); \
    int hash; \
    int test; \
 \
    hash = init_value; \
    pEnd = (unsigned char *)key + key_len; \
    for (pKey = (unsigned char *)key; pKey != pEnd; pKey++) \
    { \
        hash = (hash << OneEighth) + (*(pKey)); \
        if ((test = hash & HighBits) != 0) \
        { \
            hash = ((hash ^ (test >> ThreeQuarters)) & (~HighBits)); \
        } \
    } \
 \
    return hash; \


// P.J.Weinberger Hash Function, same as ELF Hash
int PJWHash(const void *key, const int key_len)
{
	PJW_HASH_FUNC(0)
}
 
int PJWHash_ex(const void *key, const int key_len, \
	const int init_value)
{
	PJW_HASH_FUNC(init_value)
}
 
#define ELF_HASH_FUNC(init_value) \
    unsigned char *pKey; \
    unsigned char *pEnd; \
    int hash; \
    int x; \
 \
    hash = init_value; \
    pEnd = (unsigned char *)key + key_len; \
    for (pKey = (unsigned char *)key; pKey != pEnd; pKey++) \
    { \
        hash = (hash << 4) + (*pKey); \
        if ((x = hash & 0xF0000000) != 0) \
        { \
            hash ^= (x >> 24); \
            hash &= ~x; \
        } \
    } \
 \
    return hash; \


// ELF Hash Function, same as PJW Hash
int ELFHash(const void *key, const int key_len)
{
	ELF_HASH_FUNC(0)
}

int ELFHash_ex(const void *key, const int key_len, \
	const int init_value)
{
	ELF_HASH_FUNC(init_value)
}

#define BKDR_HASH_FUNC(init_value) \
    unsigned char *pKey; \
    unsigned char *pEnd; \
    int seed = 131;  /* 31 131 1313 13131 131313 etc..*/ \
    int hash; \
 \
    hash = init_value; \
    pEnd = (unsigned char *)key + key_len; \
    for (pKey = (unsigned char *)key; pKey != pEnd; pKey++) \
    { \
        hash = hash * seed + (*pKey); \
    } \
 \
    return hash; \


// BKDR Hash Function
int BKDRHash(const void *key, const int key_len)
{
	BKDR_HASH_FUNC(0)
}

int BKDRHash_ex(const void *key, const int key_len, \
	const int init_value)
{
	BKDR_HASH_FUNC(init_value)
}

#define SDBM_HASH_FUNC(init_value) \
    unsigned char *pKey; \
    unsigned char *pEnd; \
    int hash; \
 \
    hash = init_value; \
    pEnd = (unsigned char *)key + key_len; \
    for (pKey = (unsigned char *)key; pKey != pEnd; pKey++) \
    { \
        hash = (*pKey) + (hash << 6) + (hash << 16) - hash; \
    } \
 \
    return hash; \


// SDBM Hash Function
int SDBMHash(const void *key, const int key_len)
{
	SDBM_HASH_FUNC(0)
}

int SDBMHash_ex(const void *key, const int key_len, \
	const int init_value)
{
	SDBM_HASH_FUNC(init_value)
}

#define TIME33_HASH_FUNC(init_value) \
	int nHash; \
	unsigned char *pKey; \
	unsigned char *pEnd; \
 \
	nHash = init_value; \
	pEnd = (unsigned char *)key + key_len; \
	for (pKey = (unsigned char *)key; pKey != pEnd; pKey++) \
	{ \
		nHash += (nHash << 5) + (*pKey); \
	} \
 \
	return nHash; \


int Time33Hash(const void *key, const int key_len)
{
	TIME33_HASH_FUNC(0)
}

int Time33Hash_ex(const void *key, const int key_len, \
	const int init_value)
{
	TIME33_HASH_FUNC(init_value)
}

#define DJB_HASH_FUNC(init_value) \
    unsigned char *pKey; \
    unsigned char *pEnd; \
    int hash; \
 \
    hash = init_value; \
    pEnd = (unsigned char *)key + key_len; \
    for (pKey = (unsigned char *)key; pKey != pEnd; pKey++) \
    { \
        hash += (hash << 5) + (*pKey); \
    } \
 \
    return hash; \


// DJB Hash Function
int DJBHash(const void *key, const int key_len)
{
	DJB_HASH_FUNC(5381)
}

int DJBHash_ex(const void *key, const int key_len, \
	const int init_value)
{
	DJB_HASH_FUNC(init_value)
}

#define AP_HASH_FUNC(init_value) \
    unsigned char *pKey; \
    unsigned char *pEnd; \
    int i; \
    int hash; \
 \
    hash = init_value; \
 \
    pEnd = (unsigned char *)key + key_len; \
    for (pKey = (unsigned char *)key, i=0; pKey != pEnd; pKey++, i++) \
    { \
        if ((i & 1) == 0) \
        { \
            hash ^= ((hash << 7) ^ (*pKey) ^ (hash >> 3)); \
        } \
        else \
        { \
            hash ^= (~((hash << 11) ^ (*pKey) ^ (hash >> 5))); \
        } \
    } \
 \
    return hash; \


// AP Hash Function
int APHash(const void *key, const int key_len)
{
	AP_HASH_FUNC(0)
}

int APHash_ex(const void *key, const int key_len, \
	const int init_value)
{
	AP_HASH_FUNC(init_value)
}

int calc_hashnr (const void* key, const int key_len)
{
  unsigned char *pKey;
  unsigned char *pEnd;
  int nr = 1, nr2 = 4;

  pEnd = (unsigned char *)key + key_len;
  for (pKey = (unsigned char *)key; pKey != pEnd; pKey++)
  {
      nr ^= (((nr & 63) + nr2) * (*pKey)) + (nr << 8);
      nr2 += 3;
  }

  return nr;

}

#define CALC_HASHNR1_FUNC(init_value) \
  unsigned char *pKey; \
  unsigned char *pEnd; \
  int hash; \
 \
  hash = init_value; \
  pEnd = (unsigned char *)key + key_len; \
  for (pKey = (unsigned char *)key; pKey != pEnd; pKey++) \
  { \
    hash *= 16777619; \
    hash ^= *pKey; \
  } \
  return hash; \

int calc_hashnr1(const void* key, const int key_len)
{
	CALC_HASHNR1_FUNC(0)
}

int calc_hashnr1_ex(const void* key, const int key_len, \
	const int init_value)
{
	CALC_HASHNR1_FUNC(init_value)
}

#define SIMPLE_HASH_FUNC(init_value) \
  int h; \
  unsigned char *p; \
  unsigned char *pEnd; \
 \
  h = init_value; \
  pEnd = (unsigned char *)key + key_len; \
  for (p = (unsigned char *)key; p!= pEnd; p++) \
  { \
    h = 31 * h + *p; \
  } \
 \
  return h; \

int simple_hash(const void* key, const int key_len)
{
	SIMPLE_HASH_FUNC(0)
}

int simple_hash_ex(const void* key, const int key_len, \
	const int init_value)
{
	SIMPLE_HASH_FUNC(init_value)
}

static unsigned int crc_table[256] = {
	0x00000000, 0x77073096, 0xEE0E612C, 0x990951BA,
	0x076DC419, 0x706AF48F, 0xE963A535, 0x9E6495A3,
	0x0EDB8832, 0x79DCB8A4, 0xE0D5E91E, 0x97D2D988,
	0x09B64C2B, 0x7EB17CBD, 0xE7B82D07, 0x90BF1D91,
	0x1DB71064, 0x6AB020F2, 0xF3B97148, 0x84BE41DE,
	0x1ADAD47D, 0x6DDDE4EB, 0xF4D4B551, 0x83D385C7,
	0x136C9856, 0x646BA8C0, 0xFD62F97A, 0x8A65C9EC,
	0x14015C4F, 0x63066CD9, 0xFA0F3D63, 0x8D080DF5,
	0x3B6E20C8, 0x4C69105E, 0xD56041E4, 0xA2677172,
	0x3C03E4D1, 0x4B04D447, 0xD20D85FD, 0xA50AB56B,
	0x35B5A8FA, 0x42B2986C, 0xDBBBC9D6, 0xACBCF940,
	0x32D86CE3, 0x45DF5C75, 0xDCD60DCF, 0xABD13D59,
	0x26D930AC, 0x51DE003A, 0xC8D75180, 0xBFD06116,
	0x21B4F4B5, 0x56B3C423, 0xCFBA9599, 0xB8BDA50F,
	0x2802B89E, 0x5F058808, 0xC60CD9B2, 0xB10BE924,
	0x2F6F7C87, 0x58684C11, 0xC1611DAB, 0xB6662D3D,
	0x76DC4190, 0x01DB7106, 0x98D220BC, 0xEFD5102A,
	0x71B18589, 0x06B6B51F, 0x9FBFE4A5, 0xE8B8D433,
	0x7807C9A2, 0x0F00F934, 0x9609A88E, 0xE10E9818,
	0x7F6A0DBB, 0x086D3D2D, 0x91646C97, 0xE6635C01,
	0x6B6B51F4, 0x1C6C6162, 0x856530D8, 0xF262004E,
	0x6C0695ED, 0x1B01A57B, 0x8208F4C1, 0xF50FC457,
	0x65B0D9C6, 0x12B7E950, 0x8BBEB8EA, 0xFCB9887C,
	0x62DD1DDF, 0x15DA2D49, 0x8CD37CF3, 0xFBD44C65,
	0x4DB26158, 0x3AB551CE, 0xA3BC0074, 0xD4BB30E2,
	0x4ADFA541, 0x3DD895D7, 0xA4D1C46D, 0xD3D6F4FB,
	0x4369E96A, 0x346ED9FC, 0xAD678846, 0xDA60B8D0,
	0x44042D73, 0x33031DE5, 0xAA0A4C5F, 0xDD0D7CC9,
	0x5005713C, 0x270241AA, 0xBE0B1010, 0xC90C2086,
	0x5768B525, 0x206F85B3, 0xB966D409, 0xCE61E49F,
	0x5EDEF90E, 0x29D9C998, 0xB0D09822, 0xC7D7A8B4,
	0x59B33D17, 0x2EB40D81, 0xB7BD5C3B, 0xC0BA6CAD,
	0xEDB88320, 0x9ABFB3B6, 0x03B6E20C, 0x74B1D29A,
	0xEAD54739, 0x9DD277AF, 0x04DB2615, 0x73DC1683,
	0xE3630B12, 0x94643B84, 0x0D6D6A3E, 0x7A6A5AA8,
	0xE40ECF0B, 0x9309FF9D, 0x0A00AE27, 0x7D079EB1,
	0xF00F9344, 0x8708A3D2, 0x1E01F268, 0x6906C2FE,
	0xF762575D, 0x806567CB, 0x196C3671, 0x6E6B06E7,
	0xFED41B76, 0x89D32BE0, 0x10DA7A5A, 0x67DD4ACC,
	0xF9B9DF6F, 0x8EBEEFF9, 0x17B7BE43, 0x60B08ED5,
	0xD6D6A3E8, 0xA1D1937E, 0x38D8C2C4, 0x4FDFF252,
	0xD1BB67F1, 0xA6BC5767, 0x3FB506DD, 0x48B2364B,
	0xD80D2BDA, 0xAF0A1B4C, 0x36034AF6, 0x41047A60,
	0xDF60EFC3, 0xA867DF55, 0x316E8EEF, 0x4669BE79,
	0xCB61B38C, 0xBC66831A, 0x256FD2A0, 0x5268E236,
	0xCC0C7795, 0xBB0B4703, 0x220216B9, 0x5505262F,
	0xC5BA3BBE, 0xB2BD0B28, 0x2BB45A92, 0x5CB36A04,
	0xC2D7FFA7, 0xB5D0CF31, 0x2CD99E8B, 0x5BDEAE1D,
	0x9B64C2B0, 0xEC63F226, 0x756AA39C, 0x026D930A,
	0x9C0906A9, 0xEB0E363F, 0x72076785, 0x05005713,
	0x95BF4A82, 0xE2B87A14, 0x7BB12BAE, 0x0CB61B38,
	0x92D28E9B, 0xE5D5BE0D, 0x7CDCEFB7, 0x0BDBDF21,
	0x86D3D2D4, 0xF1D4E242, 0x68DDB3F8, 0x1FDA836E,
	0x81BE16CD, 0xF6B9265B, 0x6FB077E1, 0x18B74777,
	0x88085AE6, 0xFF0F6A70, 0x66063BCA, 0x11010B5C,
	0x8F659EFF, 0xF862AE69, 0x616BFFD3, 0x166CCF45,
	0xA00AE278, 0xD70DD2EE, 0x4E048354, 0x3903B3C2,
	0xA7672661, 0xD06016F7, 0x4969474D, 0x3E6E77DB,
	0xAED16A4A, 0xD9D65ADC, 0x40DF0B66, 0x37D83BF0,
	0xA9BCAE53, 0xDEBB9EC5, 0x47B2CF7F, 0x30B5FFE9,
	0xBDBDF21C, 0xCABAC28A, 0x53B39330, 0x24B4A3A6,
	0xBAD03605, 0xCDD70693, 0x54DE5729, 0x23D967BF,
	0xB3667A2E, 0xC4614AB8, 0x5D681B02, 0x2A6F2B94,
	0xB40BBE37, 0xC30C8EA1, 0x5A05DF1B, 0x2D02EF8D
};

#define CRC32_BODY(init_value) \
	unsigned char *pKey; \
	unsigned char *pEnd; \
	int crc; \
 \
	crc = init_value; \
	pEnd = (unsigned char *)key + key_len; \
	for (pKey = (unsigned char *)key; pKey != pEnd; pKey++) \
	{ \
		crc = crc_table[(crc ^ *pKey) & 0xFF] ^ (crc >> 8); \
	} \

int CRC32(void *key, const int key_len)
{
	CRC32_BODY(CRC32_XINIT)

	return crc ^ CRC32_XOROT;
}

int CRC32_ex(void *key, const int key_len, \
	const int init_value)
{
	CRC32_BODY(init_value)

	return crc;
}

