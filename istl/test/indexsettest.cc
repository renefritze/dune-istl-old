#include"config.h"
#include<iostream>
#include<cassert>
#include<dune/istl/indexset.hh>

int testDeleteIndices()
{
  Dune::ParallelIndexSet<int,Dune::LocalIndex,15> indexSet;
  Dune::ParallelIndexSet<int,Dune::LocalIndex,25> indexSet1;

  indexSet.beginResize();
  indexSet1.beginResize();
  
  for(int i=0; i< 10; i++){
    indexSet.add(i, Dune::LocalIndex (i));
    indexSet1.add(i, Dune::LocalIndex (i));
  }
  
  indexSet.endResize();
  indexSet1.endResize();
  
  typedef Dune::ParallelIndexSet<int,Dune::LocalIndex,15>::iterator
    Iterator;
  
  Iterator entry = indexSet.begin();
  indexSet.beginResize();
  
  for(int i=0; i < 5; i++)
    ++entry;
  
  indexSet.markAsDeleted(entry);
  
  indexSet.endResize();
  
  std::cout<<"Unchanged: "<<indexSet1<<std::endl;
  std::cout<<"Deleted:   "<<indexSet<<std::endl;

  Iterator end = indexSet.end();
  int i=0, ret=0;
  
  for(entry = indexSet.begin(); entry != end; ++entry,++i)
    if(entry->global()==5){
      std::cerr<<"Entry was not deleted!"<<std::endl;
      ret++;
    }
  
  if(i>9){
    std::cerr<<"Number of entries not correct!"<<std::endl;
    ret++;
  }
  
  Dune::ParallelIndexSet<int,Dune::LocalIndex,25>::iterator iter=indexSet1.begin();
  
  // Test whether the local indices changed
  for(entry = indexSet.begin(); entry != end; ++entry){
    while(iter->global() < entry->global())
      iter++;
    if(iter->global() != entry->global()){
      std::cerr <<" Global indices do not match!"<<std::endl;
      ++ret;
    }
    if(iter->local() != entry->local()){
      std::cerr <<" Local indices do not match!"<<std::endl;
      ++ret;
    }
  }
  
  return  ret;
}

int main(int argc, char **argv)
{
  exit(testDeleteIndices());
}
