#ifndef SPFS_FUSE_H
#define SPFS_FUSE_H
#define FUSE_USE_VERSION 26
#include <fuse.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "spfs_fuse_entity.h"
#include "spfs_spotify.h"
/**
 * Returns the SPFS fuse_operations struct.
 */
struct fuse_operations spfs_get_fuse_operations();

/**
 * Copies \p _Sz or \p _SrcSz (whichever happens to be smaller) bytes from
 * buffer \p _Src starting at offset \p _Off into buffer \p _Buf.
 *
 * After invocation, the variable passed as _Sz will be set to the number
 * of bytes copied.
 */
#define READ_SIZED_OFFSET(_Src, _SrcSz, _Buf, _Sz, _Off) do {\
	if (_SrcSz > (size_t)_Off) { \
		if ( _Off + _Sz > _SrcSz) \
			_Sz = _SrcSz - _Off; \
		memcpy(_Buf, _Src + _Off, _Sz); \
	} \
	else _Sz = 0;  } while (0)


/**
 * Convenience wrapper for READ_SIZED_OFFSET() which uses the length of _Str as _SrcSz
 */
#define READ_OFFSET(_Str, _Buf, _Sz, _Off) READ_SIZED_OFFSET(_Str, strlen(_Str), _Buf, _Sz, _Off)

struct spfs_data {
	sp_session *session;
	spfs_entity *root;
	bool music_playing;
};

#define SPFS_DATA ((struct spfs_data *)(fuse_get_context()->private_data))
#define SPFS_SP_SESSION ((sp_session *)((SPFS_DATA)->session))
#define SPFS_FH2ENT(_fh) ((spfs_entity *) (uintptr_t) (_fh))
#define SPFS_ENT2FH(_ent) ((uintptr_t) (_ent))



#endif /* SPFS_FUSE_H */
