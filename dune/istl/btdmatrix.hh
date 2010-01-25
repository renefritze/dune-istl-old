#ifndef DUNE_BLOCK_TRIDIAGONAL_MATRIX_HH
#define DUNE_BLOCK_TRIDIAGONAL_MATRIX_HH

#include <dune/istl/bcrsmatrix.hh>

/** \file
    \author Oliver Sander
    \brief Implementation of the BTDMatrix class
*/

namespace Dune {
  /** 
   * @addtogroup ISTL_SPMV 
   * @{
   */
    /** \brief A block-tridiagonal matrix 

    \todo It would be safer and more efficient to have a real implementation of
    a block-tridiagonal matrix and not just subclassing from BCRSMatrix.  But that's
    quite a lot of work for that little advantage.*/
template <class B, class A=ISTLAllocator>
class BTDMatrix : public BCRSMatrix<B,A>
{
public:

    //===== type definitions and constants
    
    //! export the type representing the field
    typedef typename B::field_type field_type;
    
    //! export the type representing the components
    typedef B block_type;
    
    //! export the allocator type
    typedef A allocator_type;
    
    //! implement row_type with compressed vector
    //typedef BCRSMatrix<B,A>::row_type row_type;

    //! The type for the index access and the size
    typedef typename A::size_type size_type;

    //! increment block level counter
    enum {blocklevel = B::blocklevel+1};

    /** \brief Default constructor */
    BTDMatrix() : BCRSMatrix<B,A>() {}

    explicit BTDMatrix(int size) 
        : BCRSMatrix<B,A>(size, size, BCRSMatrix<B,A>::random) 
    {
        // Set number of entries for each row
        this->BCRSMatrix<B,A>::setrowsize(0, 2);

        for (int i=1; i<size-1; i++)
            this->BCRSMatrix<B,A>::setrowsize(i, 3);

        this->BCRSMatrix<B,A>::setrowsize(size-1, 2);

        this->BCRSMatrix<B,A>::endrowsizes();

        // The actual entries for each row
        this->BCRSMatrix<B,A>::addindex(0, 0);
        this->BCRSMatrix<B,A>::addindex(0, 1);

        for (int i=1; i<size-1; i++) {
            this->BCRSMatrix<B,A>::addindex(i, i-1);
            this->BCRSMatrix<B,A>::addindex(i, i  );
            this->BCRSMatrix<B,A>::addindex(i, i+1);
        }

        this->BCRSMatrix<B,A>::addindex(size-1, size-2);
        this->BCRSMatrix<B,A>::addindex(size-1, size-1);

        this->BCRSMatrix<B,A>::endindices();

    }

    //! assignment
    BTDMatrix& operator= (const BTDMatrix& other) {
        this->BCRSMatrix<B,A>::operator=(other);
        return *this;
    }

    //! assignment from scalar
    BTDMatrix& operator= (const field_type& k) {
        this->BCRSMatrix<B,A>::operator=(k);
        return *this;
    }

    /** \brief Use the Thomas algorithm to solve the system Ax=b
     *
     * \exception ISLTError if the matrix is singular
     *
     * \todo Implementation currently only works for scalar matrices
     */
    template <class V>
    void solve (V& x, const V& rhs) const {

        // Make copies of the rhs and the right matrix band
        V d = rhs;
        V c(this->N()-1);
        for (size_t i=0; i<this->N()-1; i++)
            c[i] = (*this)[i][i+1];

	/* Modify the coefficients. */
	c[0] /= (*this)[0][0];	/* Division by zero risk. */
	d[0] /= (*this)[0][0];	/* Division by zero would imply a singular matrix. */

	for (unsigned int i = 1; i < this->N(); i++) {

            double id = 1.0 / ((*this)[i][i] - c[i-1] * (*this)[i][i-1]);  /* Division by zero risk. */
            if (i<c.size())
                c[i] *= id;	                                       /* Last value calculated is redundant. */
            d[i] = (d[i] - d[i-1] * (*this)[i][i-1]) * id;

	}
 
	/* Now back substitute. */
	x[this->N() - 1] = d[this->N() - 1];
	for (int i = this->N() - 2; i >= 0; i--)
            x[i] = d[i] - c[i] * x[i + 1];

    }

private:	

    // ////////////////////////////////////////////////////////////////////////////
    //   The following methods from the base class should now actually be called
    // ////////////////////////////////////////////////////////////////////////////

    // createbegin and createend should be in there, too, but I can't get it to compile
    //     BCRSMatrix<B,A>::CreateIterator createbegin () {}
    //     BCRSMatrix<B,A>::CreateIterator createend () {}
    void setrowsize (size_type i, size_type s) {}
    void incrementrowsize (size_type i) {}
    void endrowsizes () {}
    void addindex (size_type row, size_type col) {}
    void endindices () {}
};
  /** @}*/

}  // end namespace Dune

#endif
