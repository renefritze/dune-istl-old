#include <config.h>

#include"mpi.h"
#include<dune/istl/paamg/hierarchy.hh>
#include<dune/istl/paamg/smoother.hh>
#include<dune/istl/preconditioners.hh>
#include<dune/istl/owneroverlapcopy.hh>
#include<dune/istl/schwarz.hh>
#include<dune/common/mpicollectivecommunication.hh>
#include"anisotropic.hh"

int main(int argc, char** argv)
{
  MPI_Init(&argc, &argv);
  
  const int BS=1, N=100;
  
  int procs, rank;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &procs);

  typedef int LocalId;
  typedef int GlobalId;
  typedef Dune::OwnerOverlapCopyCommunication<LocalId,GlobalId> Communication;
  typedef Communication::ParallelIndexSet ParallelIndexSet;
  typedef Dune::FieldMatrix<double,BS,BS> MatrixBlock;
  typedef Dune::BCRSMatrix<MatrixBlock> BCRSMat;
  typedef Dune::FieldVector<double,BS> VectorBlock;
  typedef Dune::BlockVector<VectorBlock> Vector;
  
  int n;
  Communication pinfo(MPI_COMM_WORLD);  
  ParallelIndexSet& indices = pinfo.indexSet();
  
  typedef Dune::RemoteIndices<ParallelIndexSet> RemoteIndices;
  RemoteIndices& remoteIndices = pinfo.remoteIndices();
  
  typedef Dune::CollectiveCommunication<MPI_Comm> Comm;
  Comm cc(MPI_COMM_WORLD);
  BCRSMat mat = setupAnisotropic2d<BS>(N, indices, cc, &n);
  Vector b(indices.size());
  
  remoteIndices.rebuild<false>();

  typedef Dune::Interface<ParallelIndexSet> Interface;
  
  Interface interface;
  
  typedef Dune::EnumItem<GridFlag,GridAttributes::overlap> OverlapFlags;
  typedef Dune::OverlappingSchwarzOperator<BCRSMat,Vector,Vector,Communication> Operator;
  typedef Dune::Amg::MatrixHierarchy<Operator,Communication> Hierarchy;
  typedef Dune::Amg::Hierarchy<Vector> VHierarchy;

  interface.build(remoteIndices, Dune::NegateSet<OverlapFlags>(), OverlapFlags());
  Operator op(mat, pinfo);
  Hierarchy hierarchy(op, pinfo);
  VHierarchy vh(b);
  
  typedef Dune::Amg::CoarsenCriterion<Dune::Amg::SymmetricCriterion<BCRSMat,Dune::Amg::FirstDiagonal> >
    Criterion;

  Criterion criterion(100,4);
  
  hierarchy.build<OverlapFlags>(criterion);
  hierarchy.coarsenVector(vh);
  
  
  std::cout<<"=== Vector hierarchy has "<<vh.levels()<<" levels! ==="<<std::endl;
  
  hierarchy.recalculateGalerkin(OverlapFlags());
  
  
  MPI_Finalize();
  
}
