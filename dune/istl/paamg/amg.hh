// $Id$
#ifndef DUNE_AMG_AMG_HH
#define DUNE_AMG_AMG_HH

#include<memory>
#include<dune/common/exceptions.hh>
#include<dune/istl/paamg/smoother.hh>
#include<dune/istl/paamg/transfer.hh>
#include<dune/istl/paamg/hierarchy.hh>
#include<dune/istl/solvers.hh>
#include<dune/istl/scalarproducts.hh>
#include<dune/istl/superlu.hh>
#include<dune/common/typetraits.hh>
#include<dune/common/exceptions.hh>

namespace Dune
{
  namespace Amg
  {
    /**
     * @defgroup ISTL_PAAMG Parallel Algebraic Multigrid
     * @ingroup ISTL_Prec
     * @brief A Parallel Algebraic Multigrid based on Agglomeration.
     */
    
    /**
     * @addtogroup ISTL_PAAMG
     *
     * @{
     */
    
    /** @file
     * @author Markus Blatt
     * @brief The AMG preconditioner.
     */

    template<class M, class X, class S, class P, class K, class A>
    class KAMG;
    
    template<class T>
    class KAmgTwoGrid;
    
    /**
     * @brief Parallel algebraic multigrid based on agglomeration.
     *
     * \tparam M The matrix type
     * \tparam X The vector type
     * \tparam A An allocator for X
     */
    template<class M, class X, class S, class PI=SequentialInformation,
	     class A=std::allocator<X> >
    class AMG : public Preconditioner<X,X>
    {
      template<class M1, class X1, class S1, class P1, class K1, class A1>
      friend class KAMG;
      
      friend class KAmgTwoGrid<AMG>;
      
    public:
      /** @brief The matrix operator type. */
      typedef M Operator;
      /** 
       * @brief The type of the parallel information.
       * Either OwnerOverlapCommunication or another type
       * describing the parallel data distribution and
       * providing communication methods.
       */
      typedef PI ParallelInformation;
      /** @brief The operator hierarchy type. */
      typedef MatrixHierarchy<M, ParallelInformation, A> OperatorHierarchy;
      /** @brief The parallal data distribution hierarchy type. */
      typedef typename OperatorHierarchy::ParallelInformationHierarchy ParallelInformationHierarchy;

      /** @brief The domain type. */
      typedef X Domain;
      /** @brief The range type. */
      typedef X Range;
      /** @brief the type of the coarse solver. */
      typedef InverseOperator<X,X> CoarseSolver;
      /** 
       * @brief The type of the smoother. 
       *
       * One of the preconditioners implementing the Preconditioner interface.
       * Note that the smoother has to fit the ParallelInformation.*/
      typedef S Smoother;
  
      /** @brief The argument type for the construction of the smoother. */
      typedef typename SmootherTraits<Smoother>::Arguments SmootherArgs;
      
      enum {
	/** @brief The solver category. */
	category = S::category
      };

      /**
       * @brief Construct a new amg with a specific coarse solver.
       * @param matrices The already set up matix hierarchy.
       * @param coarseSolver The set up solver to use on the coarse 
       * grid, must match the coarse matrix in the matrix hierachy.
       * @param smootherArgs The  arguments needed for thesmoother to use 
       * for pre and post smoothing
       * @param gamma The number of subcycles. 1 for V-cycle, 2 for W-cycle.
       * @param preSmoothingSteps The number of smoothing steps for premoothing.
       * @param postSmoothingSteps The number of smoothing steps for postmoothing.
       * @deprecated Use constructor
       * AMG(const OperatorHierarchy&, CoarseSolver&, const SmootherArgs, const Parameters&)
       * instead.
       * All parameters can be set in the criterion!
       */
      AMG(const OperatorHierarchy& matrices, CoarseSolver& coarseSolver, 
	  const SmootherArgs& smootherArgs, std::size_t gamma,
	  std::size_t preSmoothingSteps,
	  std::size_t postSmoothingSteps, 
          bool additive=false) DUNE_DEPRECATED;

      /**
       * @brief Construct a new amg with a specific coarse solver.
       * @param matrices The already set up matix hierarchy.
       * @param coarseSolver The set up solver to use on the coarse 
       * grid, must match the coarse matrix in the matrix hierachy.
       * @param smootherArgs The  arguments needed for thesmoother to use 
       * for pre and post smoothing.
       * @param parms The parameters for the AMG.
       */
      AMG(const OperatorHierarchy& matrices, CoarseSolver& coarseSolver, 
	  const SmootherArgs& smootherArgs, const Parameters& parms);

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
       * @param pinfo The information about the parallel distribution of the data.
       * @deprecated Use 
       * AMG(const Operator&, const C&, const SmootherArgs, const ParallelInformation)
       * instead.
       * All parameters can be set in the criterion!
       */
      template<class C>
      AMG(const Operator& fineOperator, const C& criterion,
	  const SmootherArgs& smootherArgs, std::size_t gamma,
	  std::size_t preSmoothingSteps, 
          std::size_t postSmoothingSteps,
	  bool additive=false, 
          const ParallelInformation& pinfo=ParallelInformation()) DUNE_DEPRECATED;

      /**
       * @brief Construct an AMG with an inexact coarse solver based on the smoother.
       *
       * As coarse solver a preconditioned CG method with the smoother as preconditioner
       * will be used. The matrix hierarchy is built automatically.
       * @param fineOperator The operator on the fine level.
       * @param criterion The criterion describing the coarsening strategy. E. g. SymmetricCriterion
       * or UnsymmetricCriterion, and providing the parameters.
       * @param smootherArgs The arguments for constructing the smoothers.
       * @param pinfo The information about the parallel distribution of the data.
       */
      template<class C>
      AMG(const Operator& fineOperator, const C& criterion,
	  const SmootherArgs& smootherArgs,
          const ParallelInformation& pinfo=ParallelInformation());

      ~AMG();

      /** \copydoc Preconditioner::pre */
      void pre(Domain& x, Range& b);

      /** \copydoc Preconditioner::apply */
      void apply(Domain& v, const Range& d);
      
      /** \copydoc Preconditioner::post */
      void post(Domain& x);

      /**
       * @brief Get the aggregate number of each unknown on the coarsest level.
       * @param cont The random access container to store the numbers in.
       */
      template<class A1>
      void getCoarsestAggregateNumbers(std::vector<std::size_t,A1>& cont);
      
      std::size_t levels();
      
      std::size_t maxlevels();

      /**
       * @brief Recalculate the matrix hierarchy.
       *
       * It is assumed that the coarsening for the changed fine level
       * matrix would yield the same aggregates. In this case it suffices
       * to recalculate all the Galerkin products for the matrices of the 
       * coarser levels.
       */
      void recalculateHierarchy()
      {
        matrices_->recalculateGalerkin(NegateSet<typename PI::OwnerSet>());
      }
      
    private:
      /** @brief Multigrid cycle on a level. */
      void mgc();

      typename Hierarchy<Smoother,A>::Iterator smoother;
      typename OperatorHierarchy::ParallelMatrixHierarchy::ConstIterator matrix;
      typename ParallelInformationHierarchy::Iterator pinfo;
      typename OperatorHierarchy::RedistributeInfoList::const_iterator redist;
      typename OperatorHierarchy::AggregatesMapList::const_iterator aggregates;
      typename Hierarchy<Domain,A>::Iterator lhs;
      typename Hierarchy<Domain,A>::Iterator update;
      typename Hierarchy<Range,A>::Iterator rhs;
      
      void additiveMgc();

      /** @brief Apply pre smoothing on the current level. */
      void presmooth();

      /** @brief Apply post smoothing on the current level. */
      void postsmooth();
      
      /** 
       * @brief Move the iterators to the finer level 
       * @*/
      void moveToFineLevel(bool processedFineLevel);

      /** @brief Move the iterators to the coarser level */
      bool moveToCoarseLevel();

      /** @brief Initialize iterators over levels with fine level */
      void initIteratorsWithFineLevel();

      /**  @brief The matrix we solve. */
      OperatorHierarchy* matrices_;
      /** @brief The arguments to construct the smoother */
      SmootherArgs smootherArgs_;
      /** @brief The hierarchy of the smoothers. */
      Hierarchy<Smoother,A> smoothers_;
      /** @brief The solver of the coarsest level. */
      CoarseSolver* solver_;
      /** @brief The right hand side of our problem. */
      Hierarchy<Range,A>* rhs_;
      /** @brief The left approximate solution of our problem. */
      Hierarchy<Domain,A>* lhs_;
      /** @brief The total update for the outer solver. */
      Hierarchy<Domain,A>* update_;
      /** @brief The type of the chooser of the scalar product. */
      typedef Dune::ScalarProductChooser<X,PI,M::category> ScalarProductChooser;
      /** @brief The type of the scalar product for the coarse solver. */
      typedef typename ScalarProductChooser::ScalarProduct ScalarProduct;
      /** @brief Scalar product on the coarse level. */
      ScalarProduct* scalarProduct_;
      /** @brief Gamma, 1 for V-cycle and 2 for W-cycle. */
      std::size_t gamma_;
      /** @brief The number of pre and postsmoothing steps. */
      std::size_t preSteps_;
      /** @brief The number of postsmoothing steps. */
      std::size_t postSteps_;
      std::size_t level;
      bool buildHierarchy_;
      bool additive;
      bool coarsesolverconverged;
      Smoother *coarseSmoother_;
      /** @brief The verbosity level. */
      std::size_t verbosity_;
    };

    template<class M, class X, class S, class PI, class A>
    AMG<M,X,S,PI,A>::AMG(const OperatorHierarchy& matrices, CoarseSolver& coarseSolver, 
			const SmootherArgs& smootherArgs,
			std::size_t gamma, std::size_t preSmoothingSteps,
			std::size_t postSmoothingSteps, bool additive_)
      : matrices_(&matrices), smootherArgs_(smootherArgs),
	smoothers_(), solver_(&coarseSolver), scalarProduct_(0),
	gamma_(gamma), preSteps_(preSmoothingSteps), postSteps_(postSmoothingSteps), buildHierarchy_(false),
	additive(additive_), coarsesolverconverged(true),
	coarseSmoother_(), verbosity_(2)
    {
      assert(matrices_->isBuilt());
      
      // build the necessary smoother hierarchies
      matrices_->coarsenSmoother(smoothers_, smootherArgs_);
    }

    template<class M, class X, class S, class PI, class A>
    AMG<M,X,S,PI,A>::AMG(const OperatorHierarchy& matrices, CoarseSolver& coarseSolver, 
                         const SmootherArgs& smootherArgs,
                         const Parameters& parms)
      : matrices_(&matrices), smootherArgs_(smootherArgs),
	smoothers_(), solver_(&coarseSolver), scalarProduct_(0),
	gamma_(parms.getGamma()), preSteps_(parms.getNoPreSmoothSteps()), 
        postSteps_(parms.getNoPostSmoothSteps()), buildHierarchy_(false),
	additive(parms.getAdditive()), coarsesolverconverged(true),
	coarseSmoother_(), verbosity_(parms.debugLevel())
    {
      assert(matrices_->isBuilt());
      
      // build the necessary smoother hierarchies
      matrices_->coarsenSmoother(smoothers_, smootherArgs_);
    }

    template<class M, class X, class S, class PI, class A>
    template<class C>
    AMG<M,X,S,PI,A>::AMG(const Operator& matrix,
			const C& criterion,
			const SmootherArgs& smootherArgs,
			std::size_t gamma, std::size_t preSmoothingSteps,
			std::size_t postSmoothingSteps,
			bool additive_,
			const PI& pinfo)
      : smootherArgs_(smootherArgs),
	smoothers_(), solver_(), scalarProduct_(0), gamma_(gamma),
	preSteps_(preSmoothingSteps), postSteps_(postSmoothingSteps), buildHierarchy_(true),
	additive(additive_), coarsesolverconverged(true),
	coarseSmoother_(), verbosity_(criterion.debugLevel())
    {
      dune_static_assert(static_cast<int>(M::category)==static_cast<int>(S::category),
			 "Matrix and Solver must match in terms of category!");
      // TODO: reestablish compile time checks.
      //dune_static_assert(static_cast<int>(PI::category)==static_cast<int>(S::category),
      //			 "Matrix and Solver must match in terms of category!");
      Timer watch;
      matrices_ = new OperatorHierarchy(const_cast<Operator&>(matrix), pinfo);
            
      matrices_->template build<NegateSet<typename PI::OwnerSet> >(criterion);
      
      // build the necessary smoother hierarchies
      matrices_->coarsenSmoother(smoothers_, smootherArgs_);

      if(verbosity_>0 && matrices_->parallelInformation().finest()->communicator().rank()==0)
	std::cout<<"Building Hierarchy of "<<matrices_->maxlevels()<<" levels took "<<watch.elapsed()<<" seconds."<<std::endl;
    }

    template<class M, class X, class S, class PI, class A>
    template<class C>
    AMG<M,X,S,PI,A>::AMG(const Operator& matrix,
			const C& criterion,
                         const SmootherArgs& smootherArgs,
			const PI& pinfo)
      : smootherArgs_(smootherArgs),
	smoothers_(), solver_(), scalarProduct_(0), 
        gamma_(criterion.getGamma()), preSteps_(criterion.getNoPreSmoothSteps()), 
        postSteps_(criterion.getNoPostSmoothSteps()), buildHierarchy_(true),
	additive(criterion.getAdditive()), coarsesolverconverged(true),
	coarseSmoother_(), verbosity_(criterion.debugLevel())
    {
      dune_static_assert(static_cast<int>(M::category)==static_cast<int>(S::category),
			 "Matrix and Solver must match in terms of category!");
      // TODO: reestablish compile time checks.
      //dune_static_assert(static_cast<int>(PI::category)==static_cast<int>(S::category),
      //			 "Matrix and Solver must match in terms of category!");
      Timer watch;
      matrices_ = new OperatorHierarchy(const_cast<Operator&>(matrix), pinfo);
            
      matrices_->template build<NegateSet<typename PI::OwnerSet> >(criterion);
      
      // build the necessary smoother hierarchies
      matrices_->coarsenSmoother(smoothers_, smootherArgs_);

      if(verbosity_>0 && matrices_->parallelInformation().finest()->communicator().rank()==0)
	std::cout<<"Building Hierarchy of "<<matrices_->maxlevels()<<" levels took "<<watch.elapsed()<<" seconds."<<std::endl;
    }
    
    template<class M, class X, class S, class PI, class A>
    AMG<M,X,S,PI,A>::~AMG()
    {
      if(buildHierarchy_){
	delete matrices_;
      }
      if(scalarProduct_)
	delete scalarProduct_;
    }

    
    template<class M, class X, class S, class PI, class A>
    void AMG<M,X,S,PI,A>::pre(Domain& x, Range& b)
    {
      if(smoothers_.levels()>0)
	smoothers_.finest()->pre(x,b);
      else
	// No smoother to make x consistent! Do it by hand
	matrices_->parallelInformation().coarsest()->copyOwnerToAll(x,x);
      Range* copy = new Range(b);
      rhs_ = new Hierarchy<Range,A>(*copy);
      Domain* dcopy = new Domain(x);
      lhs_ = new Hierarchy<Domain,A>(*dcopy);
      dcopy = new Domain(x);
      update_ = new Hierarchy<Domain,A>(*dcopy);
      matrices_->coarsenVector(*rhs_);
      matrices_->coarsenVector(*lhs_);
      matrices_->coarsenVector(*update_);
      
      // Preprocess all smoothers
      typedef typename Hierarchy<Smoother,A>::Iterator Iterator;
      typedef typename Hierarchy<Range,A>::Iterator RIterator;
      typedef typename Hierarchy<Domain,A>::Iterator DIterator;
      Iterator coarsest = smoothers_.coarsest();
      Iterator smoother = smoothers_.finest();
      RIterator rhs = rhs_->finest();
      DIterator lhs = lhs_->finest();
      if(smoothers_.levels()>0){
	  
      assert(lhs_->levels()==rhs_->levels());
      assert(smoothers_.levels()==lhs_->levels() || matrices_->levels()==matrices_->maxlevels());
      assert(smoothers_.levels()+1==lhs_->levels() || matrices_->levels()<matrices_->maxlevels());
      
      if(smoother!=coarsest)
	for(++smoother, ++lhs, ++rhs; smoother != coarsest; ++smoother, ++lhs, ++rhs)
	  smoother->pre(*lhs,*rhs);
      smoother->pre(*lhs,*rhs);
      }
      
      
      // The preconditioner might change x and b. So we have to 
      // copy the changes to the original vectors.
      x = *lhs_->finest();
      b = *rhs_->finest();
      
      if(buildHierarchy_ && matrices_->levels()==matrices_->maxlevels()){
	// We have the carsest level. Create the coarse Solver
	SmootherArgs sargs(smootherArgs_);
	sargs.iterations = 1;
	
	typename ConstructionTraits<Smoother>::Arguments cargs;
	cargs.setArgs(sargs);
	if(matrices_->redistributeInformation().back().isSetup()){
	  // Solve on the redistributed partitioning     
	  cargs.setMatrix(matrices_->matrices().coarsest().getRedistributed().getmat());
	  cargs.setComm(matrices_->parallelInformation().coarsest().getRedistributed());
	  
	  coarseSmoother_ = ConstructionTraits<Smoother>::construct(cargs);
	  scalarProduct_ = ScalarProductChooser::construct(matrices_->parallelInformation().coarsest().getRedistributed());
	}else{    
	  cargs.setMatrix(matrices_->matrices().coarsest()->getmat());
	  cargs.setComm(*matrices_->parallelInformation().coarsest());
	  
	  coarseSmoother_ = ConstructionTraits<Smoother>::construct(cargs);
	  scalarProduct_ = ScalarProductChooser::construct(*matrices_->parallelInformation().coarsest());
	}
#if HAVE_SUPERLU
      // Use superlu if we are purely sequential or with only one processor on the coarsest level.
	if(is_same<ParallelInformation,SequentialInformation>::value // sequential mode 
	   || matrices_->parallelInformation().coarsest()->communicator().size()==1 //parallel mode and only one processor
	   || (matrices_->parallelInformation().coarsest().isRedistributed() 
	       && matrices_->parallelInformation().coarsest().getRedistributed().communicator().size()==1
	       && matrices_->parallelInformation().coarsest().getRedistributed().communicator().size()>0)){ // redistribute and 1 proc
	  if(verbosity_>0 && matrices_->parallelInformation().coarsest()->communicator().rank()==0)
	  std::cout<<"Using superlu"<<std::endl;
	  if(matrices_->parallelInformation().coarsest().isRedistributed())
	    {
	      if(matrices_->matrices().coarsest().getRedistributed().getmat().N()>0)
		// We are still participating on this level
		solver_  = new SuperLU<typename M::matrix_type>(matrices_->matrices().coarsest().getRedistributed().getmat());
	      else
		solver_ = 0;
	    }else
	      solver_  = new SuperLU<typename M::matrix_type>(matrices_->matrices().coarsest()->getmat());
	}else
#endif
	  {
	    if(matrices_->parallelInformation().coarsest().isRedistributed())
	      {
		if(matrices_->matrices().coarsest().getRedistributed().getmat().N()>0)
		  // We are still participating on this level
		  solver_ = new BiCGSTABSolver<X>(const_cast<M&>(matrices_->matrices().coarsest().getRedistributed()), 
						  *scalarProduct_, 
						  *coarseSmoother_, 1E-2, 10000, 0);
		else
		  solver_ = 0;
	      }else
	      solver_ = new BiCGSTABSolver<X>(const_cast<M&>(*matrices_->matrices().coarsest()), 
					      *scalarProduct_, 
					      *coarseSmoother_, 1E-2, 1000, 0);
	  }
      }
    }
    template<class M, class X, class S, class PI, class A>
    std::size_t AMG<M,X,S,PI,A>::levels()
    {
      return matrices_->levels();
    }
    template<class M, class X, class S, class PI, class A>
    std::size_t AMG<M,X,S,PI,A>::maxlevels()
    {
      return matrices_->maxlevels();
    }

    /** \copydoc Preconditioner::apply */
    template<class M, class X, class S, class PI, class A>
    void AMG<M,X,S,PI,A>::apply(Domain& v, const Range& d)
    {
      if(additive){
	*(rhs_->finest())=d;
	additiveMgc();
	v=*lhs_->finest();
      }else{
        // Init all iterators for the current level
        initIteratorsWithFineLevel();

	
	*lhs = v;
	*rhs = d;
	*update=0;
	level=0;
		  
	mgc();
	
	if(postSteps_==0||matrices_->maxlevels()==1)
	  pinfo->copyOwnerToAll(*update, *update);
	
	v=*update;
      }
      
    }

    template<class M, class X, class S, class PI, class A>
    void AMG<M,X,S,PI,A>::initIteratorsWithFineLevel()
    {
      smoother = smoothers_.finest();
      matrix = matrices_->matrices().finest();
      pinfo = matrices_->parallelInformation().finest();
      redist = 
        matrices_->redistributeInformation().begin();
      aggregates = matrices_->aggregatesMaps().begin();
      lhs = lhs_->finest();
      update = update_->finest();
      rhs = rhs_->finest();
    }
    
    template<class M, class X, class S, class PI, class A>
    bool AMG<M,X,S,PI,A>
    ::moveToCoarseLevel()
    {
      
      bool processNextLevel=true;
      
      if(redist->isSetup()){
        redist->redistribute(static_cast<const Range&>(*rhs), rhs.getRedistributed());
        processNextLevel =rhs.getRedistributed().size()>0;
        if(processNextLevel){		
          //restrict defect to coarse level right hand side.
          typename Hierarchy<Range,A>::Iterator fineRhs = rhs++;
          ++pinfo;
          Transfer<typename OperatorHierarchy::AggregatesMap::AggregateDescriptor,Range,ParallelInformation>
            ::restrict(*(*aggregates), *rhs, static_cast<const Range&>(fineRhs.getRedistributed()), *pinfo);
        }
      }else{		
        //restrict defect to coarse level right hand side.
        typename Hierarchy<Range,A>::Iterator fineRhs = rhs++;
	  ++pinfo;
	  Transfer<typename OperatorHierarchy::AggregatesMap::AggregateDescriptor,Range,ParallelInformation>
	    ::restrict(*(*aggregates), *rhs, static_cast<const Range&>(*fineRhs), *pinfo);
      }
      
      if(processNextLevel){
        // prepare coarse system
        ++lhs;
        ++update;
        ++matrix;
        ++level;
        ++redist;

        if(matrix != matrices_->matrices().coarsest() || matrices_->levels()<matrices_->maxlevels()){
          // next level is not the globally coarsest one
          ++smoother;
          ++aggregates;
        }
        // prepare the update on the next level
        *update=0;
      }
      return processNextLevel;
    }
    
    template<class M, class X, class S, class PI, class A>
    void AMG<M,X,S,PI,A>
    ::moveToFineLevel(bool processNextLevel)
    {
      if(processNextLevel){
        if(matrix != matrices_->matrices().coarsest() || matrices_->levels()<matrices_->maxlevels()){
          // previous level is not the globally coarsest one
	    --smoother;
	    --aggregates;
        }
        --redist;
        --level;
        //prolongate and add the correction (update is in coarse left hand side)
        --matrix;
	
        //typename Hierarchy<Domain,A>::Iterator coarseLhs = lhs--;
        --lhs;  
        --pinfo;
      }
      if(redist->isSetup()){
        // Need to redistribute during prolongate
        lhs.getRedistributed()=0;
        Transfer<typename OperatorHierarchy::AggregatesMap::AggregateDescriptor,Range,ParallelInformation>
          ::prolongate(*(*aggregates), *update, *lhs, lhs.getRedistributed(), matrices_->getProlongationDampingFactor(), 
                       *pinfo, *redist);
      }else{
        *lhs=0;
        Transfer<typename OperatorHierarchy::AggregatesMap::AggregateDescriptor,Range,ParallelInformation>
          ::prolongate(*(*aggregates), *update, *lhs, 
                       matrices_->getProlongationDampingFactor(), *pinfo);
      }
      
      
      if(processNextLevel){
        --update;
        --rhs;
      }
      
      *update += *lhs;
    }
    
    
    template<class M, class X, class S, class PI, class A>
    void AMG<M,X,S,PI,A>
    ::presmooth()
    {
      
      for(std::size_t i=0; i < preSteps_; ++i){
	    *lhs=0;
	    SmootherApplier<S>::preSmooth(*smoother, *lhs, *rhs);
	    // Accumulate update
	    *update += *lhs;
	    
	    // update defect
	    matrix->applyscaleadd(-1,static_cast<const Domain&>(*lhs), *rhs);
	    pinfo->project(*rhs);
          }
    }
    
     template<class M, class X, class S, class PI, class A>
    void AMG<M,X,S,PI,A>
     ::postsmooth()
    { 
      
	for(std::size_t i=0; i < postSteps_; ++i){
	  // update defect
	  matrix->applyscaleadd(-1,static_cast<const Domain&>(*lhs), *rhs);
	  *lhs=0;
	  pinfo->project(*rhs);
	  SmootherApplier<S>::postSmooth(*smoother, *lhs, *rhs);
	  // Accumulate update
	  *update += *lhs;
        }
    }
    
    
    template<class M, class X, class S, class PI, class A>
    void AMG<M,X,S,PI,A>::mgc(){
      if(matrix == matrices_->matrices().coarsest() && levels()==maxlevels()){
	// Solve directly
	InverseOperatorResult res;
	res.converged=true; // If we do not compute this flag will not get updated
	if(redist->isSetup()){
	    redist->redistribute(*rhs, rhs.getRedistributed());
	  if(rhs.getRedistributed().size()>0){
	    // We are still participating in the computation
	    pinfo.getRedistributed().copyOwnerToAll(rhs.getRedistributed(), rhs.getRedistributed());
	    solver_->apply(update.getRedistributed(), rhs.getRedistributed(), res);
	  }
	  redist->redistributeBackward(*update, update.getRedistributed());
	  pinfo->copyOwnerToAll(*update, *update);
	}else{
	  pinfo->copyOwnerToAll(*rhs, *rhs);
	  solver_->apply(*update, *rhs, res);
	}

	if (!res.converged)
	  coarsesolverconverged = false;
      }else{
	// presmoothing
        presmooth();

#ifndef DUNE_AMG_NO_COARSEGRIDCORRECTION
        bool processNextLevel = moveToCoarseLevel();
        
        if(processNextLevel){
          // next level
	  for(std::size_t i=0; i<gamma_; i++)
	    mgc();
        }
        
        moveToFineLevel(processNextLevel);
#else
        *lhs=0;
#endif	

	if(matrix == matrices_->matrices().finest()){
	  coarsesolverconverged = matrices_->parallelInformation().finest()->communicator().prod(coarsesolverconverged);
	  if(!coarsesolverconverged)
	    DUNE_THROW(MathError, "Coarse solver did not converge");
	}
	// postsmoothing
        postsmooth();
        
      }
    }

    template<class M, class X, class S, class PI, class A>
    void AMG<M,X,S,PI,A>::additiveMgc(){
      
      // restrict residual to all levels
      typename ParallelInformationHierarchy::Iterator pinfo=matrices_->parallelInformation().finest();
      typename Hierarchy<Range,A>::Iterator rhs=rhs_->finest();      
      typename Hierarchy<Domain,A>::Iterator lhs = lhs_->finest();
      typename OperatorHierarchy::AggregatesMapList::const_iterator aggregates=matrices_->aggregatesMaps().begin();
      
      for(typename Hierarchy<Range,A>::Iterator fineRhs=rhs++; fineRhs != rhs_->coarsest(); fineRhs=rhs++, ++aggregates){
	++pinfo;
	Transfer<typename OperatorHierarchy::AggregatesMap::AggregateDescriptor,Range,ParallelInformation>
	  ::restrict(*(*aggregates), *rhs, static_cast<const Range&>(*fineRhs), *pinfo);
      }
      
      // pinfo is invalid, set to coarsest level
      //pinfo = matrices_->parallelInformation().coarsest
      // calculate correction for all levels
      lhs = lhs_->finest();
      typename Hierarchy<Smoother,A>::Iterator smoother = smoothers_.finest();
      
      for(rhs=rhs_->finest(); rhs != rhs_->coarsest(); ++lhs, ++rhs, ++smoother){
	// presmoothing
	*lhs=0;
	smoother->apply(*lhs, *rhs);
      }
      
      // Coarse level solve
#ifndef DUNE_AMG_NO_COARSEGRIDCORRECTION 
      InverseOperatorResult res;
      pinfo->copyOwnerToAll(*rhs, *rhs);
      solver_->apply(*lhs, *rhs, res);
      
      if(!res.converged)
	DUNE_THROW(MathError, "Coarse solver did not converge");
#else
      *lhs=0;
#endif
      // Prologate and add up corrections from all levels
      --pinfo;
      --aggregates;
      
      for(typename Hierarchy<Domain,A>::Iterator coarseLhs = lhs--; coarseLhs != lhs_->finest(); coarseLhs = lhs--, --aggregates, --pinfo){
	Transfer<typename OperatorHierarchy::AggregatesMap::AggregateDescriptor,Range,ParallelInformation>
	  ::prolongate(*(*aggregates), *coarseLhs, *lhs, 1, *pinfo);
      }
    }

    
    /** \copydoc Preconditioner::post */
    template<class M, class X, class S, class PI, class A>
    void AMG<M,X,S,PI,A>::post(Domain& x)
    {
      if(buildHierarchy_){
	if(solver_)
	  delete solver_;
	if(coarseSmoother_)
	  ConstructionTraits<Smoother>::deconstruct(coarseSmoother_);
      }
      
      // Postprocess all smoothers
      typedef typename Hierarchy<Smoother,A>::Iterator Iterator;
      typedef typename Hierarchy<Range,A>::Iterator RIterator;
      typedef typename Hierarchy<Domain,A>::Iterator DIterator;
      Iterator coarsest = smoothers_.coarsest();
      Iterator smoother = smoothers_.finest();
      DIterator lhs = lhs_->finest();
      if(smoothers_.levels()>0){
	if(smoother != coarsest  || matrices_->levels()<matrices_->maxlevels())
	  smoother->post(*lhs);
	if(smoother!=coarsest)
	  for(++smoother, ++lhs; smoother != coarsest; ++smoother, ++lhs)
	    smoother->post(*lhs);
	smoother->post(*lhs);
      }

      delete &(*lhs_->finest());
      delete lhs_;
      delete &(*update_->finest());
      delete update_;
      delete &(*rhs_->finest());
      delete rhs_;
    }

    template<class M, class X, class S, class PI, class A>
    template<class A1>
    void AMG<M,X,S,PI,A>::getCoarsestAggregateNumbers(std::vector<std::size_t,A1>& cont)
    {
      matrices_->getCoarsestAggregatesOnFinest(cont);
    }
    
  } // end namespace Amg
}// end namespace Dune
  
#endif
