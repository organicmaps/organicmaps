#include "o5mreader.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>


#define STR_PAIR_TABLE_SIZE 15000
#define STR_PAIR_STRING_SIZE 256

void o5mreader_setError(O5mreader *pReader,int code, const char* message);
void o5mreader_setNoError(O5mreader *pReader);

O5mreaderRet o5mreader_readUInt(O5mreader *pReader, uint64_t *ret) {
	uint8_t b;
	uint8_t i = 0;
	*ret = 0LL;
		
	do  {
		if ( fread(&b,1,1,pReader->f) == 0 ) {
			o5mreader_setError(pReader,
				O5MREADER_ERR_CODE_UNEXPECTED_END_OF_FILE,
				NULL
			);
			return O5MREADER_RET_ERR;
		}
		*ret |= (long long)(b & 0x7f) << (i++ * 7);			
	} while ( b & 0x80 );	
	
	o5mreader_setNoError(pReader);
	
	return O5MREADER_RET_OK;
}

O5mreaderRet o5mreader_readInt(O5mreader *pReader, uint64_t *ret) {
	if ( o5mreader_readUInt(pReader, ret) == O5MREADER_RET_ERR )
		return O5MREADER_RET_ERR;
	*ret = *ret & 1 
		? -(int64_t)(*ret >> 1) - 1
		: (int64_t)(*ret >> 1);
	return O5MREADER_RET_OK;
}


O5mreaderRet o5mreader_readStrPair(O5mreader *pReader, char **tagpair, int single) {	
	static char buffer[1024];
	char* pBuf;
	static uint64_t pointer = 0;
	int length;
	char byte;
	uint64_t key; 
	int i;
	
	if ( o5mreader_readUInt(pReader,&key) == O5MREADER_RET_ERR ) {		
		return O5MREADER_RET_ERR;
	}
	
	if ( key ) {
		*tagpair = pReader->strPairTable[(pointer+15000-key)%15000];		
		return key;
	}
	else {
		pBuf = buffer;
		for ( i=0; i<(single?1:2); i++ ) {
			do {
				if ( fread(pBuf,1,1,pReader->f) == 0 ) {
					o5mreader_setError(pReader,
						O5MREADER_ERR_CODE_UNEXPECTED_END_OF_FILE,
						NULL
					);
					return O5MREADER_RET_ERR;
				}
			} while ( *(pBuf++) );
		}			
		
		length = strlen(buffer) + (single ? 1 : strlen(buffer+strlen(buffer) +1) + 2);
		
		if ( length <= 252 ) {			
			*tagpair = pReader->strPairTable[(pointer+15000)%15000];			
			memcpy(pReader->strPairTable[((pointer++)+15000)%15000],buffer,length);						
		}
		else {
			*tagpair = buffer;
		}
				
	}
	
	return O5MREADER_RET_OK;
}

O5mreaderRet o5mreader_reset(O5mreader *pReader) {
	pReader->nodeId = pReader->wayId = pReader->wayNodeId = pReader->relId = pReader->nodeRefId = pReader->wayRefId = pReader->relRefId = 0;	
	pReader->lon = pReader->lat = 0;
	pReader->offset = 0;	
	pReader->canIterateTags = pReader->canIterateNds = pReader->canIterateRefs = 0;
  return O5MREADER_RET_OK;
}

O5mreaderRet o5mreader_open(O5mreader **ppReader,FILE* f) {
	uint8_t byte;
	int i;
	*ppReader = malloc(sizeof(O5mreader));
	if ( !(*ppReader) ) {
		return O5MREADER_RET_ERR;
	}
	(*ppReader)->errMsg = NULL;
	(*ppReader)->f = f;	
	if ( fread(&byte,1,1,(*ppReader)->f) == 0 ) {
		o5mreader_setError(*ppReader,
			O5MREADER_ERR_CODE_UNEXPECTED_END_OF_FILE,
			NULL
		);	
		return O5MREADER_RET_ERR;
	}
	if ( byte != O5MREADER_DS_RESET ) {
		o5mreader_setError(*ppReader,
			O5MREADER_ERR_CODE_FILE_HAS_WRONG_START,
			NULL
		);	
		return O5MREADER_RET_ERR;
	}
	
	o5mreader_reset(*ppReader);
	
	(*ppReader)->strPairTable = malloc(STR_PAIR_TABLE_SIZE*sizeof(char*));
	if ( (*ppReader)->strPairTable == 0 ) {
		o5mreader_setError(*ppReader,
			O5MREADER_ERR_CODE_MEMORY_ERROR,
			NULL
		);
		return O5MREADER_RET_ERR;
	}
	for ( i = 0; i < STR_PAIR_TABLE_SIZE; ++i ) {
		(*ppReader)->strPairTable[i] = malloc(sizeof(char)*STR_PAIR_STRING_SIZE);
		if ( (*ppReader)->strPairTable[i] == 0 ) {
			o5mreader_setError(*ppReader,
				O5MREADER_ERR_CODE_MEMORY_ERROR,
				NULL
			);
			return O5MREADER_RET_ERR;
		}
	}
	
	o5mreader_setNoError(*ppReader);
	return O5MREADER_RET_OK;
}

void o5mreader_close(O5mreader *pReader) {
	int i;
	if ( pReader ) {
		if ( pReader->strPairTable ) {
			for ( i = 0; i < STR_PAIR_TABLE_SIZE; ++i )
				if ( pReader->strPairTable[i] )
					free(pReader->strPairTable[i]);
			free(pReader->strPairTable);
		}		
		o5mreader_setNoError(pReader);	
		free(pReader);
	}
}

const char* o5mreader_strerror(int errCode) {
	switch ( errCode ) {		
		case O5MREADER_ERR_CODE_FILE_HAS_WRONG_START:
			return "'0xFF' isn't first byte of file.";
		case O5MREADER_ERR_CODE_MEMORY_ERROR:
			return "Memory error.";
		case O5MREADER_ERR_CODE_UNEXPECTED_END_OF_FILE:
			return "Unexpected end of file.";
		case O5MREADER_ERR_CODE_CAN_NOT_ITERATE_TAGS_HERE:
			return "Tags iteration is not allowed here.";
		case O5MREADER_ERR_CODE_CAN_NOT_ITERATE_NDS_HERE:
			return "Nodes iteration is not allowed here.";
		case O5MREADER_ERR_CODE_CAN_NOT_ITERATE_REFS_HERE:
			return "References iteration is not allowed here.";
		default:
			return "Unknown error code";
	}
}

void o5mreader_setError(O5mreader *pReader,int code, const char* message) {
	pReader->errCode = code;
	if ( pReader->errMsg ) {
		free(pReader->errMsg);
	}
	if ( message ) {
		pReader->errMsg = malloc(strlen(message)+1);
		strcpy(pReader->errMsg,message);
	}
}

void o5mreader_setNoError(O5mreader *pReader) {
	pReader->errCode = O5MREADER_ERR_CODE_OK;
	if ( pReader->errMsg ) {
		free(pReader->errMsg);
	}
	pReader->errMsg = NULL;
}

O5mreaderIterateRet o5mreader_iterateDataSet(O5mreader *pReader, O5mreaderDataset* ds) {
	for (;;) {		
		if ( pReader->offset ) {
			if (  o5mreader_skipTags(pReader) == O5MREADER_ITERATE_RET_ERR )
				return O5MREADER_ITERATE_RET_ERR;
									
			fseek(
				pReader->f,
				(pReader->current - ftell(pReader->f)) + pReader->offset,
				SEEK_CUR
			);
			
			pReader->offset = 0;
		}
		
		if ( fread(&(ds->type),1,1,pReader->f) == 0 ) {
			o5mreader_setError(pReader,
				O5MREADER_ERR_CODE_UNEXPECTED_END_OF_FILE,
				NULL
			);
			return O5MREADER_ITERATE_RET_ERR;
		}
						
		if ( O5MREADER_DS_END == ds->type )
			return O5MREADER_ITERATE_RET_DONE;
			
		if ( O5MREADER_DS_RESET == ds->type ) {	
			o5mreader_reset(pReader);
		}
		else if ( 0xf0 == ds->type ) {}
		else {		
			if ( o5mreader_readUInt(pReader,&pReader->offset) == O5MREADER_RET_ERR ) {		
				return O5MREADER_ITERATE_RET_ERR;
			}
			pReader->current = ftell(pReader->f);		
			
			switch ( ds->type ) {
				case O5MREADER_DS_NODE:					
					return o5mreader_readNode(pReader, ds);						
				case O5MREADER_DS_WAY:
					return o5mreader_readWay(pReader, ds);						
				case O5MREADER_DS_REL:
					return o5mreader_readRel(pReader, ds);	
				/*					
				case O5MREADER_DS_BBOX:
				case O5MREADER_DS_TSTAMP:
				case O5MREADER_DS_HEADER:
				case O5MREADER_DS_SYNC:
				case O5MREADER_DS_JUMP:
				default:
					break;
				*/
			}
		}
	}
	
	
}

int o5mreader_thereAreNoMoreData(O5mreader *pReader) {	
	return (int)((pReader->current - ftell(pReader->f)) + pReader->offset) <= 0;
}

O5mreaderIterateRet o5mreader_readVersion(O5mreader *pReader, O5mreaderDataset* ds) {
	uint64_t tmp;
	if ( o5mreader_readUInt(pReader,&tmp) == O5MREADER_ITERATE_RET_ERR  ) {
		return O5MREADER_ITERATE_RET_ERR;
	}
	ds->version = tmp;
	if ( tmp )  {		
		if ( o5mreader_readUInt(pReader,&tmp) == O5MREADER_ITERATE_RET_ERR  ) {			
			return O5MREADER_ITERATE_RET_ERR;		
		}
		
		if ( o5mreader_readInt(pReader,&tmp) == O5MREADER_ITERATE_RET_ERR ) {
			return O5MREADER_ITERATE_RET_ERR;
		}
		
		if ( o5mreader_thereAreNoMoreData(pReader) ) 
			return O5MREADER_ITERATE_RET_DONE;
			
		if ( o5mreader_readStrPair(pReader,&pReader->tagPair,0) == O5MREADER_ITERATE_RET_ERR ) {
			return O5MREADER_ITERATE_RET_ERR;
		}
	}
	
	if ( o5mreader_thereAreNoMoreData(pReader) )
		return O5MREADER_ITERATE_RET_DONE;
	
	return O5MREADER_ITERATE_RET_NEXT;
}

O5mreaderIterateRet o5mreader_iterateTags(O5mreader *pReader, char** pKey, char** pVal) {	
	if ( pReader->canIterateRefs  ) {
		if ( o5mreader_skipRefs(pReader) == O5MREADER_ITERATE_RET_ERR )
			return O5MREADER_ITERATE_RET_ERR;
	}
	if ( pReader->canIterateNds  ) {
		if ( o5mreader_skipNds(pReader) == O5MREADER_ITERATE_RET_ERR )
			return O5MREADER_ITERATE_RET_ERR;
	}
	if ( !pReader->canIterateTags  ) {
		o5mreader_setError(pReader,
			O5MREADER_ERR_CODE_CAN_NOT_ITERATE_TAGS_HERE,
			NULL
		);
		return O5MREADER_ITERATE_RET_ERR;
	}
	if ( o5mreader_thereAreNoMoreData(pReader) ) {	
		pReader->canIterateTags = 0;		
		return O5MREADER_ITERATE_RET_DONE;
	}
			
	if ( o5mreader_readStrPair(pReader,&pReader->tagPair,0) == O5MREADER_RET_ERR ) {		
		return O5MREADER_ITERATE_RET_ERR;
	}
	if ( pKey )
		*pKey = pReader->tagPair;
	if ( pVal )
		*pVal = pReader->tagPair + strlen(pReader->tagPair) + 1;		
		
	return O5MREADER_ITERATE_RET_NEXT;
}

O5mreaderIterateRet o5mreader_skipTags(O5mreader *pReader) {
	int ret;	
	if ( pReader->canIterateTags ) {		
		while ( O5MREADER_ITERATE_RET_NEXT == (ret = o5mreader_iterateTags(pReader, NULL, NULL)) );
	}
	
	return ret;
}

O5mreaderIterateRet o5mreader_readNode(O5mreader *pReader, O5mreaderDataset* ds) {	
	uint64_t nodeId;
	int64_t lon,lat;
	if ( o5mreader_readInt(pReader,&nodeId) == O5MREADER_RET_ERR )
		return O5MREADER_ITERATE_RET_ERR;	

	pReader->canIterateRefs = 0;
	pReader->canIterateNds = 0;
	pReader->canIterateTags = 1;

	pReader->nodeId += nodeId;
	ds->id = pReader->nodeId;				
	
	
	if ( o5mreader_readVersion(pReader, ds) == O5MREADER_ITERATE_RET_DONE  ) {
		ds->isEmpty = 1;
		return O5MREADER_ITERATE_RET_NEXT;
	}
	else 
		ds->isEmpty = 0;

	if ( o5mreader_thereAreNoMoreData(pReader) ) {		
		return O5MREADER_ITERATE_RET_NEXT;
	}

	if ( o5mreader_readInt(pReader,&lon) == O5MREADER_RET_ERR )
		return O5MREADER_ITERATE_RET_ERR;
	pReader->lon += (int32_t)lon;
	
	if ( o5mreader_readInt(pReader,&lat) == O5MREADER_RET_ERR ) {
		return O5MREADER_ITERATE_RET_ERR;
	}
	pReader->lat += (int32_t)lat;
	
	ds->lon = pReader->lon;
	ds->lat = pReader->lat;
	
	return O5MREADER_ITERATE_RET_NEXT;
}

O5mreaderIterateRet o5mreader_iterateNds(O5mreader *pReader, uint64_t *nodeId) {
	int64_t wayNodeId;
	
	if ( !pReader->canIterateNds  ) {
		o5mreader_setError(pReader,
			O5MREADER_ERR_CODE_CAN_NOT_ITERATE_NDS_HERE,
			NULL
		);
		return O5MREADER_ITERATE_RET_ERR;
	}
	if ( ftell(pReader->f) >= pReader->offsetNd ) {
		pReader->canIterateNds = 0;
		pReader->canIterateTags = 1;
		pReader->canIterateRefs = 0;
		return O5MREADER_ITERATE_RET_DONE;
	}
			
	if ( o5mreader_readInt(pReader,&wayNodeId) == O5MREADER_RET_ERR )
		return O5MREADER_ITERATE_RET_ERR;
	
	pReader->wayNodeId += wayNodeId;
	
	if ( nodeId )
		*nodeId = pReader->wayNodeId;
	
	return O5MREADER_ITERATE_RET_NEXT;
}

O5mreaderIterateRet o5mreader_skipNds(O5mreader *pReader) {
	uint8_t ret;	
	while ( pReader->canIterateNds &&
		O5MREADER_ITERATE_RET_NEXT == (ret = o5mreader_iterateNds(pReader, NULL)) );
	return ret;
}

O5mreaderIterateRet o5mreader_readWay(O5mreader *pReader, O5mreaderDataset* ds) {	
	int64_t wayId;
	if ( o5mreader_readInt(pReader,&wayId) == O5MREADER_RET_ERR)
		return O5MREADER_ITERATE_RET_ERR;
	
	pReader->wayId += wayId;
	ds->id = pReader->wayId;
	if ( o5mreader_readVersion(pReader, ds) == O5MREADER_ITERATE_RET_DONE  ) {
		ds->isEmpty = 1;
		return O5MREADER_ITERATE_RET_NEXT;
	}
	else
		ds->isEmpty = 0;
	if ( o5mreader_readUInt(pReader,&pReader->offsetNd) == O5MREADER_RET_ERR ) {
		return O5MREADER_ITERATE_RET_ERR;
	}
	pReader->offsetNd += ftell(pReader->f);
	pReader->canIterateRefs = 0;	
	pReader->canIterateNds = 1;	
	pReader->canIterateTags = 0;
	return O5MREADER_ITERATE_RET_NEXT;
}

O5mreaderIterateRet o5mreader_iterateRefs(O5mreader *pReader, uint64_t *refId, uint8_t *type, char** pRole) {
	int64_t relRefId;	
	
	if ( !pReader->canIterateRefs  ) {
		o5mreader_setError(pReader,
			O5MREADER_ERR_CODE_CAN_NOT_ITERATE_REFS_HERE,
			NULL
		);
		return O5MREADER_ITERATE_RET_ERR;
	}
	if ( ftell(pReader->f) >= pReader->offsetRf ) {
		pReader->canIterateNds = 0;
		pReader->canIterateTags = 1;
		pReader->canIterateRefs = 0;
		return O5MREADER_ITERATE_RET_DONE;
	}
			
	if ( o5mreader_readInt(pReader, &relRefId) == O5MREADER_RET_ERR )
		return O5MREADER_ITERATE_RET_ERR;
			
	
	//fread(_,1,1,pReader->f);
	
	if ( o5mreader_readStrPair(pReader, &pReader->tagPair,1) == O5MREADER_RET_ERR ) {
		return O5MREADER_ITERATE_RET_ERR;
	}
		
	switch( pReader->tagPair[0] ) {
		case '0': 
			if ( type )
				*type = O5MREADER_DS_NODE; 
			pReader->nodeRefId += relRefId;
			if ( refId )
				*refId = pReader->nodeRefId;
			break;
		case '1': 
			if ( type )
				*type = O5MREADER_DS_WAY;
			pReader->wayRefId += relRefId;
			if ( refId )
				*refId = pReader->wayRefId;
			break;
		case '2':
			if ( type )
				*type = O5MREADER_DS_REL;
			pReader->relRefId += relRefId;
			if ( refId )
				*refId = pReader->relRefId;
			break;
	}
	
	if ( pRole ) {
		*pRole = pReader->tagPair + 1;
	}		
	
	return O5MREADER_ITERATE_RET_NEXT;
}

O5mreaderIterateRet o5mreader_skipRefs(O5mreader *pReader) {
	uint8_t ret;
	while ( pReader->canIterateRefs &&
		O5MREADER_ITERATE_RET_NEXT == (ret = o5mreader_iterateRefs(pReader, NULL, NULL, NULL)) );
	return ret;
}

O5mreaderIterateRet o5mreader_readRel(O5mreader *pReader, O5mreaderDataset* ds) {
	int64_t relId;
	if ( o5mreader_readInt(pReader,&relId) == O5MREADER_RET_ERR )
		return O5MREADER_ITERATE_RET_ERR;
	pReader->relId += relId;
	ds->id = pReader->relId;
	if ( o5mreader_readVersion(pReader,ds) == O5MREADER_ITERATE_RET_DONE  ) {
		ds->isEmpty = 1;
		return O5MREADER_ITERATE_RET_NEXT;
	}
	else
		ds->isEmpty = 0;
	o5mreader_readUInt(pReader,&pReader->offsetRf);
	pReader->offsetRf += ftell(pReader->f);		
	
	pReader->canIterateRefs = 1;	
	pReader->canIterateNds = 0;	
	pReader->canIterateTags = 0;
	return O5MREADER_ITERATE_RET_NEXT;
}
