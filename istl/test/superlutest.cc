#include"config.h"
#include<dune/istl/bvector.hh>
#include<dune/istl/operators.hh>
#include<dune/common/fmatrix.hh>
#include<dune/common/fvector.hh>
#include<laplacian.hh>
#include<dune/common/timer.hh>
#include<dune/istl/superlu.hh>
int main(int argc, char** argv)
{
    
  const int BS=1;
  int N=100;
  
  if(argc>1)
    N = atoi(argv[1]);
  std::cout<<"testing for N="<<N<<" BS="<<1<<std::endl;
  

  typedef Dune::FieldMatrix<double,BS,BS> MatrixBlock;
  typedef Dune::BCRSMatrix<MatrixBlock> BCRSMat;
  typedef Dune::FieldVector<double,BS> VectorBlock;
  typedef Dune::BlockVector<VectorBlock> Vector;
  typedef Dune::MatrixAdapter<BCRSMat,Vector,Vector> Operator;
  
  BCRSMat mat;
  Operator fop(mat);
  Vector b(N*N), x(N*N);
  
  setupLaplacian(mat,N);
  b=1;
  x=0;
  
  Dune::Timer watch;

  watch.reset();

  Dune::SuperLU<BCRSMat> solver(mat, true);
  
  Dune::InverseOperatorResult res;
  
  Dune::SuperLU<BCRSMat> solver1;
  
  solver1.setMatrix(mat);
  
  solver.apply(x,b, res);

  std::cout<<"Defect reduction is "<<res.reduction<<std::endl;
  
}
