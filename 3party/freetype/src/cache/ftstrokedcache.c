#include <ft2build.h>
#include FT_INTERNAL_OBJECTS_H
#include FT_INTERNAL_DEBUG_H
#include FT_CACHE_H
#include FT_STROKER_H
#include "ftcglyph.h"
#include "ftcimage.h"
#include "ftcsbits.h"

#include "ftccback.h"
#include "ftcerror.h"

#define FT_COMPONENT  trace_cache

/*
 *  Basic Families
 *
 */
typedef struct  FTC_StrokedAttrRec_
{
  FTC_ScalerRec  scaler;
  FT_UInt        load_flags;
  FT_Stroker     stroker;

} FTC_StrokedAttrRec, *FTC_StrokedAttrs;

#define FTC_STROKED_ATTR_COMPARE( a, b )                             \
        FT_BOOL( FTC_SCALER_COMPARE( &(a)->scaler, &(b)->scaler ) && \
                 ((a)->load_flags == (b)->load_flags )            && \
                 ((a)->stroker == (b)->stroker) )


#define FTC_STROKED_ATTR_HASH( a )                                   \
       ( FTC_SCALER_HASH( &(a)->scaler ) + 31*(a)->load_flags + 61 * (int)((a)->stroker))


typedef struct  FTC_StrokedQueryRec_
{
  FTC_GQueryRec     gquery;
  FTC_StrokedAttrRec  attrs;

} FTC_StrokedQueryRec, *FTC_StrokedQuery;


typedef struct  FTC_StrokedFamilyRec_
{
  FTC_FamilyRec     family;
  FTC_StrokedAttrRec  attrs;

} FTC_StrokedFamilyRec, *FTC_StrokedFamily;


FT_CALLBACK_DEF( FT_Bool )
ftc_stroked_family_compare( FTC_MruNode  ftcfamily,
                            FT_Pointer   ftcquery )
{
  FTC_StrokedFamily  family = (FTC_StrokedFamily)ftcfamily;
  FTC_StrokedQuery   query  = (FTC_StrokedQuery)ftcquery;

  return FTC_STROKED_ATTR_COMPARE( &family->attrs, &query->attrs );
}


FT_CALLBACK_DEF( FT_Error )
ftc_stroked_family_init( FTC_MruNode  ftcfamily,
                       FT_Pointer   ftcquery,
                       FT_Pointer   ftccache )
{
  FTC_StrokedFamily  family = (FTC_StrokedFamily)ftcfamily;
  FTC_StrokedQuery   query  = (FTC_StrokedQuery)ftcquery;
  FTC_Cache        cache  = (FTC_Cache)ftccache;

  FTC_Family_Init( FTC_FAMILY( family ), cache );
  family->attrs = query->attrs;
  return 0;
}


FT_CALLBACK_DEF( FT_UInt )
ftc_stroked_family_get_count( FTC_Family   ftcfamily,
                            FTC_Manager  manager )
{
  FTC_StrokedFamily  family = (FTC_StrokedFamily)ftcfamily;
  FT_Error         error;
  FT_Face          face;
  FT_UInt          result = 0;


  error = FTC_Manager_LookupFace( manager, family->attrs.scaler.face_id,
                                  &face );

  if ( error || !face )
    return result;

  if ( (FT_ULong)face->num_glyphs > FT_UINT_MAX || 0 > face->num_glyphs )
  {
    FT_TRACE1(( "ftc_basic_family_get_count: too large number of glyphs " ));
    FT_TRACE1(( "in this face, truncated\n", face->num_glyphs ));
  }

  if ( !error )
    result = (FT_UInt)face->num_glyphs;

  return result;
}


FT_CALLBACK_DEF( FT_Error )
ftc_stroked_family_load_glyph( FTC_Family  ftcfamily,
                             FT_UInt     gindex,
                             FTC_Cache   cache,
                             FT_Glyph   *aglyph )
{
  FTC_StrokedFamily  family = (FTC_StrokedFamily)ftcfamily;
  FT_Error         error;
  FTC_Scaler       scaler = &family->attrs.scaler;
  FT_Face          face;
  FT_Size          size;


  /* we will now load the glyph image */
  error = FTC_Manager_LookupSize( cache->manager,
                                  scaler,
                                  &size );
  if ( !error )
  {
    face = size->face;

    error = FT_Load_Glyph( face, gindex, FT_LOAD_DEFAULT );
    if ( !error )
    {
      if ( face->glyph->format == FT_GLYPH_FORMAT_BITMAP  ||
           face->glyph->format == FT_GLYPH_FORMAT_OUTLINE )
      {
        /* ok, stroke and copy it */
        FT_Glyph  glyph;

        error = FT_Get_Glyph( face->glyph, &glyph );

        if (error)
            goto Exit;

        error = FT_Glyph_Stroke(&glyph, family->attrs.stroker, 1);

        if (error)
          goto Exit;

        error = FT_Glyph_To_Bitmap(&glyph, FT_RENDER_MODE_NORMAL, 0, 1);

        if (!error)
        {
          *aglyph = glyph;
          goto Exit;
        }

      }
      else
        error = FTC_Err_Invalid_Argument;
    }
  }

Exit:
  return error;
}

FT_CALLBACK_DEF( FT_Bool )
ftc_stroked_gnode_compare_faceid( FTC_Node    ftcgnode,
                                  FT_Pointer  ftcface_id,
                                  FTC_Cache   cache,
                                  FT_Bool*    list_changed )
{
  FTC_GNode        gnode   = (FTC_GNode)ftcgnode;
  FTC_FaceID       face_id = (FTC_FaceID)ftcface_id;
  FTC_StrokedFamily  family  = (FTC_StrokedFamily)gnode->family;
  FT_Bool          result;


  if ( list_changed )
    *list_changed = FALSE;
  result = FT_BOOL( family->attrs.scaler.face_id == face_id );
  if ( result )
  {
    /* we must call this function to avoid this node from appearing
     * in later lookups with the same face_id!
     */
    FTC_GNode_UnselectFamily( gnode, cache );
  }
  return result;
}


/*
 *
 * stroked image cache
 *
 */

 FT_CALLBACK_TABLE_DEF
 const FTC_IFamilyClassRec  ftc_stroked_image_family_class =
 {
   {
     sizeof ( FTC_StrokedFamilyRec ),
     ftc_stroked_family_compare,
     ftc_stroked_family_init,
     0,                        /* FTC_MruNode_ResetFunc */
     0                         /* FTC_MruNode_DoneFunc  */
   },
   ftc_stroked_family_load_glyph
 };


 FT_CALLBACK_TABLE_DEF
 const FTC_GCacheClassRec  ftc_stroked_image_cache_class =
 {
   {
     ftc_inode_new,
     ftc_inode_weight,
     ftc_gnode_compare,
     ftc_stroked_gnode_compare_faceid,
     ftc_inode_free,

     sizeof ( FTC_GCacheRec ),
     ftc_gcache_init,
     ftc_gcache_done
   },
   (FTC_MruListClass)&ftc_stroked_image_family_class
 };


 /* documentation is in ftcache.h */

 FT_EXPORT_DEF( FT_Error )
 FTC_StrokedImageCache_New( FTC_Manager      manager,
                            FTC_ImageCache  *acache)
 {
   return FTC_GCache_New( manager, &ftc_stroked_image_cache_class,
                          (FTC_GCache*)acache );
 }

 /* documentation is in ftcache.h */

 FT_EXPORT_DEF( FT_Error )
 FTC_StrokedImageCache_Lookup( FTC_ImageCache  cache,
                               FTC_ImageType   type,
                               FT_Stroker      stroker,
                               FT_UInt         gindex,
                               FT_Glyph       *aglyph,
                               FTC_Node       *anode )
 {
   FTC_StrokedQueryRec  query;
   FTC_Node           node = 0; /* make compiler happy */
   FT_Error           error;
   FT_PtrDist         hash;


   /* some argument checks are delayed to FTC_Cache_Lookup */
   if ( !aglyph )
   {
     error = FTC_Err_Invalid_Argument;
     goto Exit;
   }

   *aglyph = NULL;
   if ( anode )
     *anode  = NULL;

#if defined( FT_CONFIG_OPTION_OLD_INTERNALS ) && ( FT_INT_MAX > 0xFFFFU )

   /*
    *  This one is a major hack used to detect whether we are passed a
    *  regular FTC_ImageType handle, or a legacy FTC_OldImageDesc one.
    */
   if ( (FT_ULong)type->width >= 0x10000L )
   {
     FTC_OldImageDesc  desc = (FTC_OldImageDescRec_)type;


     query.attrs.scaler.face_id = desc->font.face_id;
     query.attrs.scaler.width   = desc->font.pix_width;
     query.attrs.scaler.height  = desc->font.pix_height;
     query.attrs.load_flags     = desc->flags;
     query.attrs.stroker        = stroker;
   }
   else

#endif /* FT_CONFIG_OPTION_OLD_INTERNALS */

   {
     if ( (FT_ULong)(type->flags - FT_INT_MIN) > FT_UINT_MAX )
     {
       FT_TRACE1(( "FTC_ImageCache_Lookup: higher bits in load_flags" ));
       FT_TRACE1(( "0x%x are dropped\n", (type->flags & ~((FT_ULong)FT_UINT_MAX)) ));
     }

     query.attrs.scaler.face_id = type->face_id;
     query.attrs.scaler.width   = type->width;
     query.attrs.scaler.height  = type->height;
     query.attrs.load_flags     = (FT_UInt)type->flags;
     query.attrs.stroker        = stroker;
   }

   query.attrs.scaler.pixel = 1;
   query.attrs.scaler.x_res = 0;  /* make compilers happy */
   query.attrs.scaler.y_res = 0;

   hash = FTC_STROKED_ATTR_HASH( &query.attrs ) + gindex;

#if 1  /* inlining is about 50% faster! */
   FTC_GCACHE_LOOKUP_CMP( cache,
                          ftc_stroked_family_compare,
                          FTC_GNode_Compare,
                          hash, gindex,
                          &query,
                          node,
                          error );
#else
   error = FTC_GCache_Lookup( FTC_GCACHE( cache ),
                              hash, gindex,
                              FTC_GQUERY( &query ),
                              &node );
#endif
   if ( !error )
   {
     *aglyph = FTC_INODE( node )->glyph;

     if ( anode )
     {
       *anode = node;
       node->ref_count++;
     }
   }

 Exit:
   return error;
 }


 /* documentation is in ftcache.h */

 FT_EXPORT_DEF( FT_Error )
 FTC_StrokedImageCache_LookupScaler( FTC_ImageCache  cache,
                              FTC_Scaler      scaler,
                              FT_Stroker      stroker,
                              FT_ULong        load_flags,
                              FT_UInt         gindex,
                              FT_Glyph       *aglyph,
                              FTC_Node       *anode )
 {
   FTC_StrokedQueryRec  query;
   FTC_Node             node = 0; /* make compiler happy */
   FT_Error             error;
   FT_PtrDist           hash;


   /* some argument checks are delayed to FTC_Cache_Lookup */
   if ( !aglyph || !scaler )
   {
     error = FTC_Err_Invalid_Argument;
     goto Exit;
   }

   *aglyph = NULL;
   if ( anode )
     *anode  = NULL;

   /* FT_Load_Glyph(), FT_Load_Char() take FT_UInt flags */
   if ( load_flags > FT_UINT_MAX )
   {
     FT_TRACE1(( "FTC_StrokedImageCache_LookupScaler: higher bits in load_flags" ));
     FT_TRACE1(( "0x%x are dropped\n", (load_flags & ~((FT_ULong)FT_UINT_MAX)) ));
   }

   query.attrs.scaler     = scaler[0];
   query.attrs.load_flags = (FT_UInt)load_flags;
   query.attrs.stroker    = stroker;

   hash = FTC_STROKED_ATTR_HASH( &query.attrs ) + gindex;

   FTC_GCACHE_LOOKUP_CMP( cache,
                          ftc_stroked_family_compare,
                          FTC_GNode_Compare,
                          hash, gindex,
                          &query,
                          node,
                          error );
   if ( !error )
   {
     *aglyph = FTC_INODE( node )->glyph;

     if ( anode )
     {
       *anode = node;
       node->ref_count++;
     }
   }

 Exit:
   return error;
 }
