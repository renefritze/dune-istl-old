#include"config.h"
#include"anisotropic.hh"
#include<dune/common/timer.hh>
#include<dune/istl/paamg/amg.hh>
#include<dune/istl/paamg/pinfo.hh>
#include<dune/istl/indexset.hh>
#include<dune/common/collectivecommunication.hh>
#include<cstdlib>
#include<ctime>

template<class M, class V>
void randomize(const M& mat, V& b)
{
  V x=b;
  
  srand((unsigned)std::clock());
 
  typedef typename V::iterator iterator;
  for(iterator i=x.begin(); i != x.end(); ++i)
    *i=(rand() / (RAND_MAX + 1.0));

  mat.mv(static_cast<const V&>(x), b);
}

int main(int argc, char** argv)
{
    
  const int BS=1;

  int N=500/BS;
  int coarsenTarget=1200;
  int ml=10;
  
  if(argc>1)
    N = atoi(argv[1]);
  
  if(argc>2)
    coarsenTarget = atoi(argv[2]);

  if(argc>3)
    ml = atoi(argv[3]);
  
  std::cout<<"N="<<N<<" coarsenTarget="<<coarsenTarget<<" maxlevel="<<ml<<std::endl;
  

  typedef Dune::ParallelIndexSet<int,LocalIndex,512> ParallelIndexSet;
  
  ParallelIndexSet indices;
  typedef Dune::FieldMatrix<double,BS,BS> MatrixBlock;
  typedef Dune::BCRSMatrix<MatrixBlock> BCRSMat;
  typedef Dune::FieldVector<double,BS> VectorBlock;
  typedef Dune::BlockVector<VectorBlock> Vector;
  typedef Dune::MatrixAdapter<BCRSMat,Vector,Vector> Operator;
  typedef Dune::CollectiveCommunication<void*> Comm;
  int n;
  
  Comm c;
  BCRSMat mat = setupAnisotropic2d<BS>(N, indices, c, &n, 1);

  Vector b(mat.N()), x(mat.M());
  
  b=0;
  x=100;

  setBoundary(x, b, N);

  x=0;
  randomize(mat, b);
  
  if(N<6){
    Dune::printmatrix(std::cout, mat, "A", "row");
    Dune::printvector(std::cout, x, "x", "row");
  }

  Dune::Timer watch;
  
  watch.reset();
  Operator fop(mat);

  typedef Dune::Amg::CoarsenCriterion<Dune::Amg::UnSymmetricCriterion<BCRSMat,Dune::Amg::FirstDiagonal> >
    Criterion;
  //typedef Dune::SeqSSOR<BCRSMat,Vector,Vector> Smoother;
  typedef Dune::SeqSOR<BCRSMat,Vector,Vector> Smoother;
  //typedef Dune::SeqJac<BCRSMat,Vector,Vector> Smoother;
  //typedef Dune::SeqOverlappingSchwarz<BCRSMat,Vector,Dune::MultiplicativeSchwarzMode> Smoother;
  //typedef Dune::SeqOverlappingSchwarz<BCRSMat,Vector,Dune::SymmetricMultiplicativeSchwarzMode> Smoother;
  //typedef Dune::SeqOverlappingSchwarz<BCRSMat,Vector> Smoother;
  typedef Dune::Amg::SmootherTraits<Smoother>::Arguments SmootherArgs;

  SmootherArgs smootherArgs;
    
  smootherArgs.iterations = 1;
  
  //smootherArgs.overlap=SmootherArgs::vertex;
  //smootherArgs.overlap=SmootherArgs::none;
  //smootherArgs.overlap=SmootherArgs::aggregate;
  
  smootherArgs.relaxationFactor = 1;
  
  Criterion criterion(15,coarsenTarget);
  criterion.setMaxDistance(4);
  criterion.setMinAggregateSize(9);
  criterion.setMaxAggregateSize(11);
  criterion.setAlpha(.67);
  criterion.setBeta(1.0e-4);
  criterion.setMaxLevel(ml);
  
  Dune::SeqScalarProduct<Vector> sp;
  typedef Dune::Amg::AMG<Operator,Vector,Smoother> AMG;
    
  AMG amg(fop, criterion, smootherArgs, 1, 1, 1, false);


  double buildtime = watch.elapsed();  
  
  std::cout<<"Building hierarchy took "<<buildtime<<" seconds"<<std::endl;

  Dune::CGSolver<Vector> amgCG(fop,amg,1e-6,80,2);
    watch.reset();
  Dune::InverseOperatorResult r;
  amgCG.apply(x,b,r);
  
  double solvetime = watch.elapsed();
  
  std::cout<<"AMG solving took "<<solvetime<<" seconds"<<std::endl;
  
    std::cout<<"AMG building took "<<(buildtime/r.elapsed*r.iterations)<<" iterations"<<std::endl;
  std::cout<<"AMG building together with slving took "<<buildtime+solvetime<<std::endl;
  
  /*
  watch.reset();
  cg.apply(x,b,r);

  std::cout<<"CG solving took "<<watch.elapsed()<<" seconds"<<std::endl;
  */											  
}
