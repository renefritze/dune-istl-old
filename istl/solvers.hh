#ifndef DUNE_SOLVERS_HH
#define DUNE_SOLVERS_HH

#include<math.h>
#include<complex>
#include<iostream>
#include<iomanip>
#include<string>
#include<stdio.h> // there is nothing better than printf

#include "istlexception.hh"
#include "operators.hh"
#include "preconditioners.hh"
#include "scalarproducts.hh"
#include "dune/common/timer.hh"
#include "dune/common/helpertemplates.hh"

/** \file

 \brief   Define general, extensible interface for inverse operators.

	Implementation here covers only inversion of linear operators,
	but the implementation might be used for nonlinear operators
    as well.
*/

namespace Dune {
   
  /** @addtogroup ISTL
	  @{
  */


  /** \brief Statistics about the application of an inverse operator

  The return value of an application of the inverse
      operator delivers some important information about 
	  the iteration.
  */
  struct InverseOperatorResult
  {
      /** \brief Default constructor */
	InverseOperatorResult ()
	{
	  clear();
	}

      /** \brief Resets all data */
	void clear ()
	{
	  iterations = 0;
	  reduction = 0;
	  converged = false;
	  conv_rate = 1;
	  elapsed = 0;
	}

      /** \brief Number of iterations */
	int iterations;	

      /** \brief Reduction achieved: \f$ \|b-A(x^n)\|/\|b-A(x^0)\|\f$ */
	double reduction;

      /** \brief True if convergence criterion has been met */
	bool converged;	

      /** \brief Convergence rate (average reduction per step) */
	double conv_rate;

      /** \brief Elapsed time in seconds */
	double elapsed;
  };


  //=====================================================================
  /*! An InverseOperator computes the solution of \f$ A(x)=b\f$ where
      \f$ A : X \to Y \f$ is an operator.
      Note that the solver "knows" which operator
      to invert and which preconditioner to apply (if any). The
      user is only interested in inverting the operator.
	  InverseOperator might be a Newton scheme, a Krylov subspace method,
      or a direct solver or just anything. 
  */
  //=====================================================================
  template<class X, class Y>
  class InverseOperator {
  public:
	//! export types, usually they come from the derived class
	typedef X domain_type;

      /** \todo Please doc me! */
	typedef Y range_type;
      /** \todo Please doc me! */
	typedef typename X::field_type field_type;

	//! apply inverse operator, Note: right hand side b may be overwritten!
	virtual void apply (X& x, Y& b, InverseOperatorResult& r) = 0;

	//! apply inverse operator, with given convergence criteria, right hand side b may be overwritten!
	virtual void apply (X& x, Y& b, double reduction, InverseOperatorResult& r) = 0;

	//! the usual thing
	virtual ~InverseOperator () {}
  };


  //=====================================================================
  // Implementation of this interface
  //=====================================================================

  /*! Implements a preconditioned loop. 

	  Verbose levels are:
          <ul>
	  <li> 0 : print nothing </li>
	  <li> 1 : print initial and final defect and statistics </li>
	  <li> 2 : print line for each iteration </li>
          </ul>
   */
  template<class X>
  class LoopSolver : public InverseOperator<X,X> {
  public:
	//! export types, usually they come from the derived class
	typedef X domain_type;
	typedef X range_type;
	typedef typename X::field_type field_type;

	//! set up Loop solver
	template<class L, class P>
	LoopSolver (L& op, P& prec,
				double reduction, int maxit, int verbose) : 
	  ssp(), _op(op), _prec(prec), _sp(ssp), _reduction(reduction), _maxit(maxit), _verbose(verbose)
	{
	  IsTrue< static_cast<int>(L::category) == static_cast<int>(P::category) >::yes();
	  IsTrue< static_cast<int>(L::category) == static_cast<int>(SolverCategory::sequential) >::yes();
	}

      /** \brief Set up loop solver

      \param P The preconditioner type

      \param prec A preconditioner object
      \param maxit Maximum number of iterations
      \param verbose Verbosity level
      */
	template<class L, class S, class P>
	LoopSolver (L& op, S& sp, P& prec,
				double reduction, int maxit, int verbose) : 
	  _op(op), _sp(sp), _prec(prec), _reduction(reduction), _maxit(maxit), _verbose(verbose)
	{
	  IsTrue< static_cast<int>(L::category) == static_cast<int>(P::category) >::yes();
	  IsTrue< static_cast<int>(L::category) == static_cast<int>(S::category) >::yes();
	}


	//! apply inverse operator
	virtual void apply (X& x, X& b, InverseOperatorResult& r)
	{
	  // clear solver statistics
	  r.clear();

	  // start a timer
	  Timer watch;

	  // prepare preconditioner
	  _prec.pre(x,b);

	  // overwrite b with defect
	  _op.applyscaleadd(-1,x,b);

	  // compute norm, \todo parallelization
	  double def0 = _sp.norm(b);

	  // printing
	  if (_verbose>0)
		{
		  printf("=== LoopSolver\n");
		  if (_verbose>1) printf(" Iter       Defect         Rate\n");
		  if (_verbose>1) printf("%5d %12.4E\n",0,def0);
		}

	  // allocate correction vector
	  X v(x);

	  // iteration loop
	  int i=1; double def=def0;
	  for ( ; i<=_maxit; i++ )	
		{		  
		  v = 0;                      // clear correction
		  _prec.apply(v,b);           // apply preconditioner
		  x += v;                     // update solution
		  _op.applyscaleadd(-1,v,b);  // update defect
		  double defnew=_sp.norm(b);// comp defect norm
		  if (_verbose>1)             // print
			printf("%5d %12.4E %12.4g\n",i,defnew,defnew/def);
		  def = defnew;               // update norm
		  if (def<def0*_reduction || def<1E-30)    // convergence check	
			{
			  r.converged  = true;
			  break;
			}
		}

	  // print
	  if (_verbose==1)
		printf("%5d %12.4E\n",i,def);
	
	  // postprocess preconditioner
	  _prec.post(x);

	  // fill statistics
	  r.iterations = i;
	  r.reduction = def/def0;
	  r.conv_rate  = pow(r.reduction,1.0/i);
	  r.elapsed = watch.elapsed();

	  // final print
	  if (_verbose>0) 
		printf("=== rate=%g, T=%g, TIT=%g\n",r.conv_rate,r.elapsed,r.elapsed/i);
	}

	//! apply inverse operator, with given reduction factor
	virtual void apply (X& x, X& b, double reduction, InverseOperatorResult& r)
	{
	  _reduction = reduction;
	  (*this).apply(x,b,r);
	}

  private:
	SeqScalarProduct<X> ssp;
	LinearOperator<X,X>& _op;
	Preconditioner<X,X>& _prec;
	ScalarProduct<X>& _sp;
	double _reduction;
	int _maxit;
	int _verbose; 	
  };


  // all these solvers are taken from the SUMO library
  //! gradient method
  template<class X>
  class GradientSolver : public InverseOperator<X,X> {
  public:
	//! export types, usually they come from the derived class
	typedef X domain_type;
	typedef X range_type;
	typedef typename X::field_type field_type;

	//! set up Loop solver
	template<class L, class P>
	GradientSolver (L& op, P& prec,
				double reduction, int maxit, int verbose) : 
	  ssp(), _op(op), _prec(prec), _sp(ssp), _reduction(reduction), _maxit(maxit), _verbose(verbose)
	{
	  IsTrue< static_cast<int>(L::category) == static_cast<int>(P::category) >::yes();
	  IsTrue< static_cast<int>(L::category) == static_cast<int>(SolverCategory::sequential) >::yes();
	}
	template<class L, class S, class P>
	GradientSolver (L& op, S& sp, P& prec,
				double reduction, int maxit, int verbose) : 
	  _op(op), _sp(sp), _prec(prec), _reduction(reduction), _maxit(maxit), _verbose(verbose)
	{
	  IsTrue< static_cast<int>(L::category) == static_cast<int>(P::category) >::yes();
	  IsTrue< static_cast<int>(L::category) == static_cast<int>(S::category) >::yes();
	}

	//! apply inverse operator
	virtual void apply (X& x, X& b, InverseOperatorResult& r)
	{
   	  r.clear();                  // clear solver statistics
	  Timer watch;                // start a timer
	  _prec.pre(x,b);             // prepare preconditioner
	  _op.applyscaleadd(-1,x,b);  // overwrite b with defect
  
	  X p(x);                     // create local vectors
	  X q(b);
	
   	  double def0 = _sp.norm(b);// compute norm

	  if (_verbose>0)             // printing
		{
		  printf("=== GradientSolver\n");
		  if (_verbose>1) printf(" Iter       Defect         Rate\n");
		  if (_verbose>1) printf("%5d %12.4E\n",0,def0);
		}

	  int i=1; double def=def0;   // loop variables
	  field_type lambda;
	  for ( ; i<=_maxit; i++ )	
		{
		  p = 0;                      // clear correction
		  _prec.apply(p,b);           // apply preconditioner
		  _op.apply(p,q);             // q=Ap
		  lambda = _sp.dot(p,b)/_sp.dot(q,p);// minimization
		  x.axpy(lambda,p);           // update solution
		  b.axpy(-lambda,q);          // update defect

		  double defnew=_sp.norm(b);// comp defect norm
		  if (_verbose>1)             // print
			printf("%5d %12.4E %12.4g\n",i,defnew,defnew/def);
		  def = defnew;               // update norm
		  if (def<def0*_reduction || def<1E-30)    // convergence check	
			{
			  r.converged  = true;
			  break;
			}
		}

	  if (_verbose==1)                // printing for non verbose
		printf("%5d %12.4E\n",i,def);
	  _prec.post(x);                  // postprocess preconditioner
	  r.iterations = i;               // fill statistics
	  r.reduction = def/def0;
	  r.conv_rate  = pow(r.reduction,1.0/i);
	  r.elapsed = watch.elapsed();
	  if (_verbose>0)                 // final print 
		printf("=== rate=%g, T=%g, TIT=%g\n",r.conv_rate,r.elapsed,r.elapsed/i);
	}

	//! apply inverse operator, with given reduction factor
	virtual void apply (X& x, X& b, double reduction, InverseOperatorResult& r)
	{
	  _reduction = reduction;
	  (*this).apply(x,b,r);
	}

  private:
	SeqScalarProduct<X> ssp;
	LinearOperator<X,X>& _op;
	Preconditioner<X,X>& _prec;
	ScalarProduct<X>& _sp;
	double _reduction;
	int _maxit;
	int _verbose; 	
  };



  //! conjugate gradient method
  template<class X>
  class CGSolver : public InverseOperator<X,X> {
  public:
	//! export types, usually they come from the derived class
	typedef X domain_type;
	typedef X range_type;
	typedef typename X::field_type field_type;

	//! set up Loop solver
	template<class L, class P>
	CGSolver (L& op, P& prec, double reduction, int maxit, int verbose) : 
	  ssp(), _op(op), _prec(prec), _sp(ssp), _reduction(reduction), _maxit(maxit), _verbose(verbose)
	{
	  IsTrue< static_cast<int>(L::category) == static_cast<int>(P::category) >::yes();
	  IsTrue< static_cast<int>(L::category) == static_cast<int>(SolverCategory::sequential) >::yes();
	}
	template<class L, class S, class P>
	CGSolver (L& op, S& sp, P& prec, double reduction, int maxit, int verbose) : 
	  _op(op), _sp(sp), _prec(prec), _reduction(reduction), _maxit(maxit), _verbose(verbose)
	{
	  IsTrue< static_cast<int>(L::category) == static_cast<int>(P::category) >::yes();
	  IsTrue< static_cast<int>(L::category) == static_cast<int>(S::category) >::yes();
	}

	//! Apply inverse operator
	virtual void apply (X& x, X& b, InverseOperatorResult& r)
	{
   	  r.clear();                  // clear solver statistics
	  Timer watch;                // start a timer
	  _prec.pre(x,b);             // prepare preconditioner
	  _op.applyscaleadd(-1,x,b);  // overwrite b with defect
  
	  X p(x);                     // create local vectors
	  X q(b);
	
   	  double def0 = _sp.norm(b);// compute norm
	  if (def0<1E-30)    // convergence check	
		{
		  r.converged  = true;
		  r.iterations = 0;               // fill statistics
		  r.reduction = 0;
		  r.conv_rate  = 0;
		  r.elapsed=0;
		  if (_verbose>0)                 // final print 
			printf("=== rate=%g, T=%g, TIT=%g\n",r.conv_rate,r.elapsed,r.elapsed);
		  return;
		}

	  if (_verbose>0)             // printing
		{
		  printf("=== CGSolver\n");
		  if (_verbose>1) printf(" Iter       Defect         Rate\n");
		  if (_verbose>1) printf("%5d %12.4E\n",0,def0);
		}

	  int i=1; double def=def0;   // loop variables
	  field_type rho,rholast,lambda;
	  for ( ; i<=_maxit; i++ )	
		{
		  p = 0;                      // clear correction
		  _prec.apply(p,b);           // apply preconditioner
		  rho = _sp.dot(p,b);       // orthogonalization
		  if (i>1) p.axpy(rho/rholast,q);
		  rholast = rho;              // remember rho for recurrence
		  _op.apply(p,q);             // q=Ap
		  lambda = rho/_sp.dot(q,p);// minimization
		  x.axpy(lambda,p);           // update solution
		  b.axpy(-lambda,q);          // update defect
		  q=p;                        // remember search direction
		  double defnew=_sp.norm(b);// comp defect norm
		  if (_verbose>1)             // print
			printf("%5d %12.4E %12.4g\n",i,defnew,defnew/def);
		  def = defnew;               // update norm
		  if (def<def0*_reduction || def<1E-30)    // convergence check	
			{
			  r.converged  = true;
			  break;
			}
		}

	  if (_verbose==1)                // printing for non verbose
		printf("%5d %12.4E\n",i,def);
	  _prec.post(x);                  // postprocess preconditioner
	  r.iterations = i;               // fill statistics
	  r.reduction = def/def0;
	  r.conv_rate  = pow(r.reduction,1.0/i);
	  r.elapsed = watch.elapsed();
	  if (_verbose>0)                 // final print 
		printf("=== rate=%g, T=%g, TIT=%g\n",r.conv_rate,r.elapsed,r.elapsed/i);
	}

	//! apply inverse operator, with given reduction factor
	virtual void apply (X& x, X& b, double reduction, InverseOperatorResult& r)
	{
	  _reduction = reduction;
	  (*this).apply(x,b,r);
	}

  private:
	SeqScalarProduct<X> ssp;
	LinearOperator<X,X>& _op;
	Preconditioner<X,X>& _prec;
	ScalarProduct<X>& _sp;
	double _reduction;
	int _maxit;
	int _verbose; 	
  };


  // Ronald Kriemanns BiCG-STAB implementation from Sumo
  //! Bi-conjugate Gradient Stabilized (BiCG-STAB)
  template<class X>
  class BiCGSTABSolver : public InverseOperator<X,X> {
  public:
	//! export types, usually they come from the derived class
	typedef X domain_type;
	typedef X range_type;
	typedef typename X::field_type field_type;

	//! set up Loop solver
	template<class L, class P>
	BiCGSTABSolver (L& op, P& prec,
				double reduction, int maxit, int verbose) : 
	  ssp(), _op(op), _prec(prec), _sp(ssp), _reduction(reduction), _maxit(maxit), _verbose(verbose)
	{
	  IsTrue< static_cast<int>(L::category) == static_cast<int>(P::category) >::yes();
	  IsTrue< static_cast<int>(L::category) == static_cast<int>(SolverCategory::sequential) >::yes();
	}
	template<class L, class S, class P>
	BiCGSTABSolver (L& op, S& sp, P& prec,
				double reduction, int maxit, int verbose) : 
	  _op(op), _sp(sp), _prec(prec), _reduction(reduction), _maxit(maxit), _verbose(verbose)
	{
	  IsTrue< static_cast<int>(L::category) == static_cast<int>(P::category) >::yes();
	  IsTrue< static_cast<int>(L::category) == static_cast<int>(S::category) >::yes();
	}

	//! apply inverse operator
	virtual void apply (X& x, X& b, InverseOperatorResult& res)
	{
	  const double EPSILON=1e-40;
    
	  int                  it;
	  field_type           rho, rho_new, alpha, beta, h, omega;
	  field_type           norm, norm_old, norm_0;
    
	  //
	  // get vectors and matrix
	  //
	  X& r=b;
	  X p(x);
	  X v(x);
	  X t(x);
	  X y(x);
	  X rt(x);

	  //
	  // begin iteration
	  //
    
	  // r = r - Ax; rt = r
   	  res.clear();                  // clear solver statistics
	  Timer watch;                // start a timer
	  _op.applyscaleadd(-1,x,r);  // overwrite b with defect
	  _prec.pre(x,r);             // prepare preconditioner

	  rt=r;

	  norm = norm_old = norm_0 = _sp.norm(r);
    
	  p=0;
	  v=0;

	  rho   = 1;
	  alpha = 1;
	  omega = 1;

	  if (_verbose>0)             // printing
		{
		  printf("=== BiCGSTABSolver\n");
		  if (_verbose>1) printf(" Iter       Defect         Rate\n");
		  if (_verbose>1) printf("%5d %12.4E\n",0,norm_0);
		}

          if ( norm < (_reduction * norm_0)  || norm<1E-30)
		{
	          res.converged = 1;
	          _prec.post(x);                  // postprocess preconditioner
	          res.iterations = 0;             // fill statistics
	          res.reduction = 0;
	          res.conv_rate  = 0;
	          res.elapsed = watch.elapsed();
	          return;
		}

	  //
	  // iteration
	  //
    
	  it = 0;

	  while ( true )
		{
		  //
		  // preprocess, set vecsizes etc.
		  //
        
		  // rho_new = < rt , r >
		  rho_new = _sp.dot(rt,r);

		  // look if breakdown occured
		  if (std::abs(rho) <= EPSILON)
            DUNE_THROW(ISTLError,"breakdown in BiCGSTAB - rho "
                       << rho << " <= EPSILON " << EPSILON
                       << " after " << res.iterations << " iterations");
          if (std::abs(omega) <= EPSILON)
            DUNE_THROW(ISTLError,"breakdown in BiCGSTAB - omega "
                       << omega << " <= EPSILON " << EPSILON
                       << " after " << res.iterations << " iterations");

        
		  if (it==0)
			p = r;
		  else
			{
			  beta = ( rho_new / rho ) * ( alpha / omega );       
			  p.axpy(-omega,v); // p = r + beta (p - omega*v)
			  p *= beta;
			  p += r;
			}

		  // y = W^-1 * p
		  y = 0;
		  _prec.apply(y,p);           // apply preconditioner

		  // v = A * y
		  _op.apply(y,v);

		  // alpha = rho_new / < rt, v >
		  h = _sp.dot(rt,v);

		  if ( std::abs(h) < EPSILON )
			DUNE_THROW(ISTLError,"h=0 in BiCGSTAB");
		  
		  alpha = rho_new / h;
        
		  // apply first correction to x
		  // x <- x + alpha y
		  x.axpy(alpha,y);

		  // r = r - alpha*v
		  r.axpy(-alpha,v);
        
		  //
		  // test stop criteria
		  //

		  it++;
   		  norm = _sp.norm(r);

		  if (_verbose>1)             // print
			printf("%5d %12.4E %12.4g\n",it,norm,norm/norm_old);
        
		  if ( norm < (_reduction * norm_0) )
			{
			  res.converged = 1;
			  break;
			}

		  if (it >= _maxit)
            break;
        
		  norm_old = norm;

		  // y = W^-1 * r
		  y = 0;
		  _prec.apply(y,r);

		  // t = A * y
		  _op.apply(y,t);
		  
		  // omega = < t, r > / < t, t >
		  omega = _sp.dot(t,r)/_sp.dot(t,t);
        
		  // apply second correction to x
		  // x <- x + omega y
		  x.axpy(omega,y);
        
		  // r = s - omega*t (remember : r = s)
		  r.axpy(-omega,t);
        
		  rho = rho_new;

		  //
		  // test stop criteria
		  //

		  it++;
        
		  norm = _sp.norm(r);

		  if (_verbose>1)             // print
			printf("%5d %12.4E %12.4g\n",it,norm,norm/norm_old);
        
		  if ( norm < (_reduction * norm_0)  || norm<1E-30)
			{
			  res.converged = 1;
			  break;
			}

		  if (it >= _maxit)
            break;
        
		  norm_old = norm;
		}// while

	  if (_verbose==1)                // printing for non verbose
		printf("%5d %12.4E\n",it,norm);
	  _prec.post(x);                  // postprocess preconditioner
	  res.iterations = it;              // fill statistics
	  res.reduction = norm/norm_0;
	  res.conv_rate  = pow(res.reduction,1.0/it);
	  res.elapsed = watch.elapsed();
	  if (_verbose>0)                 // final print 
		printf("=== rate=%g, T=%g, TIT=%g\n",res.conv_rate,res.elapsed,res.elapsed/it);
	}

	//! apply inverse operator, with given reduction factor
	virtual void apply (X& x, X& b, double reduction, InverseOperatorResult& r)
	{
	  _reduction = reduction;
	  (*this).apply(x,b,r);
	}

  private:
	SeqScalarProduct<X> ssp;
	LinearOperator<X,X>& _op;
	Preconditioner<X,X>& _prec;
	ScalarProduct<X>& _sp;
	double _reduction;
	int _maxit;
	int _verbose; 	
  };


  /** @} end documentation */

} // end namespace

#endif
