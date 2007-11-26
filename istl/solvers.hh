#ifndef DUNE_SOLVERS_HH
#define DUNE_SOLVERS_HH

#include<cmath>
#include<complex>
#include<iostream>
#include<iomanip>
#include<string>

#include "istlexception.hh"
#include "operators.hh"
#include "preconditioners.hh"
#include "scalarproducts.hh"
#include <dune/common/timer.hh>
#include <dune/common/helpertemplates.hh>

namespace Dune {
  /** @defgroup ISTL_Solvers Iterative Solvers
      @ingroup ISTL
  */
  /** @addtogroup ISTL_Solvers
    @{
  */


  /** \file
      
  \brief   Define general, extensible interface for inverse operators.
  
  Implementation here covers only inversion of linear operators,
  but the implementation might be used for nonlinear operators
  as well.
  */

  /** 
      \brief Statistics about the application of an inverse operator

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
  /*! 
    \brief Abstract base class for all solvers.

    An InverseOperator computes the solution of \f$ A(x)=b\f$ where
    \f$ A : X \to Y \f$ is an operator.
    Note that the solver "knows" which operator
    to invert and which preconditioner to apply (if any). The
    user is only interested in inverting the operator.
    InverseOperator might be a Newton scheme, a Krylov subspace method,
    or a direct solver or just anything. 
  */
  template<class X, class Y>
  class InverseOperator {
  public:
    //! \brief Type of the domain of the operator to be inverted.
    typedef X domain_type;
    
    //! \brief Type of the range of the operator to be inverted.
    typedef Y range_type;
    
    /** \brief The field type of the operator. */
    typedef typename X::field_type field_type;

    /** 
  \brief Apply inverse operator, 
  
  \warning Note: right hand side b may be overwritten!

  \param x The left hand side to store the result in.
  \param b The right hand side
  \param res Object to store the statistics about applying the operator.
    */
    virtual void apply (X& x, Y& b, InverseOperatorResult& res) = 0;

    /*! 
      \brief apply inverse operator, with given convergence criteria.

      \warning Right hand side b may be overwritten!

      \param x The left hand side to store the result in.
      \param b The right hand side
      \param reduction The minimum defect reduction to achieve.
      \param res Object to store the statistics about applying the operator.
    */
    virtual void apply (X& x, Y& b, double reduction, InverseOperatorResult& res) = 0;

    //! \brief Destructor
    virtual ~InverseOperator () {}

  protected: 
    // spacing values 
    enum { iterationSpacing = 5 , normSpacing = 16 }; 

    //! helper function for printing header of solver output 
    void printHeader(std::ostream& s) const 
    {
      s << std::setw(iterationSpacing)  << " Iter";
      s << std::setw(normSpacing) << "Defect";
      s << std::setw(normSpacing) << "Rate" << std::endl;
    }

    //! helper function for printing solver output 
    template <class DataType> 
    void printOutput(std::ostream& s, 
                     const int iter,
                     const DataType& norm,
                     const DataType& norm_old) const 
    {
      const DataType rate = norm/norm_old;
      s << std::setw(iterationSpacing)  << iter << " ";
      s << std::setw(normSpacing) << norm << " ";
      s << std::setw(normSpacing) << rate << std::endl;
    }

    //! helper function for printing solver output 
    template <class DataType> 
    void printOutput(std::ostream& s, 
                     const int iter,
                     const DataType& norm) const 
    {
      s << std::setw(iterationSpacing)  << iter << " ";
      s << std::setw(normSpacing) << norm << std::endl;
    }
  };


  //=====================================================================
  // Implementation of this interface
  //=====================================================================

  /*! 
    \brief Preconditioned loop solver.
    
    Implements a preconditioned loop. 
    Unsing this class every Preconditioner can be turned 
    into a solver. The solver will apply one preconditioner
    step in each iteration loop.
   */
  template<class X>
  class LoopSolver : public InverseOperator<X,X> {
  public:
    //! \brief The domain type of the operator that we do the inverse for.
    typedef X domain_type;
    //! \brief The range type of the operator that we do the inverse for.
    typedef X range_type;
    //! \brief The field type of the operator that we do the inverse for.
    typedef typename X::field_type field_type;

    /*! 
      \brief Set up Loop solver.

      \param op The operator we solve.
      \param prec The preconditioner to apply in each iteration of the loop.
      Has to inherit from Preconditioner.
      \param reduction The relative defect reduction to achieve when applying
      the operator.
      \param maxit The maximum number of iteration steps allowed when applying
      the operator.
      \param verbose The verbosity level.

      Verbose levels are:
      <ul>
      <li> 0 : print nothing </li>
      <li> 1 : print initial and final defect and statistics </li>
      <li> 2 : print line for each iteration </li>
      </ul>
    */
    template<class L, class P>
    LoopSolver (L& op, P& prec,
    double reduction, int maxit, int verbose) : 
      ssp(), _op(op), _prec(prec), _sp(ssp), _reduction(reduction), _maxit(maxit), _verbose(verbose)
    {
      IsTrue< static_cast<int>(L::category) == static_cast<int>(P::category) >::yes();
      IsTrue< static_cast<int>(L::category) == static_cast<int>(SolverCategory::sequential) >::yes();
    }

    /** 
  \brief Set up loop solver
  
  \param op The operator we solve.
  \param sp The scalar product to use, e. g. SeqScalarproduct.
  \param prec The preconditioner to apply in each iteration of the loop.
  Has to inherit from Preconditioner.
  \param reduction The relative defect reduction to achieve when applying
  the operator.
  \param maxit The maximum number of iteration steps allowed when applying
  the operator.
  \param verbose The verbosity level.
  
  Verbose levels are:
  <ul>
  <li> 0 : print nothing </li>
  <li> 1 : print initial and final defect and statistics </li>
  <li> 2 : print line for each iteration </li>
  </ul>
    */
    template<class L, class S, class P>
    LoopSolver (L& op, S& sp, P& prec,
        double reduction, int maxit, int verbose) : 
      _op(op), _prec(prec), _sp(sp), _reduction(reduction), _maxit(maxit), _verbose(verbose)
    {
      IsTrue< static_cast<int>(L::category) == static_cast<int>(P::category) >::yes();
      IsTrue< static_cast<int>(L::category) == static_cast<int>(S::category) >::yes();
    }


  //! \copydoc InverseOperator::apply(X&,Y&,InverseOperatorResult&)
  virtual void apply (X& x, X& b, InverseOperatorResult& res)
  {
    // clear solver statistics
    res.clear();

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
      std::cout << "=== LoopSolver" << std::endl;
      if (_verbose>1) 
      {
        this->printHeader(std::cout);
        this->printOutput(std::cout,0,def0);
      }
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
        this->printOutput(std::cout,i,defnew,def);
                      //std::cout << i << " " << defnew << " " << defnew/def << std::endl;
      def = defnew;               // update norm
      if (def<def0*_reduction || def<1E-30)    // convergence check 
      {
        res.converged  = true;
        break;
      }
    }

    // print
    if (_verbose==1)
       this->printOutput(std::cout,i,def);
  
    // postprocess preconditioner
    _prec.post(x);

    // fill statistics
    res.iterations = i;
    res.reduction = def/def0;
    res.conv_rate  = pow(res.reduction,1.0/i);
    res.elapsed = watch.elapsed();

    // final print
    if (_verbose>0)
    {
      std::cout << "=== rate=" << res.conv_rate
                << ", T=" << res.elapsed
                << ", TIT=" << res.elapsed/i
                << ", IT=" << i << std::endl;
    }
  }

  //! \copydoc InverseOperator::apply(X&,Y&,double,InverseOperatorResult&)
  virtual void apply (X& x, X& b, double reduction, InverseOperatorResult& res)
  {
    _reduction = reduction;
    (*this).apply(x,b,res);
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
    //! \brief The domain type of the operator that we do the inverse for.
    typedef X domain_type;
    //! \brief The range type of the operator  that we do the inverse for.
    typedef X range_type;
    //! \brief The field type of the operator  that we do the inverse for.
    typedef typename X::field_type field_type;

    /*! 
      \brief Set up solver.

      \copydoc LoopSolver::LoopSolver(L&,P&,double,int,int)
    */
    template<class L, class P>
    GradientSolver (L& op, P& prec,
        double reduction, int maxit, int verbose) : 
      ssp(), _op(op), _prec(prec), _sp(ssp), _reduction(reduction), _maxit(maxit), _verbose(verbose)
    {
      IsTrue< static_cast<int>(L::category) == static_cast<int>(P::category) >::yes();
      IsTrue< static_cast<int>(L::category) == static_cast<int>(SolverCategory::sequential) >::yes();
    }
    /*! 
      \brief Set up solver.

      \copydoc LoopSolver::LoopSolver(L&,S&,P&,double,int,int)
    */
    template<class L, class S, class P>
    GradientSolver (L& op, S& sp, P& prec,
        double reduction, int maxit, int verbose) : 
      _op(op), _prec(prec), _sp(sp), _reduction(reduction), _maxit(maxit), _verbose(verbose)
    {
      IsTrue< static_cast<int>(L::category) == static_cast<int>(P::category) >::yes();
      IsTrue< static_cast<int>(L::category) == static_cast<int>(S::category) >::yes();
    }

    /*! 
      \brief Apply inverse operator.

      \copydoc InverseOperator::apply(X&,Y&,InverseOperatorResult&)
    */
    virtual void apply (X& x, X& b, InverseOperatorResult& res)
    {
      res.clear();                  // clear solver statistics
      Timer watch;                // start a timer
      _prec.pre(x,b);             // prepare preconditioner
      _op.applyscaleadd(-1,x,b);  // overwrite b with defect
      
      X p(x);                     // create local vectors
      X q(b);
  
      double def0 = _sp.norm(b);// compute norm
      
      if (_verbose>0)             // printing
      {
        std::cout << "=== GradientSolver" << std::endl;
        if (_verbose>1) 
        {
          this->printHeader(std::cout);
          this->printOutput(std::cout,0,def0);
        }
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
          this->printOutput(std::cout,i,defnew,def);

        def = defnew;               // update norm
        if (def<def0*_reduction || def<1E-30)    // convergence check 
        {
          res.converged  = true;
          break;
        }
      }
          
      if (_verbose==1)                // printing for non verbose
        this->printOutput(std::cout,i,def);

      _prec.post(x);                  // postprocess preconditioner
      res.iterations = i;               // fill statistics
      res.reduction = def/def0;
      res.conv_rate  = pow(res.reduction,1.0/i);
      res.elapsed = watch.elapsed();
      if (_verbose>0)                 // final print 
          std::cout << "=== rate=" << res.conv_rate
                    << ", T=" << res.elapsed
                    << ", TIT=" << res.elapsed/i
                    << ", IT=" << i << std::endl;
    }

    /*! 
      \brief Apply inverse operator with given reduction factor.

      \copydoc InverseOperator::apply(X&,Y&,double,InverseOperatorResult&)
    */
    virtual void apply (X& x, X& b, double reduction, InverseOperatorResult& res)
    {
      _reduction = reduction;
      (*this).apply(x,b,res);
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



  //! \brief conjugate gradient method
  template<class X>
  class CGSolver : public InverseOperator<X,X> {
  public:
    //! \brief The domain type of the operator to be inverted.
    typedef X domain_type;
    //! \brief The range type of the operator to be inverted.
    typedef X range_type;
    //! \brief The field type of the operator to be inverted.
    typedef typename X::field_type field_type;

    /*! 
      \brief Set up conjugate gradient solver.

      \copydoc LoopSolver::LoopSolver(L&,P&,double,int,int)
    */
    template<class L, class P>
    CGSolver (L& op, P& prec, double reduction, int maxit, int verbose) : 
      ssp(), _op(op), _prec(prec), _sp(ssp), _reduction(reduction), _maxit(maxit), _verbose(verbose)
    {
      IsTrue< static_cast<int>(L::category) == static_cast<int>(P::category) >::yes();
      IsTrue< static_cast<int>(L::category) == static_cast<int>(SolverCategory::sequential) >::yes();
    }
    /*! 
      \brief Set up conjugate gradient solver.

      \copydoc LoopSolver::LoopSolver(L&,S&,P&,double,int,int)
    */
    template<class L, class S, class P>
    CGSolver (L& op, S& sp, P& prec, double reduction, int maxit, int verbose) : 
      _op(op), _prec(prec), _sp(sp), _reduction(reduction), _maxit(maxit), _verbose(verbose)
    {
      IsTrue< static_cast<int>(L::category) == static_cast<int>(P::category) >::yes();
      IsTrue< static_cast<int>(L::category) == static_cast<int>(S::category) >::yes();
    }

    /*! 
      \brief Apply inverse operator.

      \copydoc InverseOperator::apply(X&,Y&,InverseOperatorResult&)
    */
    virtual void apply (X& x, X& b, InverseOperatorResult& res)
    {
      res.clear();                  // clear solver statistics
      Timer watch;                // start a timer
      _prec.pre(x,b);             // prepare preconditioner
      _op.applyscaleadd(-1,x,b);  // overwrite b with defect
    
      X p(x);              // the search direction
      X q(x);              // a temporary vector
    
      double def0 = _sp.norm(b);// compute norm
      if (def0<1E-30)    // convergence check 
      {
        res.converged  = true;
        res.iterations = 0;               // fill statistics
        res.reduction = 0;
        res.conv_rate  = 0;
        res.elapsed=0;
        if (_verbose>0)                 // final print 
                        std::cout << "=== rate=" << res.conv_rate
                                  << ", T=" << res.elapsed << ", TIT=" << res.elapsed
                                  << ", IT=0" << std::endl;
        return;
      }

      if (_verbose>0)             // printing
      {
        std::cout << "=== CGSolver" << std::endl;
        if (_verbose>1) {
          this->printHeader(std::cout);
          this->printOutput(std::cout,0,def0);
        }
      }

      // some local variables
      double def=def0;   // loop variables
      field_type rho,rholast,lambda,alpha,beta;

      // determine initial search direction
      p = 0;                          // clear correction
      _prec.apply(p,b);               // apply preconditioner
      rholast = _sp.dot(p,b);         // orthogonalization

      // the loop
      int i=1; 
      for ( ; i<=_maxit; i++ )  
      {
        // minimize in given search direction p
        _op.apply(p,q);             // q=Ap
        alpha = _sp.dot(p,q);       // scalar product
        lambda = rholast/alpha;     // minimization
        x.axpy(lambda,p);           // update solution
        b.axpy(-lambda,q);          // update defect

        // convergence test
        double defnew=_sp.norm(b);// comp defect norm

        if (_verbose>1)             // print
          this->printOutput(std::cout,i,defnew,def);

        def = defnew;               // update norm
        if (def<def0*_reduction || def<1E-30 || i==_maxit)    // convergence check  
        {
          res.converged  = true;
          break;
        }

        // determine new search direction
        q = 0;                      // clear correction
        _prec.apply(q,b);           // apply preconditioner
        rho = _sp.dot(q,b);         // orthogonalization
        beta = rho/rholast;         // scaling factor
        p *= beta;                  // scale old search direction
        p += q;                     // orthogonalization with correction
        rholast = rho;              // remember rho for recurrence
      }

      if (_verbose==1)                // printing for non verbose
        this->printOutput(std::cout,i,def);

      _prec.post(x);                  // postprocess preconditioner
      res.iterations = i;               // fill statistics
      res.reduction = def/def0;
      res.conv_rate  = pow(res.reduction,1.0/i);
      res.elapsed = watch.elapsed();

      if (_verbose>0)                 // final print 
      {
        std::cout << "=== rate=" << res.conv_rate
                  << ", T=" << res.elapsed
                  << ", TIT=" << res.elapsed/i
                  << ", IT=" << i << std::endl;
      }
  }

    /*! 
      \brief Apply inverse operator with given reduction factor.

      \copydoc InverseOperator::apply(X&,Y&,double,InverseOperatorResult&)
    */
  virtual void apply (X& x, X& b, double reduction, InverseOperatorResult& res)
  {
    _reduction = reduction;
    (*this).apply(x,b,res);
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
  //! \brief Bi-conjugate Gradient Stabilized (BiCG-STAB)
  template<class X>
  class BiCGSTABSolver : public InverseOperator<X,X> {
  public:
    //! \brief The domain type of the operator to be inverted.
    typedef X domain_type;
    //! \brief The range type of the operator to be inverted.
    typedef X range_type;
    //! \brief The field type of the operator to be inverted
    typedef typename X::field_type field_type;

    /*! 
      \brief Set up solver.

      \copydoc LoopSolver::LoopSolver(L&,P&,double,int,int);
    */
    template<class L, class P>
    BiCGSTABSolver (L& op, P& prec,
        double reduction, int maxit, int verbose) : 
      ssp(), _op(op), _prec(prec), _sp(ssp), _reduction(reduction), _maxit(maxit), _verbose(verbose)
    {
      IsTrue< static_cast<int>(L::category) == static_cast<int>(P::category) >::yes();
      IsTrue< static_cast<int>(L::category) == static_cast<int>(SolverCategory::sequential) >::yes();
    }
    /*! 
      \brief Set up solver.

      \copydoc LoopSolver::LoopSolver(L&,S&,P&,double,int,int);
    */
    template<class L, class S, class P>
    BiCGSTABSolver (L& op, S& sp, P& prec,
        double reduction, int maxit, int verbose) : 
      _op(op), _prec(prec), _sp(sp), _reduction(reduction), _maxit(maxit), _verbose(verbose)
    {
      IsTrue< static_cast<int>(L::category) == static_cast<int>(P::category) >::yes();
      IsTrue< static_cast<int>(L::category) == static_cast<int>(S::category) >::yes();
    }

    /*! 
      \brief Apply inverse operator.

      \copydoc InverseOperator::apply(X&,Y&,InverseOperatorResult&)
    */
    virtual void apply (X& x, X& b, InverseOperatorResult& res)
    {
    const double EPSILON=1e-80;
    
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
      std::cout << "=== BiCGSTABSolver" << std::endl;
      if (_verbose>1) 
      {
        this->printHeader(std::cout);
        this->printOutput(std::cout,0,norm_0);
        //std::cout << " Iter       Defect         Rate" << std::endl;
        //std::cout << "    0" << std::setw(14) << norm_0 << std::endl;
      }
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
                       << " after " << it << " iterations");
          if (std::abs(omega) <= EPSILON)
            DUNE_THROW(ISTLError,"breakdown in BiCGSTAB - omega "
                       << omega << " <= EPSILON " << EPSILON
                       << " after " << it << " iterations");

        
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

      if (_verbose>1) // print
      {
        this->printOutput(std::cout,it,norm,norm_old);
      }
        
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

      if (_verbose > 1)             // print
      {
        this->printOutput(std::cout,it,norm,norm_old);
      }
        
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
      this->printOutput(std::cout,it,norm);

    _prec.post(x);                  // postprocess preconditioner
    res.iterations = it;              // fill statistics
    res.reduction = norm/norm_0;
    res.conv_rate  = pow(res.reduction,1.0/it);
    res.elapsed = watch.elapsed();
    if (_verbose>0)                 // final print 
              std::cout << "=== rate=" << res.conv_rate
                        << ", T=" << res.elapsed
                        << ", TIT=" << res.elapsed/it
                        << ", IT=" << it << std::endl;
    }

    /*! 
      \brief Apply inverse operator with given reduction factor.

      \copydoc InverseOperator::apply(X&,Y&,double,InverseOperatorResult&)
    */
    virtual void apply (X& x, X& b, double reduction, InverseOperatorResult& res)
    {
      _reduction = reduction;
      (*this).apply(x,b,res);
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
