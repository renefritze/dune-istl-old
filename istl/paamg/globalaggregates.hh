#ifndef GLOBALAGGREGATES_HH
#define GLOBALAGGREGATES_HH

#include "aggregates.hh"
#include<dune/istl/indexset.hh>
#include<dune/istl/communicator.hh>
namespace Dune
{
  namespace Amg
  {
    
    template<typename T, typename TI>
    struct GlobalAggregatesMap
    {
    public:
      typedef TI ParallelIndexSet;
      
      typedef typename ParallelIndexSet::GlobalIndex GlobalIndex;
      
      typedef typename ParallelIndexSet::GlobalIndex IndexedType;
  
      typedef typename ParallelIndexSet::LocalIndex LocalIndex;
      
      typedef T Vertex;
      
      GlobalAggregatesMap(AggregatesMap<Vertex>& aggregates,
			  ParallelIndexSet& indexset)
	: aggregates_(aggregates), indexset_(indexset)
      {}
  
      inline const GlobalIndex& operator[](std::size_t index)const
      {
	const Vertex& aggregate = aggregates_[index];
	const Dune::IndexPair<GlobalIndex,LocalIndex >& pair = indexset_.pair(aggregate);
	return pair.global();
      }
  
      inline void put(const GlobalIndex& global, size_t i)
      {
	aggregates_[i]=indexset_[global].local();
    
      }

    private:
      AggregatesMap<Vertex>& aggregates_;
      GlobalLookupIndexSet<ParallelIndexSet> indexset_;
    };

    template<typename T, typename TI>
    struct AggregatesGatherScatter
    {
      typedef TI ParallelIndexSet;
      typedef typename ParallelIndexSet::GlobalIndex GlobalIndex;
      
      static const GlobalIndex& gather(const GlobalAggregatesMap<T,TI>& ga, size_t i)
      {
	return ga[i];
      }
  
      static void scatter(GlobalAggregatesMap<T,TI>& ga, GlobalIndex global, size_t i)
      {
	ga.put(global, i);
      }
    };
  } // end Amg namespace

 
  template<typename T, typename TI>
  struct CommPolicy<Amg::GlobalAggregatesMap<T,TI> >
  {
    typedef Amg::AggregatesMap<T> Type;
    typedef typename Amg::GlobalAggregatesMap<T,TI>::IndexedType IndexedType;
    typedef SizeOne IndexedTypeFlag;
    static int getSize(const Type&, int)
    {
      return 1;
    }
  };

} // end Dune namespace

#endif
