#ifndef __DUNE_ISTLEXC_HH__
#define __DUNE_ISTLEXC_HH__

#include <stdlib.h>

#include "../../common/exceptions.hh"

namespace Dune {
   
    /** 
		@addtogroup ISTL
		@{
     */

  //! derive error class from the base class in common
  class ISTLError : public Exception {};

 
  /** @} end documentation */

} // end namespace

#endif
