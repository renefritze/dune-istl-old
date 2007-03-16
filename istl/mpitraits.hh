// $Id$
#ifndef DUNE_MPITRAITS_HH
#define DUNE_MPITRAITS_HH

#if HAVE_MPI
#include"mpi.h"
#endif

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

#if HAVE_MPI

  // A Macro for defining traits for the primitive data types
#define ComposeMPITraits(p,m) \
  template<> \
  struct MPITraits<p>{ \
    static inline MPI_Datatype getType(){ \
      return m; \
    } \
  };
  
  ComposeMPITraits(char, MPI_CHAR);
  ComposeMPITraits(unsigned char,MPI_UNSIGNED_CHAR);
  ComposeMPITraits(short,MPI_SHORT);
  ComposeMPITraits(unsigned short,MPI_UNSIGNED_SHORT);
  ComposeMPITraits(int,MPI_INT);
  ComposeMPITraits(unsigned int,MPI_UNSIGNED);
  ComposeMPITraits(long,MPI_LONG);
  ComposeMPITraits(unsigned long,MPI_UNSIGNED_LONG);
  ComposeMPITraits(float,MPI_FLOAT);
  ComposeMPITraits(double,MPI_DOUBLE);
  ComposeMPITraits(long double,MPI_LONG_DOUBLE);

#undef ComposeMPITraits

  template<class K, int n> class FieldVector;
  
  template<class K, int n>
  struct MPITraits<FieldVector<K,n> >
  {
    static MPI_Datatype datatype;
    static MPI_Datatype vectortype;
    
    static inline MPI_Datatype getType()
    {
      if(datatype==MPI_DATATYPE_NULL){
	MPI_Type_contiguous(n, MPITraits<K>::getType(), &vectortype);
	MPI_Type_commit(&vectortype);
	FieldVector<K,n> fvector;
	MPI_Aint base;
	MPI_Aint displ;
	MPI_Address(&fvector, &base);
	MPI_Address(&(fvector[0]), &displ);
	displ -= base;
	int length[1]={1};
	    
	MPI_Type_struct(1, length, &displ, &vectortype, &datatype);
	MPI_Type_commit(&datatype);
      }
      return datatype;
    }
        
  };

  template<class K, int n>
  MPI_Datatype MPITraits<FieldVector<K,n> >::datatype = MPI_DATATYPE_NULL;
  template<class K, int n>
  MPI_Datatype MPITraits<FieldVector<K,n> >::vectortype = {MPI_DATATYPE_NULL};


  template<int k>
  class bigunsignedint;
  
  template<int k>
  struct MPITraits<bigunsignedint<k> >
  {
    static MPI_Datatype datatype;
    static MPI_Datatype vectortype;
    
    static inline MPI_Datatype getType()
    {
      if(datatype==0){
	MPI_Type_contiguous(bigunsignedint<k>::n, MPITraits<unsigned short>::getType(),
			    &vectortype);
	//MPI_Type_commit(&vectortype);
	bigunsignedint<k> data;
	MPI_Aint base;
	MPI_Aint displ;
	MPI_Address(&data, &base);
	MPI_Address(&(data.digit), &displ);
	displ -= base;
	int length[1]={1};	
	MPI_Type_struct(1, length, &displ, &vectortype, &datatype);
	MPI_Type_commit(&datatype);
      }
      return datatype;
    }
  };

  
  template<int k>
  MPI_Datatype MPITraits<bigunsignedint<k> >::datatype = MPI_DATATYPE_NULL;
  template<int k>
  MPI_Datatype MPITraits<bigunsignedint<k> >::vectortype = MPI_DATATYPE_NULL;

#endif

  /** @} */
}

#endif
