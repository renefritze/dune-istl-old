// $Id$
#ifndef DUNE_SOLVERCATEGORY_HH
#define DUNE_SOLVERCATEGORY_HH


namespace Dune {
   
  /**
     @addtogroup ISTL_Solvers
     @{
  */
  
  /**
   * @brief Categories for the solvers.
   */
  struct SolverCategory
  { 
    enum  Category { 
      //! \brief Category for sequential solvers.
      sequential,
      //! \brief Category for on overlapping solvers.
      nonoverlapping,
      //! \brief Category for ovelapping solvers.
      overlapping
    };
  };
 
  /** @} end documentation */

} // end namespace

#endif
