// $Id$ 
#ifndef DUNE_AMG_AGGREGATES_HH
#define DUNE_AMG_AGGREGATES_HH

#include"graph.hh"
#include"properties.hh"
#include"combinedfunctor.hh"

#include<dune/common/timer.hh>
#include<dune/common/tuples.hh>
#include<dune/common/stdstreams.hh>
#include<dune/common/poolallocator.hh>
#include<dune/common/sllist.hh>

#include<utility>
#include<set>
#include<algorithm>
#include<limits>
#include<ostream>

namespace Dune
{
  namespace Amg
  {
    
    /** 
     * @addtogroup ISTL_PAAMG
     *
     * @{ 
     */
    /** @file
     * @author Markus Blatt
     * @brief Provides classes for the Coloring process of AMG
     */
    
    /**
     * @brief Base class of all aggregation criterions.
     */
    template<class T>
    class AggregationCriterion : public T
    {

    public:
      /**
       * @brief The policy for calculating the dependency graph.
       */
      typedef T DependencyPolicy;
      
      /**
       * @brief Constructor.
       *
       * The paramters will be initialized with default values suitable
       * for 2D isotropic problems.
       *
       * If that does not fit your needs either use setDefaultValuesIsotropic
       * setDefaultValuesAnisotropic or setup the values by hand
       */
      AggregationCriterion()
	: maxDistance_(2), minAggregateSize_(4), maxAggregateSize_(6), 
	  connectivity_(15), debugLevel_(2)
      {}
      
      /**
       * @brief Sets reasonable default values for an isotropic problem.
       *
       * Reasonable means that we should end up with cube aggregates of 
       * diameter 2.
       *
       * @param dim The dimension of the problem.
       * @param diameter The preferred diameter for the aggregation.
       */
      void setDefaultValuesIsotropic(std::size_t dim, std::size_t diameter=2)
      {
	maxDistance_=0;
	std::size_t csize=1;
	for(;dim>0;dim--){
	    
	  csize*=diameter;
	  maxDistance_+=diameter-1;
	}
	minAggregateSize_=csize;
	maxAggregateSize_=csize*1.5;
      }

      /**
       * @brief Sets reasonable default values for an aisotropic problem.
       *
       * Reasonable means that we should end up with cube aggregates with 
       * sides of diameter 2 and sides in one dimension that are longer
       * (e.g. for 3D: 2x2x3).
       *
       * @param dim The dimension of the problem.
       * @param diameter The preferred diameter for the aggregation.
       */
      void setDefaultValuesAnisotropic(std::size_t dim,std::size_t diameter=2)
      {
	setDefaultValuesIsotropic(dim, diameter);
	maxDistance_+=dim-1;
      }
      /**
       * @brief Get the maximal distance allowed between to nodes in a aggregate.
       * 
       * The distance between two nodes in a aggregate is the minimal number of edges 
       * it takes to travel from one node to the other without leaving the aggregate.
       * @return The maximum distance allowed.
       */
      std::size_t maxDistance() const { return maxDistance_;}

      /**
       * @brief Set the maximal distance allowed between to nodes in a aggregate.
       * 
       * The distance between two nodes in a aggregate is the minimal number of edges 
       * it takes to travel from one node to the other without leaving the aggregate.
       * The default value is 2.
       * @param distance The maximum distance allowed.
       */
      void setMaxDistance(std::size_t distance) { maxDistance_ = distance;}

      /** 
       * @brief Get the minimum number of nodes a aggregate has to consist of.
       * @return The minimum number of nodes.
       */
      std::size_t minAggregateSize() const { return minAggregateSize_;}
    
      /** 
       * @brief Set the minimum number of nodes a aggregate has to consist of.
       *
       * the default value is 4.
       * @return The minimum number of nodes.
       */
      void setMinAggregateSize(std::size_t size){ minAggregateSize_=size;}

      /**
       * @brief Get the maximum number of nodes a aggregate is allowed to have.
       * @return The maximum number of nodes.
       */
      std::size_t maxAggregateSize() const{ return maxAggregateSize_;}

      /**
       * @brief Set the maximum number of nodes a aggregate is allowed to have.
       *
       * The default values is 6.
       * @param size The maximum number of nodes.
       */
      void setMaxAggregateSize(std::size_t size){ maxAggregateSize_ = size;}

      /**
       * @brief Get the maximum number of connections a aggregate is allowed to have.
       *
       * This limit exists to achieve sparsity of the coarse matrix. the default value is 15.
       *
       * @return The maximum number of connections a aggregate is allowed to have.
       */
      std::size_t maxConnectivity() const{ return connectivity_;}

      /**
       * @brief Set the maximum number of connections a aggregate is allowed to have.
       *
       * This limit exists to achieve sparsity of the coarse matrix. the default value is 15.
       *
       * @param connectivity The maximum number of connections a aggregate is allowed to have.
       */
      void setMaxConnectivity(std::size_t connectivity){ connectivity_ = connectivity;}

      /**
       * @brief Set the debugging level.
       * 
       * @param level If 0 no debugging output will be generated.
       */
      void setDebugLevel(int level)
      {
	debugLevel_ = level;
      }
      
      /**
       * @brief Get the debugging Level.
       *
       * @return 0 if no debugging output will be generated.
       */
      int debugLevel() const
      {
	return debugLevel_;
      }
      
      
    private:
      std::size_t maxDistance_, minAggregateSize_, maxAggregateSize_, connectivity_;
      int debugLevel_;
    };
    
    template<class T>
    std::ostream& operator<<(std::ostream& os, const AggregationCriterion<T>& criterion)
    {
      os<<"{ maxdistance="<<criterion.maxDistance()<<" minAggregateSize="
	<<criterion.minAggregateSize()<< " maxAggregateSize="<<criterion.maxAggregateSize()
	<<" connectivity="<<criterion.maxConnectivity()<<" debugLevel="<<criterion.debugLevel()<<"}";
      return os;
    }

    /**
     * @brief Paramters needed to check whether a node depends on another.
     */
    class DependencyParameters
    {
    public:
      /** @brief Constructor */
      DependencyParameters()
	: alpha_(1.0/3.0), beta_(1.0E-5)
      {}
      
      /**
       * @brief Set threshold for marking nodes as isolated.
       * The default value is 1.0E-5.
       */
      void setBeta(double b)
      {
	beta_ = b;
      }
      
      /**
       * @brief Get the threshold for marking nodes as isolated.
       * The default value is 1.0E-5.
       * @return beta
       */
      double beta() const
      {
	return beta_;
      }
      
      /**
       * @brief Set the scaling value for marking connections as strong.
       * Default value is 1/3
       */
      void setAlpha(double a)
      {
	alpha_ = a;
      }

      /**
       * @brief Get the scaling value for marking connections as strong.
       * Default value is 1/3
       */
      double alpha() const
      {
	return alpha_;
      }
      
    private:
      double alpha_, beta_;
    };
    
    
    /**
     * @brief Dependency policy for symmetric matrices.
     */
    template<class M, class N>
    class Dependency : public DependencyParameters
    {
    public:
      /**
       * @brief The matrix type we build the dependency of.
       */
      typedef M Matrix;
      
      /**
       * @brief The norm to use for examining the matrix entries.
       */
      typedef N Norm;

      /**
       * @brief Constant Row iterator of the matrix.
       */
      typedef typename Matrix::row_type Row;

      /**
       * @brief Constant column iterator of the matrix.
       */
      typedef typename Matrix::ConstColIterator ColIter;
      
      void init(const Matrix* matrix);
      
      void initRow(const Row& row, int index);

      void examine(const ColIter& col);
      
      template<class G>
      void examine(G& graph, const typename G::EdgeIterator& edge, const ColIter& col);

      bool isIsolated();
    private:
      /** @brief The matrix we work on. */
      const Matrix* matrix_;
      /** @brief The current max value.*/
      typename Matrix::field_type maxValue_;
      /** @brief The functor for calculating the norm. */
      Norm norm_;      
      /** @brief index of the currently evaluated row. */
      int row_;
      /** @brief The norm of the current diagonal. */
      typename Matrix::field_type diagonal_;
    };
        
    /**
     * @brief Dependency policy for symmetric matrices.
     */
    template<class M, class N>
    class SymmetricDependency : public DependencyParameters
    {
    public:
      /**
       * @brief The matrix type we build the dependency of.
       */
      typedef M Matrix;
      
      /**
       * @brief The norm to use for examining the matrix entries.
       */
      typedef N Norm;

      /**
       * @brief Constant Row iterator of the matrix.
       */
      typedef typename Matrix::row_type Row;

      /**
       * @brief Constant column iterator of the matrix.
       */
      typedef typename Matrix::ConstColIterator ColIter;
      
      void init(const Matrix* matrix);
      
      void initRow(const Row& row, int index);

      void examine(const ColIter& col);
      
      template<class G>
      void examine(G& graph, const typename G::EdgeIterator& edge, const ColIter& col);

      bool isIsolated();
    private:
      /** @brief The matrix we work on. */
      const Matrix* matrix_;
      /** @brief The current max value.*/
      typename Matrix::field_type maxValue_;
      /** @brief The functor for calculating the norm. */
      Norm norm_;      
      /** @brief index of the currently evaluated row. */
      int row_;
      /** @brief The norm of the current diagonal. */
      typename Matrix::field_type diagonal_;
    };

    /**
     * @brief Norm that uses only the [0][0] entry of the block to determine couplings.
     *
     */
    class FirstDiagonal
    {
    public:
      enum{ /* @brief We preserve the sign.*/
	is_sign_preserving = true
      };
      
      /** 
       * @brief compute the norm of a matrix.
       * @param m The matrix ro compute the norm of.
       */
      template<class M>
      typename M::field_type operator()(const M& m) const
      {
	return m[0][0];
      }
    };
    
    /**
     * @brief Functor using the row sum (infinity) norm to determine strong couplings.
     *
     * The is proposed by several people for elasticity problems.
     */
    struct RowSum
    {
      
      enum{ /* @brief We preserve the sign.*/
	is_sign_preserving = false
      };
      /** 
       * @brief compute the norm of a matrix.
       * @param m The matrix ro compute the norm of.
       */
      template<class M>
      typename M::field_type operator()(const M& m) const
      {
	return m.infinity_norm();
      }
    };

    /**
     * @brief Criterion taking advantage of symmetric matrices. 
     *
     * The two template parameters are:
     * <dl>
     * <dt>M</dt> <dd>The type of the matrix the amg coarsening works on, e. g. BCRSMatrix</dd>
     * <dt>Norm</dt> <dd>The norm to use to determine the strong couplings between the nodes, e.g. FirstDiagonal or RowSum.</dd>
     * </dl>
     */
    template<class M, class Norm>
    class SymmetricCriterion : public AggregationCriterion<SymmetricDependency<M,Norm> >
    {};

    
    /**
     * @brief Criterion suited for unsymmetric matrices.
     *
     * Nevertheless the sparsity pattern has to be symmetric.
     *
     * The two template parameters are:
     * <dl>
     * <dt>M</dt> <dd>The type of the matrix the amg coarsening works on, e. g. BCRSMatrix</dd>
     * <dt>Norm</dt> <dd>The norm to use to determine the strong couplings between the nodes, e.g. FirstDiagonal or RowSum.</dd>
     * </dl>
     */
    template<class M, class Norm>
    class UnSymmetricCriterion : public AggregationCriterion<Dependency<M,Norm> >
    {};
    // forward declaration
    template<class G> class Aggregator;

    
    /**
     * @brief Class providing information about the mapping of 
     * the vertices onto aggregates.
     *
     * It is assumed that the vertices are consecutively numbered
     * from 0 to the maximum vertex number.
     */
    template<class V>
    class AggregatesMap
    {
    public:

      /**
       * @brief Identifier of not yet aggregated vertices.
       */
      static const V UNAGGREGATED;
      
      /**
       * @brief Identifier of isolated vertices.
       */
      static const V ISOLATED;
      /**
       * @brief The vertex descriptor type.
       */
      typedef V VertexDescriptor;
      
      /**
       * @brief The aggregate descriptor type.
       */
      typedef V AggregateDescriptor;

      /**
       * @brief The allocator we use for our lists and the
       * set.
       */
      typedef PoolAllocator<VertexDescriptor,100> Allocator;
      
      /**
       * @brief The type of a single linked list of vertex
       * descriptors.
       */
      typedef SLList<VertexDescriptor,Allocator> VertexList;

      /**
       * @brief A Dummy visitor that does nothing for each visited edge.
       */
      class DummyEdgeVisitor
      {
      public:
	template<class EdgeIterator>
	void operator()(const EdgeIterator& egde) const
	{}
      };
      
	
      /**
       * @brief Constructs without allocating memory.
       */
      AggregatesMap();
      
      /**
       * @brief Constructs with allocating memory.
       * @param noVertices The number of vertices we will hold information
       * for.
       */
      AggregatesMap(std::size_t noVertices);

      /**
       * @brief Destructor.
       */
      ~AggregatesMap();

      /**
       * @brief Build the aggregates.
       * @param matrix The matrix describing the dependency.
       * @param graph The graph corresponding to the matrix.
       * @param criterion The aggregation criterion.
       * @return The number of aggregates built.
       */
      template<class M, class G, class C>
      tuple<int,int,int> buildAggregates(const M& matrix, G& graph, const C& criterion);
      
      /**
       * @brief Breadth first search within an aggregate
       *
       * The template parameters: <br>
       * <dl><dt>reset</dt><dd>If true the visited flags of the vertices
       *  will be reset after
       * the search</dd>
       * <dt>G</dt><dd>The type of the graph we perform the search on.</dd>
       * <dt>F</dt><dd>The type of the visitor to operate on the vertices</dd>
       * </dl>
       * @param start The vertex where the search should start
       * from. This does not need to belong to the aggregate.
       * @param aggregate The aggregate id.
       * @param graph The matrix graph to perform the search on.
       * @param visitedMap A map to mark the already visited vertices
       * @param aggregateVisitor A functor that is called with
       * each G::ConstEdgeIterator with an edge pointing to the
       * aggregate. Use DummyVisitor if these are of no interest.
       */
      template<bool reset, class G, class F, class VM>
      std::size_t breadthFirstSearch(const VertexDescriptor& start,
			     const AggregateDescriptor& aggregate, 
			     const G& graph,
			     F& aggregateVisitor,
				     VM& visitedMap) const;
      
      /**
       * @brief Breadth first search within an aggregate
       *
       * The template parameters: <br>
       * <dl><dt>L</dt><dd>A container type providing push_back(Vertex), and
       * pop_front() in case remove is true</dd>
       * <dt>remove</dt><dd> If true the entries in the visited list
       * will be removed.</dd>
       * <dt>reset</dt><dd>If true the visited flag will be reset after
       * the search</dd></dl>
       * @param start The vertex where the search should start
       * from. This does not need to belong to the aggregate.
       * @param aggregate The aggregate id.
       * @param graph The matrix graph to perform the search on.
       * @param visited A list to store the visited vertices in.
       * @param aggregateVisitor A functor that is called with
       * each G::ConstEdgeIterator with an edge pointing to the
       * aggregate. Use DummyVisitor these are of no interest.
       * @param nonAggregateVisitor A functor that is called with
       * each G::ConstEdgeIterator with an edge pointing to another
       * aggregate. Use DummyVisitor these are of no interest.
       * @param visitedMap A map to mark the already visited vertices
       */
      template<bool remove, bool reset, class G, class L, class F1, class F2, class VM>
      std::size_t breadthFirstSearch(const VertexDescriptor& start, 
			     const AggregateDescriptor& aggregate, 
			     const G& graph, L& visited, F1& aggregateVisitor,
			     F2& nonAggregateVisitor,
			     VM& visitedMap) const;

      /**
       * @brief Allocate memory for holding the information.
       * @param noVertices The total number of vertices to be
       * mapped.
       */
      void allocate(std::size_t noVertices);
      
      /**
       * @brief Get the number of vertices.
       */
      std::size_t noVertices() const;
      
      /**
       * @brief Free the allocated memory.
       */
      void free();
            
      /**
       * @brief Get the aggregate a vertex belongs to.
       * @param v The vertex we want to know the aggregate of.
       * @return The aggregate the vertex is mapped to.
       */
      AggregateDescriptor& operator[](const VertexDescriptor& v);
      
      /**
       * @brief Get the aggregate a vertex belongs to.
       * @param v The vertex we want to know the aggregate of.
       * @return The aggregate the vertex is mapped to.
       */
      const AggregateDescriptor& operator[](const VertexDescriptor& v) const;

      typedef const AggregateDescriptor* const_iterator;
      
      const_iterator begin() const
      {
	return aggregates_;
      }
      
      const_iterator end() const
      {
	return aggregates_+noVertices();
      }
      
      typedef AggregateDescriptor* iterator;
      
      iterator begin()
      {
	return aggregates_;
      }
      
      iterator end()
      {
	return aggregates_+noVertices();
      }
    private:
      /** @brief Prevent copying. */
      AggregatesMap(const AggregatesMap<V>& map)
      {
	throw "Auch!";
      }
      
      /** @brief Prevent assingment. */
      AggregatesMap<V>& operator=(const AggregatesMap<V>& map)
      {
	throw "Auch!";
	return this;
      }
      
      /**
       * @brief The aggregates the vertices belong to.
       */
      AggregateDescriptor* aggregates_;

      /**
       * @brief The number of vertices in the map.
       */
      std::size_t noVertices_;
    };
    
    
    /**
     * @brief A class for temporarily storing the vertices of an
     * aggregate in.
     */
    template<class G, class S>
    class Aggregate
    {
      
    public:
            
      /***
       * @brief The type of the matrix graph we work with.
       */
      typedef G MatrixGraph;
      /**
       * @brief The vertex descriptor type.
       */
      typedef typename MatrixGraph::VertexDescriptor Vertex;

      /**
       * @brief The allocator we use for our lists and the
       * set.
       */
      typedef PoolAllocator<Vertex,100> Allocator;
      
      /**
       * @brief The type of a single linked list of vertex
       * descriptors.
       */
      typedef SLList<Vertex,Allocator> VertexList;

      
      /**
       * @brief The type of a single linked list of vertex
       * descriptors.
       */
      typedef S VertexSet;

      /** @brief Const iterator over a vertex list. */
      typedef typename VertexList::const_iterator const_iterator;

      /** 
       * @brief Type of the mapping of aggregate members onto distance spheres.
       */
      typedef std::size_t* SphereMap;
      
      /**
       * @brief Constructor.
       * @param graph The matrix graph we work on.
       * @param aggregates The mapping of vertices onto aggregates.
       * @param connectivity The set of vertices connected to the aggregate.
       * distance spheres.
       */
      Aggregate(const MatrixGraph& graph, AggregatesMap<Vertex>& aggregates, 
		VertexSet& connectivity);
            
      /**
       * @brief Reconstruct the aggregat from an seed node.
       *
       * Will determine all vertices of the same agggregate
       * and reference those.
       */
      void reconstruct(const Vertex& vertex);
      
      /**
       * @brief Initialize the aggregate with one vertex.
       */
      void seed(const Vertex& vertex);
      
      /**
       * @brief Add a vertex to the aggregate.
       */
      void add(const Vertex& vertex);
      
      /**
       * @brief Clear the aggregate.
       */
      void clear();
      
      /**
       * @brief Get the size of the aggregate.
       */
      typename VertexList::size_type size();
      
      /**
       * @brief Get the id identifying the aggregate.
       */
      int id();
      
      /** @brief get an iterator over the vertices of the aggregate. */
      const_iterator begin() const;
      
      /** @brief get an iterator over the vertices of the aggregate. */
      const_iterator end() const;
      
    private:
      /**
       * @brief The vertices of the aggregate.
       */
      VertexList vertices_;
      
      /**
       * @brief The number of the currently referenced
       * aggregate.
       */
      int id_;

      /**
       * @brief The matrix graph the aggregates live on.
       */
      const MatrixGraph& graph_;
      
      /**
       * @brief The aggregate mapping we build.
       */
      AggregatesMap<Vertex>& aggregates_;

      /**
       * @brief The connections to other aggregates.
       */
      VertexSet& connected_;
    };
    
    /**
     * @brief Class for building the aggregates.
     */
    template<class G>
    class Aggregator
    {
    public:
            
      /**
       * @brief The matrix graph type used.
       */
      typedef G MatrixGraph;

      /**
       * @brief The vertex identifier
       */
      typedef typename MatrixGraph::VertexDescriptor Vertex;
      
      /** @brief The type of the aggregate descriptor. */
      typedef typename MatrixGraph::VertexDescriptor AggregateDescriptor;

      /**
       * @brief Constructor.
       */
      Aggregator();
      
      /**
       * @brief Destructor.
       */
      ~Aggregator();
      
      /**
       * @brief Build the aggregates.
       *
       * The template parameter C Is the type of the coarsening Criterion to
       * use.
       * @param m The matrix to build the aggregates accordingly.
       * @param graph A (sub) graph of the matrix.
       * @param aggregates Aggregate map we will build. All entries should be initialized
       * to UNAGGREGATED!
       * @param c The coarsening criterion to use.
       * @return The number of (not skipped) aggregates built.
       */
      template<class M, class C>
      tuple<int,int,int> build(const M& m, G& graph, 
			       AggregatesMap<Vertex>& aggregates, const C& c);
    private:
      /**
       * @brief The allocator we use for our lists and the
       * set.
       */
      typedef PoolAllocator<Vertex,1> Allocator;
      
      /**
       * @brief The single linked list we use.
       */
      typedef SLList<Vertex,Allocator> VertexList;

      /**
       * @brief The set of vertices we use.
       */
      typedef std::set<Vertex,std::less<Vertex>,Allocator> VertexSet;

      /**
       * @brief The type of mapping of aggregate members to spheres.
       */
      typedef std::size_t* SphereMap;
      
      /**
       * @brief The graph we aggregate for.
       */
      MatrixGraph* graph_;
      
      /**
       * @brief The vertices of the current aggregate-
       */
      Aggregate<MatrixGraph,VertexSet>* aggregate_;
      
      /**
       * @brief The vertices of the current aggregate front.
       */
      VertexList front_;

      /**
       * @brief The set of connected vertices.
       */
      VertexSet connected_;
            
      /**
       * @brief Number of vertices mapped.
       */
      int size_;
      
      /**
       * @brief Stack.
       */
      class Stack
      {
      public:
	static const Vertex NullEntry;
	
	Stack(const MatrixGraph& graph, 
	      const Aggregator<G>& aggregatesBuilder,
	      const AggregatesMap<Vertex>& aggregates);
	~Stack();
	bool push(const Vertex& v);
	void fill();
	Vertex pop();
      private:
	enum{ N = 256000 };
	
	/** @brief The graph we work on. */
	const MatrixGraph& graph_;
	/** @brief The aggregates builder. */
	const Aggregator<G>& aggregatesBuilder_;
	/** @brief The aggregates information. */
	const AggregatesMap<Vertex>& aggregates_;
	/** @brief The current size. */
	int size_;
	int maxSize_;
	/** @brief The index of the top element. */
	int head_;
	int filled_;
	
	/** @brief The values on the stack. */
	Vertex* vals_;

	void localPush(const Vertex& v);
      };

      friend class Stack;
      
      /**
       * @brief Build the dependency of the matrix graph.
       */
      template<class C>
      void buildDependency(MatrixGraph& graph,
			   const typename C::Matrix& matrix,
			   C criterion);

      /**
       * @brief Visits all neighbours of vertex belonging to a
       * specific aggregate.
       *
       * @param vertex The vertex whose neighbours we want to
       * visit.
       * @param aggregate The id of the aggregate.
       * @param visitor The visitor evaluated for each EdgeIterator
       * (by its method operator()(ConstEdgeIterator edge)
       */
      template<class V>
      void visitAggregateNeighbours(const Vertex& vertex, const AggregateDescriptor& aggregate,  
				    const AggregatesMap<Vertex>& aggregates,
				    V& visitor) const;

      /**
       * @brief An Adaptor for vsitors that only
       * evaluates edges pointing to a specific aggregate.
       */
      template<class V>
      class AggregateVisitor
      {
      public:
	/**
	 * @brief The type of the adapted visitor
	 */
	typedef V Visitor;
	/**
	 * @brief Constructor.
	 * @param aggregates The aggregate numbers of the
	 * vertices.
	 * @param  aggregate The id of the aggregate to visit.
	 * @param visitor The visitor.
	 */
	AggregateVisitor(const AggregatesMap<Vertex>& aggregates, const AggregateDescriptor& aggregate,
			 Visitor& visitor);
	
	/**
	 * @brief Examine an edge.
	 * 
	 * The edge will be examined by the adapted visitor if
	 * it belongs to the right aggregate.
	 */
	void operator()(const typename MatrixGraph::ConstEdgeIterator& edge);
	
      private:
	/** @brief Mapping of vertices to aggregates. */
	const AggregatesMap<Vertex>& aggregates_;
	/** @brief The aggregate id we want to visit. */
	AggregateDescriptor aggregate_;
	/** @brief The visitor to use on the aggregate. */
	Visitor* visitor_;
      };

      /**
       * @brief A simple counter functor.
       */
      class Counter
      {
      public:
	/** @brief Constructor */
	Counter();
	/** @brief Access the current count. */
	int value();
	
      protected:
	/** @brief Increment counter */
	void increment();
	/** @brief Decrement counter */
	void decrement();
	
      private:
	int count_;
      };

      
      /**
       * @brief Counts the number of edges to vertices belonging
       * to the aggregate front.
       */
      class FrontNeighbourCounter : public Counter
      {
      public:
	/**
	 * @brief Constructor.
	 * @param front The vertices of the front.
	 */
	FrontNeighbourCounter(const MatrixGraph& front);
	
	void operator()(const typename MatrixGraph::ConstEdgeIterator& edge);
	
      private:
	const MatrixGraph& graph_;
      };
      
      /**
       * @brief Count the number of neighbours of a vertex that belong
       * to the aggregate front.
       */
      int noFrontNeighbours(const Vertex& vertex) const;
      
      /**
       * @brief Counter of TwoWayConnections.
       */
      class TwoWayCounter : public Counter
      {
      public:
	void operator()(const typename MatrixGraph::ConstEdgeIterator& edge);
      };

      /**
       * @brief Count the number of twoway connection from
       * a vertex to an aggregate.
       *
       * @param vertex The vertex whose connections are counted.
       * @param aggregate The id of the aggregate the connections
       * should point to.
       * @param aggregates The mapping of the vertices onto aggregates.
       * @return The number of one way connections from the vertex to
       * the aggregate.
       */
      int twoWayConnections(const Vertex&, const AggregateDescriptor& aggregate,  
			     const AggregatesMap<Vertex>& aggregates) const;

      /**
       * @brief Counter of OneWayConnections.
       */
      class OneWayCounter : public Counter
      {
      public:
	void operator()(const typename MatrixGraph::ConstEdgeIterator& edge);
      };	

      /**
       * @brief Count the number of oneway connection from
       * a vertex to an aggregate.
       *
       * @param vertex The vertex whose connections are counted.
       * @param aggregate The id of the aggregate the connections
       * should point to.
       * @param aggregates The mapping of the vertices onto aggregates.
       * @return The number of one way connections from the vertex to
       * the aggregate.
       */
      int oneWayConnections(const Vertex&, const AggregateDescriptor& aggregate,  
			    const AggregatesMap<Vertex>& aggregates) const;
      
      /**
       * @brief Connectivity counter
       *
       * Increments count if the neighbour is already known as
       * connected or is not yet aggregated.
       */
      class ConnectivityCounter : public Counter
      {
      public:
	/**
	 * @brief Constructor.
	 * @param connected The set of connected aggregates.
	 * @param aggregates Mapping of the vertices onto the aggregates.
	 * @param aggregates The mapping of aggregates to vertices.
	 */
	ConnectivityCounter(const VertexSet& connected, const AggregatesMap<Vertex>& aggregates);

	void operator()(const typename MatrixGraph::ConstEdgeIterator& edge);

      private:
	/** @brief The connected aggregates. */
	const VertexSet& connected_;
	/** @brief The mapping of vertices to aggregates. */
	const AggregatesMap<Vertex>& aggregates_;
	
      };

      /**
       * @brief Get the connectivity of a vertex.
       *
       * For each unaggregated neighbour or neighbour of an aggregate 
       * that is already known as connected the count is increased by 
       * one. In all other cases by two.
       *
       * @param vertex The vertex whose connectivity we want.
       * @param aggregates The mapping of the vertices onto the aggregates.
       * @return The value of the connectivity.
       */
      double connectivity(const Vertex& vertex, const AggregatesMap<Vertex>& aggregates) const;
      
      /**
       * @brief Counts the edges depending on the dependency.
       *
       * If the inluence flag of the edge is set the counter is
       * increased and/or if the depends flag is set it is 
       * incremented, too.
       */
      class DependencyCounter: public Counter
      {
      public:
	/**
	 * @brief Constructor.
	 */
	DependencyCounter();
	
	void operator()(const typename MatrixGraph::ConstEdgeIterator& edge);
      };
      
      /**
       * @brief Adds the targets of each edge to
       * the list of front vertices.
       *
       * Vertices already marked as front nodes will not get added.
       */
      class FrontMarker
      {
      public:
	/**
	 * @brief Constructor.
	 *
	 * @param front The list to store the front vertices in.
	 * @param graph The matrix graph we work on.
	 */
	FrontMarker(VertexList& front, MatrixGraph& graph);
	
	void operator()(const typename MatrixGraph::ConstEdgeIterator& edge);
	
      private:
	/** @brief The list of front vertices. */
	VertexList& front_;
	/** @brief The matrix graph we work on. */
	MatrixGraph& graph_;
      };

      /**
       * @brief Mark the front of the current aggregate.
       * 
       * The front are the direct (unaggregated) neighbours of
       * the aggregate vertices.
       */
      void markFront(const AggregatesMap<Vertex>& aggregates);
      
      /**
       * @brief Unmarks all front vertices.
       */
      void unmarkFront();
      
      /**
       * @brief counts the dependency between a vertex and unaggregated
       * neighbours.
       *
       * If the inluence flag of the edge is set the counter is
       * increased and/or if the depends flag is set it is 
       * incremented, too.
       *
       * @param vertex The vertex whose neighbours we count.
       * @param aggregates The mapping of the vertices onto the aggregates.
       * @return The sum of the number of unaggregated
       * neighbours the vertex depends on and the number of unaggregated 
       * neighbours the vertex influences.
       */
      int unusedNeighbours(const Vertex& vertex, const AggregatesMap<Vertex>& aggregates) const;

      /**
       * @brief Count connections to neighbours.
       *
       * Counts the number of strong connections of a vertex to vertices
       * that are not yet aggregated
       * and the ones that belong to specific aggregate.
       *
       * @param vertex The vertex that we count the neighbours of.
       * @param aggregates The mapping of the vertices into aggregates.
       * @param aggregate The descriptor of the aggregate.
       * @return The pair of number of connections to unaggregate vertices
       * and number of connections to vertices of the specific aggregate.
       */
      std::pair<int,int> neighbours(const Vertex& vertex, 
				    const AggregateDescriptor& aggregate,
				    const AggregatesMap<Vertex>& aggregates) const;
      /**
       * @brief Counts the number of neighbours belonging to an aggregate.
       *
       *
       * If the inluence flag of the edge is set the counter is
       * increased and/or if the depends flag is set it is 
       * incremented, too.
       * 
       * @param vertex The vertex whose neighbours we count.
       * @param aggregate The aggregate id.
       * @param aggregates The mapping of the vertices onto the aggregates.
       * @return The sum of the number of 
       * neighbours belonging to the aggregate 
       * the vertex depends on and the number of
       * neighbours of the aggregate the vertex influences.
       */
      int aggregateNeighbours(const Vertex& vertex, const AggregateDescriptor& aggregate, const AggregatesMap<Vertex>& aggregates) const;
      
      /**
       * @brief Checks wether a vertex is admisible to be added to an aggregate.
       *
       * @param vertex The vertex whose admissibility id to be checked.
       * @param aggregate The id of the aggregate.
       * @param aggregates The mapping of the vertices onto aggregates.
       */
      bool admissible(const Vertex& vertex, const AggregateDescriptor& aggregate, const AggregatesMap<Vertex>& aggregates) const;
      
      /**
       * @brief Push the neighbours of the current aggregate on the stack.
       * 
       * @param stack The stack to push them on.
       * @param isolated If true only isolated vertices are push onto the stack.
       */
      void seedFromFront(Stack& stack,  bool isolated);
	
      /**
       * @brief The maximum distance of the vertex to any vertex in the 
       * current aggregate.
       *
       * @return The maximum of all shortest paths from the vertex to any 
       * vertex of the aggregate.
       */
      std::size_t distance(const Vertex& vertex, const AggregatesMap<Vertex>& aggregates);

      /**
       * @brief Find a strongly connected cluster of a vertex.
       *
       * @param vertex The vertex whose neighbouring aggregate we search.
       * @param aggregates The mapping of the vertices onto aggregates.
       * @return A vertex of neighbouring aggregate the vertex is allowed to
       * be added to.
       */
      Vertex mergeNeighbour(const Vertex& vertex, const AggregatesMap<Vertex>& aggregates) const;

      /**
       * @brief Grows the aggregate from a seed.
       *
       * @param seed The first vertex of the aggregate.
       * @param aggregates The mapping of he vertices onto the aggregates.
       * @param c The coarsen criterium.
       */
      template<class C>
      void growAggregate(const Vertex& vertex, const AggregatesMap<Vertex>& aggregates, const C& c);
    };

    template<class M, class N>
    inline void SymmetricDependency<M,N>::init(const Matrix* matrix)
    {
      matrix_ = matrix;
    }
    
    template<class M, class N>
    inline void SymmetricDependency<M,N>::initRow(const Row& row, int index)
    {
      maxValue_ = std::min(- std::numeric_limits<typename Matrix::field_type>::max(), std::numeric_limits<typename Matrix::field_type>::min());
      row_ = index;
      diagonal_ = norm_(matrix_->operator[](row_)[row_]);
    }

    template<class M, class N>
    inline void SymmetricDependency<M,N>::examine(const ColIter& col)
    {
      typename Matrix::field_type eij = norm_(*col);
      typename Matrix::field_type eji = norm_(matrix_->operator[](col.index())[row_]);
      
      // skip positve offdiagonals if norm preserves sign of them.
      if(!N::is_sign_preserving || eij<0 || eji<0)
	maxValue_ = std::max(maxValue_, 
			     eij /diagonal_ * eji/
			     norm_(matrix_->operator[](col.index())[col.index()]));
    }
    
    template<class M, class N>
    template<class G>
    inline void SymmetricDependency<M,N>::examine(G& graph, const typename G::EdgeIterator& edge, const ColIter& col)
    {      
      typename Matrix::field_type eij = norm_(*col);
      typename Matrix::field_type eji = norm_(matrix_->operator[](col.index())[row_]);
      
      // skip positve offdiagonals if norm preserves sign of them.
      if(!N::is_sign_preserving || (eij<0 || eji<0))
	if(eji / norm_(matrix_->operator[](edge.target())[edge.target()]) * 
	   eij/ diagonal_ > alpha() * maxValue_){
	edge.properties().setDepends();
	edge.properties().setInfluences();

	typename G::EdgeProperties& other = graph.getEdgeProperties(edge.target(), edge.source());
	other.setInfluences();
	other.setDepends();
      }
    }
    
    template<class M, class N>
    inline bool SymmetricDependency<M,N>::isIsolated()
    {
      return maxValue_  < beta();
    }
    
    
    template<class M, class N>
    inline void Dependency<M,N>::init(const Matrix* matrix)
    {
      matrix_ = matrix;
    }
    
    template<class M, class N>
    inline void Dependency<M,N>::initRow(const Row& row, int index)
    {
      maxValue_ = std::min(- std::numeric_limits<typename Matrix::field_type>::max(), std::numeric_limits<typename Matrix::field_type>::min());
      row_ = index;
      diagonal_ = norm_(matrix_->operator[](row_)[row_]);
    }

    template<class M, class N>
    inline void Dependency<M,N>::examine(const ColIter& col)
    {
      maxValue_ = std::max(maxValue_, 
			   -norm_(*col));
    }
    
    template<class M, class N>
    template<class G>
    inline void Dependency<M,N>::examine(G& graph, const typename G::EdgeIterator& edge, const ColIter& col)
    {      
      if(-norm_(*col) >= maxValue_ * alpha()){
        edge.properties().setDepends();
        typename G::EdgeProperties& other = graph.getEdgeProperties(edge.target(), edge.source());
        other.setInfluences();
	  }
    }
    
    template<class M, class N>
    inline bool Dependency<M,N>::isIsolated()
    {
      return maxValue_  < beta() * diagonal_;
    }
    
    template<class G,class S>
    Aggregate<G,S>::Aggregate(const MatrixGraph& graph, AggregatesMap<Vertex>& aggregates,
			    VertexSet& connected)
      : vertices_(), id_(-1), graph_(graph), aggregates_(aggregates),
	connected_(connected)
    {}
    
    template<class G,class S>
    void Aggregate<G,S>::reconstruct(const Vertex& vertex)
    {
      vertices_.push_back(vertex);
      typedef typename VertexList::const_iterator iterator;
      iterator begin = vertices_.begin();
      iterator end   = vertices_.end();
      throw "Not yet implemented";

      while(begin!=end){
	//for();
      }
      
    }
    
    template<class G,class S>
    inline void Aggregate<G,S>::seed(const Vertex& vertex)
    {
      dvverb<<"Connected cleared"<<std::endl;
      connected_.clear();
      vertices_.clear();
      connected_.insert(vertex);
      dvverb << " Inserting "<<vertex<<" size="<<connected_.size();
      id_ = vertex;
      add(vertex);
    }
    

    template<class G,class S>
    inline void Aggregate<G,S>::add(const Vertex& vertex)
    {
      vertices_.push_back(vertex);
      aggregates_[vertex]=id_;

      typedef typename MatrixGraph::ConstEdgeIterator iterator;
      const iterator end = graph_.endEdges(vertex);
      for(iterator edge = graph_.beginEdges(vertex); edge != end; ++edge){
	dvverb << " Inserting "<<aggregates_[edge.target()];
	connected_.insert(aggregates_[edge.target()]);
	dvverb <<" size="<<connected_.size();
      }
      dvverb <<std::endl;
    }
    template<class G,class S>
    inline void Aggregate<G,S>::clear()
    {
      vertices_.clear();
      connected_.clear();
      id_=-1;
    }
    
    template<class G,class S>
    inline typename Aggregate<G,S>::VertexList::size_type 
    Aggregate<G,S>::size()
    {
      return vertices_.size();
    }
    
    template<class G,class S>
    inline int Aggregate<G,S>::id()
    {
      return id_;
    }

    template<class G,class S>
    inline typename Aggregate<G,S>::const_iterator Aggregate<G,S>::begin() const
    {
      return vertices_.begin();
    }
    
    template<class G,class S>
    inline typename Aggregate<G,S>::const_iterator Aggregate<G,S>::end() const
    {
      return vertices_.end();
    }

    template<class V> 
    const V AggregatesMap<V>::UNAGGREGATED = std::numeric_limits<V>::max();
    
    template<class V> 
    const V AggregatesMap<V>::ISOLATED = std::numeric_limits<V>::max()-1;
    
    template<class V>
    AggregatesMap<V>::AggregatesMap()
      : aggregates_(0)
    {}

    template<class V>
    AggregatesMap<V>::~AggregatesMap()
    {
      if(aggregates_!=0)
	delete[] aggregates_;
    }
    

    template<class V>
    inline AggregatesMap<V>::AggregatesMap(std::size_t noVertices)
    {
      allocate(noVertices);
    }

    template<class V>
    inline std::size_t AggregatesMap<V>::AggregatesMap::noVertices() const
    {
      return noVertices_;
    }
    
    template<class V>
    inline void AggregatesMap<V>::allocate(std::size_t noVertices)
    {
      aggregates_ = new AggregateDescriptor[noVertices];
      noVertices_ = noVertices;
      
      for(std::size_t i=0; i < noVertices; i++)
	aggregates_[i]=UNAGGREGATED;
    }

    template<class V>
    inline void AggregatesMap<V>::free()
    {
      assert(aggregates_ != 0);
      delete[] aggregates_;
      aggregates_=0;
    }
    
    template<class V>
    inline typename AggregatesMap<V>::AggregateDescriptor& 
    AggregatesMap<V>::operator[](const VertexDescriptor& v)
    {
      return aggregates_[v];
    }

    template<class V>
    inline const typename AggregatesMap<V>::AggregateDescriptor& 
    AggregatesMap<V>::operator[](const VertexDescriptor& v) const
    {
      return aggregates_[v];
    }

    template<class V>
    template<bool reset, class G, class F,class VM>
    inline std::size_t AggregatesMap<V>::breadthFirstSearch(const V& start,
						    const AggregateDescriptor& aggregate, 
						    const G& graph, F& aggregateVisitor,
							    VM& visitedMap) const
    {
      VertexList vlist;
      
      DummyEdgeVisitor dummy;
      return breadthFirstSearch<true,reset>(start, aggregate, graph, vlist, aggregateVisitor, dummy, visitedMap);
    }
      
    template<class V>
    template<bool remove, bool reset, class G, class L, class F1, class F2, class VM>
    std::size_t AggregatesMap<V>::breadthFirstSearch(const V& start,
					     const AggregateDescriptor& aggregate, 
					     const G& graph,
					     L& visited,
					     F1& aggregateVisitor,
					     F2& nonAggregateVisitor,
					     VM& visitedMap) const
    {
      typedef typename L::const_iterator ListIterator;
      int visitedSpheres = 0;
      
      visited.push_back(start);
      put(visitedMap, start, true);
      
      ListIterator current = visited.begin();
      ListIterator end = visited.end();
      std::size_t i=0, size=visited.size();
      
      // visit the neighbours of all vertices of the
      // current sphere.
      while(current != end){
	
	for(;i<size; ++current, ++i){
	  typedef typename G::ConstEdgeIterator EdgeIterator;
	  const EdgeIterator endEdge = graph.endEdges(*current);
	  
	  for(EdgeIterator edge = graph.beginEdges(*current);
	      edge != endEdge; ++edge){
	  
	    if(aggregates_[edge.target()]==aggregate){
	      if(!get(visitedMap, edge.target())){
		put(visitedMap, edge.target(), true);
		visited.push_back(edge.target());
		aggregateVisitor(edge);
	      }
	    }else
	      nonAggregateVisitor(edge);
	  }
	}	
	end = visited.end();
	size = visited.size();
	if(current != end)
	  visitedSpheres++;
      }
      
      if(reset)
	for(current = visited.begin(); current != end; ++current)
	  put(visitedMap, *current, false);
      
	
      if(remove)
	visited.clear();
      
      return visitedSpheres;
    }
    
    template<class G>
    Aggregator<G>::Aggregator()
      : graph_(0), aggregate_(0), front_(), connected_(), size_(-1)
    {}
    
    template<class G>
    Aggregator<G>::~Aggregator()
    {
      size_=-1;
    }
    
    template<class G>
    template<class C>
    void Aggregator<G>::buildDependency(MatrixGraph& graph,
					const typename C::Matrix& matrix,
					C criterion)
    {
      // The Criterion we use for building the dependency.
      typedef C Criterion;
     
      //      assert(graph.isBuilt());
      typedef typename C::Matrix Matrix;
      typedef typename MatrixGraph::VertexIterator VertexIterator;
      
      criterion.init(&matrix);
      
      for(VertexIterator vertex = graph.begin(); vertex != graph.end(); ++vertex){
	typedef typename Matrix::row_type Row;
	
	const Row& row = matrix[*vertex];
	
	// Tell the criterion what row we will examine now
	// This might for example be used for calculating the
	// maximum offdiagonal value
	criterion.initRow(row, *vertex);
	
	// On a first path all columns are examined. After this 
	// the calculator should know whether the vertex is isolated.
	typedef typename Matrix::ConstColIterator ColIterator;
	ColIterator end = row.end();
	for(ColIterator col = row.begin(); col != end; ++col)
	  if(col.index()!=*vertex)
	    criterion.examine(col);
	
	// reset the vertex properties
	//vertex.properties().reset();
		
	// Check whether the vertex is isolated.
	if(criterion.isIsolated()){
	  //std::cout<<"ISOLATED: "<<*vertex<<std::endl;
	  vertex.properties().setIsolated();
	}else{
	  // Examine all the edges beginning at this vertex.
	  typedef typename MatrixGraph::EdgeIterator EdgeIterator;
	  typedef typename Matrix::ConstColIterator ColIterator;
	  EdgeIterator end = vertex.end();
	  ColIterator col = matrix[*vertex].begin();
	  
	  for(EdgeIterator edge = vertex.begin(); edge!= end; ++edge, ++col){
	    // Move to the right column.
	    while(col.index()!=edge.target())
	      ++col;
	    criterion.examine(graph, edge, col);
	  }
	}
	
      }
    }

    
    template<class G>
    template<class V>
    inline Aggregator<G>::AggregateVisitor<V>::AggregateVisitor(const AggregatesMap<Vertex>& aggregates, 
							const AggregateDescriptor& aggregate, V& visitor)
      : aggregates_(aggregates), aggregate_(aggregate), visitor_(&visitor)
    {}
    
    template<class G>
    template<class V>
    inline void Aggregator<G>::AggregateVisitor<V>::operator()(const typename MatrixGraph::ConstEdgeIterator& edge)
    {
      if(aggregates_[edge.target()]==aggregate_)
	visitor_->operator()(edge);
    }
    
    template<class G>
    template<class V>
    inline void Aggregator<G>::visitAggregateNeighbours(const Vertex& vertex,
							const AggregateDescriptor& aggregate, 
							const AggregatesMap<Vertex>& aggregates,
							V& visitor) const
    {  
      // Only evaluates for edge pointing to the aggregate
      AggregateVisitor<V> v(aggregates, aggregate, visitor);
      visitNeighbours(*graph_, vertex, v);
    }
    
    
    template<class G>
    inline Aggregator<G>::Counter::Counter()
      : count_(0)
    {}
    
    template<class G>
    inline void Aggregator<G>::Counter::increment()
    {
      ++count_;
    }

    template<class G>
    inline void Aggregator<G>::Counter::decrement()
    {
      --count_;
    }
    template<class G>
    inline int Aggregator<G>::Counter::value()
    {
      return count_;
    }
    
    template<class G>
    inline void  Aggregator<G>::TwoWayCounter::operator()(const typename MatrixGraph::ConstEdgeIterator& edge)
    {
      if(edge.properties().isTwoWay())
	Counter::increment();
    }
    
    template<class G>
    int Aggregator<G>::twoWayConnections(const Vertex& vertex, const AggregateDescriptor& aggregate,
					 const AggregatesMap<Vertex>& aggregates) const
    {
      TwoWayCounter counter;
      visitAggregateNeighbours(vertex, aggregate, aggregates, counter);
      return counter.value();
    }
    
    template<class G>
    int Aggregator<G>::oneWayConnections(const Vertex& vertex, const AggregateDescriptor& aggregate,
					 const AggregatesMap<Vertex>& aggregates) const
    {
      OneWayCounter counter;
      visitAggregateNeighbours(vertex, aggregate, aggregates, counter);
      return counter.value();
    }
    
    template<class G>
    inline void Aggregator<G>::OneWayCounter::operator()(const typename MatrixGraph::ConstEdgeIterator& edge)
    {
      if(edge.properties().isOneWay())
	Counter::increment();
    }
    
    template<class G>
    inline Aggregator<G>::ConnectivityCounter::ConnectivityCounter(const VertexSet& connected,
								   const AggregatesMap<Vertex>& aggregates)
      : Counter(), connected_(connected), aggregates_(aggregates)
    {}
    
    
    template<class G>
    inline void Aggregator<G>::ConnectivityCounter::operator()(const typename MatrixGraph::ConstEdgeIterator& edge)
    {
      if(connected_.find(aggregates_[edge.target()]) == connected_.end() || aggregates_[edge.target()]==AggregatesMap<Vertex>::UNAGGREGATED)
	// Would be a new connection
	Counter::increment();
      else{
	Counter::increment();
	Counter::increment();
      }
    }
    
    template<class G>
    inline double Aggregator<G>::connectivity(const Vertex& vertex, const AggregatesMap<Vertex>& aggregates) const
    {
      ConnectivityCounter counter(connected_, aggregates);
      double noNeighbours=visitNeighbours(*graph_, vertex, counter);
      return (double)counter.value()/noNeighbours;
    }
    
    template<class G>
    inline Aggregator<G>::DependencyCounter::DependencyCounter()
      : Counter()
    {}
    
    template<class G>
    inline void Aggregator<G>::DependencyCounter::operator()(const typename MatrixGraph::ConstEdgeIterator& edge)
    {
      if(edge.properties().depends())
	Counter::increment();
      if(edge.properties().influences())
	Counter::increment();
    }
    
    template<class G>
    int Aggregator<G>::unusedNeighbours(const Vertex& vertex, const AggregatesMap<Vertex>& aggregates) const
    {
      return aggregateNeighbours(vertex, AggregatesMap<Vertex>::UNAGGREGATED, aggregates);
    }
    
    template<class G>
    std::pair<int,int> Aggregator<G>::neighbours(const Vertex& vertex,
						 const AggregateDescriptor& aggregate,
						 const AggregatesMap<Vertex>& aggregates) const
    {
      DependencyCounter unused, aggregated;
      typedef AggregateVisitor<DependencyCounter> Counter;
      typedef tuple<Counter,Counter> CounterTuple;
      CombinedFunctor<CounterTuple> visitors(CounterTuple(Counter(aggregates, AggregatesMap<Vertex>::UNAGGREGATED, unused), Counter(aggregates, aggregate, aggregated)));
      visitNeighbours(*graph_, vertex, visitors);
      return std::make_pair(unused.value(), aggregated.value());
}
      
      
    template<class G>
    int Aggregator<G>::aggregateNeighbours(const Vertex& vertex, const AggregateDescriptor& aggregate, const AggregatesMap<Vertex>& aggregates) const
    {
      DependencyCounter counter;
      visitAggregateNeighbours(vertex, aggregate, aggregates, counter);
      return counter.value();
    }
    
    template<class G>
    std::size_t Aggregator<G>::distance(const Vertex& vertex, const AggregatesMap<Vertex>& aggregates)
    {
      typename PropertyMapTypeSelector<VertexVisitedTag,G>::Type visitedMap = get(VertexVisitedTag(), *graph_);
      VertexList vlist;
      typename AggregatesMap<Vertex>::DummyEdgeVisitor dummy;
      return aggregates.template breadthFirstSearch<true,true>(vertex, 
						      aggregate_->id(), *graph_, 
					   vlist, dummy, dummy, visitedMap);
    }
    
    template<class G>
    inline Aggregator<G>::FrontMarker::FrontMarker(VertexList& front, MatrixGraph& graph)
      : front_(front), graph_(graph)
    {}
    
    template<class G>
    inline void Aggregator<G>::FrontMarker::operator()(const typename MatrixGraph::ConstEdgeIterator& edge)
    {
      Vertex target = edge.target();
      
      if(!graph_.getVertexProperties(target).front()){
	front_.push_back(target);
	graph_.getVertexProperties(target).setFront();
      }
    }
	

    template<class G>
    void Aggregator<G>::markFront(const AggregatesMap<Vertex>& aggregates)
    {
      assert(front_.size()==0);
      FrontMarker frontBuilder(front_, *graph_);
      typedef typename Aggregate<G,VertexSet>::const_iterator Iterator;
      
      for(Iterator vertex=aggregate_->begin(); vertex != aggregate_->end(); ++vertex)
	visitAggregateNeighbours(*vertex, AggregatesMap<Vertex>::UNAGGREGATED, aggregates, frontBuilder);
      
    }

    template<class G>
    inline bool Aggregator<G>::admissible(const Vertex& vertex, const AggregateDescriptor& aggregate, const AggregatesMap<Vertex>& aggregates) const
    {
      // Todo
      Dune::dvverb<<" Admissible not yet implemented!"<<std::endl;
      return true;
    }
    
    template<class G>
    void Aggregator<G>::unmarkFront()
    {      
      typedef typename VertexList::const_iterator Iterator;
      
      for(Iterator vertex=front_.begin(); vertex != front_.end(); ++vertex)
	graph_->getVertexProperties(*vertex).resetFront();      
      
      front_.clear();
    }

    template<class G>
    inline typename G::VertexDescriptor Aggregator<G>::mergeNeighbour(const Vertex& vertex, const AggregatesMap<Vertex>& aggregates) const
    {
      typedef typename MatrixGraph::ConstEdgeIterator Iterator;
      
      Iterator end = graph_->endEdges(vertex);
      for(Iterator edge = graph_->beginEdges(vertex); edge != end; ++edge){
	if(aggregates[edge.target()] != AggregatesMap<Vertex>::UNAGGREGATED &&
	   graph_->getVertexProperties(edge.target()).isolated() == graph_->getVertexProperties(edge.source()).isolated()){
	  if( graph_->getVertexProperties(vertex).isolated() || 
	      ((edge.properties().depends() || edge.properties().influences())
	       && admissible(vertex, aggregates[edge.target()], aggregates)))
	    return edge.target();
	}
      }
      return AggregatesMap<Vertex>::UNAGGREGATED;
    }
    
    template<class G>
    Aggregator<G>::FrontNeighbourCounter::FrontNeighbourCounter(const MatrixGraph& graph)
      : Counter(), graph_(graph)
    {}

    template<class G>
    void Aggregator<G>::FrontNeighbourCounter::operator()(const typename MatrixGraph::ConstEdgeIterator& edge)
    {
      if(graph_.getVertexProperties(edge.target()).front())
	Counter::increment();
    }
    
    template<class G>
    int Aggregator<G>::noFrontNeighbours(const Vertex& vertex) const
    {
      FrontNeighbourCounter counter(*graph_);
      visitNeighbours(*graph_, vertex, counter);
      return counter.value();
    }
    
    template<class G>
    template<class C>
    void Aggregator<G>::growAggregate(const Vertex& seed, const AggregatesMap<Vertex>& aggregates, const C& c)
    {
      while(aggregate_->size() < c.minAggregateSize()){
	int maxTwoCons=0, maxOneCons=0, maxNeighbours=-1;
	double maxCon=-1;
		
	Vertex candidate = AggregatesMap<Vertex>::UNAGGREGATED;
	
	unmarkFront();
	markFront(aggregates);

	typedef typename VertexList::const_iterator Iterator;
	  
	for(Iterator vertex = front_.begin(); vertex != front_.end(); ++vertex){
	  // Only nonisolated nodes are considered
	  if(graph_->getVertexProperties(*vertex).isolated())
	    continue;
	  
	  int twoWayCons = twoWayConnections(*vertex, aggregate_->id(), aggregates);
	  
	  /* The case of two way connections. */
	  if( maxTwoCons == twoWayCons && twoWayCons > 0){
	    double con = connectivity(*vertex, aggregates);

	    if(con == maxCon){
	      int neighbours = noFrontNeighbours(*vertex);
	      
	      if(neighbours > maxNeighbours){
		maxNeighbours = neighbours;
		
		std::size_t distance_ = distance(*vertex, aggregates);

		if(c.maxDistance() >= distance_){
		  candidate = *vertex;
		}
	      }
	    }else if( con > maxCon){
	      maxCon = con;
	      maxNeighbours = noFrontNeighbours(*vertex);
	      std::size_t distance_ = distance(*vertex, aggregates);

	      if(c.maxDistance() >= distance_){
		candidate = *vertex;
	      }
	    }
	  }else if(twoWayCons > maxTwoCons){
	    maxTwoCons = twoWayCons;
	    maxCon = connectivity(*vertex, aggregates);
	    maxNeighbours = noFrontNeighbours(*vertex);
	    std::size_t distance_ = distance(*vertex, aggregates);
	    
	    if(c.maxDistance() >= distance_){
	      candidate = *vertex;
	    }
	    
	    // two way connections preceed
	    maxOneCons = std::numeric_limits<int>::max();
	  }
	  
	  if(twoWayCons > 0)
	    continue; // THis is a two-way node, skip tests for one way nodes

	  /* The one way case */
	  int oneWayCons = oneWayConnections(*vertex, aggregate_->id(), aggregates);
	  
	  if(oneWayCons==0)
	    continue; // No strong connections, skip the tests.

	  if(!admissible(*vertex, aggregate_->id(), aggregates))
	    continue;
	  
	  if( maxOneCons == oneWayCons && oneWayCons > 0){
	    double con = connectivity(*vertex, aggregates);

	    if(con == maxCon){
	      int neighbours = noFrontNeighbours(*vertex);
	      
	      if(neighbours > maxNeighbours){
		maxNeighbours = neighbours;
		std::size_t distance_ = distance(*vertex, aggregates);
		
		if(c.maxDistance() >= distance_){
		  candidate = *vertex;
		}
	      }
	    }else if( con > maxCon){
	      maxCon = con;
	      maxNeighbours = noFrontNeighbours(*vertex);
	      std::size_t distance_ = distance(*vertex, aggregates);
	      if(c.maxDistance() >= distance_){
		candidate = *vertex;
	      }
	    }
	  }else if(oneWayCons > maxOneCons){
	    maxOneCons = oneWayCons;
	    maxCon = connectivity(*vertex, aggregates);
	    maxNeighbours = noFrontNeighbours(*vertex);
	    std::size_t distance_ = distance(*vertex, aggregates);
		
	    if(c.maxDistance() >= distance_){
	      candidate = *vertex;
	    }
	  }
	}
	
	
	if(candidate == AggregatesMap<Vertex>::UNAGGREGATED)
	  break; // No more candidates found

	aggregate_->add(candidate);
      }
    }
      
    template<typename V>
    template<typename M, typename G, typename C>
    tuple<int,int,int> AggregatesMap<V>::buildAggregates(const M& matrix, G& graph, const C& criterion)
    {
      Aggregator<G> aggregator;
      return aggregator.build(matrix, graph, *this, criterion);
    }
    
    template<class G>
    template<class M, class C>
    tuple<int,int,int> Aggregator<G>::build(const M& m, G& graph, AggregatesMap<Vertex>& aggregates, const C& c)
    {
      // Stack for fast vertex access
      Stack stack_(graph, *this, aggregates);
      
      graph_ = &graph;
            
      aggregate_ = new Aggregate<G,VertexSet>(graph, aggregates, connected_);
      
      // Allocate the mapping to aggregate.
      size_ = graph.maxVertex();

      Timer watch;
      watch.reset();

      buildDependency(graph, m, c);

      dverb<<"Build dependency took "<< watch.elapsed()<<" seconds."<<std::endl;
      int noAggregates, conAggregates, isoAggregates, oneAggregates;
      noAggregates = conAggregates = isoAggregates = oneAggregates = 0;
      
      while(true){
	Vertex seed = stack_.pop();
	
	if(seed == Stack::NullEntry)
	  // No more unaggregated vertices. We are finished!
	  break;
	
	// Debugging output
	if((noAggregates+1)%10000 == 0)
	  Dune::dverb<<"c";
	
	aggregate_->seed(seed);
	
	
	if(graph.getVertexProperties(seed).isolated()){
	  // isolated vertices are not aggregated but skipped on the coarser levels.
	  aggregates[seed]=AggregatesMap<Vertex>::ISOLATED;
	  ++isoAggregates;
	  // skip rest as no agglomeration is done.
	  continue;
	}else
	  growAggregate(seed, aggregates, c);
	
	
	/* The rounding step. */
	while(aggregate_->size() < c.maxAggregateSize()){
	  
	  unmarkFront();
	  markFront(aggregates);
	  
	  Vertex candidate = AggregatesMap<Vertex>::UNAGGREGATED;
	  
	  typedef typename VertexList::const_iterator Iterator;
	  
	  for(Iterator vertex = front_.begin(); vertex != front_.end(); ++vertex){

	    if(graph.getVertexProperties(*vertex).isolated())
	      continue; // No isolated nodes here

	    if(twoWayConnections( *vertex, aggregate_->id(), aggregates) == 0 && 
	       (oneWayConnections( *vertex, aggregate_->id(), aggregates) == 0 || 
		!admissible( *vertex, aggregate_->id(), aggregates) ))
	      continue;
	  
	    std::pair<int,int> neighbourPair=neighbours(*vertex, aggregate_->id(),
						     aggregates);
	    
	    //if(aggregateNeighbours(*vertex, aggregate_->id(), aggregates) <= unusedNeighbours(*vertex, aggregates))
	    // continue;
	    
	    if(neighbourPair.first >= neighbourPair.second)
	      continue;
	    	    
	    if(distance(*vertex, aggregates) > c.maxDistance())
	      continue; // Distance too far
	    candidate = *vertex;
	    break;
	  }
	  	  
	  if(candidate == AggregatesMap<Vertex>::UNAGGREGATED) break; // no more candidates found.
	  
	  aggregate_->add(candidate);
	  
	}
	
	// try to merge aggregates consisting of only one nonisolated vertex with other aggregates
	if(aggregate_->size()==1)
	  if(!graph.getVertexProperties(seed).isolated()){
	    Vertex mergedNeighbour = mergeNeighbour(seed, aggregates);

	    if(mergedNeighbour != AggregatesMap<Vertex>::UNAGGREGATED){
	      // assign vertex to the neighbouring cluster
	      aggregates[seed] = aggregates[mergedNeighbour];	      
	    }else{
	      ++oneAggregates;
	      ++conAggregates;
	    }
	    
	  }else{
	    ++oneAggregates;
	    ++isoAggregates;
	  }
	else{
	  if(graph.getVertexProperties(seed).isolated())
	    ++isoAggregates;
	  else
	    ++conAggregates;
	}
	unmarkFront();
	markFront(aggregates);
	seedFromFront(stack_, graph.getVertexProperties(seed).isolated());
	unmarkFront();
      }
      
      Dune::dinfo<<"connected aggregates: "<<conAggregates;
      Dune::dinfo<<" isolated aggregates: "<<isoAggregates;
      Dune::dinfo<<" one node aggregates: "<<oneAggregates<<std::endl;
      
      delete aggregate_;
      return make_tuple(conAggregates,isoAggregates,oneAggregates);
    }
    
    template<class G>
    inline void Aggregator<G>::seedFromFront(Stack& stack_, bool isolated)
    {
      typedef typename VertexList::const_iterator Iterator;
      
      Iterator end= front_.end();
      int count=0;
      for(Iterator vertex=front_.begin(); vertex != end; ++vertex,++count)
	stack_.push(*vertex);
      /*
      if(MINIMAL_DEBUG_LEVEL<=2 && count==0 && !isolated)
	Dune::dverb<< " no vertices pushed for nonisolated aggregate!"<<std::endl;
      */
    }

    template<class G>
    Aggregator<G>::Stack::Stack(const MatrixGraph& graph, const Aggregator<G>& aggregatesBuilder,
				const AggregatesMap<Vertex>& aggregates)
      : graph_(graph), aggregatesBuilder_(aggregatesBuilder), aggregates_(aggregates), size_(0), maxSize_(0), head_(0), filled_(0)
    {
      vals_ = new  Vertex[N];
    }
    
    template<class G>
    Aggregator<G>::Stack::~Stack()
    {
      Dune::dverb << "Max stack size was "<<maxSize_<<" filled="<<filled_<<std::endl;
      delete[] vals_;
    }
    
    template<class G> 
    const typename Aggregator<G>::Vertex Aggregator<G>::Stack::NullEntry 
    = std::numeric_limits<typename G::VertexDescriptor>::max();

    template<class G>
    inline bool Aggregator<G>::Stack::push(const Vertex& v)
    {
      if(aggregates_[v] == AggregatesMap<Vertex>::UNAGGREGATED){
	localPush(v);
	return true;
      }else
	return false;
    }
    
    template<class G>
    inline void Aggregator<G>::Stack::localPush(const Vertex& v)
    {
      vals_[head_] = v;
      size_ = std::min<int>(size_+1, N);
      head_ = (head_+N+1)%N;
    }
    
    template<class G>
    void Aggregator<G>::Stack::fill()
    {
      int isolated = 0, connected=0;
      int isoumin, umin;
      filled_++;
      
      head_ = size_ = 0;
      isoumin = umin = std::numeric_limits<int>::max();
      
      typedef typename MatrixGraph::ConstVertexIterator Iterator;
      
      const Iterator end = graph_.end();
      
      for(Iterator vertex = graph_.begin(); vertex != end; ++vertex){
	// Skip already aggregated vertices
	if(aggregates_[*vertex] != AggregatesMap<Vertex>::UNAGGREGATED)
	  continue;
	
	if(vertex.properties().isolated()){
	  isoumin = std::min(isoumin, aggregatesBuilder_.unusedNeighbours(*vertex, aggregates_));
	  isolated++;
	}else{
	  umin = std::min(umin, aggregatesBuilder_.unusedNeighbours(*vertex, aggregates_));
	  connected++;
	}
      }
      
      if(connected + isolated == 0)
	// No unaggregated vertices.
	return;
	
      if(connected > 0){
	// Connected vertices have higher priority.
	 for(Iterator vertex = graph_.begin(); vertex != end; ++vertex)
	   if(aggregates_[*vertex] == AggregatesMap<Vertex>::UNAGGREGATED && !vertex.properties().isolated() 
	      && aggregatesBuilder_.unusedNeighbours(*vertex, aggregates_) == umin)
	     localPush(*vertex);
      }else{
	for(Iterator vertex = graph_.begin(); vertex != end; ++vertex)
	   if(aggregates_[*vertex] == AggregatesMap<Vertex>::UNAGGREGATED && vertex.properties().isolated() 
	      && aggregatesBuilder_.unusedNeighbours(*vertex, aggregates_) == isoumin)
	     localPush(*vertex);
      }
      maxSize_ = std::max(size_, maxSize_);
    }

    template<class G>
    inline typename G::VertexDescriptor Aggregator<G>::Stack::pop()
    {
      while(size_>0){
	head_ = (head_ + N -1) % N;
	size_--;
	Vertex v = vals_[head_];
	if(aggregates_[v]==AggregatesMap<Vertex>::UNAGGREGATED)
	  return v;
      }
      // Stack is empty try to fill it
      fill();
      
      // try again
      while(size_>0){
	head_ = (head_ + N -1) % N;
	size_--;
	Vertex v = vals_[head_];
	if(aggregates_[v]==AggregatesMap<Vertex>::UNAGGREGATED)
	  return v;
      }
      return NullEntry;
    }

    template<class V>
    void printAggregates2d(const AggregatesMap<V>& aggregates, int n, int m,  std::ostream& os)
    {
      std::ios_base::fmtflags oldOpts=os.flags();
      
      os.setf(std::ios_base::right, std::ios_base::adjustfield);
      
      V max=0;
      int width=1;

      for(int i=0; i< n*m; i++)
	max=std::max(max, aggregates[i]);
      
      for(int i=10; i < 1000000; i*=10)
	if(max/i>0)
	  width++;
	else
	  break;
      
      for(int j=0, entry=0; j < m; j++){
	for(int i=0; i<n; i++, entry++){
	  os.width(width);
	  os<<aggregates[entry]<<" ";
	}
	
	os<<std::endl;
      }
      os<<std::endl;
      os.flags(oldOpts);
    }
    
    
  }// namespace Amg
  
}// namespace Dune


#endif
