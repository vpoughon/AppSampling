#ifndef __otbMaskedIteratorDecorator_h
#define __otbMaskedIteratorDecorator_h

#include <boost/move/utility_core.hpp>

namespace otb
{

// Questions:
// - how to redirect method calls to both iterators (image and mask) ?
// - composition vs inheritance and public vs private
// - GoToBegin implementation (first valid can be not first)
template <typename TIteratorType>
class MaskedIteratorDecorator : public TIteratorType
{
public:
  typedef MaskedIteratorDecorator<TIteratorType> Self;
  typedef TIteratorType                          Superclass;
  typedef typename TIteratorType::ImageType      ImageType;
  typedef typename TIteratorType::ImageType      MaskType;

  template <typename T1>
  MaskedIteratorDecorator(typename MaskType::Pointer mask,
                          typename ImageType::Pointer image,
                          BOOST_FWD_REF(T1) arg1):
                            TIteratorType(image,
                                          boost::forward<T1>(arg1)),
                            m_mask(mask),
                            m_itMask(mask,
                                     boost::forward<T1>(arg1))
  {
  }

  template <typename T1,
            typename T2>
  MaskedIteratorDecorator(typename MaskType::Pointer mask,
                          typename ImageType::Pointer image,
                          BOOST_FWD_REF(T1) arg1,
                          BOOST_FWD_REF(T2) arg2):
                            TIteratorType(image,
                                          boost::forward<T1>(arg1),
                                          boost::forward<T2>(arg2)),
                            m_mask(mask),
                            m_itMask(mask,
                                     boost::forward<T1>(arg1),
                                     boost::forward<T2>(arg2))
  {
  }

  template <typename T1,
            typename T2,
            typename T3>
  MaskedIteratorDecorator(typename MaskType::Pointer mask,
                          typename ImageType::Pointer image,
                          BOOST_FWD_REF(T1) arg1,
                          BOOST_FWD_REF(T2) arg2,
                          BOOST_FWD_REF(T3) arg3):
                            TIteratorType(image,
                                          boost::forward<T1>(arg1),
                                          boost::forward<T2>(arg2),
                                          boost::forward<T2>(arg3)),
                            m_mask(mask),
                            m_itMask(mask,
                                     boost::forward<T1>(arg1),
                                     boost::forward<T2>(arg2),
                                     boost::forward<T3>(arg3))
  {
  }

  void GoToBegin()
  {
    //m_isAtBegin = true;
    this->Superclass::GoToBegin();
    this->m_itMask.GoToBegin();
    while (!this->IsAtEnd() && !m_itMask.IsAtEnd() && m_itMask.Value() == 0)
    {
      this->Superclass::operator++();
      this->m_itMask.operator++();
    }
  }

  void GoToEnd()
  {
    this->Superclass::GoToEnd();
    this->m_itMask.GoToEnd();
  }

  bool IsAtBegin() const
  {
    // Current position is the actual beginning
    if (this->IsAtBegin() || m_itMask.IsAtBegin())
    {
      return true;
    }

    // Can be begin also if there are no unmasked pixel before current position
    TIteratorType itBeginChecker(*this);
    TIteratorType itMaskBeginChecker(m_itMask);
    do
    {
      itBeginChecker--;
      itMaskBeginChecker--;
      if (itMaskBeginChecker.Value() == 0)
      {
        return false;
      }
    } while (!itBeginChecker.IsAtBegin() && !itMaskBeginChecker.IsAtBegin());

    // True iff iterators' begin position is masked
    return itMaskBeginChecker.Value() != 0;
  }

  bool IsAtEnd() const
  {
    return this->Superclass::IsAtEnd() || m_itMask.IsAtEnd();
  }

  // Wrap the underlying iterator to ignore masked pixels
  Self& operator++()
  {
    do
    {
      this->Superclass::operator++();
      this->m_itMask.operator++();
    } while (!this->IsAtEnd() && !m_itMask.IsAtEnd() && m_itMask.Value() == 0);
    return *this;
  }

  Self & operator--()
  {
    do
    {
      this->Superclass::operator--();
      this->m_itMask.operator--();
    } while (m_itMask->Value() == 0);
    return *this;
  }

private:
  typename MaskType::Pointer m_mask;
  TIteratorType m_itMask;
};

}

#endif
