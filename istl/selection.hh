// $Id$
#ifndef DUNE_SELECTION_HH
#define DUNE_SELECTION_HH

#include"indexset.hh"
#include<dune/common/iteratorfacades.hh>

namespace Dune
{
  /** @addtogroup ISTL_Comm
   *
   * @{
   */
  /**
   * @file
   * @brief Provides classes for selecting
   * indices base on attribute flags.
   * @author Markus Blatt
   */
  
  /**
   * @brief A const iterator over an uncached selection.
   */
  template<typename TS, typename TG, typename TL>
  class SelectionIterator
  {
  public:
    /**
     * @brief The type of the Set of attributes.
     *
     * It has to provide a static method
     * <pre> bool contains(AttributeType a);</pre>
     * that returns true if a is in the set.
     * Such types are EnumItem, EnumRange, Combine.
     */
    typedef TS AttributeSet;
    
    /**
     * @brief The type of the underlying index set.
     */
    typedef IndexSet<TG,TL> IndexSet;
    
    //typedef typename IndexSet::const_iterator IndexSetIterator;

    typedef ConstArrayListIterator<IndexPair<TG,TL>,100, std::allocator<Dune::IndexPair<TG,TL> > > IndexSetIterator;
    /**
     * @brief Constructor.
     * @param iter The iterator over the index set.
     * @param end The iterator over the index set positioned at the end.
     */
    SelectionIterator(const IndexSetIterator& iter, const IndexSetIterator& end)
      : iter_(iter), end_(end)
    {
      // Step to the first valid entry
      while(iter_!=end_ && !AttributeSet::contains(iter_->local().attribute()))
	++iter_;
    }
    
    void operator++()
    {
      assert(iter_!=end_);
      for(++iter_;iter_!=end_; ++iter_)
	if(AttributeSet::contains(iter_->local().attribute()))
	  break;
    }
    
	
    const uint32_t operator*() const
    {
      return iter_->local().local();
    }
    
    bool operator==(const SelectionIterator<TS,TG,TL>& other) const
    {
      return iter_ == other.iter_;
    }
      
    bool operator!=(const SelectionIterator<TS,TG,TL>& other) const
    {
      return iter_ != other.iter_;
    }
      
  private:
    IndexSetIterator iter_;
    const IndexSetIterator& end_;
  };
  
  
  /**
   * @brief An uncached selection of indices.
   */
  template<typename TS, typename TG, typename TL>
  class UncachedSelection
  {
  public:
    /**
     * @brief The type of the Set of attributes.
     *
     * It has to provide a static method
     * <pre> bool contains(AttributeType a);</pre>
     * that returns true if a is in the set.
     * Such types are EnumItem, EnumRange, Combine.
     */
    typedef TS AttributeSet;
   
    /**
     * @brief The type of the global index of the underlying index set.
     */
    typedef TG GlobalIndex;
    
    /**
     * @brief The type of the local index of the underlying index set.
     *
     * It has to provide a function
     * <pre>AttributeType attribute();</pre>
     */
    typedef TL LocalIndex;

    /**
     * @brief The type of the underlying index set.
     */
    typedef IndexSet<GlobalIndex,LocalIndex> IndexSet;
    
    /**
     * @brief The type of the iterator of the selected indices.
     */
    typedef SelectionIterator<TS,TG,TL> iterator;
    
    /**
     * @brief The type of the iterator of the selected indices.
     */
    typedef iterator const_iterator;
  
    UncachedSelection()
      : indexSet_()
    {}
    
    UncachedSelection(const IndexSet& indexset)
      : indexSet_(&indexset)
    {}
    /**
     * @brief Set the index set of the selection.
     * @param indexset The index set to use.
     */
    void setIndexSet(const IndexSet& indexset);
    
    /**
     * @brief Get the index set we are a selection for.
     */
    //const IndexSet& indexSet() const;
    
    /**
     * @brief Get an iterator over the selected indices.
     * @return An iterator positioned at the first selected index.
     */
    const_iterator begin() const;
    
    /** 
     * @brief Get an iterator over the selected indices.
     * @return An iterator positioned at the first selected index.
     */
    const_iterator end() const;
    
	
  private:
    const IndexSet* indexSet_;
    
  };
  

   
  /**
   * @brief An cached selection of indices.
   */
  template<typename TS, typename TG, typename TL>
  class Selection
  {
  public:
    /**
     * @brief The type of the Set of attributes.
     *
     * It has to provide a static method
     * <pre> bool contains(AttributeType a);</pre>
     * that returns true if a is in the set.
     * Such types are EnumItem, EnumRange, Combine.
     */
    typedef TS AttributeSet;
    
    /**
     * @brief The type of the global index of the underlying index set.
     */
    typedef TG GlobalIndex;
    
    /**
     * @brief The type of the local index of the underlying index set.
     *
     * It has to provide a function
     * <pre>AttributeType attribute();</pre>
     */
    typedef TL LocalIndex;
    
    /**
     * @brief The type of the underlying index set.
     */
    typedef IndexSet<GlobalIndex,LocalIndex> IndexSet;
    
    /**
     * @brief The type of the iterator of the selected indices.
     */
    typedef uint32_t* iterator;
    
    /**
     * @brief The type of the iterator of the selected indices.
     */
    typedef uint32_t* const_iterator;
  
    Selection()
      : selected_()
    {}
  
    Selection(const IndexSet& indexset)
      : selected_(), size_(0), built_(false)
    {
      setIndexSet(indexset);
    }
    
    ~Selection();
    
    /**
     * @brief Set the index set of the selection.
     * @param indexset The index set to use.
     */
    void setIndexSet(const IndexSet& indexset);

    /**
     * @brief Free allocated memory.
     */
    void free();
    
    /**
     * @brief Get the index set we are a selection for.
     */
    //IndexSet indexSet() const;
    
    /**
     * @brief Get an iterator over the selected indices.
     * @return An iterator positioned at the first selected index.
     */
    const_iterator begin() const;
    
    /** 
     * @brief Get an iterator over the selected indices.
     * @return An iterator positioned at the first selected index.
     */
    const_iterator end() const;
    
	
  private:
    uint32_t* selected_;
    size_t size_;
    bool built_;
    
  };

  template<typename TS, typename TG, typename TL>
  inline void Selection<TS,TG,TL>::setIndexSet(const IndexSet& indexset)
  {
    if(built_)
      free();
    
    // Count the number of entries the selection has to hold
    typedef typename IndexSet::const_iterator const_iterator;
    const const_iterator end = indexset.end();
    int entries = 0;
    
    for(const_iterator index = indexset.begin(); index != end; ++index)
      if(AttributeSet::contains(index->local().attribute()))
	 ++entries;

    selected_ = new uint32_t[entries];
    built_ = true;
    
    entries = 0;
    for(const_iterator index = indexset.begin(); index != end; ++index)
      if(AttributeSet::contains(index->local().attribute()))
	selected_[entries++]= index->local().local();
    
    size_=entries;
    built_=true;
  }
  
  template<typename TS, typename TG, typename TL>
  uint32_t* Selection<TS,TG,TL>::begin() const
  {
    return selected_;
  }

  template<typename TS, typename TG, typename TL>
  uint32_t* Selection<TS,TG,TL>::end() const
  {
    return selected_+size_;
  }
  
  template<typename TS, typename TG, typename TL>
  inline void Selection<TS,TG,TL>::free()
  {
    delete[] selected_;
    size_=0;
    built_=false;
  }

  template<typename TS, typename TG, typename TL>
  inline Selection<TS,TG,TL>::~Selection()
  {
    if(built_)
      free();
  }
   
  template<typename TS, typename TG, typename TL>
  SelectionIterator<TS,TG,TL> UncachedSelection<TS,TG,TL>::begin() const
  {
    return SelectionIterator<TS,TG,TL>(indexSet_->begin(),
				       indexSet_->end());
  }
  
  template<typename TS, typename TG, typename TL>
  SelectionIterator<TS,TG,TL> UncachedSelection<TS,TG,TL>::end() const
  {
    return SelectionIterator<TS,TG,TL>(indexSet_->end(),
				       indexSet_->end());
  }
  template<typename TS, typename TG, typename TL>
  void UncachedSelection<TS,TG,TL>::setIndexSet(const IndexSet& indexset)
  {
    indexSet_ = &indexset;
  }
  
  /** @} */

  
}
#endif
