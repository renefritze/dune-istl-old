#ifndef __DUNE_ISTLEXC_HH__
#define __DUNE_ISTLEXC_HH__

#include <stdlib.h>

#include "dune/common/exceptions.hh"

namespace Dune {
   
    /** @defgroup ISTL Iterative Solvers Template Library
		@addtogroup ISTL
		@{
     */

  //! derive error class from the base class in common
  class ISTLError : public Exception {};

 
  /** @} end documentation */

} // end namespace

#endif
