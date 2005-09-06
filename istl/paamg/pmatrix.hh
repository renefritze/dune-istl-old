#ifndef DUME_AMGPMATRIX_HH
#define DUNE_AMGPMATRIX_HH

#include<dune/common/helpertemplates.hh>
#include<dune/common/typetraits.hh>
#include"hierarchy.hh"

namespace Dune
{
  namespace Amg
  {
    
    /**
     * @brief Matrix together with information about the distrubution 
     * between the processes.
     *
     */
    template<class M, class IS, class RI>
    class ParallelMatrix
    {
    public:
      /** @brief The type of the matrix. */
      typedef M Matrix;
      /** @brief The type of the index set. */
      typedef IS IndexSet;
      /** @brief The type of the remote indices. */
      typedef RI RemoteIndices;
      
      ParallelMatrix(const Matrix& matrix, const IndexSet& indexSet,
		     const RemoteIndices& rindices)
	: matrix_(&matrix), indices_(&indexSet), rIndices_(&rindices)
      {
	IsTrue<SameType<IndexSet,typename RemoteIndices::IndexSet>::value>::yes();
      }
      

      /**
       * @brief Get the locally stored matrix.
       * @return The locally stored matrix.
       */
      const Matrix& matrix() const
      {
	return *matrix_;
      }
      /**
       * @brief Get the index set that maps global indices to matrix rows.
       *  @return The index set.
       */
      const IndexSet& indexSet() const
      {
	return *indices_;
      }
      
      /**
       * @brief Get the information about remote indices also present locally.
       * @return The remote index information.
       */
      const RemoteIndices& remoteIndices() const
      {
	return *rIndices_;
      }
      
    private:
      /** @brief The local part of the matrix. */
      const Matrix* matrix_;
      /** @brief The index set. */
      const IndexSet* indices_;
      /** @brief Remote index information. */
      const RemoteIndices* rIndices_;
    };

    template<class M, class IS, class RI>
    struct ParallelMatrixArgs
    {
      M& matrix;
      IS& indexSet;
      RI& remoteIndices;
      
      ParallelMatrixArgs(M& m, IS& is, RI& ri)
	: matrix(m), indexSet(is), remoteIndices(ri)
      {}
    };
    
    template<class T>
    class ConstructionTraits;
    
    template<class M, class IS, class RI>
    class ConstructionTraits<ParallelMatrix<M,IS,RI> >
    {
    public:
      typedef const ParallelMatrixArgs<M,IS,RI> Arguments;
      
      static inline ParallelMatrix<M,IS,RI>* construct(Arguments& args)
      {
	return new ParallelMatrix<M,IS,RI>(args.matrix, args.indexSet,
					   args.remoteIndices);
      }
    };
  }// end namespace Amg

}//end namespace Dune
#endif
