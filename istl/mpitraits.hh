#ifndef __MPITRAITS_HH__
#define __MPITRAITS_HH__

#include"mpi.h"

namespace Dune
{
   /**
   * @file
   * @brief Traits classes for mapping types onto MPI_Datatype.
   * @author Markus Blatt
   */
  /** @addtogroup ISTL_Comm
   *
   * @{
   */
  /**
   * @brief A traits class describing the mapping of types onto MPI_Datatypes.
   *
   * Specializations exist for the default types.
   * Specializations should provide a static method
   * <pre>
   * static MPI_Datatype getType();
   * </pre>
   */
  template<typename T>
  class MPITraits
  {};
  
  template<typename T, MPI_Datatype t>
  struct MPITraitsHelper
  {
    /**
     * brief Get the corresponding MPI Type.
     */
    inline static MPI_Datatype getType();
  };
  
  template<typename T, MPI_Datatype t>
  inline MPI_Datatype MPITraitsHelper<T,t>::getType()
  {
    return t;
  }
  
  template<> 
  struct MPITraits<char> 
    : MPITraitsHelper<char, MPI_CHAR>
  {}; 

  template<> 
  struct MPITraits<unsigned char> 
    : public MPITraitsHelper<unsigned char,MPI_UNSIGNED_CHAR>
  {};

  template<> 
  struct MPITraits<short>  
    : public MPITraitsHelper<short,MPI_SHORT>
  {};

  template<> 
  struct MPITraits<unsigned short> 
    : public MPITraitsHelper<unsigned short,MPI_UNSIGNED_SHORT>
  {};

  template<> 
  struct MPITraits<int> 
    : public MPITraitsHelper<int,MPI_INT>
  {};

  template<> 
  struct MPITraits<unsigned int> :
    public MPITraitsHelper<unsigned int,MPI_UNSIGNED>
  {};


  template<> 
  struct MPITraits<long> : 
    public MPITraitsHelper<long,MPI_LONG>
  {};

  
  template<> 
  struct MPITraits<unsigned long> 
    : public MPITraitsHelper<unsigned long,MPI_UNSIGNED_LONG>
  {};


  template<> 
  struct MPITraits<float> 
    : public MPITraitsHelper<float,MPI_FLOAT>
  {};
  
  

  template<> 
  struct MPITraits<double> 
    : public MPITraitsHelper<double,MPI_DOUBLE>
  {};
  

  
  template<> 
  struct MPITraits<long double> 
    : public MPITraitsHelper<long double,MPI_LONG_DOUBLE>
  {};

  /** @} */
}


#endif
