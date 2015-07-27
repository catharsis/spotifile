#ifndef SPFS_FUSE_H
#define SPFS_FUSE_H
#define FUSE_USE_VERSION 26
#include <fuse.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "spfs_fuse_entity.h"
struct fuse_operations spfs_get_fuse_operations();

#define READ_OFFSET(_Str, _Buf, _Sz, _Off) do {\
	if ((strlen(_Str)) > (size_t)_Off) { \
		if ( _Off + _Sz > strlen(_Str)) \
			_Sz = strlen(_Str) - _Off; \
		memcpy(_Buf, _Str + _Off, _Sz); \
	} \
	else _Sz = 0;  } while (0)

struct spfs_data {
	sp_session *session;
	spfs_entity *root;
};

#define SPFS_DATA ((struct spfs_data *)(fuse_get_context()->private_data))
#define SPFS_SP_SESSION ((sp_session *)((SPFS_DATA)->session))



#endif /* SPFS_FUSE_H */
