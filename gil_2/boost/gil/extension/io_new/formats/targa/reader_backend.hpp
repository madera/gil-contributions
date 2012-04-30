/*
    Copyright 2012 Christian Henning
    Use, modification and distribution are subject to the Boost Software License,
    Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt).
*/

/*************************************************************************************************/

#ifndef BOOST_GIL_EXTENSION_IO_TARGA_IO_BACKEND_HPP
#define BOOST_GIL_EXTENSION_IO_TARGA_IO_BACKEND_HPP

////////////////////////////////////////////////////////////////////////////////////////
/// \file
/// \brief
/// \author Christian Henning \n
///
/// \date 2012 \n
///
////////////////////////////////////////////////////////////////////////////////////////

#include <boost/gil/extension/io_new/targa_tags.hpp>

namespace boost { namespace gil {

///
/// Targa Backend
///
template< typename Device >
struct reader_backend< Device
                     , targa_tag
                     >
{
    reader_backend( Device&                                 device
                  , const image_read_settings< targa_tag >& settings
                  )
    : _io_dev  ( device   )
    , _settings( settings )
    , _info()
    , _scanline_length(0)
    {
        read_header();    
    }

    void read_header()
    {
        _info._header_size = targa_header_size::_size;
        
        _info._offset = _io_dev.read_uint8() + _info._header_size;
        
        _info._color_map_type = _io_dev.read_uint8();
        if( _info._color_map_type != targa_color_map_type::_rgb )
        {
            io_error( "scanline reader cannot read indexed targa files." );
        }
        
        _info._image_type = _io_dev.read_uint8();
        if( _info._image_type != targa_image_type::_rgb )
        {
            io_error( "scanline reader cannot read this targa image type." );
        }
        
        _info._color_map_start  = _io_dev.read_uint16();
        _info._color_map_length = _io_dev.read_uint16();
        _info._color_map_depth  = _io_dev.read_uint8();
        
        _info._x_origin = _io_dev.read_uint16();
        _info._y_origin = _io_dev.read_uint16();
        
        _info._width  = _io_dev.read_uint16();
        _info._height = _io_dev.read_uint16();

        if( _info._width < 1 || _info._height < 1 )
        {
            io_error( "Invalid dimension for targa file" );
        }
        
        _info._bits_per_pixel = _io_dev.read_uint8();
        if( _info._bits_per_pixel != 24 && _info._bits_per_pixel != 32 )
        {
            io_error( "Unsupported bit depth for targa file" );
        }
        
        _info._descriptor = _io_dev.read_uint8();
        if(    ( _info._bits_per_pixel == 24 && _info._descriptor != 0 ) 
            || ( _info._bits_per_pixel == 32 && _info._descriptor != 8 )
          )
        {
            io_error( "Unsupported descriptor for targa file" );
        }
        
        _info._valid = true;
    }


public:

    Device _io_dev;

    std::size_t _scanline_length;

    image_read_settings< targa_tag > _settings;
    image_read_info< targa_tag >     _info;
};


} // namespace gil
} // namespace boost

#endif // BOOST_GIL_EXTENSION_IO_TARGA_IO_BACKEND_HPP
