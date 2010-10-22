#ifndef DUNE_AMG_KAMG_HH
#define DUNE_AMG_KAMG_HH

#include<dune/istl/preconditioners.hh>
#include"amg.hh"

namespace Dune
{
  namespace Amg
  {

    /**
     * @addtogroup ISTL_PAAMG
     * @{
     */
    /** @file
     * @author Markus Blatt
     * @brief Provides an algebraic multigrid using a Krylov cycle.
     *
     */

    /**
     * @brief Two grid operator for AMG with Krylov cycle.
     * @tparam AMG The type of the underlying agglomeration AMG.
     */
    template<class AMG>
    class KAmgTwoGrid
      : public Preconditioner<typename AMG::Domain,typename AMG::Range>
    {
      /** @brief The type of the domain. */
      typedef typename AMG::Domain Domain;
      /** @brief the type of the range. */
      typedef typename AMG::Range Range;
    public:

      enum {
	/** @brief The solver category. */
	category = AMG::category
      };
      
      /**
       * @brief Constructor. 
       * @param amg The underlying amg. It is used as the storage for the hierarchic
       * data structures.
       * @param coarseSolver The solver used for the coarse grid correction.
       */
      
      KAmgTwoGrid(AMG& amg, InverseOperator<Domain,Range>* coarseSolver)
        : amg_(amg), coarseSolver_(coarseSolver)
      {}
      
      /**  \copydoc Preconditioner::pre(X&,Y&) */
      void pre(typename AMG::Domain& x, typename AMG::Range& b)
      {}
      
      /**  \copydoc Preconditioner::post(X&) */
      void post(typename AMG::Domain& x)
      {}

      /** \copydoc Preconditioner::pre(X&,Y&) */
      void apply(typename AMG::Domain& v, const typename AMG::Range& d)
      {
        *amg_.rhs = d;
        *amg_.lhs = v;
          // Copy data
        *amg_.update=0;

        amg_.presmooth();
        bool processFineLevel = 
          amg_.moveToCoarseLevel();

        if(processFineLevel){
          typename AMG::Range b=*amg_.rhs;
          typename AMG::Domain x=*amg_.update;
          InverseOperatorResult res;
          coarseSolver_->apply(x, b, res);
          *amg_.update=x;
        }
        
        amg_.moveToFineLevel(processFineLevel);
        
        amg_.postsmooth();
        v=*amg_.update;
      }

      /** 
       * @brief Get a pointer to the coarse grid solver. 
       * @return The coarse grid solver.
       */
      InverseOperator<Domain,Range>* coarseSolver()
      {
        return coarseSolver_;
      }
      
      /** @brief Destructor. */
      ~KAmgTwoGrid()
      {}
      
    private:
      /** @brief Underlying AMG used as storage and engine. */
      AMG& amg_;
      /** @brief The coarse grid solver.*/
      InverseOperator<Domain,Range>* coarseSolver_;
    };
    


    /**
     * @brief an algebraic multigrid method using a Krylov-cycle.
     *
     * @tparam M The type of the linear operator.
     * @tparam X The type of the range and domain.
     * @tparam PI The parallel information object. Use SequentialInformation (default)
     * for a sequential AMG, OwnerOverlapCopyCommunication for the parallel case.
     * @tparam K The type of the Krylov method to use for the cycle.
     * @tparam A The type of the allocator to use.
     */
    template<class M, class X, class S, class PI=SequentialInformation,
	     class K=BiCGSTABSolver<X>, class A=std::allocator<X> >
    class KAMG : public Preconditioner<X,X>
    {
    public:
      /** @brief The type of the underlying AMG. */ 
      typedef AMG<M,X,S,PI,A> Amg;
      /** @brief The type of the Krylov solver for the cycle. */ 
      typedef K KrylovSolver;
      /** @brief The type of the hierarchy of operators. */
      typedef typename Amg::OperatorHierarchy OperatorHierarchy;
      /** @brief The type of the coarse solver. */
      typedef typename Amg::CoarseSolver CoarseSolver;
      /** @brief the type of the parallelinformation to use.*/
      typedef typename Amg::ParallelInformation ParallelInformation;
      /** @brief The type of the arguments for construction of the smoothers. */
      typedef typename Amg::SmootherArgs SmootherArgs;
      /** @brief the type of the lineatr operator. */
      typedef typename Amg::Operator Operator;
      /** @brief the type of the domain. */
      typedef typename Amg::Domain Domain;
      /** @brief The type of the range. */
      typedef typename Amg::Range Range;
      /** @brief The type of the hierarchy of parallel information. */
      typedef typename Amg::ParallelInformationHierarchy ParallelInformationHierarchy;
      /** @brief The type of the scalar product. */
      typedef typename Amg::ScalarProduct ScalarProduct;
      
      enum {
	/** @brief The solver category. */
	category = Amg::category
      };
      /**
       * @brief Construct a new amg with a specific coarse solver.
       * @param matrices The already set up matix hierarchy.
       * @param coarseSolver The set up solver to use on the coarse 
       * grid, must natch the soarse matrix in the matrix hierachy.
       * @param smootherArgs The  arguments needed for thesmoother to use 
       * for pre and post smoothing
       * @param gamma The number of subcycles. 1 for V-cycle, 2 for W-cycle.
       * @param preSmoothingSteps The number of smoothing steps for premoothing.
       * @param postSmoothingSteps The number of smoothing steps for postmoothing.
       */
      KAMG(const OperatorHierarchy& matrices, CoarseSolver& coarseSolver, 
           const SmootherArgs& smootherArgs, std::size_t gamma,
           std::size_t preSmoothingSteps =1, std::size_t postSmoothingSteps = 1,
           std::size_t maxLevelKrylovSteps = 3 , double minDefectReduction =1e-1);

      /**
       * @brief Construct an AMG with an inexact coarse solver based on the smoother.
       *
       * As coarse solver a preconditioned CG method with the smoother as preconditioner
       * will be used. The matrix hierarchy is built automatically.
       * @param fineOperator The operator on the fine level.
       * @param criterion The criterion describing the coarsening strategy. E. g. SymmetricCriterion
       * or UnsymmetricCriterion.
       * @param smootherArgs The arguments for constructing the smoothers.
       * @param gamma 1 for V-cycle, 2 for W-cycle
       * @param preSmoothingSteps The number of smoothing steps for premoothing.
       * @param postSmoothingSteps The number of smoothing steps for postmoothing.
       * @param maxLevelKrylovSteps The maximum number of Krylov steps allowed at each level.
       * @param minDefectReduction The defect reduction to achieve on each krylov level.
       * @param pinfo The information about the parallel distribution of the data.
       */
      template<class C>
      KAMG(const Operator& fineOperator, const C& criterion,
           const SmootherArgs& smootherArgs, std::size_t gamma=1,
           std::size_t preSmoothingSteps=1, std::size_t postSmoothingSteps=1,
           std::size_t maxLevelKrylovSteps=3, double minDefectReduction=1e-1,
           const ParallelInformation& pinfo=ParallelInformation());

      /**  \copydoc Solver::pre(X&,Y&) */
      void pre(Domain& x, Range& b);
      /**  \copydoc Solver::post(X&) */
      void post(Domain& x);
      /**  \copydoc Solver::apply(X&,Y&) */
      void apply(Domain& x, const Range& b);

      std::size_t maxlevels();
      
    private:
      /** @brief The underlying amg. */
      Amg amg;
      
      /** \brief The maximum number of Krylov steps allowed at each level. */
      std::size_t maxLevelKrylovSteps;

      /** \brief The defect reduction to achieve on each krylov level. */
      double levelDefectReduction;
      
      /** @brief pointers to the allocated scalar products. */
      std::vector<typename Amg::ScalarProduct*> scalarproducts;
      
      /** @brief pointers to the allocated krylov solvers. */
      std::vector<KAmgTwoGrid<Amg>*> ksolvers;
    };
    
    template<class M, class X, class S, class P, class K, class A>
    KAMG<M,X,S,P,K,A>::KAMG(const OperatorHierarchy& matrices, CoarseSolver& coarseSolver, 
                            const SmootherArgs& smootherArgs,
                            std::size_t gamma, std::size_t preSmoothingSteps,
                            std::size_t postSmoothingSteps,
                            std::size_t ksteps, double reduction)
      : amg(matrices, coarseSolver, smootherArgs, gamma, preSmoothingSteps,
            postSmoothingSteps), maxLevelKrylovSteps(ksteps), levelDefectReduction(reduction)
    {}
    
    template<class M, class X, class S, class P, class K, class A>
      template<class C>
    KAMG<M,X,S,P,K,A>::KAMG(const Operator& fineOperator, const C& criterion,
                            const SmootherArgs& smootherArgs, std::size_t gamma,
                            std::size_t preSmoothingSteps, std::size_t postSmoothingSteps,
                            std::size_t ksteps, double reduction,
                            const ParallelInformation& pinfo)
      : amg(fineOperator, criterion, smootherArgs, gamma, preSmoothingSteps,
            postSmoothingSteps, false, pinfo), maxLevelKrylovSteps(ksteps), levelDefectReduction(reduction)
    {}

    
    template<class M, class X, class S, class P, class K, class A>
    void KAMG<M,X,S,P,K,A>::pre(Domain& x, Range& b)
    {
      amg.pre(x,b);
      scalarproducts.reserve(amg.levels());
      ksolvers.reserve(amg.levels());

      typename OperatorHierarchy::ParallelMatrixHierarchy::Iterator
        matrix = amg.matrices_->matrices().coarsest();
      typename ParallelInformationHierarchy::Iterator 
        pinfo = amg.matrices_->parallelInformation().coarsest();      
      bool hasCoarsest=(amg.levels()==amg.maxlevels());
      KrylovSolver* ks=0;

      if(hasCoarsest){
        if(matrix==amg.matrices_->matrices().finest())
        return;
        --matrix;
        --pinfo;
        ksolvers.push_back(new KAmgTwoGrid<Amg>(amg, amg.solver_));
      }else
        ksolvers.push_back(new KAmgTwoGrid<Amg>(amg, ks));

      std::ostringstream s;
      
      if(matrix!=amg.matrices_->matrices().finest())
        while(true){
          scalarproducts.push_back(Amg::ScalarProductChooser::construct(*pinfo));
          ks = new KrylovSolver(*matrix, *(scalarproducts.back()), 
                                *(ksolvers.back()), levelDefectReduction, 
                                maxLevelKrylovSteps, 0);
          ksolvers.push_back(new KAmgTwoGrid<Amg>(amg, ks)); 
          --matrix;
          --pinfo;
          if(matrix==amg.matrices_->matrices().finest())
            break;
        }
    }
    
    
    template<class M, class X, class S, class P, class K, class A>
    void KAMG<M,X,S,P,K,A>::post(Domain& x)
    {
      typedef typename std::vector<KAmgTwoGrid<Amg>*>::reverse_iterator KIterator;
      typedef typename std::vector<ScalarProduct*>::iterator SIterator;
      for(KIterator kiter = ksolvers.rbegin(); kiter != ksolvers.rend();
          ++kiter)
        {
          if((*kiter)->coarseSolver()!=amg.solver_)
            delete (*kiter)->coarseSolver();
          delete *kiter;
        }
      for(SIterator siter = scalarproducts.begin(); siter!=scalarproducts.end();
          ++siter)
        delete *siter;
      amg.post(x);
      
    }

    template<class M, class X, class S, class P, class K, class A>
    void KAMG<M,X,S,P,K,A>::apply(Domain& x, const Range& b)
    {
      amg.initIteratorsWithFineLevel();
      if(ksolvers.size()==0)
        {
          Range tb=b;
          InverseOperatorResult res;
          amg.solver_->apply(x,tb,res);
        }else
        ksolvers.back()->apply(x,b);
    }

    template<class M, class X, class S, class P, class K, class A>
    std::size_t KAMG<M,X,S,P,K,A>::maxlevels()
    {
      return amg.maxlevels();
    }
    
    /** @}*/
  } // Amg
} // Dune

#endif
