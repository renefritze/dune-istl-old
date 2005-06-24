// $Id$

#ifndef DUNE_ISTL_LOCALINDEX_HH
#define DUNE_ISTL_LOCALINDEX_HH

#include"config.h"
#include<cstddef>

namespace Dune
{


/** @addtogroup ISTL_Comm
   *
   * @{
   */
  /**
   * @file
   * @brief Provides classes for use as the local index in IndexSet.
   * @author Markus Blatt
   */
  /**
   * @brief The states avaiable for the local indices.
   * @see LocalIndex::state()
   */
  enum LocalIndexState {VALID, DELETED};
  
    
  /**
   * @brief An index present on the local process.
   */
  class LocalIndex
  {
  public:    
    /**
     * @brief Constructor.
     * known to other processes.
     */
    LocalIndex() :
      localIndex_(0), state_(VALID){}

       
    /**
     * @brief Constructor.
     * @param index The value of the index.
     */
    LocalIndex(std::ptrdiff_t index) :
      localIndex_(index), state_(VALID){}
    /**
     * @brief get the local index.
     * @return The local index.
     */
    inline const std::ptrdiff_t& local() const;

    /**
     * @brief Convert to the local index represented by an int.
     */
    inline operator std::ptrdiff_t() const;

    /**
     * @brief Assign a new local index.
     *
     * @param index The new local index.
     */
    inline LocalIndex& operator=(std::ptrdiff_t index);

    /**
     * @brief Get the state.
     * @return The state.
     */
    inline LocalIndexState state() const;

    /**
     * @brief Set the state.
     * @param state The state to set.
     */
    inline void setState(LocalIndexState state);

  private:
    /** @brief The local index. */
    std::ptrdiff_t localIndex_;
    
    /** 
     * @brief The state of the index.
     *
     * Has to be one of LocalIndexState!
     * @see LocalIndexState.
     */
    char state_;
       
  };


    
  inline const std::ptrdiff_t& LocalIndex::local() const{
    return localIndex_;
  }

  inline LocalIndex::operator std::ptrdiff_t() const{
    return localIndex_;
  }

  inline LocalIndex& LocalIndex::operator=(std::ptrdiff_t index){
    localIndex_ = index;
    return *this;
  }
    
  inline LocalIndexState LocalIndex::state() const{
    return  static_cast<LocalIndexState>(state_);
  }

  inline void LocalIndex::setState(LocalIndexState state){
    state_ = static_cast<char>(state);
  }

  /** @} */

} // namespace Dune

#endif
