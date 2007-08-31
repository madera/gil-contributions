/*
  Copyright 2005-2007 Adobe Systems Incorporated
  Distributed under the MIT License (see accompanying file LICENSE_1_0_0.txt
  or a copy at http://opensource.adobe.com/licenses.html)
*/

/*************************************************************************************************/

#ifndef BASE_HPP
#define BASE_HPP

/// \file
/// \brief  Defining some basic data types.
//
/// \author Christian Henning
///         

#include <boost/type_traits/is_base_of.hpp>

#include <boost/gil/utilities.hpp>

namespace boost { namespace gil {

typedef boost::gil::point2< std::ptrdiff_t > point_t;

namespace detail {

typedef bit_aligned_image1_type< 1, gray_layout_t >::type gray1_image_t;
typedef gray1_image_t::view_t gray1_view_t;

inline void io_error( const std::string& descr )
{
   throw std::ios_base::failure( descr );
}

inline void io_error_if( bool expr, const std::string& descr )
{
   if( expr ) 
      io_error( descr );
}

template< typename View >
struct is_bit_aligned_impl : public boost::mpl::bool_< byte_to_memunit< typename View::x_iterator >::value == 8 >
{};

template< typename View >
struct is_bit_aligned
{
   static const value = is_base_of< boost::mpl::true_, is_bit_aligned_impl< View > >::value;

   typedef boost::mpl::bool_< boost::is_base_of< boost::mpl::true_
                                               , is_bit_aligned_impl< View > >::value > type;
};

std::string convert_to_string( const std::wstring& s )
{
	std::size_t len = wcslen( s.c_str() );
	char* c = reinterpret_cast<char*>( alloca( len ));
	wcstombs( c, s.c_str(), len );

   return std::string( c, c + len );
}

inline unsigned char swap_bits( unsigned char c )
{
   unsigned char result = 0;
   for( int i = 0; i < 8; ++i )
   {
      result = result << 1;
      result |= ( c & 1 );
      c = c >> 1;
   }

   return result;
}

} // namespace details
} // namespace gil
} // namespace boost

#endif // BASE_HPP
