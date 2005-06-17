// $Id$
#ifndef DUNE_SOLVERCATEGORY_HH
#define DUNE_SOLVERCATEGORY_HH


namespace Dune {
   
    /**
		@addtogroup ISTL
		@{
     */

  struct SolverCategory
  { 
	enum { sequential, nonoverlapping, overlapping };
  };
 
  /** @} end documentation */

} // end namespace

#endif
