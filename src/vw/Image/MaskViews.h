// __BEGIN_LICENSE__
// Copyright (C) 2006, 2007 United States Government as represented by
// the Administrator of the National Aeronautics and Space Administration.
// All Rights Reserved.
// __END_LICENSE__


/// \file PixelMask.h
/// 
/// Defines the useful pixel utility type that can wrap any existing
/// pixel type and add mask semantics.  Any operations with an
/// "invalid" pixel returns an invalid pixel as a result.  
///
#ifndef __VW_IMAGE_MASK_VIEWS_H__
#define __VW_IMAGE_MASK_VIEWS_H__

#include <vw/Image/PixelMask.h>
#include <vw/Image/ImageViewBase.h>
#include <vw/Image/PerPixelViews.h>
//#include <vw/Image/BlockCacheView.h>

namespace vw {

  // *******************************************************************
  /// create_mask( view, value )
  ///
  /// Given a view with pixels of type PixelT and a pixel value to
  /// consider as the "no data" or masked value, returns a view with
  /// pixels that are of the PixelMask<PixelT>, with the appropriate
  /// pixels masked.
  ///
  template <class PixelT>
  class CreatePixelMask : public ReturnFixedType<PixelMask<PixelT> > {
    PixelT m_nodata_value;
  public:
    CreatePixelMask( PixelT const& nodata_value ) : m_nodata_value(nodata_value) {}
    PixelMask<PixelT> operator()( PixelT const& value ) const {
      return (value==m_nodata_value) ? PixelMask<PixelT>() : PixelMask<PixelT>(value);
    }
  };

  template <class ViewT>
  UnaryPerPixelView<ViewT,CreatePixelMask<typename ViewT::pixel_type> >
  create_mask( ImageViewBase<ViewT> const& view,
               typename ViewT::pixel_type const& value ) {
    typedef UnaryPerPixelView<ViewT,CreatePixelMask<typename ViewT::pixel_type> > view_type;
    return view_type( view.impl(), CreatePixelMask<typename ViewT::pixel_type>(value) );
  }

  // We overload the function rather than defaulting the value
  // argument to work around a compiler issue in MSVC 2005.
  template <class ViewT>
  UnaryPerPixelView<ViewT,CreatePixelMask<typename ViewT::pixel_type> >
  create_mask( ImageViewBase<ViewT> const& view ) {
    return create_mask( view, typename ViewT::pixel_type() );
  }


  // Indicate that create_mask is "reasonably fast" and should never
  // induce an extra rasterization step during prerasterization.
  template <class ViewT>
  struct IsMultiplyAccessible<UnaryPerPixelView<ViewT,CreatePixelMask<typename UnmaskedPixelType<typename ViewT::pixel_type>::type> > >
    : public IsMultiplyAccessible<ViewT> {};

  // *******************************************************************
  /// apply_mask( view, value )
  ///
  /// Given a view with pixels of the type PixelMask<T>, this view
  /// returns an image with pixels of type T where any pixel that was
  /// marked as "invalid" in the mask is replaced with the constant
  /// pixel value passed in as value.  The value is T() by default.
  ///
  template <class PixelT>
  class ApplyPixelMask : public ReturnFixedType<PixelT const&> {
    PixelT m_nodata_value;
  public:
    ApplyPixelMask( PixelT const& nodata_value ) : m_nodata_value(nodata_value) {}
    PixelT const& operator()( PixelMask<PixelT> const& value ) const {
      return value.valid() ? value.child() : m_nodata_value;
    }
  };

  template <class ViewT>
  UnaryPerPixelView<ViewT,ApplyPixelMask<typename UnmaskedPixelType<typename ViewT::pixel_type>::type> >
  apply_mask( ImageViewBase<ViewT> const& view, 
              typename UnmaskedPixelType<typename ViewT::pixel_type>::type const& value ) {
    typedef UnaryPerPixelView<ViewT,ApplyPixelMask<typename UnmaskedPixelType<typename ViewT::pixel_type>::type> > view_type;
    return view_type( view.impl(), ApplyPixelMask<typename UnmaskedPixelType<typename ViewT::pixel_type>::type>(value) );
  }

  // We overload the function rather than defaulting the value
  // argument to work around a compiler issue in MSVC 2005.
  template <class ViewT>
  UnaryPerPixelView<ViewT,ApplyPixelMask<typename UnmaskedPixelType<typename ViewT::pixel_type>::type> >
  apply_mask( ImageViewBase<ViewT> const& view ) {
    return apply_mask( view, typename UnmaskedPixelType<typename ViewT::pixel_type>::type() );
  }

  // Indicate that apply_mask is "reasonably fast" and should never
  // induce an extra rasterization step during prerasterization.
  template <class ViewT>
  struct IsMultiplyAccessible<UnaryPerPixelView<ViewT,ApplyPixelMask<typename UnmaskedPixelType<typename ViewT::pixel_type>::type> > >
    : public IsMultiplyAccessible<ViewT> {};

  // *******************************************************************
  /// copy_mask(view, mask)
  ///
  /// Copies a mask from one image to another.  
  ///
  template <class PixelT>
  class CopyPixelMask : public ReturnFixedType<typename MaskedPixelType<PixelT>::type> {
  public:
    template <class MaskPixelT>
    typename MaskedPixelType<PixelT>::type operator()( PixelT const& value, MaskPixelT const& mask ) const {
      typename MaskedPixelType<PixelT>::type result = value;
      if (is_transparent(mask)) result.invalidate();
      return result;
    }
  };

  template <class ViewT, class MaskViewT>
  BinaryPerPixelView<ViewT,MaskViewT,CopyPixelMask<typename ViewT::pixel_type> >
  copy_mask( ImageViewBase<ViewT> const& view,
             ImageViewBase<MaskViewT> const& mask_view ) {
    typedef BinaryPerPixelView<ViewT,MaskViewT,CopyPixelMask<typename ViewT::pixel_type> > view_type;
    return view_type( view.impl(), mask_view.impl(), CopyPixelMask<typename ViewT::pixel_type>() );
  }

  // Indicate that copy_mask is "reasonably fast" and should never
  // induce an extra rasterization step during prerasterization.
  template <class ViewT, class MaskViewT>
  struct IsMultiplyAccessible<BinaryPerPixelView<ViewT,MaskViewT,CopyPixelMask<typename ViewT::pixel_type> > >
    : public boost::mpl::and_<IsMultiplyAccessible<ViewT>,IsMultiplyAccessible<MaskViewT> >::type {};

  // *******************************************************************
  /// mask_to_alpha(view)
  ///
  /// Converts a mask channel to an alpha channel, generating an image that 
  /// is transparent wherever the data is masked.
  ///
  template <class PixelT>
  class MaskToAlpha : public ReturnFixedType<typename PixelWithAlpha<typename UnmaskedPixelType<PixelT>::type>::type> {
  public:
    typedef typename PixelWithAlpha<typename UnmaskedPixelType<PixelT>::type>::type result_type;
    result_type operator()( PixelT const& pixel ) const {
      if (is_transparent(pixel)) return result_type();
      else return result_type(pixel.child());
    }
  };

  template <class ViewT>
  UnaryPerPixelView<ViewT,MaskToAlpha<typename ViewT::pixel_type> >
  mask_to_alpha( ImageViewBase<ViewT> const& view ) {
    typedef UnaryPerPixelView<ViewT,MaskToAlpha<typename ViewT::pixel_type> > view_type;
    return view_type( view.impl(), MaskToAlpha<typename ViewT::pixel_type>() );
  }

  // Indicate that mask_to_alpha is "reasonably fast" and should never
  // induce an extra rasterization step during prerasterization.
  template <class ViewT>
  struct IsMultiplyAccessible<UnaryPerPixelView<ViewT,MaskToAlpha<typename ViewT::pixel_type> > >
    : public IsMultiplyAccessible<ViewT> {};


 // *******************************************************************
  /// EdgeMaskView
  ///
  /// Create an image with zero-valued (i.e. default contructor)
  /// pixels around the edges masked out.
  template <class ViewT>
  class EdgeMaskView : public ImageViewBase<EdgeMaskView<ViewT> >
  {
    ViewT m_view;
    //BlockCacheView<typename ViewT::pixel_type> m_view;
  
    // These vectors contain the indices of the first good pixel from
    // the edge of the image on each side.
    Vector<int> m_left, m_right;
    Vector<int> m_top, m_bottom;

    // Use the edge vectors to determine if a pixel is valid.  Note:
    // this check fails for non convex edge masks!
    inline bool valid(int32 i, int32 j) const {
      if (i > m_left[j] && i < m_right[j] && j > m_top[i] && j < m_bottom[i]) 
        return true;
      else
        return false;
    }

  public:  
    typedef typename ViewT::pixel_type orig_pixel_type;
    typedef typename boost::remove_cv<typename boost::remove_reference<orig_pixel_type>::type>::type unmasked_pixel_type;
    typedef PixelMask<unmasked_pixel_type> pixel_type;
    typedef PixelMask<unmasked_pixel_type> result_type;
    typedef ProceduralPixelAccessor<EdgeMaskView> pixel_accessor;
  
    // EdgeMaskView( ViewT const& view, 
    //               const ProgressCallback &progress_callback = ProgressCallback::dummy_instance() ) : 
    //   m_view(view, Vector2i(512,512) ) {
    EdgeMaskView( ViewT const& view, 
                  const ProgressCallback &progress_callback = ProgressCallback::dummy_instance() ) : 
      m_view(view) {
    
      m_left.set_size(view.rows());
      m_right.set_size(view.rows());

      for (int i = 0; i < view.rows(); ++i) {
        m_left[i] = 0;
        m_right[i] = view.cols();
      }      

      m_top.set_size(view.cols());
      m_bottom.set_size(view.cols());
      m_bottom = view.rows();

      for (int j = 0; j < view.cols(); ++j) {
        m_top[j] = 0;
        m_bottom[j] = view.rows();
      }

      // Scan over the image
      for (int j = 0; j < m_view.impl().rows(); ++j) {
        progress_callback.report_progress(float(j)/m_view.impl().rows()*0.5);
        // Search from the left side
        int i = 0;
        while ( i < m_view.impl().cols() && m_view.impl()(i,j) == unmasked_pixel_type() ) {
          m_left[j] = i;
          ++i;
        }
        
        // Search from the right side
        i = m_view.impl().cols() - 1;
        while ( i >= 0 && m_view.impl()(i,j) == unmasked_pixel_type()) {
          m_right[j] = i;
          --i;
        }
      }
      
      for (int i = 0; i < m_view.impl().cols(); ++i) {
        progress_callback.report_progress(0.5 + float(i)/m_view.impl().cols()*0.5);
        // Search from the top side of the image for black pixels
        int j = 0;
        while ( j < m_view.impl().rows() && m_view.impl()(i,j) == unmasked_pixel_type() ) {
          m_top[i] = j;
          ++j;
        }
        
        // Search from the right side of the image for black pixels 
        j = m_view.impl().rows() - 1;
        while ( j >= 0 && m_view.impl()(i,j) == unmasked_pixel_type() ) {
          m_bottom[i] = j;
          --j;
        }
      }
      progress_callback.report_finished();
    }

    inline int32 cols() const { return m_view.cols(); }
    inline int32 rows() const { return m_view.rows(); }
    inline int32 planes() const { return m_view.planes(); }

    inline pixel_accessor origin() const { return pixel_accessor(*this); }

    inline result_type operator()( int32 i, int32 j, int32 p=0 ) const { 
      if ( this->valid(i,j) ) 
        return pixel_type(m_view(i,j,p));
      else 
        return pixel_type();
    }
    
    /// \cond INTERNAL
    typedef EdgeMaskView<ViewT> prerasterize_type;
    inline prerasterize_type prerasterize( BBox2i const& bbox ) const { return *this; }
    template <class DestT> inline void rasterize( DestT const& dest, BBox2i const& bbox ) const { 
      vw::rasterize( prerasterize(bbox), dest, bbox ); 
    }
    /// \endcond
  };

  /// \cond INTERNAL
  template <class ViewT>
  struct IsMultiplyAccessible<EdgeMaskView<ViewT> > : public true_type {};
  /// \endcond

  /// edge_mask(view)
  ///
  /// Search from the edges of an image for the first "valid" pixels,
  /// masking invalid pixels along the way.  Unlike some other image
  /// views, the EdgeMaskView does a good portion of its work in its
  /// constructor, where it searches for valid/invalid pixels in the
  /// source view.  The results are efficiently stored in four "edge
  /// location" vectors (one for each side).
  ///
  /// XXX The following note currently appears to be false:
  /// Performance note: this algorithm stores the input view in an
  /// additional BlockCacheView<> since it scans over every pixel in
  /// the image both horizontally and vertically.  Be sure that your
  /// cache is large enough to store a full row or column of blocks!!
  template <class ViewT>
  EdgeMaskView<ViewT> edge_mask( ImageViewBase<ViewT> const& v, 
                                 const ProgressCallback &progress_callback = ProgressCallback::dummy_instance() ) {
    return EdgeMaskView<ViewT>( v.impl(), progress_callback );
  }
} // namespace vw

#endif // __VW_IMAGE_MASK_VIEWS_H__