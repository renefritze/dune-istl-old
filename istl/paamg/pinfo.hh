// $Id$
#ifndef DUNE_AMG_PINFO_HH
#define DUNE_AMG_PINFO_HH

#ifdef HAVE_MPI

#include<dune/istl/mpitraits.hh>
#include<dune/istl/remoteindices.hh>
#include<dune/istl/interface.hh>
#include<dune/istl/communicator.hh>

#endif

#include<dune/istl/solvercategory.hh>
namespace Dune
{
  namespace Amg
  {

#ifdef HAVE_MPI

    template<class T>
    class ParallelInformation
    {
    public:
      typedef T IndexSet;
      typedef RemoteIndices<IndexSet> RemoteIndices;
      typedef Interface<IndexSet> Interface;
      typedef BufferedCommunicator<IndexSet>Communicator;
      typedef MPI_Comm MPICommunicator;
      
      enum{
	category = SolverCategory::overlapping
	  };
      
      ParallelInformation(const MPI_Comm& comm);
      
      ~ParallelInformation();
      
      MPICommunicator communicator() const;
      
      void processes(int* procs) const;
      
      template<typename T1>
      T1 globalSum(const T1& t) const;

      template<bool ignorePublic>
      void rebuildRemoteIndices();
      
      template<typename OverlapFlags>
      void buildInterface();
      
      template<typename Data>
      void buildCommunicator(const Data& source, const Data& dest);
      
      void freeCommunicator();
      
      template<class GatherScatter, class Data>
      void communicateForward(const Data& source, Data& dest);
      
      template<class GatherScatter, class Data>
      void communicateBackward(Data& source, const Data& dest);

      IndexSet& indexSet();
      
      const IndexSet& indexSet() const;

      RemoteIndices& remoteIndices();
      
      const RemoteIndices& remoteIndices() const;

      Interface& interface();
      
      const Interface& interface() const;
    private:
      IndexSet* indexSet_;
      RemoteIndices* remoteIndices_;
      Interface* interface_;
      Communicator* communicator_;
    };

#endif    

    class SequentialInformation
    {
    public:
      typedef void* MPICommunicator;
      
      enum{
	category = SolverCategory::sequential
	  };

      MPICommunicator communicator() const
      {
	return 0;
      }

      void processes(int * procs) const
      {
	*procs=1;
      }

      template<typename T>
      T globalSum(const T& t) const
      {
	return t;
      }
      
      SequentialInformation(void*)
      {}

      SequentialInformation()
      {}

      SequentialInformation(const SequentialInformation&)
      {}
    };

#ifdef HAVE_MPI    
    template<class T>
    ParallelInformation<T>::ParallelInformation(const MPI_Comm& comm)
      : indexSet_(new IndexSet()), 
	remoteIndices_(new RemoteIndices(*indexSet_, *indexSet_, comm)),
	interface_(new Interface()), communicator_(new Communicator())
    {}
    
    template<class T>
    ParallelInformation<T>::~ParallelInformation()
    {
      delete communicator_;
      delete interface_;
      delete remoteIndices_;
      delete indexSet_;
    }

    template<class T>
    inline typename ParallelInformation<T>::MPICommunicator 
    ParallelInformation<T>::communicator() const
    {
      return remoteIndices_->communicator();
    }
    
    template<class T>
    inline void ParallelInformation<T>::processes(int* procs) const
    {
      MPI_Comm_size(communicator(), procs);
    }
    
    template<class T>
    template<typename T1>
    inline T1 ParallelInformation<T>::globalSum(const T1& t) const
    {
      T1 res;
      
      MPI_Allreduce(const_cast<T1*>(&t), &res, 1, MPITraits<T1>::getType(), MPI_SUM, communicator());
      
      return res;
    }
    
    template<class T>
    template<bool ignorePublic>
    inline void ParallelInformation<T>::rebuildRemoteIndices()
    {
      remoteIndices_->template rebuild<ignorePublic>();
    }
    
    template<class T>
    template<typename OverlapFlags>
    inline void ParallelInformation<T>::buildInterface()
    {
      interface_->build(*remoteIndices_, NegateSet<OverlapFlags>(), 
			OverlapFlags());
    }
    
    
    template<class T>
    template<typename Data>
    inline void ParallelInformation<T>::buildCommunicator(const Data& source,
						   const Data& dest)
    {
      communicator_->build(source, dest, *interface_);
    }
    
    
    template<class T>
    inline void ParallelInformation<T>::freeCommunicator()
    {
      communicator_->free();
    }
    
    template<class T>
    template<class GatherScatter, class Data>
    inline void ParallelInformation<T>::communicateForward(const Data& source, Data& dest)
    {
      communicator_->template forward<GatherScatter>(source, dest);
    }
    
    template<class T>
    template<class GatherScatter, class Data>
    inline void ParallelInformation<T>::communicateBackward(Data& source, const Data& dest)
    {
      communicator_->template backward<GatherScatter>(source, dest);
    }
    
    template<class T>
    typename ParallelInformation<T>::IndexSet& ParallelInformation<T>::indexSet(){
      return *indexSet_;
    }
      
    template<class T>
    const typename ParallelInformation<T>::IndexSet& ParallelInformation<T>::indexSet() const{
      return *indexSet_;
    }

    template<class T>
    typename ParallelInformation<T>::RemoteIndices& ParallelInformation<T>::remoteIndices(){
      return *remoteIndices_;
    }
      
    template<class T>
    const typename ParallelInformation<T>::RemoteIndices& ParallelInformation<T>::remoteIndices() const{
      return *remoteIndices_;
    }

    template<class T>
    typename ParallelInformation<T>::Interface& ParallelInformation<T>::interface(){
      return *interface_;
    }
      
    template<class T>
    const typename ParallelInformation<T>::Interface& ParallelInformation<T>::interface() const{
      return interface_;
    }

#endif

  }// namespace Amg
} //namespace Dune
#endif
