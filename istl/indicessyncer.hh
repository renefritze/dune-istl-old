// $Id$
#ifndef DUNE_INDICESSYNCER_HH
#define DUNE_INDICESSYNCER_HH

#include"mpi.h"
#include"indexset.hh"
#include"remoteindices.hh"
#include<dune/common/tuples.hh>
#include<dune/common/sllist.hh>
#include<cassert>
#include<cmath>

namespace Dune
{
  /** @addtogroup ISTL_Comm
   *
   * @{
   */
   /**
   * @file
   * @brief Class for adding missing indices of a distributed index set in a local 
   * communication.
   * @author Markus Blatt
   */

  /**
   * @brief Class for recomputing missing indices of a distributed index set.
   *
   * Missing local and remote indices will be added.
   */
  template<typename TG, typename TA, int N=100>
  class IndicesSyncer
  {
  public:

    /** @brief The type of the index set. */
    typedef IndexSet<TG,ParallelLocalIndex<TA>,N> IndexSet;

    /**
     * @brief Type of the remote indices. 
     */
    typedef RemoteIndices<TG,TA,N> RemoteIndices;
    
    /**
     * @brief Constructor.
     *
     * The source as well as the target index set of the remote
     * indices have to be the same as the provided index set.
     * @param indexSet The index set with the information
     * of the locally present indices.
     * @param remoteIndices The remoteIndices.
     */
    IndicesSyncer(IndexSet& indexSet, 
		  RemoteIndices& remoteIndices);
    
    /**
     * @brief Sync the index set.
     *
     * Computes the missing indices in the local and the remote index list and adds them.
     * No global communication is necessary!
     */
    void sync();
    
  private:    
    
    /** @brief The set of locally present indices.*/
    IndexSet& indexSet_;

    /** @brief The remote indices. */
    RemoteIndices& remoteIndices_;
    
    /** @brief The send and receive buffers. */
    char* buffer_;

    /** @brief The size of the buffer in bytes. */
    int bufferSize_;
    
    /**
     * @brief Information about the messages to send.
     */
    struct MessageInformation
    {
      MessageInformation()
      {
	publish=0;
	pairs=0;
      }
      /** @brief The number of indices we publish for each process. */
      int publish;
      /**
       * @brief The number of pairs (attribute and process number)
       * we publish to each neighbour process.
       */
      int pairs;
    };

    /** @brief The mpi datatype for the MessageInformation */
    MPI_Datatype datatype_;

    /** @brief Our rank. */
    int rank_;
    
    /** 
     * @brief List type for temporarily storing the global indices of the
     * remote indices.
     */
    typedef SLList<TG, typename RemoteIndices::Allocator> GlobalIndexList;

    /** 
     * @brief The type of the iterator of GlobalIndexList
     */
    typedef typename SLList<TG, typename RemoteIndices::Allocator>::iterator 
    GlobalIndexIterator;

    /** @brief Type of the map of ranks onto GlobalIndexLists. */
    typedef std::map<int, GlobalIndexList> GlobalIndicesMap;
    
    /** 
     * @brief Map of global index lists onto process ranks.
     *
     * As the pointers in the remote index lists become invalid due to
     * resorting the index set entries one has store the corresponding 
     * global index for each remote index. Thus the pointers can be adjusted
     * properly as a last step.
     */
    GlobalIndicesMap globalMap_;

    /**
     * @brief The type of the single linked list of bools.
     */
    typedef SLList<bool, typename RemoteIndices::Allocator> BoolList;

    /**
     * @brief The mutable iterator of the single linked bool list.
     */
    typedef typename BoolList::iterator BoolIterator;

    typedef std::map<int,BoolList> BoolMap;

    /**
     * @brief Map of lists of bool indicating whether the remote index was present before
     * call of sync.
     */
    BoolMap oldMap_;
    
    /** @brief Information about the messages we send. */
    std::map<int,MessageInformation> infoSend_;
    
    /** @brief The type of the remote index list. */
    typedef typename RemoteIndices::RemoteIndexList RemoteIndexList;

    /** @brief The type of the remote inde. */
    typedef RemoteIndex<TG,TA> RemoteIndex;
    
    /** @brief The iterator of the remote index list. */
    typedef typename RemoteIndexList::iterator RemoteIndexIterator;

    /** @brief The const iterator of the remote index list. */
    typedef typename RemoteIndexList::const_iterator ConstRemoteIndexIterator;

    /** @brief Type of the tuple of iterators needed for the adding of indices. */
    typedef Tuple<RemoteIndexIterator,GlobalIndexIterator,BoolIterator,RemoteIndexIterator,
      GlobalIndexIterator,BoolIterator,const ConstRemoteIndexIterator> IteratorTuple;

    /**
     * @brief A tuple of iterators.
     *
     * Insertion into a single linked list is only possible at the position after the one of the iterator.
     * Therefore for each linked list two iterators are needed: One position before the actual entry 
     * (for insertion) and one positioned at the actual position (for searching).
     */
    class Iterators
    {
      friend class IndicesSyncer<TG,TA,N>;
    public:
      /**
       * @brief Constructor.
       * 
       * Initializes all iterator to first entry and the one before the first entry, respectively.
       * @param remoteIndices The list of the remote indices.
       * @param globalIndices The list of the coresponding global indices. This is needed because the
       * the pointers to the local index will become invalid due to the merging of the index sets.
       * @param booleans Whether the remote index was there before the sync process started.
       */
      Iterators(RemoteIndexList& remoteIndices, GlobalIndexList& globalIndices,
		BoolList& booleans);
      
      /**
       * @brief Default constructor.
       */
      Iterators();
      
      /**
       * @brief Increment all iteraors.
       */
      Iterators& operator++();
      
      /**
       * @brief Insert a new remote index to the underlying remote index list.
       * @param index The remote index.
       * @param global The global index corresponding to the remote index.
       */
      void insert(const RemoteIndex& index, const TG& global);
      
      /**
       * @brief Get the remote index at current position.
       * @return The current remote index.
       */
      RemoteIndex& remoteIndex() const;
      
      /**
       * @brief Get the global index of the remote index at current position.
       * @return The current global index.
       */
      TG& globalIndex() const;
      
      /**
       * @brief Was this entry already in the remote index list before the sync process?
       * @return True if the current index wasalready in the remote index list 
       * before the sync process.
       */
      bool isOld() const;
      
      /**
       * @brief Reset all the underlying iterators.
       *
       * Position them to first list entry and the entry before the first entry respectively.
       * @param remoteIndices The list of the remote indices.
       * @param globalIndices The list of the coresponding global indices. This is needed because the
       * the pointers to the local index will become invalid due to the merging of the index sets.
       * @param booleans Whether the remote index was there before the sync process started.
       */
      void reset(RemoteIndexList& remoteIndices, GlobalIndexList& globalIndices,
		 BoolList& booleans);

      /**
       * @brief Are we not at the end of the list?
       * @return True if the iterators are not positioned at the end of the list
       * and the tail of the list respectively.
       */
      bool isNotAtEnd() const;
      
      /**
       * @brief Are we at the end of the list?
       * @return True if the iterators are positioned at the end of the list
       * and the tail of the list respectively.
       */
      bool isAtEnd() const;
      
    private:
      /**
       * @brief The iterator tuple.
       *
       * The tuple consists of one iterator over a single linked list of remote indices
       * initially positioned before the first entry, one over a sll of global indices
       * , one over a all of bool values both postioned at the same entry. The another three
       * iterators of the same type positioned at the first entry. Last an iterator over the 
       * sll of remote indices positioned at the end.
       */
      IteratorTuple iterators_;
    };
    
    /** @brief Type of the map from ranks to iterator tuples. */
    typedef std::map<int,Iterators> IteratorsMap;
    
    /**
     * @brief The iterator tuples mapped on the neighbours.
     *
     * The key of the map is the rank of the neighbour.
     * The first entry in the tuple is an iterator over the remote indices 
     * initially positioned before the first entry. The second entry is an
     * iterator over the corresponding global indices also initially positioned 
     * before the first entry. The third entry an iterator over remote indices
     * initially positioned at the beginning. The last entry is the iterator over
     * the remote indices positioned at the end.
     */
    IteratorsMap iteratorsMap_;
        
    /** @brief Calculates the message sizes to send. */
    void calculateMessageSizes();

    /**
     * @brief Pack and send the message for another process. 
     * @param destination The rank of the process we send to.
     */
    void packAndSend(int destination);
    
    /** 
     * @brief Recv and unpack the message from another process and add the indices.
     * @param source The rank of the process we receive from.
     */
    void recvAndUnpack(int source);

    /**
     * @brief Register the MPI datatype for the MessageInformation.
     */
    void registerMessageDatatype();
    
    /**
     * @brief Insert an entry into the  remote index list if not yet present.
     */
    void insertIntoRemoteIndexList(int process, const TG& global, char attribute);

    /**
     * @brief Reset the iterator tuples of all neighbouring processes.
     */
    void resetIteratorsMap();
    
    /**
     * @brief Check whether the iterator tuples of all neighbouring processes
     * are reset.
     */
    bool checkReset();
    
    /**
     * @brief Check whether the iterator tuple is reset.
     *
     * @param iterators The iterator tuple to check.
     * @param rlist The SLList of the remote indices.
     * @param gList The SLList of the global indices.
     * @param bList The SLList of the bool values.
     */
    bool checkReset(const Iterators& iterators, RemoteIndexList& rlist, GlobalIndexList& gList,
		    BoolList& bList);
  };
  
  /**
   * @brief Repair the pointers to the local indices in the remote indices.
   *
   * @param globalMap The map of the process number to the list of global indices
   * corresponding to the remote index list of the process.
   * @param remoteIndices The known remote indices.
   * @param indexSet The set of local indices of the current process.
   */
  template<typename TG, typename TA, typename A, int N>
  inline void repairLocalIndexPointers(std::map<int,SLList<TG,A> >& globalMap,
				       RemoteIndices<TG,TA,N>& remoteIndices,
				       const IndexSet<TG,ParallelLocalIndex<TA>,N>& indexSet)
  {
    typedef typename RemoteIndices<TG,TA,N>::RemoteIndexMap::iterator RemoteIterator;
    typedef typename RemoteIndices<TG,TA,N>::RemoteIndexList::iterator RemoteIndexIterator;
    typedef typename SLList<TG,A>::iterator GlobalIndexIterator;
    
    // Repair pointers to index set in remote indices.
    typename std::map<int,SLList<TG,A> >::iterator global = globalMap.begin();
    RemoteIterator end = remoteIndices.remoteIndices_.end();
    
    for(RemoteIterator remote = remoteIndices.remoteIndices_.begin(); remote != end; ++remote, ++global){
      typedef typename IndexSet<TG,ParallelLocalIndex<TA>,N>::const_iterator IndexIterator;

      assert(remote->first==global->first);
      assert(remote->second.first->size() == global->second.size());
      
      RemoteIndexIterator riEnd  = remote->second.first->end();
      RemoteIndexIterator rIndex = remote->second.first->begin();
      GlobalIndexIterator gIndex = global->second.begin();
      IndexIterator       index  = indexSet.begin();
      
      while(rIndex != riEnd){
	// Search for the index in the set.
	assert(gIndex != global->second.end());
	
	while(index->global() < *gIndex)
	  ++index;
	
	assert(index != indexSet.end() && index->global() == *gIndex);
	
	rIndex->localIndex_ = &(*index);
	
	++rIndex;
	++gIndex;
      }
    }
    
  }
  
  template<typename  TG, typename TA, int N>
  IndicesSyncer<TG,TA,N>::IndicesSyncer(IndexSet& indexSet, 
				      RemoteIndices& remoteIndices)
    : indexSet_(indexSet), remoteIndices_(remoteIndices)
  {
    // index sets must match.
    assert(&(remoteIndices.source_) == &(remoteIndices.target_));
    assert(&(remoteIndices.source_) == &indexSet);
    MPI_Comm_rank(remoteIndices_.communicator(), &rank_);
  }
    
  template<typename  TG, typename TA, int N>
  IndicesSyncer<TG,TA,N>::Iterators::Iterators(RemoteIndexList& remoteIndices, 
					       GlobalIndexList& globalIndices,
					       BoolList& booleans)
    : iterators_(remoteIndices.oneBeforeBegin(), globalIndices.oneBeforeBegin(),
		    booleans.oneBeforeBegin(), remoteIndices.begin(), 
		    globalIndices.begin(), booleans.begin(), remoteIndices.end())
  {
    int k=3;
    ++k;
    
  }

  template<typename  TG, typename TA, int N>
  IndicesSyncer<TG,TA,N>::Iterators::Iterators()
    : iterators_()
  {
    int k=3;
    ++k;
  }
  
  template<typename  TG, typename TA, int N>
  inline typename IndicesSyncer<TG,TA,N>::Iterators& IndicesSyncer<TG,TA,N>::Iterators::operator++()
  {
      ++(Element<0>::get(iterators_));
      ++(Element<1>::get(iterators_));
      ++(Element<2>::get(iterators_));
      ++(Element<3>::get(iterators_));
      ++(Element<4>::get(iterators_));
      ++(Element<5>::get(iterators_));
      return *this;
  }

  template<typename  TG, typename TA, int N>
  inline void IndicesSyncer<TG,TA,N>::Iterators::insert(const RemoteIndex& index, 
						 const TG& global)
  {
    Element<0>::get(iterators_).insertAfter(index);
    Element<1>::get(iterators_).insertAfter(global);
    Element<2>::get(iterators_).insertAfter(false);
    // Move to the position we just inserted.
    ++(Element<0>::get(iterators_));
    ++(Element<1>::get(iterators_));
    ++(Element<2>::get(iterators_));
  }
  
  template<typename  TG, typename TA, int N>
  inline typename IndicesSyncer<TG,TA,N>::RemoteIndex& 
  IndicesSyncer<TG,TA,N>::Iterators::remoteIndex() const
  {
    return *(Element<3>::get(iterators_));
  }
  
  template<typename  TG, typename TA, int N>
  inline TG&  IndicesSyncer<TG,TA,N>::Iterators::globalIndex() const
  {
    return *(Element<4>::get(iterators_));
  }
  
  template<typename  TG, typename TA, int N>
  inline bool IndicesSyncer<TG,TA,N>::Iterators::isOld() const
  {
    return *(Element<5>::get(iterators_));
  }
  
  template<typename  TG, typename TA, int N>
  inline void IndicesSyncer<TG,TA,N>::Iterators::reset(RemoteIndexList& remoteIndices, 
						GlobalIndexList& globalIndices,
						BoolList& booleans)
  {
    Element<0>::get(iterators_) = remoteIndices.oneBeforeBegin();
    Element<1>::get(iterators_) = globalIndices.oneBeforeBegin();
    Element<2>::get(iterators_) = booleans.oneBeforeBegin();
    Element<3>::get(iterators_) = remoteIndices.begin();
    Element<4>::get(iterators_) = globalIndices.begin();
    Element<5>::get(iterators_) = booleans.begin();
  }
  
  template<typename  TG, typename TA, int N>
  inline bool IndicesSyncer<TG,TA,N>::Iterators::isNotAtEnd() const
  {
    return Element<3>::get(iterators_)!=Element<6>::get(iterators_);
  }
  
  template<typename  TG, typename TA, int N>
  inline bool IndicesSyncer<TG,TA,N>::Iterators::isAtEnd() const
  {
    return Element<3>::get(iterators_)==Element<6>::get(iterators_);
  }

  template<typename TG, typename TA, int N>
  void IndicesSyncer<TG,TA,N>::registerMessageDatatype()
  {
    MPI_Datatype type[2] = {MPI_INT, MPI_INT};
    int blocklength[2] = {1,1};
    MPI_Aint displacement[2];
    MPI_Aint base;
    
    // Compute displacement
    MessageInformation message;
    
    MPI_Address( &(message.publish), displacement);
    MPI_Address( &(message.pairs), displacement+1);
    
    // Make the displacement relative
    MPI_Address(&message, &base);
    displacement[0] -= base;
    displacement[1] -= base;
    
    MPI_Type_struct( 2, blocklength, displacement, type, &datatype_); 
    MPI_Type_commit(&datatype_);
  }
  
  template<typename TG, typename TA, int N>
  void IndicesSyncer<TG,TA,N>::calculateMessageSizes()
  {    
    typedef typename IndexSet::const_iterator IndexIterator;
    typedef CollectiveIterator<TG,TA,N> CollectiveIterator;
    
    IndexIterator iEnd = indexSet_.end();
    CollectiveIterator collIter = remoteIndices_.template iterator<true>();
    
    for(IndexIterator index = indexSet_.begin(); index != iEnd;	++index){
      collIter.advance(index->global());
      if(collIter.empty())
	break;
      int knownRemote=0;

      typedef typename CollectiveIterator::iterator ValidIterator;
      ValidIterator end = collIter.end();

      // Count the remote indices we know.
      for(ValidIterator valid = collIter.begin(); valid != end; ++valid)
	++knownRemote;
      
      if(knownRemote>0){
	std::cout<<rank_<<": publishing "<<knownRemote<<" for index "<<index->global()<< " for processes ";
	
	// Update MessageInformation
	for(ValidIterator valid = collIter.begin(); valid != end; ++valid){
	  ++(infoSend_[valid.process()].publish);
	  (infoSend_[valid.process()].pairs) += knownRemote;
	  std::cout<<valid.process()<<" ";
	}
	std::cout<<std::endl;
      }
    }
    
    typedef typename std::map<int,MessageInformation>::const_iterator 
      MessageIterator;
    
    const MessageIterator end = infoSend_.end();

    registerMessageDatatype();
    
    MessageInformation maxSize;
    for(MessageIterator message = infoSend_.begin(); message != end; ++message){
      MessageInformation recv;
      MPI_Status status;
      
      if(message->first < rank_){
	MPI_Send(const_cast<MessageInformation*>(&(message->second)), 1, datatype_, message->first, 122, remoteIndices_.communicator());
	MPI_Recv(&recv, 1, datatype_, message->first, 122, remoteIndices_.communicator(), &status);
      }else{
	MPI_Recv(&recv, 1, datatype_, message->first, 122, remoteIndices_.communicator(), &status);
	MPI_Send(const_cast<MessageInformation*>(&(message->second)), 1, datatype_, message->first, 122, remoteIndices_.communicator());
      }
      // calculate max message size
      maxSize.publish = std::max(maxSize.publish, message->second.publish);
      maxSize.pairs = std::max(maxSize.pairs, message->second.pairs);
      maxSize.publish = std::max(maxSize.publish, recv.publish);
      maxSize.pairs = std::max(maxSize.pairs, recv.pairs);
    }

    MPI_Type_free(&datatype_);
    

    bufferSize_=0;
    int tsize;
    // The number of indices published
    MPI_Pack_size(1, MPI_INT,remoteIndices_.communicator(), &tsize);
    bufferSize_ += tsize;

    for(int i=0; i < maxSize.publish; ++i){
      // The global index
      MPI_Pack_size(1, MPITraits<TG>::getType(), remoteIndices_.communicator(), &tsize);
      bufferSize_ += tsize;
      // The attribute in the local index
      MPI_Pack_size(1, MPI_CHAR, remoteIndices_.communicator(), &tsize);
      bufferSize_ += tsize;
      // The number of corresponding remote indices
      MPI_Pack_size(1, MPI_INT, remoteIndices_.communicator(), &tsize);
      bufferSize_ += tsize;
    }
    for(int i=0; i < maxSize.pairs; ++i){
      // The process of the remote index
      MPI_Pack_size(1, MPI_INT, remoteIndices_.communicator(), &tsize);
      bufferSize_ += tsize;
      // The attribute of the remote index
      MPI_Pack_size(1, MPI_CHAR, remoteIndices_.communicator(), &tsize);
      bufferSize_ += tsize;
    }

    // Allocate the buffer
    buffer_ = new char[bufferSize_];
      
  }
  
  template<typename  TG, typename TA, int N>
  void IndicesSyncer<TG,TA,N>::sync()
  {
    
    // The pointers to the local indices in the remote indices
    // will become invalid due to the resorting of the index set.
    // Therefore store the corresponding global indices.
    // Mark all indices as not added
    
    typedef typename RemoteIndices::RemoteIndexMap::const_iterator 
      RemoteIterator;

    const RemoteIterator end = remoteIndices_.end();
    
    for(RemoteIterator remote = remoteIndices_.begin(); remote != end; ++remote){
      typedef typename RemoteIndices::RemoteIndexList::const_iterator
	RemoteIndexIterator;
      // Make sure we only have one remote index list.
      assert(remote->second.first==remote->second.second);

      RemoteIndexList& rList = *(remote->second.first);
            
      // Store the corresponding global indices.
      GlobalIndexList& global = globalMap_[remote->first];
      BoolList& added = oldMap_[remote->first];
      RemoteIndexIterator riEnd = rList.end();
      
      for(RemoteIndexIterator index = rList.begin();
	  index != riEnd; ++index){
	global.push_back(index->localIndexPair().global());
	added.push_back(true);
      }

      Iterators iterators(rList, global, added);
      iteratorsMap_.insert(std::make_pair(remote->first, iterators));
      assert(checkReset(iteratorsMap_[remote->first], rList,global,added));
    }
    
    // Exchange indices with each neighbour
    const RemoteIterator rend = remoteIndices_.end();

    calculateMessageSizes();
    
    indexSet_.beginResize();
    
    for(RemoteIterator remote = remoteIndices_.begin(); remote != rend; ++remote)
      if(remote->first < rank_){
	packAndSend(remote->first);
	recvAndUnpack(remote->first);
      }else{
	recvAndUnpack(remote->first);
	packAndSend(remote->first);
      }
    // No need for the iterator tuples any more
    iteratorsMap_.clear();
    
    indexSet_.endResize();
    
    repairLocalIndexPointers<TG,TA,typename RemoteIndices::Allocator,N>(globalMap_, remoteIndices_, indexSet_);
    
    oldMap_.clear();    
    globalMap_.clear();
	
    // update the sequence number
    remoteIndices_.sourceSeqNo_ = remoteIndices_.destSeqNo_ = indexSet_.seqNo();
  }

  
  template<typename TG, typename TA, int N>
  void IndicesSyncer<TG,TA,N>::packAndSend(int destination)
  {
    typedef typename IndexSet::const_iterator IndexIterator;
    typedef typename RemoteIndexList::const_iterator RemoteIndexIterator;
    typedef typename GlobalIndexList::const_iterator GlobalIterator;
    typedef typename BoolList::const_iterator BoolIterator;
        
    IndexIterator      iEnd       = indexSet_.end();
    int                bpos       = 0;
    int                published  = 0;
    int                pairs      = 0;
    
    assert(checkReset());
    
    // Pack the number of indices we publish
    MPI_Pack(&(infoSend_[destination].publish), 1, MPI_INT, buffer_, bufferSize_, &bpos, 
	     remoteIndices_.communicator());

    for(IndexIterator index = indexSet_.begin(); index != iEnd; ++index){
      // Search for corresponding remote indices in all iterator tuples
      typedef typename IteratorsMap::iterator Iterator;
      Iterator iteratorsEnd = iteratorsMap_.end();
      
      // advance all iterators to a position with global index >= index->global()
      for(Iterator iterators = iteratorsMap_.begin(); iteratorsEnd != iterators; ++iterators)
	while(iterators->second.isNotAtEnd() && iterators->second.globalIndex() < index->global())
	  ++(iterators->second);
	  
      // Add all remote indices positioned at global which were already present before calling sync
      // to the message.
      // Count how many remote indices we will send
      int indices = 0;
      bool knownRemote = false; // Is the remote process supposed to know this index?
      
      for(Iterator iterators = iteratorsMap_.begin(); iteratorsEnd != iterators; ++iterators)
	if(iterators->second.isNotAtEnd() && iterators->second.isOld() 
	   && iterators->second.globalIndex() == index->global()){
	  indices++;
	  if(destination == iterators->first)
	    knownRemote = true;
	}
      
      if(!knownRemote || indices==0)
	// We do not need to send any indices
	continue;
      
      std::cout<<rank_<<": sending "<<indices<<" for index "<<index->global()<<" to "<<destination<<std::endl;

      pairs+=indices;
      assert(pairs <= infoSend_[destination].pairs);

      // Pack the global index, the attribute and the number
      MPI_Pack(const_cast<TG*>(&(index->global())), 1, MPITraits<TG>::getType(), buffer_, bufferSize_, &bpos, 
	       remoteIndices_.communicator());
      
      char attr = index->local().attribute();
      MPI_Pack(&attr, 1, MPI_CHAR, buffer_, bufferSize_, &bpos, 
	       remoteIndices_.communicator());

      // Pack the number of remote indices we send.
      MPI_Pack(&indices, 1, MPI_INT, buffer_, bufferSize_, &bpos, 
	       remoteIndices_.communicator());
	
      // Pack the information about the remote indices
      for(Iterator iterators = iteratorsMap_.begin(); iteratorsEnd != iterators; ++iterators)
	if(iterators->second.isNotAtEnd() && iterators->second.isOld() 
	   && iterators->second.globalIndex() == index->global()){
	  int process = iterators->first;
	  MPI_Pack(&process, 1, MPI_INT, buffer_, bufferSize_, &bpos, 
		   remoteIndices_.communicator());
	  char attr = iterators->second.remoteIndex().attribute();
	  
	  MPI_Pack(&attr, 1, MPI_CHAR, buffer_, bufferSize_, &bpos, 
		   remoteIndices_.communicator());
	}
      ++published;
      assert(published <= infoSend_[destination].publish);
    }

    // Make sure we send all expected entries
    assert(published == infoSend_[destination].publish);

    resetIteratorsMap();
    
    MPI_Send(buffer_, bpos, MPI_PACKED, destination, 111, remoteIndices_.communicator());
  }

  template<typename TG, typename TA, int N>
  inline void IndicesSyncer<TG,TA,N>::insertIntoRemoteIndexList(int process, const TG& global, 
								char attribute)
  {
    Iterators& iterators = iteratorsMap_[process];	
    
    // Search for the remote index
    while(iterators.isNotAtEnd() && iterators.globalIndex() < global){
      // Increment all iterators
      ++iterators;
      
    }
    
    if(iterators.isAtEnd() || iterators.globalIndex() != global){
      // The entry is not yet known
      // Insert in the the list and do not change the first iterator.
      iterators.insert(RemoteIndex(TA(attribute)),global);
    }else if(iterators.isNotAtEnd()){
      // Assert that the attributes match 
      assert(iterators.remoteIndex().attribute() == attribute);
    }
  }
  
  template<typename TG, typename TA, int N>
  void IndicesSyncer<TG,TA,N>::recvAndUnpack(int source)
  {
    typedef typename IndexSet::const_iterator IndexIterator;
    typedef typename RemoteIndexList::iterator RemoteIndexIterator;
    typedef typename GlobalIndexList::iterator GlobalIndexIterator;

    IndexIterator    iEnd   = indexSet_.end();
    IndexIterator    index  = indexSet_.begin();
    int              bpos   = 0;
    int              publish;
    
    assert(checkReset());
    
    // Receive the data
    MPI_Status status;
    MPI_Recv(buffer_, bufferSize_, MPI_PACKED, source, 111, remoteIndices_.communicator(), &status);
    
    // How many global entries were published?
    MPI_Unpack(buffer_, bufferSize_, &bpos, &publish, 1, MPI_INT, remoteIndices_.communicator());

    // Now unpack the remote indices and add them.
    while(publish>0){
      
      // Unpack information about the local index on the source process
      TG  global;          // global index of the current entry
      char sourceAttribute; // Attribute on the source process
      int pairs;
      
      MPI_Unpack(buffer_, bufferSize_, &bpos, &global, 1, MPITraits<TG>::getType(), 
		 remoteIndices_.communicator());
      MPI_Unpack(buffer_, bufferSize_, &bpos, &sourceAttribute, 1, MPI_CHAR, 
		 remoteIndices_.communicator());
      MPI_Unpack(buffer_, bufferSize_, &bpos, &pairs, 1, MPI_INT, 
		 remoteIndices_.communicator());
    
      // Insert the entry on the remote process to our
      // remote index list
      insertIntoRemoteIndexList(source, global, sourceAttribute);

      // Unpack the remote indices
      while(pairs>0){
	// Unpack the process id that knows the index
	int process;
	char attribute;
	MPI_Unpack(buffer_, bufferSize_, &bpos, &process, 1, MPI_INT, 
		 remoteIndices_.communicator());
	// Unpack the attribute
	MPI_Unpack(buffer_, bufferSize_, &bpos, &attribute, 1, MPI_CHAR, 
		   remoteIndices_.communicator());
	
	if(process==rank_){
	  // Now we know the local attribute of the global index
	  // Do we know that global index already?
	  while(index != iEnd && index->global() < global)
	    ++index;
	  
	  if(index == iEnd || index->global() != global){
	    // No, we do not. Add it!
	    indexSet_.add(global,ParallelLocalIndex<TA>(TA(attribute), true));
	  }else{
	    // Attributes have to match!
	    assert(attribute==index->local().attribute());
	  }
	}else{
	  insertIntoRemoteIndexList(process, global, attribute);
	}
	
	--pairs;
      }
      --publish;
    }
    
    resetIteratorsMap();
  }
  
  template<typename TG, typename TA, int N>
  void IndicesSyncer<TG,TA,N>::resetIteratorsMap(){
    
    // Reset iterators in all tuples.
    typedef typename IteratorsMap::iterator Iterator;
    typedef typename RemoteIndices::RemoteIndexMap::iterator 
      RemoteIterator;
    typedef typename GlobalIndicesMap::iterator GlobalIterator;
    typedef typename BoolMap::iterator BoolIterator;
    
    const RemoteIterator remoteEnd = remoteIndices_.remoteIndices_.end();
    Iterator iterators = iteratorsMap_.begin();
    GlobalIterator global = globalMap_.begin();
    BoolIterator added = oldMap_.begin();
    
    for(RemoteIterator remote = remoteIndices_.remoteIndices_.begin();
	remote != remoteEnd; ++remote, ++global, ++added, ++iterators){
      iterators->second.reset(*(remote->second.first), global->second, added->second);
    }
  }
 
  template<typename TG, typename TA, int N>
  bool IndicesSyncer<TG,TA,N>::checkReset(const Iterators& iterators, RemoteIndexList& rList, GlobalIndexList& gList,
					  BoolList& bList){
    typename RemoteIndexList::iterator before=rList.oneBeforeBegin(),
      begin = rList.begin(), ri1=Element<0>::get(iterators.iterators_),
      ri2 = Element<3>::get(iterators.iterators_);
    
    if(Element<0>::get(iterators.iterators_)!=rList.oneBeforeBegin())
      return false;
    if(Element<1>::get(iterators.iterators_)!=gList.oneBeforeBegin())
      return false;
    if(Element<2>::get(iterators.iterators_)!=bList.oneBeforeBegin())
      return false;
    if(Element<3>::get(iterators.iterators_)!=rList.begin())
      return false;
    if(Element<4>::get(iterators.iterators_)!=gList.begin())
      return false;
    if(Element<5>::get(iterators.iterators_)!=bList.begin())
      return false;
    return true;
  }
  

  template<typename TG, typename TA, int N>
  bool IndicesSyncer<TG,TA,N>::checkReset(){
    
    // Reset iterators in all tuples.
    typedef typename IteratorsMap::iterator Iterator;
    typedef typename RemoteIndices::RemoteIndexMap::iterator 
      RemoteIterator;
    typedef typename GlobalIndicesMap::iterator GlobalIterator;
    typedef typename BoolMap::iterator BoolIterator;
    
    const RemoteIterator remoteEnd = remoteIndices_.remoteIndices_.end();
    Iterator iterators = iteratorsMap_.begin();
    GlobalIterator global = globalMap_.begin();
    BoolIterator added = oldMap_.begin();
    bool ret = true;
    
    for(RemoteIterator remote = remoteIndices_.remoteIndices_.begin();
	remote != remoteEnd; ++remote, ++global, ++added, ++iterators){
      if(!checkReset(iterators->second, *(remote->second.first), global->second,
		     added->second))
	ret=false;
    }
    return ret;
  } 
}

#endif
