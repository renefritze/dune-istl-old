#include<dune/istl/indexset.hh>
#include<dune/istl/communicator.hh>
#include<dune/istl/remoteindices.hh>
#include<dune/common/enumset.hh>
#include<algorithm>
#include<iostream>
#include"mpi.h"

enum GridFlags{ 
  owner, overlap, border 
};

class Array;

std::ostream& operator<<(std::ostream& os, const Array& a);

class Array
{
  friend std::ostream& operator<<(std::ostream& os, const Array& a);
public:
  typedef double IndexedType;
  Array() : size_(-1)
  {}
  
  Array(int size) :size_(size)
  {
    vals_ = new double[size];
  }

  void build(int size)
  {
    vals_ = new double[size];
    size_ = size;
  }
  
  Array& operator+=(double d)
  {
    for(int i=0; i < size_; i++)
      vals_[i]+=d;
    return *this;
  }
  
  ~Array()
  {
    delete[] vals_;
  }
  
  const double& operator[](int i) const
  {
    return vals_[i];
  }

  double& operator[](int i)
  {
    return vals_[i];
  }
private:
  double *vals_;
  int size_;
};

struct ArrayGatherScatter
{
  static double gather(const Array& a, int i);
  
  static void scatter(Array& a, double v, int i);
  
};


inline double ArrayGatherScatter::gather(const Array& a, int i)
{
  return a[i];
}

inline void ArrayGatherScatter::scatter(Array& a, double v, int i)
{
  a[i]=v;
  
}
    
std::ostream& operator<<(std::ostream& os, const Array& a)
{
  if(a.size_>0)
    os<< "{ "<<a.vals_[0];
  
  for(int i=1; i<a.size_; i++)
    os <<", "<< a.vals_[i];
  
  os << " }";
  return os;
}

void testIndices()
{
  //using namespace Dune;
  
  // The global grid size
  const int Nx = 20;
  const int Ny = 2;
  
  // Process configuration
  int procs, rank, master=0;
  MPI_Comm_size(MPI_COMM_WORLD, &procs);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  
  // shift the ranks
  //rank = (rank + 1) % procs;
  //master= (master+1) %procs;

  // The local grid
  int nx = Nx/procs;
  // distributed indexset
  //  typedef ParallelLocalIndex<GridFlags> LocalIndexType;
  
  Dune::IndexSet<int,Dune::ParallelLocalIndex<GridFlags> > distIndexSet;
  // global indexset
  Dune::IndexSet<int,Dune::ParallelLocalIndex<GridFlags> > globalIndexSet;
  
  // Set up the indexsets.
  int start = std::max(rank*nx-1,0);
  int end = std::min((rank + 1) * nx+1, Nx);
  
  distIndexSet.beginResize();
  
  int localIndex=0;
  int size = Ny*(end-start);
  Array distArray(size);
  Array* globalArray;
  int index=0;
  
  for(int j=0; j<Ny; j++)
    for(int i=start; i<end; i++){
      bool isPublic = (i<=start+1)||(i>=end-2);
      GridFlags flag = owner;
      if((i==start && i!=0)||(i==end-1 && i!=Nx-1)){
	distArray[index++]=-(i+j*Nx+rank*Nx*Ny);
	flag = overlap;
      }else
	distArray[index++]=i+j*Nx+rank*Nx*Ny;

      distIndexSet.add(i+j*Nx, Dune::ParallelLocalIndex<GridFlags> (localIndex++,flag,isPublic));
    }
  
  distIndexSet.endResize();

  if(rank==master){  
    // build global indexset on first process
    globalIndexSet.beginResize();
    globalArray=new Array(Nx*Ny);
    int k=0;
    for(int j=0; j<Ny; j++)
      for(int i=0; i<Nx; i++){
	globalIndexSet.add(i+j*Nx, Dune::ParallelLocalIndex<GridFlags> (i+j*Nx,owner,false));
	globalArray->operator[](i+j*Nx)=-(i+j*Nx);
	k++;
	
      }
    
    globalIndexSet.endResize();
  }else
    globalArray=new Array(0);
  
    Dune::RemoteIndices<int,GridFlags> accuIndices(distIndexSet, globalIndexSet, MPI_COMM_WORLD);
    Dune::RemoteIndices<int,GridFlags> overlapIndices(distIndexSet, distIndexSet, MPI_COMM_WORLD);
    accuIndices.rebuild<true>();
    overlapIndices.rebuild<false>();
    
    Dune::DatatypeCommunicator<int,GridFlags> accumulator, overlapExchanger;
    
    Dune::EnumItem<GridFlags,owner> sourceFlags;
    Dune::Combine<Dune::EnumItem<GridFlags,overlap>,Dune::EnumItem<GridFlags,owner>,GridFlags> destFlags;

    accumulator.build(accuIndices, sourceFlags, distArray, destFlags, *globalArray);
    
    overlapExchanger.build(overlapIndices, Dune::EnumItem<GridFlags,owner>(), distArray, Dune::EnumItem<GridFlags,overlap>(), distArray);
    
    std::cout<< rank<<": before forward distArray="<< distArray<<std::endl;
    
    // Exchange the overlap
    overlapExchanger.forward();
    
    std::cout<<rank<<": overlap exchanged distArray"<< distArray<<std::endl;
    
    if(rank==master)
      std::cout<<": before forward globalArray="<< *globalArray<<std::endl;

    accumulator.forward();

    
    if(rank==master){
      std::cout<<"after forward global: "<<*globalArray<<std::endl;
      *globalArray+=1;
      std::cout<<" added one: globalArray="<<*globalArray<<std::endl;
    }
    
    accumulator.backward();
    std::cout<< rank<<": after backward distArray"<< distArray<<std::endl;

    
    // Exchange the overlap
    overlapExchanger.forward();
    
    std::cout<<rank<<": overlap exchanged distArray"<< distArray<<std::endl;
    
    //std::cout << rank<<": source and dest are the same:"<<std::endl;
    //std::cout << remote<<std::endl<<std::flush;
    if(rank==master)
      delete globalArray;
}



void testIndicesBuffered()
{
  //using namespace Dune;
  
  // The global grid size
  const int Nx = 8;
  const int Ny = 1;
  
  // Process configuration
  int procs, rank, master=0;
  MPI_Comm_size(MPI_COMM_WORLD, &procs);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  
  // shift the ranks
  //rank = (rank + 1) % procs;
  //master= (master+1) %procs;

  // The local grid
  int nx = Nx/procs;
  // distributed indexset
  //  typedef ParallelLocalIndex<GridFlags> LocalIndexType;
  
  Dune::IndexSet<int,Dune::ParallelLocalIndex<GridFlags> > distIndexSet;
  // global indexset
  Dune::IndexSet<int,Dune::ParallelLocalIndex<GridFlags> > globalIndexSet;
  
  // Set up the indexsets.
  int start = std::max(rank*nx-1,0);
  int end = std::min((rank + 1) * nx+1, Nx);
  
  distIndexSet.beginResize();
  
  int localIndex=0;
  int size = Ny*(end-start);
  Array distArray(size);
  Array *globalArray;
  int index=0;
  
  for(int j=0; j<Ny; j++)
    for(int i=start; i<end; i++){
      bool isPublic = (i<=start+1)||(i>=end-2);
      GridFlags flag = owner;
      if((i==start && i!=0)||(i==end-1 && i!=Nx-1)){
	distArray[index++]=-(i+j*Nx+rank*Nx*Ny);
	flag = overlap;
      }else
	distArray[index++]=i+j*Nx+rank*Nx*Ny;

      distIndexSet.add(i+j*Nx, Dune::ParallelLocalIndex<GridFlags> (localIndex++,flag,isPublic));
    }
  
  distIndexSet.endResize();

  if(rank==master){  
    // build global indexset on first process
    globalIndexSet.beginResize();
    globalArray=new Array(Nx*Ny);
    int k=0;
    for(int j=0; j<Ny; j++)
      for(int i=0; i<Nx; i++){
	globalIndexSet.add(i+j*Nx, Dune::ParallelLocalIndex<GridFlags> (i+j*Nx,owner,false));
	globalArray->operator[](i+j*Nx)=-(i+j*Nx);
	k++;
	
      }
    
    globalIndexSet.endResize();
  }else
    globalArray=new Array(0);
  
    Dune::RemoteIndices<int,GridFlags> accuIndices(distIndexSet, globalIndexSet, MPI_COMM_WORLD);
        
    accuIndices.rebuild<true>();
    //    std::cout << accuIndices<<std::endl<<std::flush;

    Dune::RemoteIndices<int,GridFlags> overlapIndices(distIndexSet, distIndexSet, MPI_COMM_WORLD);
    overlapIndices.rebuild<false>();
    
    Dune::Interface<int,GridFlags> accuInterface;
    Dune::Interface<int,GridFlags>  overlapInterface;
    Dune::EnumItem<GridFlags,owner> sourceFlags;
    Dune::Combine<Dune::EnumItem<GridFlags,overlap>,Dune::EnumItem<GridFlags,owner>,GridFlags> destFlags;
    //    Dune::Bool2Type<true> flag;

    accuInterface.build(accuIndices, sourceFlags, destFlags);
    overlapInterface.build(overlapIndices, Dune::EnumItem<GridFlags,owner>(), 
			   Dune::EnumItem<GridFlags,overlap>());
    overlapInterface.print();
    accuInterface.print();
    
    //accuInterface.print();

    Dune::BufferedCommunicator<int,GridFlags> accumulator, overlapExchanger;
    
    accumulator.build<Array>(accuInterface);
    
    overlapExchanger.build<Array>(overlapInterface);
     
    std::cout<< rank<<": before forward distArray="<< distArray<<std::endl;
    
    // Exchange the overlap
    overlapExchanger.forward<ArrayGatherScatter>(distArray, distArray);
    
    std::cout<<rank<<": overlap exchanged distArray"<< distArray<<std::endl;
    
    if(rank==master)
      std::cout<<": before forward globalArray="<< *globalArray<<std::endl;

    accumulator.forward<ArrayGatherScatter>(distArray, *globalArray);

    
    if(rank==master){
      std::cout<<"after forward global: "<<*globalArray<<std::endl;
      *globalArray+=1;
      std::cout<<" added one: globalArray="<<*globalArray<<std::endl;
    }
    
    accumulator.backward<ArrayGatherScatter>(distArray, *globalArray);
    std::cout<< rank<<": after backward distArray"<< distArray<<std::endl;

    
    // Exchange the overlap
    overlapExchanger.forward<ArrayGatherScatter>(distArray);
    
    std::cout<<rank<<": overlap exchanged distArray"<< distArray<<std::endl;
    
    //std::cout << rank<<": source and dest are the same:"<<std::endl;
    //std::cout << remote<<std::endl<<std::flush;
    if(rank==master)
    delete globalArray;
}


 void testRedistributeIndices()
{
  using namespace Dune;
  
  // The global grid size
  const int Nx = 20;
  const int Ny = 2;
  
  // Process configuration
  int procs, rank;
  MPI_Comm_size(MPI_COMM_WORLD, &procs);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  
  // The local grid
  int nx = Nx/procs;
  // distributed indexset
  
  IndexSet<int,ParallelLocalIndex<GridFlags> > sendIndexSet;
  // global indexset
  IndexSet<int,ParallelLocalIndex<GridFlags> > receiveIndexSet;

  Array array, redistributedArray;
  
  // Set up the indexsets.
  {
    
    int start = std::max(rank*nx-1,0);
    int end = std::min((rank + 1) * nx+1, Nx);
    
    sendIndexSet.beginResize();
  
    
    array.build(Ny*(end-start));
    
    for(int j=0, localIndex=0; j<Ny; j++)
      for(int i=start; i<end; i++, localIndex++){
	bool isPublic = (i<=start+1)||(i>=end-2);
	GridFlags flag = owner;
	
	if((i==start && i!=0)||(i==end-1 && i!=Nx-1))
	  flag = overlap;
	
	sendIndexSet.add(i+j*Nx, ParallelLocalIndex<GridFlags> (localIndex,flag,isPublic));
	array[localIndex]=i+j*Nx+rank*Nx*Ny;
      }
  
    sendIndexSet.endResize();
  }
  {
    int newrank = (rank + 1) % procs;
    
    int start = std::max(newrank*nx-1,0);
    int end = std::min((newrank + 1) * nx+1, Nx);
    
    std::cout<<rank<<": "<<newrank<<" start="<<start<<" end"<<end<<std::endl;
    
    redistributedArray.build(Ny*(end-start));
    
    receiveIndexSet.beginResize();
    
    for(int j=0, localIndex=0; j<Ny; j++)
      for(int i=start; i<end; i++, localIndex++){
	bool isPublic = (i<=start+1)||(i>=end-2);
	GridFlags flag = owner;
	
	if((i==start && i!=0)||(i==end-1 && i!=Nx-1))
	  flag = overlap;
	
	receiveIndexSet.add(i+j*Nx, ParallelLocalIndex<GridFlags> (localIndex,flag,isPublic));
	redistributedArray[localIndex]=-1;
      }
  
    receiveIndexSet.endResize();
  }
  
  
    std::cout<< rank<<": distributed and global index set!"<<std::endl<<std::flush;
    RemoteIndices<int,GridFlags> redistributeIndices(sendIndexSet, 
						     receiveIndexSet, MPI_COMM_WORLD);
    RemoteIndices<int,GridFlags> overlapIndices(receiveIndexSet, receiveIndexSet, MPI_COMM_WORLD);
      
    redistributeIndices.rebuild<true>();
    overlapIndices.rebuild<false>();
    
    DatatypeCommunicator<int,GridFlags> redistribute, overlapComm;
    EnumItem<GridFlags,owner> fowner;
    EnumItem<GridFlags,overlap> foverlap;
    
    redistribute.build(redistributeIndices, fowner, array, fowner, redistributedArray);

    overlapComm.build(overlapIndices, fowner, redistributedArray, foverlap, redistributedArray);
    std::cout<<rank<<": initial array: "<<array<<std::endl;
    
    redistribute.forward();

    std::cout<<rank<<": redistributed array: "<<redistributedArray<<std::endl;

    overlapComm.forward();

    std::cout<<rank<<": redistributed array with overlap communicated: "<<redistributedArray<<std::endl;
} 

 void testRedistributeIndicesBuffered()
{
  using namespace Dune;
  
  // The global grid size
  const int Nx = 20;
  const int Ny = 2;
  
  // Process configuration
  int procs, rank;
  MPI_Comm_size(MPI_COMM_WORLD, &procs);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  
  // The local grid
  int nx = Nx/procs;
  // distributed indexset
  
  IndexSet<int,ParallelLocalIndex<GridFlags> > sendIndexSet;
  // global indexset
  IndexSet<int,ParallelLocalIndex<GridFlags> > receiveIndexSet;

  Array array, redistributedArray;
  
  // Set up the indexsets.
  {
    
    int start = std::max(rank*nx-1,0);
    int end = std::min((rank + 1) * nx+1, Nx);
    
    sendIndexSet.beginResize();
  
    
    array.build(Ny*(end-start));
    
    for(int j=0, localIndex=0; j<Ny; j++)
      for(int i=start; i<end; i++, localIndex++){
	bool isPublic = (i<=start+1)||(i>=end-2);
	GridFlags flag = owner;
	
	if((i==start && i!=0)||(i==end-1 && i!=Nx-1))
	  flag = overlap;
	
	sendIndexSet.add(i+j*Nx, ParallelLocalIndex<GridFlags> (localIndex,flag,isPublic));
	array[localIndex]=i+j*Nx;//+rank*Nx*Ny;
	if(flag==overlap)
	  array[localIndex]=-array[localIndex];
      }
  
    sendIndexSet.endResize();
  }
  {
    int newrank = (rank + 1) % procs;
    
    int start = std::max(newrank*nx-1,0);
    int end = std::min((newrank + 1) * nx+1, Nx);
    
    std::cout<<rank<<": "<<newrank<<" start="<<start<<" end"<<end<<std::endl;
    
    redistributedArray.build(Ny*(end-start));
    
    receiveIndexSet.beginResize();
    
    for(int j=0, localIndex=0; j<Ny; j++)
      for(int i=start; i<end; i++, localIndex++){
	bool isPublic = (i<=start+1)||(i>=end-2);
	GridFlags flag = owner;
	
	if((i==start && i!=0)||(i==end-1 && i!=Nx-1))
	  flag = overlap;
	
	receiveIndexSet.add(i+j*Nx, ParallelLocalIndex<GridFlags> (localIndex,flag,isPublic));
	redistributedArray[localIndex]=-1;
      }
  
    receiveIndexSet.endResize();
  }
  
  
    std::cout<< rank<<": distributed and global index set!"<<std::endl<<std::flush;
    RemoteIndices<int,GridFlags> redistributeIndices(sendIndexSet, 
						   receiveIndexSet, MPI_COMM_WORLD);
    RemoteIndices<int,GridFlags> overlapIndices(receiveIndexSet, receiveIndexSet, MPI_COMM_WORLD);

    overlapIndices.rebuild<false>();
    redistributeIndices.rebuild<true>();
    
    Interface<int,GridFlags> redistributeInterface, overlapInterface;
    EnumItem<GridFlags,owner> fowner;
    EnumItem<GridFlags,overlap> foverlap;

    redistributeInterface.build(redistributeIndices, fowner, fowner);
    overlapInterface.build(overlapIndices, fowner, foverlap);
    
    BufferedCommunicator<int,GridFlags> redistribute;
    BufferedCommunicator<int, GridFlags> overlapComm;
    
    redistribute.build(array, redistributedArray, redistributeInterface);
    overlapComm.build<Array>(overlapInterface);
        
    std::cout<<rank<<": initial array: "<<array<<std::endl;
    
    redistribute.forward<ArrayGatherScatter>(array, redistributedArray);

    std::cout<<rank<<": redistributed array: "<<redistributedArray<<std::endl;

    redistributedArray +=1;

    std::cout<<rank<<": redistributed array (added one): "<<redistributedArray<<std::endl;

    overlapComm.forward<ArrayGatherScatter>(redistributedArray);

    std::cout<<rank<<": redistributed array with overlap communicated: "<<redistributedArray<<std::endl;

    redistribute.backward<ArrayGatherScatter>(array, redistributedArray);

    std::cout<<rank<<": final array: "<<array<<std::endl;
} 

int main(int argc, char **argv)
{
  MPI_Init(&argc, &argv);
  int rank;
  int size;
  MPI_Comm_size(MPI_COMM_WORLD, &size);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  
#ifdef DEBUG
  bool wait=1;
  while(size>1 && rank==0 && wait);
#endif
  testIndices();
  testIndicesBuffered();
  /*  
  if(rank==0)
    std::cout<<std::endl<<"Redistributing!"<<std::endl<<std::endl;
  */
  testRedistributeIndices();
  testRedistributeIndicesBuffered();
 
  MPI_Finalize();
}

  
