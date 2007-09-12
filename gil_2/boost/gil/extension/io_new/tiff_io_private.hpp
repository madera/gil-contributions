/*
  Copyright 2005-2007 Adobe Systems Incorporated
  Distributed under the MIT License (see accompanying file LICENSE_1_0_0.txt
  or a copy at http://opensource.adobe.com/licenses.html)
*/

/*************************************************************************************************/

#ifndef TIFF_IO_PRIVATE_HPP
#define TIFF_IO_PRIVATE_HPP

/// \file
/// \brief  Support for reading and writing TIFF files
//
/// \author Christian Henning
///         

/// see http://www.remotesensing.org/libtiff/
/// or http://www.libtiff.org/man/


#include <fstream>
#include <algorithm>
#include <string>

#include <boost/static_assert.hpp>

#include <boost/shared_ptr.hpp>

#include <boost/gil/extension/io_new/tiff_base.hpp>
#include <boost/gil/extension/io_new/tiff_supported_read_types.hpp>
#include <boost/gil/extension/io_new/tiff_io_private_write.hpp>

namespace boost { namespace gil { namespace detail {

template <typename Property>
bool get_property( const std::string& file_name
                 , typename Property::type& value
                 , tiff_tag                        )
{
   return get_property<Property>( tiff_open_for_read( file_name )
                                , value                           );

}

void read_image_info( tiff_file_t                 file
                    , basic_tiff_image_read_info& info )
{
   get_property<tiff_image_width>( file
                                 , info._width );

   get_property<tiff_image_height>( file
                                 , info._height );

   get_property<tiff_compression>( file
                                 , info._compression );

   get_property<tiff_samples_per_pixel>( file
                                       , info._samples_per_pixel );

   get_property<tiff_bits_per_sample>( file
                                     , info._bits_per_sample );

   get_property<tiff_sample_format>( file
                                   , info._sample_format );

   get_property<tiff_planar_configuration>( file
                                          , info._planar_configuration );

   get_property<tiff_photometric_interpretation>( file
                                                , info._photometric_interpretation  );
}

template< typename SrcImageIsPlanar >
struct read_and_no_convert_impl
{
   template< typename Image_TIFF
           , typename View_User
           >
   void operator() ( const View_User&                  src_view
                   , const point_t&                    top_left
                   , const basic_tiff_image_read_info& info
                   , tiff_file_t                       file      ) {}

};

// Specialization for reading interleaved images.
template<>
struct read_and_no_convert_impl< boost::mpl::bool_< false > >
{
   template< typename Image_TIFF
           , typename View_User
           >
   void operator() ( const View_User&                  src_view
                   , const point_t&                    top_left
                   , const basic_tiff_image_read_info& info
                   , tiff_file_t                       file      )
   {
      io_error_if( views_are_compatible< View_User
                                       , typename Image_TIFF::view_t >::value != true
                 , "Views are incompatible. You might want to use read_and_convert_image()" );

      read_data( src_view, top_left, 0, info, file );
   }
};


template < int K >
struct plane_recursion
{
   template< typename View >
   static void read_plane( const View&                       src_view
                         , const point_t&                    top_left
                         , const basic_tiff_image_read_info& info
                         , tiff_file_t                       file      )
   {
      typedef kth_channel_view_type< K, typename View >::type plane_t;
      plane_t plane = kth_channel_view<K>( src_view );
      read_data( plane, top_left, K, info, file );

      plane_recursion< K - 1 >::read_plane( src_view, top_left, info, file );
   }
};

template <>
struct plane_recursion< 0 >
{
   template< typename View >
   static void read_plane( const View&                       src_view
                         , const point_t&                    top_left
                         , const basic_tiff_image_read_info& info
                         , tiff_file_t                       file      )
   {
      typedef kth_channel_view_type< 0, typename View >::type plane_t;
      plane_t plane = kth_channel_view<0>( src_view );

      read_data( plane, top_left, 0, info, file );
   }
};

// Specialization for reading planar images.
template<>
struct read_and_no_convert_impl< boost::mpl::bool_< true > >
{
   template< typename Image_TIFF
           , typename View_User
           >
   void operator() ( const View_User&                  src_view
                   , const point_t&                    top_left
                   , const basic_tiff_image_read_info& info
                   , tiff_file_t                       file      )
   {
      typedef typename Image_TIFF::view_t      view_tiff_t;
      typedef typename view_tiff_t::value_type pixel_tiff_t;

      io_error_if( views_are_compatible<View_User, view_tiff_t>::value != true
                 , "Views are incompatible. You might want to use read_and_convert_image()" );

      plane_recursion< num_channels< View_User >::value - 1 >::read_plane( src_view, top_left, info, file );
   }
};

struct read_and_no_convert
{
   read_and_no_convert( tiff_file_t file )
   : _file( file )
   {}

   // To be called by when image type is not_allowed_t.
   template< typename Image_TIFF
           , typename View_User
           >
   void read( const View_User&                  src_view
            , const point_t&                    top_left
            , const basic_tiff_image_read_info& info
            , boost::mpl::true_                            ) {}

   // To be called by real image types.
   template< typename Image_TIFF
           , typename View_User
           >
   void read( const View_User&                  src_view
            , const point_t&                    top_left
            , const basic_tiff_image_read_info& info
            , boost::mpl::false_                          )
   {
      typedef typename Image_TIFF::view_t View_TIFF;

      typedef is_planar< View_TIFF > src_image_is_planar_t;

      read_and_no_convert_impl< src_image_is_planar_t > impl;
      impl.operator()< Image_TIFF >( src_view, top_left, info, _file );
   }

protected:

   tiff_file_t _file;
};


template< typename SrcImageIsPlanar >
struct read_and_convert_impl
{
   template< typename Image_TIFF
           , typename View_User
           , typename Color_Converter
           >
   void operator() ( const View_User&                  src_view
                   , const point_t&                    top_left
                   , Color_Converter                   cc
                   , const basic_tiff_image_read_info& info
                   , tiff_file_t                       file      ) {}
};

// Specialization for reading and converting interleaved images.
template<>
struct read_and_convert_impl< boost::mpl::bool_< false > >
{
   template< typename Image_TIFF
           , typename View_User
           , typename Color_Converter
           >
   void operator() ( const View_User&                  src_view
                   , const point_t&                    top_left
                   , Color_Converter                   cc
                   , const basic_tiff_image_read_info& info
                   , tiff_file_t                       file       )
   {
      read_interleaved_data_and_convert< Image_TIFF >( src_view, top_left, cc, info, file );
   }
};

// Specialization for reading and converting planar images.
template<>
struct read_and_convert_impl< boost::mpl::bool_< true > >
{
   template< typename Image_TIFF
           , typename View_User
           , typename Color_Converter
           >
   void operator() ( const View_User&                  src_view
                   , const point_t&                    top_left
                   , Color_Converter                   cc
                   , const basic_tiff_image_read_info& info
                   , tiff_file_t                       file      )
   {
      typedef typename Image_TIFF::view_t      view_tiff_t;
      typedef typename view_tiff_t::value_type pixel_tiff_t;

      Image_TIFF tiff_img( src_view.dimensions() );

      plane_recursion< num_channels< view_tiff_t >::value - 1 >::read_plane( view( tiff_img ), top_left, info, file );

      transform_pixels( view( tiff_img )
                      , src_view
                      , color_convert_deref_fn< typename view_tiff_t::value_type
                                              , typename View_User::value_type
                                              , Color_Converter
                                              >( cc ));
   }
};


template< typename Color_Converter >
struct read_and_convert
{
   read_and_convert( tiff_file_t file )
   : _file( file )
   {}

   void set_color_converter( const Color_Converter& cc )
   {
      _cc = cc;
   }

   // To be called by when image type is not_allowed_t.
   template< typename Image_TIFF
           , typename View_User
           >
   void read( const View_User&                  src_view
            , const point_t&                    top_left
            , const basic_tiff_image_read_info& info
            , boost::mpl::true_                          ) {}

   // To be called by real image types.
   template< typename Image_TIFF
           , typename View_User
           >
   void read( const View_User&                  src_view
            , const point_t&                    top_left
            , const basic_tiff_image_read_info& info
            , boost::mpl::false_                         )
   {
      typedef typename Image_TIFF::view_t View_TIFF;

      typedef is_planar< View_TIFF > src_image_is_planar_t;

      read_and_convert_impl< src_image_is_planar_t > impl;
      impl.operator()< Image_TIFF >( src_view, top_left, _cc, info, _file );
   }

protected:

   tiff_file_t _file;

   Color_Converter _cc;
};

template< typename Reader >
class tiff_reader : public Reader
{
public:

   tiff_reader( tiff_file_t file )
   : Reader( file )
   {
      read_image_info( file, _info );
   }

   template< typename Image >
   void read_image( Image&         src_img
                  , const point_t& top_left )
   {
      src_img.recreate( _info._width  - top_left.x
                      , _info._height - top_left.y );

      _top_left = top_left;

      _apply( view( src_img ));
   }

   template< typename View >
   void read_view( const View&    src_view
                 , const point_t& top_left )
   {

      io_error_if( src_view.dimensions() != point_t( _info._width  - top_left.x
                                                   , _info._height - top_left.y )
                 , "User provided view has incorrect size."                       );
      
      _top_left = top_left;

      _apply( src_view );
   }


private:

   template< typename View >
   void _apply( const View& src_view )
   {
      if( _info._photometric_interpretation == PHOTOMETRIC_PALETTE )
      {
         // Steps:
         // 1. Read indices. It's an array of grayX_pixel_t.
         // 2. Read palette. It's an array of rgb16_pixel_t.
         // 3. ??? Create virtual image or transform the two arrays
         //    into a rgb16_image_t object. The latter might
         //    be a good first solution.

         unsigned int num_colors = 0;
         switch( _info._bits_per_sample )
         {
            case 1:
            {
               gray1_image_t indices( _info._width  - _top_left.x
                                    , _info._height - _top_left.y );
               _read_samples_per_pixel( view( indices ));

               read_palette_image( src_view
                                 , view( indices )
                                 , _info
                                 , _file
                                 , boost::is_same< View, rgb16_view_t >::type() );
               break;
            }

            case 2:
            {
               gray2_image_t indices( _info._width  - _top_left.x
                                    , _info._height - _top_left.y );
               _read_samples_per_pixel( view( indices ));

               read_palette_image( src_view
                                 , view( indices )
                                 , _info
                                 , _file
                                 , boost::is_same< View, rgb16_view_t >::type() );
               break;
            }

            case 4:
            {
               gray4_image_t indices( _info._width  - _top_left.x
                                    , _info._height - _top_left.y );
               _read_samples_per_pixel( view( indices ));

               read_palette_image( src_view
                                 , view( indices )
                                 , _info
                                 , _file
                                 , boost::is_same< View, rgb16_view_t >::type() );
               break;
            }

            case 8:
            {
               gray8_image_t indices( _info._width  - _top_left.x
                                    , _info._height - _top_left.y );
               _read_samples_per_pixel( view( indices ));

               read_palette_image( src_view
                                 , view( indices )
                                 , _info
                                 , _file
                                 , boost::is_same< View, rgb16_view_t >::type() );
               break;
            }

            case 16:
            {
               gray16_image_t indices( _info._width  - _top_left.x
                                     , _info._height - _top_left.y );
               _read_samples_per_pixel( view( indices ));

               read_palette_image( src_view
                                 , view( indices )
                                 , _info
                                 , _file
                                 , boost::is_same< View, rgb16_view_t >::type() );
               break;
            }

            default:
            {
               io_error( "Not supported palette " );
            }
         }

         return;
      }

      _read_samples_per_pixel( src_view );
   }

   template< typename View >
   void _read_samples_per_pixel( const View& src_view )
   {
      switch( _info._samples_per_pixel )
      {
         case 1:
         {
            _read_bits_per_sample< gray_layout_t >( src_view );
            break;
         }

         case 3:
         {
            _read_bits_per_sample< rgb_layout_t >( src_view );
            break;
         }

         case 4:
         {
            _read_bits_per_sample< rgba_layout_t >( src_view );
            break;
         }

         default:
         {
            io_error( "Samples_Per_Pixel not supported." );
         }
      }
   }

   template< typename Layout
           , typename View
           >
   void _read_bits_per_sample( const View& src_view )
   {
      switch( _info._bits_per_sample )
      {
         case 1:
         {
            _read_sample_format< 1, Layout >( src_view );
            break;
         }

         case 4:
         {
            _read_sample_format< 4, Layout >( src_view );
            break;
         }

         case 8:
         {
            _read_sample_format< 8, Layout >( src_view );
            break;
         }

         case 16:
         {
            _read_sample_format< 16, Layout >( src_view );
            break;
         }

         case 32:
         {
            _read_sample_format< 32, Layout >( src_view );
            break;
         }

         case 64:
         {
            _read_sample_format< 64, Layout >( src_view );
            break;
         }

         default:
         {
            io_error( "Bits_Per_Sample not supported." );
         }
      }
   }

   template< int      BitsPerSample
           , typename Layout
           , typename View
           >
   void _read_sample_format( const View& src_view )
   {
      switch( _info._sample_format )
      {
         case SAMPLEFORMAT_UINT:
         {
            _read_planar< BitsPerSample, SAMPLEFORMAT_UINT, Layout >( src_view );
            break;
         }

         case SAMPLEFORMAT_INT:
         {
            _read_planar< BitsPerSample, SAMPLEFORMAT_INT, Layout >( src_view );
            break;
         }

         case SAMPLEFORMAT_IEEEFP:
         {
            _read_planar< BitsPerSample, SAMPLEFORMAT_IEEEFP, Layout >( src_view );
            break;
         }
         
         default:
         {
            io_error( "Sample format not supported." );
         }
      }
   }

   template< int      BitsPerSample
           , int      SampleFormat
           , typename Layout
           , typename View
           >
   void _read_planar( const View& src_view )
   {
      typedef pixel_type_factory< BitsPerSample, SampleFormat, Layout >::type pixel_t;

      if( _info._planar_configuration == PLANARCONFIG_CONTIG )
      {
         typedef image_type_factory< pixel_t, false >::type image_t;

         read< image_t >( src_view
                        , _top_left
                        , _info
                        , is_same< pixel_t, not_allowed_t >::type() );

      }
      else if( _info._planar_configuration == PLANARCONFIG_SEPARATE )
      {
         typedef image_type_factory< pixel_t, true >::type image_t;

         typedef mpl::or_< is_same< pixel_t, not_allowed_t >::type
                         , is_same< image_t, not_allowed_t >::type
                         >::type unspecified_t;

         read< image_t >( src_view
                        , _top_left
                        , _info
                        , unspecified_t() );
      }
      else
      {
         io_error( "Wrong planar configuration setting." );
      }
   }

private:

   point_t _top_left;

   basic_tiff_image_read_info _info;
};

} // detail
} // namespace gil
} // namespace boost

#endif // TIFF_IO_PRIVATE_HPP
