#include<iostream>
#include<dune/istl/selection.hh>
#include<dune/common/timer.hh>
#include<dune/common/enumset.hh>
#include<dune/istl/remoteindices.hh>

enum GridFlags{ 
  owner, overlap, border 
};

template<class T>
int meassure(const T& selection)
{
  /*
  return meassure<1>(selection);
}

template<int LOOPS, class T>
int meassure(const T& selection)
{*/
  typedef typename T::const_iterator iterator;
  
  const iterator end = selection.end();
  int count=0;
  Dune::Timer timer;
  timer.reset();
  for(int i=0; i<100; i++)
    for(iterator iter = selection.begin(); iter != end; ++iter)
      count+=*iter;

  std::cout<<" took "<< timer.elapsed()<<" seconds"<<std::endl;
  
  return count;
}

template<int SIZE>
void test()
{
  const int Nx = SIZE;
  const int Ny = SIZE;
  
  // Process configuration
  
  Dune::IndexSet<int,Dune::ParallelLocalIndex<GridFlags> > distIndexSet;

  distIndexSet.beginResize();
  
  for(int y=0, i=0; y < Ny; y++)
    for(int x=0; x < Nx; x++, i++){
      GridFlags flag = owner;
      if(x==0 || x == Nx-1 || y ==0 || y==Ny-1)
	flag = overlap;
      
      distIndexSet.add(i, Dune::ParallelLocalIndex<GridFlags> (i, flag, true));
    }
  
  distIndexSet.endResize();
  
  Dune::UncachedSelection<Dune::EnumItem<GridFlags,owner>,int,Dune::ParallelLocalIndex<GridFlags> >
    ownerUncached(distIndexSet);
  
  Dune::Selection<Dune::EnumItem<GridFlags,owner>,int,Dune::ParallelLocalIndex<GridFlags> >
    ownerCached(distIndexSet);

  Dune::UncachedSelection<Dune::EnumItem<GridFlags,overlap>,int,Dune::ParallelLocalIndex<GridFlags> >
    overlapUncached(distIndexSet);
  
  Dune::Selection<Dune::EnumItem<GridFlags,overlap>,int,Dune::ParallelLocalIndex<GridFlags> >
    overlapCached(distIndexSet);

  int count=0;
  
  std::cout<<" Owner selection uncached:";
  count+=meassure(ownerUncached);
  std::cout<<" Owner selection cached:";
  count+=meassure(ownerCached);
  std::cout<<" Overlap selection uncached:";
  count+=meassure(overlapUncached);
  std::cout<<" Overlap selection cached:";
  count+=meassure(overlapCached);
std::cout<<count<<std::endl;
}

int main()
{
  test<1000>();
}